#include "./server.hpp"
#include "./ConfigParsing.hpp"

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Error\n Usage: ./webserv <config file>" << std::endl;
        return EXIT_FAILURE;
    }
    
    ConfigParsing configParser;
    try {
        configParser.start(argv[1]);
    } catch (const std::exception &e) {
        std::cerr << "Configuration parsing failed: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    Server *server = new Server();
    server->configData = configParser.configData;
    server->getListenPairs();
    server->epollfd = epoll_create1(0);
    if (server->epollfd == -1) {
        perror("Failed to create epoll file descriptor");
        delete server;
        return EXIT_FAILURE;
    }

    for (size_t i = 0; i < server->listenVec.size(); i++) {
        if (server->startMultipleServers(server->listenVec[i]) == EXIT_FAILURE) {
            std::cerr << "Failed to start server on port " << server->configData[i].getPort() << std::endl;
            delete server;
            return EXIT_FAILURE;
        }
    }

    while(true){
        if (server->handleClientConnectionsForMultipleServers() == EXIT_FAILURE) {
            break;
        }
    }
    delete server;
    return EXIT_SUCCESS;
}
