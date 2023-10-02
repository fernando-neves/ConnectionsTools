/* ASIO INCLUDES */
#include <asio.hpp>

/* PLOG INCLUDES */
#include <plog/Log.h>
#include <plog/Init.h>
#include <plog/Formatters/TxtFormatter.h>
#include <plog/Appenders/ColorConsoleAppender.h>

static std::shared_ptr<asio::io_service> io_service;

static void service_thread(const std::shared_ptr<asio::io_service>& io_service)
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
	} while (true);
}

class udp_echo_server
	: public std::enable_shared_from_this<udp_echo_server>
{
public:
	explicit udp_echo_server(std::shared_ptr<asio::io_service> service)
		: m_io_service(std::move(service)), m_local_port(0)
	{
	}

	void listen(const std::string& address, const uint16_t port)
	{
		if (m_started)
			return;

		m_started = true;

		PLOGD << "create server socket";

		m_local_address = address;
		m_local_port = port;

		m_endpoint = std::make_shared<asio::ip::udp::endpoint>(asio::ip::make_address(address), port);
		m_socket = std::make_shared<asio::ip::udp::socket>(*m_io_service, *m_endpoint);

		m_socket->set_option(asio::socket_base::send_buffer_size(262144));
		m_socket->set_option(asio::socket_base::receive_buffer_size(262144));

		m_receive_buffer.resize(262144);

		set_receive_from();
	}

	void terminate()
	{
		if (m_is_terminated)
		{
			PLOGI << "downstream already terminated";
			return;
		}

		m_is_terminated = true;

		const auto self(shared_from_this());
		PLOGD << "call terminate downstream - m_is_terminated: " << self->m_is_terminated;
		try
		{
			self->m_socket->close();
			self->m_socket.reset();
		}
		catch (const std::exception& e)
		{
			PLOGE << e.what();
		}
	}

	void set_receive_from()
	{
		if (m_is_terminated || m_is_receiving)
		{
			PLOGI << "m_is_terminated: " << m_is_terminated << " - m_is_receiving: " << m_is_receiving;
			return;
		}

		m_is_receiving = true;

		std::shared_ptr<asio::ip::udp::endpoint> last_received_endpoint =
			std::make_shared<asio::ip::udp::endpoint>();

		auto self(shared_from_this());
		auto bounded_function = [self, last_received_endpoint](const std::error_code& error, const size_t bytes_transferred)
			{
				self->handler_receive_from(last_received_endpoint, error, bytes_transferred);
			};

		const auto asio_buffer = asio::buffer(m_receive_buffer.data(), m_receive_buffer.size());
		m_socket->async_receive_from(asio_buffer, *last_received_endpoint, bounded_function);
	}

	void handler_receive_from(const std::shared_ptr<asio::ip::udp::endpoint>& last_received_endpoint, const std::error_code& error, const size_t bytes_transferred)
	{
		if (error)
		{
			PLOGE << "error value: " << error.value() << " - message: " << error.message();
			terminate();
			return;
		}

		PLOGD << "recv from " << last_received_endpoint->address().to_v4().to_string()
			<< ":" << last_received_endpoint->port()
			<< " - bytes: " << bytes_transferred;

		send_packet_to(last_received_endpoint, m_receive_buffer.data(), bytes_transferred);

		m_is_receiving = false;
		set_receive_from();
	}

	void send_packet_to(const std::shared_ptr<asio::ip::udp::endpoint>& last_received_endpoint, void* buffer, const size_t size)
	{
		if (m_is_terminated || m_is_sending)
		{
			return;
		}

		m_is_sending = true;

		auto self(shared_from_this());
		auto bounded_function = [self, last_received_endpoint](const std::error_code& error, const size_t bytes_transferred)
			{
				self->handler_send_packet_to(last_received_endpoint, error, bytes_transferred);
			};

		std::string data_buffer = std::string((char*)buffer, size);
		if (data_buffer == "get_remote_address")
			data_buffer = last_received_endpoint->address().to_v4().to_string() + ":" + std::to_string(last_received_endpoint->port());

		const auto asio_buffer = asio::buffer(data_buffer.data(), data_buffer.size());
		m_socket->async_send_to(asio_buffer, *last_received_endpoint, bounded_function);
	}

	void handler_send_packet_to(const std::shared_ptr<asio::ip::udp::endpoint>& last_received_endpoint, const std::error_code& error, size_t bytes_transferred)
	{
		if (error)
		{
			PLOGE << "code: " << error.value() << " - message: " << error.message();
			terminate();
			return;
		}

		PLOGD << "send packet to " << last_received_endpoint->address().to_v4().to_string()
			<< ":" << last_received_endpoint->port()
			<< " - bytes: " << bytes_transferred;

		m_is_sending = false;
	}

private:
	std::shared_ptr<asio::ip::udp::endpoint> m_endpoint;
	std::shared_ptr<asio::ip::udp::socket> m_socket;

	std::shared_ptr<asio::io_service> m_io_service;

	std::string m_local_address;
	uint16_t m_local_port;

	std::atomic<bool> m_started{ false };
	std::atomic<bool> m_is_sending{ false };
	std::atomic<bool> m_is_receiving{ false };
	std::atomic<bool> m_is_terminated{ false };

	std::vector<uint8_t> m_receive_buffer;
};

int main()
{
	static plog::ColorConsoleAppender<plog::TxtFormatter> console_appender;
	init(plog::verbose, &console_appender);

	PLOGD << "started plog verbose";

	io_service = std::make_shared<asio::io_service>();

	const auto current_server = std::make_shared<udp_echo_server>(io_service);
	PLOGD << "created udp_echo_server class";

	current_server->listen("0.0.0.0", 7172);

	service_thread(io_service);

	PLOGD << "started io_service";
	return 0;
}