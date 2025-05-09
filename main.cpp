#include "./server.hpp"
#include "./ConfigParsing.hpp"

int main(int argc, char **argv)
{
    if (argc != 2)
    {
		std::cerr << "Error\n  Usage : ./webserv <config file>" << std::endl;
		return EXIT_FAILURE;
    }
    ConfigParsing configParser;
    try{
        configParser.start(argv[1]);
    } catch (const std::exception& e) {
        std::cerr << e.what();
        return EXIT_FAILURE;
    }
    Server *server = new Server();
    server->configData = configParser.configData;
    server->configData[0].printData();
    if (server->startServer() == EXIT_FAILURE)
        return EXIT_FAILURE;
    
    while (true)
    {
        if (server->handleClientConnections() == EXIT_FAILURE)
            break;
    }

    delete server;
    return EXIT_SUCCESS;
}
