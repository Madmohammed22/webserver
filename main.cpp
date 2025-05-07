#include "./server.hpp"


int main()
{
    Server *server = new Server();
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
