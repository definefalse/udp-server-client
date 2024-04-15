#ifndef UDP_SERVER_H
#define UDP_SERVER_H

#include <array>
#include <atomic>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <boost/thread.hpp>

class UdpServer;

struct UdpSession final : std::enable_shared_from_this<UdpSession>
{
    UdpSession(UdpServer* server)
        : m_server(server)
    {}

    void handle_request(const boost::system::error_code& error, const std::size_t bytes_received);

    boost::asio::ip::udp::endpoint m_remote_endpoint;
    std::array<char, 1024> m_recv_buffer;
    std::string message;
    std::size_t bytes_received;
    UdpServer* m_server;

private:
    std::string get_info(const std::string& resource_name) const;
    std::string get_error(std::string_view explanation) const;
};

class UdpServer final
{
    typedef std::shared_ptr<UdpSession> shared_session;
    using resource_info_type = std::unordered_map<std::string, std::string>;

public:
    explicit UdpServer(boost::asio::io_context& io_context, std::size_t port, const resource_info_type& info);
    ~UdpServer();

    void enqueue_response(const shared_session& session);

    void enqueue_response_strand(const shared_session& session);

    void handle_sent(const shared_session& session, const boost::system::error_code& ec, std::size_t);

    void stop();

    const resource_info_type& get_resources_info() const;

private:
    void receive_session();

    void handle_receive(shared_session session, const boost::system::error_code& ec, std::size_t bytes_received);

    boost::asio::io_service& m_io_context;
    boost::asio::ip::udp::socket m_socket;
    boost::asio::io_context::strand m_strand;

    std::unordered_map<std::string, std::string> m_server_resource_info;

    std::atomic<int> m_pending_send_count{0};
    friend struct udp_session;
};

#endif // UDP_SERVER_H
