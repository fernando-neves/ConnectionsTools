/* ASIO INCLUDES */
#include <asio.hpp>
#include <utility>

/* PLOG INCLUDES */
#include <random>
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
			} while (true);
		}).detach();
}

class udp_echo_client
	: public std::enable_shared_from_this<udp_echo_client>
{
public:
	explicit udp_echo_client(
		std::shared_ptr<asio::io_service> service)
		: m_io_service(std::move(service))
	{
	}

	void start()
	{
		if (m_started)
			return;

		m_started = true;

		try
		{
			PLOGD << "create upstream socket";
			m_socket = std::make_shared<asio::ip::udp::socket>(*m_io_service, asio::ip::udp::endpoint());

			m_receive_buffer.resize(262144);

			set_receive_from();
		}
		catch (const std::exception& e)
		{
			PLOGE << e.what();
			terminate();
		}
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
			if (self->m_socket)
			{
				self->m_socket->close();
				self->m_socket.reset();
			}
		}
		catch (const std::exception& e)
		{
			PLOGE << e.what();
		}
	}

	void send_packet_to(const std::shared_ptr<asio::ip::udp::endpoint>& remote_endpoint, char* buffer, const size_t size)
	{
		if (m_is_terminated || m_is_sending)
		{
			return;
		}

		m_is_sending = true;

		auto self(shared_from_this());
		auto bounded_function = [self, remote_endpoint](const std::error_code& error, const size_t bytes_transferred)
			{
				self->handler_send_packet_to(remote_endpoint, error, bytes_transferred);
			};

		const auto asio_buffer = asio::buffer(buffer, size);
		m_socket->async_send_to(asio_buffer, *remote_endpoint, bounded_function);
	}

	void handler_send_packet_to(const std::shared_ptr<asio::ip::udp::endpoint>& remote_endpoint, const std::error_code& error, const size_t bytes_transferred)
	{
		if (error)
		{
			PLOGE << "code: " << error.value() << " - message: " << error.message();
			terminate();
			return;
		}

		m_start_time = std::chrono::high_resolution_clock::now();

		PLOGD << "send packet to " << remote_endpoint->address().to_v4().to_string()
			<< ":" << remote_endpoint->port()
			<< " - bytes: " << bytes_transferred;

		m_is_sending = false;
	}

	void set_receive_from()
	{
		if (m_is_terminated || m_is_receiving)
		{
			PLOGI << "m_is_terminated: " << m_is_terminated << " - m_is_receiving: " << m_is_receiving;
			return;
		}

		m_is_receiving = true;

		auto last_received_endpoint = std::make_shared<asio::ip::udp::endpoint>();

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

		const auto end_time = std::chrono::high_resolution_clock::now();
		const auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - m_start_time);

		PLOGD << "recv from " << last_received_endpoint->address().to_v4().to_string()
			<< ":" << last_received_endpoint->port()
			<< " - bytes: " << bytes_transferred
			<< " - latency: " << elapsed_time.count() << " ms"
			<< " - buffer: " << std::string(reinterpret_cast<char*>(m_receive_buffer.data()), bytes_transferred);

		send_packet_to(last_received_endpoint, reinterpret_cast<char*>(m_receive_buffer.data()), bytes_transferred);

		m_is_receiving = false;
		set_receive_from();
	}

private:
	std::atomic<bool> m_started{ false };
	std::atomic<bool> m_is_sending{ false };
	std::atomic<bool> m_is_receiving{ false };
	std::atomic<bool> m_is_terminated{ false };

	std::shared_ptr<asio::io_service> m_io_service;

	std::vector<uint8_t> m_receive_buffer;
	std::chrono::time_point<std::chrono::high_resolution_clock> m_start_time;

	std::shared_ptr<asio::ip::udp::socket> m_socket;
};

int main()
{
	static plog::ColorConsoleAppender<plog::TxtFormatter> console_appender;
	init(plog::verbose, &console_appender);

	PLOGD << "started plog verbose";

	io_service = std::make_shared<asio::io_service>();
	service_thread(io_service);

	const auto current_client = std::make_shared<udp_echo_client>(io_service);
	PLOGD << "created udp_echo_client class";

	current_client->start();

	const auto remote_endpoint = std::make_shared<asio::ip::udp::endpoint>(asio::ip::address_v4::from_string("198.16.109.30"), 7172);

	const auto buffer = std::string("get_remote_address");
	current_client->send_packet_to(remote_endpoint, const_cast<char*>(buffer.data()), buffer.size());

	while (true)
		std::this_thread::sleep_for(std::chrono::milliseconds(UINT16_MAX));

	PLOGD << "started io_service";
	return 0;
}