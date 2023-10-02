/* ASIO INCLUDES */
#include <asio.hpp>
#include <utility>

/* PLOG INCLUDES */
#include <plog/Log.h>
#include <plog/Init.h>
#include <plog/Formatters/TxtFormatter.h>
#include <plog/Appenders/ColorConsoleAppender.h>

static std::shared_ptr<asio::io_service> io_service;

static void service_thread(const std::shared_ptr<asio::io_service>& io_service)
{
	std::thread([io_service]()
	{
		asio::io_service::work work(*io_service);

		do
		{
			try
			{
				io_service->run();
				break;
			}
			catch (const std::system_error& ex)
			{
				if (ex.code().value() == (10057) /*asio::error::not_connected*/)
					continue;

				break;
			}
		}
		while (true);
	}).detach();
}

class tcp_echo_client
	: public std::enable_shared_from_this<tcp_echo_client>
{
public:
	explicit tcp_echo_client(
		std::shared_ptr<asio::io_service> service)
		: m_io_service(std::move(service))
	{
	}

	void start(
		const std::string& remote_address,
		const uint16_t remote_port)
	{
		if (m_started)
			return;

		m_started = true;

		try
		{
			PLOGD << "create upstream socket";
			const asio::ip::tcp::endpoint remote_endpoint = { asio::ip::address_v4::from_string(remote_address), remote_port};
			m_upstream_socket = std::make_shared<asio::ip::tcp::socket>(*m_io_service);

			m_remote_address = remote_address;
			m_remote_port = remote_port;

			m_receive_buffer.resize(262144);

			connect(remote_endpoint);
		}
		catch (const std::exception& e)
		{
			PLOGE << e.what();
			terminate();
		}
	}

	void connect(const asio::ip::tcp::endpoint& remote_endpoint)
	{
		if (m_is_connecting || m_is_connected || m_is_terminated)
		{
			PLOGI << "m_is_connecting: " << m_is_connecting << " - m_is_connected: " << m_is_connected;
			return;
		}

		m_is_connecting = true;

		auto self(shared_from_this());
		auto bounded_function = [self, remote_endpoint](const std::error_code& error)
		{
			self->handler_connect(remote_endpoint, error);
		};

		PLOGD << "create and try connect new upstream - " << remote_endpoint;
		m_upstream_socket->async_connect(remote_endpoint, bounded_function);
	}

	void handler_connect(const asio::ip::tcp::endpoint& remote_endpoint, const std::error_code& error)
	{
		if (error)
		{
			PLOGE << "code: " << error.value() << " - message: " << error.message();
			terminate();
			return;
		}

		try
		{
			m_upstream_socket->set_option(asio::ip::tcp::no_delay(true));
			m_upstream_socket->set_option(asio::socket_base::send_buffer_size(262144));
			m_upstream_socket->set_option(asio::socket_base::receive_buffer_size(262144));
		}
		catch (const std::exception& e)
		{
			PLOGE << e.what();
			terminate();
			return;
		}

		m_is_connecting = false;
		m_is_connected = true;

		PLOGD << "on connect - remote_endpoint " << remote_endpoint;

		set_receive();
	}

	void set_receive()
	{
		if (m_is_terminated || m_is_receiving)
		{
			PLOGI << "m_is_terminated: " << m_is_terminated << " - m_is_receiving: " << m_is_receiving;
			return;
		}

		m_is_receiving = true;

		auto self(shared_from_this());
		auto bounded_function = [self](const std::error_code& error, const size_t bytes_transferred)
		{
			self->handler_receive(error, bytes_transferred);
		};

		const auto asio_buffer = asio::buffer(m_receive_buffer.data(), m_receive_buffer.size());
		m_upstream_socket->async_receive(asio_buffer, bounded_function);
	}

	void handler_receive(const std::error_code& error, const size_t bytes_transferred)
	{
		if (error)
		{
			PLOGE << "error value: " << error.value() << " - message: " << error.message();
			terminate();
			return;
		}

		auto end_time = std::chrono::high_resolution_clock::now();
		auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - m_start_time);

		PLOGD << "recv from " << m_remote_address << ":" << m_remote_port 
			<< " - bytes: " << bytes_transferred
			<< " - latency: " << elapsed_time.count() << " ms"
			<< " - buffer: " << std::string((char*)m_receive_buffer.data(), bytes_transferred);

		send_packet(m_receive_buffer.data(), bytes_transferred);

		m_is_receiving = false;
		set_receive();
	}

	void send_packet(void* buffer, const size_t size)
	{
		if (m_is_terminated && !m_is_connected)
		{
			return;
		}

		auto self(shared_from_this());
		auto bounded_function = [self](const std::error_code& error, const size_t bytes_transferred)
		{
			self->handler_send_packet(error, bytes_transferred);
		};

		const auto asio_buffer = asio::buffer(buffer, size);
		m_upstream_socket->async_send(asio_buffer, bounded_function);
	}

	void handler_send_packet(const std::error_code& error, const size_t bytes_transferred)
	{
		if (error)
		{
			PLOGE << "code: " << error.value() << " - message: " << error.message();
			terminate();
			return;
		}

		m_start_time = std::chrono::high_resolution_clock::now();

		PLOGD << "send packet to " << m_remote_address << ":" << m_remote_port << " - bytes: " << bytes_transferred;
		m_is_sending = false;
	}

	void terminate()
	{
		if (m_is_terminated)
		{
			PLOGI << "upstream already terminated";
			return;
		}

		m_is_terminated = true;

		const auto self(shared_from_this());
		PLOGD << "call terminate upstream - m_is_terminated: " << self->m_is_terminated;

		try
		{
			if (self->m_upstream_socket)
			{
				self->m_upstream_socket->close();
				self->m_upstream_socket.reset();
			}
		}
		catch (const std::exception& e)
		{
			PLOGE << e.what();
		}
	}

	bool get_is_connected()
	{
		return m_is_connected;
	}

private:
	std::atomic<bool> m_started{false};
	std::atomic<bool> m_is_connecting{false};
	std::atomic<bool> m_is_sending{false};
	std::atomic<bool> m_is_receiving{false};

	std::atomic<bool> m_is_connected{false};
	std::atomic<bool> m_is_terminated{false};

	std::string m_remote_address{};
	uint16_t m_remote_port{0};

	std::shared_ptr<asio::io_service> m_io_service;

	std::vector<uint8_t> m_receive_buffer;
	std::chrono::steady_clock::time_point m_start_time;

	std::shared_ptr<asio::ip::tcp::socket> m_upstream_socket;
};

int main()
{
	static plog::ColorConsoleAppender<plog::TxtFormatter> console_appender;
	init(plog::verbose, &console_appender);

	PLOGD << "started plog verbose";

	io_service = std::make_shared<asio::io_service>();
	service_thread(io_service);

	const auto current_server = std::make_shared<tcp_echo_client>(io_service);
	PLOGD << "created tcp_echo_server class";

	current_server->start("208.167.245.168", 7171);

	while (!current_server->get_is_connected())
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

	const auto buffer = std::string("get_remote_address");
	current_server->send_packet((void*)buffer.data(), buffer.size());

	while (true)
		std::this_thread::sleep_for(std::chrono::milliseconds(UINT16_MAX));

	PLOGD << "started io_service";
	return 0;
}
