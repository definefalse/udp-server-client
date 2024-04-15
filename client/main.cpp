#include <array>
#include <iostream>

#include <boost/asio.hpp>
#include <boost/program_options.hpp>

int main(int argc, char* argv[])
{
    using boost::asio::ip::udp;
    namespace po = boost::program_options;
    po::options_description desc{"Usage: ./client --port <port> --resource <name>."};
    desc.add_options()("port", po::value<int>(), "Port to send data")(
        "resource", po::value<std::string>(), "Name of resource to get info about."
    );
    po::variables_map vm;
    po::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (!vm.count("port") && !vm.count("resource")) {
        std::cout << desc << std::endl;
        return 0;
    }

    const auto port = static_cast<boost::asio::ip::port_type>(vm["port"].as<int>());
    const auto resource = vm["resource"].as<std::string>();

    try {
        boost::asio::io_context io_context;

        udp::resolver resolver(io_context);
        udp::endpoint receiver_endpoint{boost::asio::ip::make_address("127.0.0.1"), port};

        udp::socket socket(io_context, receiver_endpoint.protocol());

        socket.send_to(boost::asio::buffer(resource), receiver_endpoint);

        std::array<char, 1024> recv_buf;
        udp::endpoint sender_endpoint;
        size_t len = socket.receive_from(boost::asio::buffer(recv_buf), sender_endpoint);

        std::cout.write(recv_buf.data(), len);
    } catch (const boost::system::system_error& e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
