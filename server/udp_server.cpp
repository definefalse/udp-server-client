#include "udp_server.h"

#include <format>
#include <iostream>

namespace io = boost::asio;
namespace bs = boost::system;

namespace {
constexpr std::size_t MaxResourceNameSize = 64;
constexpr std::size_t MaxResponseSize = 1024;
} // namespace

namespace ServerReplies {
const std::string server_internal_error = "Internal server error.";
const std::string zero_bytes_received = "Received zero bytes.";
const std::string no_value_error = "No such resource name";
const std::string resource_name_size_exceeds = "Received resource name size is more than 64 bytes.";
const std::string resource_info_size_exceeds = "Resource info is more than 1024 bytes.";
} // namespace ServerReplies

void UdpSession::handle_request(const boost::system::error_code& error, const std::size_t bytes_received)
{
    if (error) {
        message = get_error(error.message());
    } else if (bytes_received == 0) {
        message = get_error(ServerReplies::zero_bytes_received);
    } else if (bytes_received > MaxResourceNameSize) {
        message = get_error(ServerReplies::resource_name_size_exceeds);
    } else {
        const std::string received_message(std::begin(m_recv_buffer), std::begin(m_recv_buffer) + bytes_received);

        message = get_info(received_message);
        if (message.size() > MaxResponseSize)
            message = get_error(ServerReplies::resource_info_size_exceeds);
    }
    /// передаем сессию в UdpServer для дальнейшей отправки клиенту
    m_server->enqueue_response(shared_from_this());
}

std::string UdpSession::get_info(const std::string& resource_name) const
{
    try {
        decltype(auto) resources_info = m_server->get_resources_info();

        if (auto it = resources_info.find(resource_name); it != resources_info.cend()) {
            return std::format("-BEGIN-\n{}\n-END-", it->second);
        }

        return std::format("-ERROR-\n{}\n-END-", ServerReplies::no_value_error);
    } catch (const std::bad_alloc& ex) {
        return ServerReplies::server_internal_error;
    }
}

std::string UdpSession::get_error(std::string_view explanation) const
{
    try {
        return std::format("-ERROR-\n{}\n-END-", explanation);
    } catch (const std::bad_alloc& ex) {
        return ServerReplies::server_internal_error;
    }
}

UdpServer::UdpServer(io::io_context& io_context, std::size_t port, const resource_info_type& info)
    : m_io_context(io_context)
    , m_socket(m_io_context, io::ip::udp::endpoint(io::ip::make_address("127.0.0.1"), port))
    , m_strand(m_io_context)
    , m_server_resource_info(info)
{
    std::cout << "Starting server...\n";
    receive_session();
}

UdpServer::~UdpServer()
{
    std::cout << "Server stoped...\n";
}

void UdpServer::receive_session()
{
    /// сессия для сохранения буффера и эндпоинта
    auto session = std::make_shared<UdpSession>(this);

    if (!m_is_stop && m_socket.is_open()) {
        m_socket.async_receive_from(
            boost::asio::buffer(session->m_recv_buffer),
            session->m_remote_endpoint,
            m_strand.wrap(boost::bind(
                &UdpServer::handle_receive, this, session, io::placeholders::error, io::placeholders::bytes_transferred
            ))
        );
    }
}

void UdpServer::handle_receive(shared_session session, const bs::error_code& ec, std::size_t bytes_received)
{
    if (!m_socket.is_open())
        return;

    if (m_socket.is_open()) {
        /// обрабатываем запрос в отдельном доступном из пула потоке
        io::post(m_socket.get_executor(), boost::bind(&UdpSession::handle_request, session, ec, bytes_received));
        m_pending_send_count++;
    } else if (ec != boost::asio::error::operation_aborted) {
        std::cout << "Error: " << ec.message() << std::endl;
    }
    /// продолжаем принимать запросы
    receive_session();
}

void UdpServer::enqueue_response(const shared_session& session)
{
    /// io::ip::udp::socket::async_send_to() не потокобезопасный, поэтому используем strand
    io::post(m_socket.get_executor(), m_strand.wrap(boost::bind(&UdpServer::enqueue_response_strand, this, session)));
}

void UdpServer::enqueue_response_strand(const shared_session& session)
{
    if (m_socket.is_open()) {
        m_socket.async_send_to(
            io::buffer(session->message),
            session->m_remote_endpoint,
            m_strand.wrap(boost::bind(
                &UdpServer::handle_sent, this, session, io::placeholders::error, io::placeholders::bytes_transferred
            ))
        );
    }
}

void UdpServer::handle_sent(const shared_session& session, const boost::system::error_code& ec, std::size_t)
{
    if (ec) {
        std::cout << "Error sending response to " << session->m_remote_endpoint << ": " << ec.message() << "\n";
    }
    m_pending_send_count--;
}

void UdpServer::stop()
{
    std::cout << "Stoping server...\n";
    m_is_stop = true;
    if (m_socket.is_open()) {
        while (m_pending_send_count != 0) {
        }

        try {
            m_socket.close();
        } catch (const boost::system::system_error& ex) {
            std::cout << "Exception: " << ex.what() << std::endl;
        }
    }
}

const UdpServer::resource_info_type& UdpServer::get_resources_info() const
{
    return m_server_resource_info;
}
