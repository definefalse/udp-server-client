#include "udp_server.h"

#include <chrono>
#include <csignal>
#include <iostream>
#include <signal.h>
#include <thread>
#include <unordered_map>

#include <boost/asio.hpp>
#include <boost/foreach.hpp>
#include <boost/json.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

auto running{true};

void signal_handler(int signal)
{
    std::cout << "Signal received: " << signal << "\n";
    switch (signal) {
    case SIGHUP:
    case SIGINT:
    case SIGKILL:
    case SIGTERM:
    case SIGABRT:
        running = false;
        break;
    }
}

void register_signals()
{
    std::signal(SIGCHLD, SIG_IGN);
    std::signal(SIGHUP, signal_handler);
    std::signal(SIGKILL, signal_handler);
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    std::signal(SIGABRT, signal_handler);
}

void wait()
{
    while (running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

std::unordered_map<std::string, std::string> parse_input_file(const std::string& path)
{
    try {
        boost::property_tree::ptree json_file;
        boost::property_tree::json_parser::read_json(path, json_file);
        std::unordered_map<std::string, std::string> server_resources_info;
        for (auto& [k, v] : json_file) {
            server_resources_info.emplace(k, v.get_value<std::string>());
        }
        return server_resources_info;
    } catch (const boost::property_tree::json_parser_error& ex) {
        std::cout << ex.what() << std::endl;
    }

    return {};
}

int main(int argc, char* argv[])
{
    namespace po = boost::program_options;
    po::options_description desc{"Usage: ./server <port> <resource_info_path.json."};
    desc.add_options()("port", po::value<int>(), "server port")(
        "path", po::value<std::string>(), "path to resource_file. Expecting json file"
    );
    po::variables_map vm;
    po::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (!vm.count("path") && !vm.count("port")) {
        std::cout << desc << std::endl;
        return 0;
    }

    const auto path = vm["path"].as<std::string>();
    const auto server_resource_info = parse_input_file(path);
    if (server_resource_info.empty()) {
        std::cout << "Empty resource info.\n";
        return 0;
    }
    const auto port = static_cast<std::size_t>(vm["port"].as<int>());

    std::cout << "Creating server.\n";

    register_signals();
    boost::asio::io_context io_context;

    boost::thread_group group;
    try {
        UdpServer server(io_context, port, server_resource_info);
        for (std::size_t i = 0; i < boost::thread::hardware_concurrency(); ++i)
            group.create_thread(bind(&boost::asio::io_context::run, boost::ref(io_context)));
        wait();
        server.stop();
        group.join_all();
    } catch (const boost::thread_interrupted& ex) {
        std::cout << "Thread interrupted\n";
    } catch (const boost::system::system_error& ex) {
        std::cout << ex.what();
    } catch (...) {
        std::cout << "Unknown exception\n";
    }

    return 0;
}
