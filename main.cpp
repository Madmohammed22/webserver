#include "./server.hpp"
#include "./ConfigParsing.hpp"
int Server::establishingMultiServer(ConfigData configData)
{
    int serverSocket = 0;
    serverSocket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, getprotobyname("tcp")->p_proto);
    if (serverSocket < 0)
        return std::cerr << "Error opening stream socket." << std::endl, EXIT_FAILURE;

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(configData.getPort());
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    int len = sizeof(serverAddress);
    int a = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEPORT, &a, sizeof(int)) < 0)
        return perror("setsockopt failed"), EXIT_FAILURE;
    if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1)
        return perror("binding stream socket"), EXIT_FAILURE;

    if (getsockname(serverSocket, (struct sockaddr *)&serverAddress, (socklen_t *)&len) == -1)
        return perror("getting socket name"), EXIT_FAILURE;
    std::cout << "Socket port " << ntohs(serverAddress.sin_port) << std::endl;

    if (listen(serverSocket, 5) < 0)
        return perror("listen stream socket"), EXIT_FAILURE;
    return serverSocket;
}

int Server::startMultipleServers(ConfigData configData)
{
    int listen_sock = 0;
    int epollfd = 0;
    struct epoll_event ev;

    if ((listen_sock = establishingMultiServer(configData)) == EXIT_FAILURE)
        return EXIT_FAILURE;

    if ((epollfd = epoll_create1(0)) == -1)
    {
        std::cout << "Failed to create epoll file descriptor" << std::endl;
        close(listen_sock);
        return EXIT_FAILURE;
    }

    ev.events = EPOLLIN | EPOLLOUT;
    ev.data.fd = listen_sock;

    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listen_sock, &ev) == -1)
    {
        std::cerr << "Failed to add file descriptor to epoll" << std::endl;
        close(listen_sock);
        close(epollfd);
        return EXIT_FAILURE;
    }

    this->listen_sock = listen_sock;
    this->epollfd = epollfd;
    this->multiServers[listen_sock] = configData;
    this->sockets.push_back(listen_sock);
    
    return EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        std::cerr << "Error\n Usage : ./webserv <config file>" << std::endl;
        return EXIT_FAILURE;
    }

    ConfigParsing configParser;
    try
    {
        configParser.start(argv[1]);
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what();
        return EXIT_FAILURE;
    }

    Server *server = new Server();
    server->configData = configParser.configData;
    
    server->epollfd = epoll_create1(0);
    if (server->epollfd == -1)
    {
        std::cerr << "Failed to create epoll file descriptor" << std::endl;
        delete server;
        return EXIT_FAILURE;
    }

    bool allServersStarted = true;
    for (size_t i = 0; i < server->configData.size(); i++)
    {
        if (server->startMultipleServers(server->configData[i]) == EXIT_FAILURE)
        {
            std::cerr << "Failed to start server on port " << server->configData[i].getPort() << std::endl;
            allServersStarted = false;
            break;
        }
        std::cout << "Server started on port " << server->configData[i].getPort() << std::endl;
    }

    if (!allServersStarted)
    {
        delete server;
        return EXIT_FAILURE;
    }

    while (true)
    {
        if (server->handleClientConnectionsForMultipleServers() == EXIT_FAILURE)
            break;
    }

    delete server;
    return EXIT_SUCCESS;
}
