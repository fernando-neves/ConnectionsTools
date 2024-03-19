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

class tcp_downstream
	: public std::enable_shared_from_this<tcp_downstream>
{
public:
	using shared_ptr = std::shared_ptr<tcp_downstream>;

	explicit tcp_downstream(std::shared_ptr<asio::io_service> service)
		: m_io_service(std::move(service))
		, m_remote_port(0)
	{
		m_downstream_socket = std::make_shared<asio::ip::tcp::socket>(*m_io_service);
	}

	void start()
	{
		if (m_started)
			return;

		m_started = true;

		try
		{
			PLOGD << "create downstream socket";

			m_downstream_socket->set_option(asio::ip::tcp::no_delay(true));
			m_downstream_socket->set_option(asio::socket_base::send_buffer_size(262144));
			m_downstream_socket->set_option(asio::socket_base::receive_buffer_size(262144));

			m_receive_buffer.resize(262144);
		}
		catch (const std::exception& e)
		{
			PLOGE << e.what();
			terminate();
			return;
		}

		m_remote_address = m_downstream_socket->remote_endpoint().address().to_v4().to_string();
		m_remote_port = m_downstream_socket->remote_endpoint().port();

		set_receive();
	}

	std::shared_ptr<asio::ip::tcp::socket> socket()
	{
		return m_downstream_socket;
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
			self->m_downstream_socket->close();
			self->m_downstream_socket.reset();
		}
		catch (const std::exception& e)
		{
			PLOGE << e.what();
		}
	}

	void send_packet(void* buffer, const size_t size)
	{
		if (m_is_terminated || m_is_sending)
		{
			return;
		}

		m_is_sending = true;

		auto self(shared_from_this());
		auto bounded_function = [self](const std::error_code& error, const size_t bytes_transferred)
			{
				self->handler_send_packet(error, bytes_transferred);
			};

		auto data_buffer = std::string(static_cast<char*>(buffer), size);
		if (data_buffer == "get_remote_address")
			data_buffer = m_remote_address + ":" + std::to_string(m_remote_port);

		const auto asio_buffer = asio::buffer(data_buffer.data(), data_buffer.size());
		m_downstream_socket->async_send(asio_buffer, bounded_function);
	}

	void handler_send_packet(const std::error_code& error, size_t bytes_transferred)
	{
		if (error)
		{
			PLOGE << "code: " << error.value() << " - message: " << error.message();
			terminate();
			return;
		}

		PLOGD << "send packet to " << m_remote_address << ":" << m_remote_port << " - bytes: " << bytes_transferred;

		m_is_sending = false;
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
		m_downstream_socket->async_receive(asio_buffer, bounded_function);
	}

	void handler_receive(const std::error_code& error, const size_t bytes_transferred)
	{
		if (error)
		{
			PLOGE << "error value: " << error.value() << " - message: " << error.message();
			terminate();
			return;
		}

		PLOGD << "recv from " << m_remote_address << ":" << m_remote_port << " - bytes: " << bytes_transferred;

		send_packet(m_receive_buffer.data(), bytes_transferred);

		m_is_receiving = false;
		set_receive();
	}

private:
	std::shared_ptr<asio::io_service> m_io_service;

	std::string m_remote_address;
	uint16_t m_remote_port;

	std::atomic<bool> m_started{ false };
	std::atomic<bool> m_is_sending{ false };
	std::atomic<bool> m_is_receiving{ false };
	std::atomic<bool> m_is_terminated{ false };

	std::vector<uint8_t> m_receive_buffer;

	std::shared_ptr<asio::ip::tcp::socket> m_downstream_socket;
};

class tcp_echo_server
	: public std::enable_shared_from_this<tcp_echo_server>
{
public:
	explicit tcp_echo_server(std::shared_ptr<asio::io_service> service)
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

		// Prepare endpoint
		m_endpoint = std::make_shared<asio::ip::tcp::endpoint>(asio::ip::make_address(address), port);
		m_acceptor = std::make_shared<asio::ip::tcp::acceptor>(*m_io_service);

		m_acceptor->open(m_endpoint->protocol());
		m_acceptor->bind(*m_endpoint);

		m_acceptor->listen();

		set_accept();
	}

	void set_accept()
	{
		if (!m_started || m_accepting)
		{
			PLOGE << "is accepting";
			return;
		}

		m_accepting = true;

		auto self(this->shared_from_this());

		const auto downstream_socket = std::make_shared<tcp_downstream>(m_io_service);
		auto bounded_function = [self, downstream_socket](const std::error_code error)
			{
				self->handler_accept(downstream_socket, error);
			};

		PLOGD << "create and try listen new downstream - ";
		PLOGD << "endpoint " << m_acceptor->local_endpoint().address().to_v4().to_string() << ":" << m_acceptor->local_endpoint().port();

		m_acceptor->async_accept(*downstream_socket->socket(), bounded_function);
	}

	void handler_accept(const std::shared_ptr<tcp_downstream>& downstream_socket, const std::error_code& error)
	{
		if (error)
		{
			PLOGE << "code: " << error.value() << " - message: " << error.message();
			terminate();
			return;
		}

		m_accepting = false;

		downstream_socket->start();
		PLOGD << "on accept - remote_endpoint " << downstream_socket->socket()->remote_endpoint();

		set_accept();
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
		self->m_is_terminated = false;
		self->m_accepting = false;
		self->m_started = false;

		self->m_endpoint.reset();
		self->m_acceptor.reset();

		self->listen(self->m_local_address, self->m_local_port);
	}

private:
	std::shared_ptr<asio::ip::tcp::endpoint> m_endpoint;
	std::shared_ptr<asio::ip::tcp::acceptor> m_acceptor;

	std::shared_ptr<asio::io_service> m_io_service;

	std::string m_local_address;
	uint16_t m_local_port;

	std::atomic<bool> m_started{ false };
	std::atomic<bool> m_accepting{ false };
	std::atomic<bool> m_is_terminated{ false };
};

int main()
{
	static plog::ColorConsoleAppender<plog::TxtFormatter> console_appender;
	init(plog::verbose, &console_appender);

	PLOGD << "started plog verbose";

	io_service = std::make_shared<asio::io_service>();

	const auto current_server = std::make_shared<tcp_echo_server>(io_service);
	PLOGD << "created tcp_echo_server class";

	current_server->listen("0.0.0.0", 7171);

	service_thread(io_service);

	PLOGD << "started io_service";
	return 0;
}
