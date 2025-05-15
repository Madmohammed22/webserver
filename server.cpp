#include "server.hpp"

Server::Server()
{
    this->pageNotFound = 0;
    this->LARGE_FILE_THRESHOLD = 1024 * 1024;
    this->listen_sock = 0;
    this->epollfd = 0;
    this->ev.events = 0;
}

Server::Server(const Server &Init)
{
    this->pageNotFound = Init.pageNotFound;
    this->LARGE_FILE_THRESHOLD = Init.LARGE_FILE_THRESHOLD;
    this->listen_sock = Init.listen_sock;
    this->epollfd = Init.epollfd;
    this->request = Init.request;
    this->ev = Init.ev;
}

Server &Server::operator=(const Server &Init)
{
    if (this == &Init)
    {
        return *this;
    }
    this->pageNotFound = Init.pageNotFound;
    this->LARGE_FILE_THRESHOLD = Init.LARGE_FILE_THRESHOLD;
    this->listen_sock = Init.listen_sock;
    this->epollfd = Init.epollfd;
    this->request = Init.request;
    this->ev = Init.ev;
    return *this;
}

Server::~Server()
{
    request.clear();
    if (close(listen_sock) == -1)
        std::cerr << "Failed to close listen socket" << std::endl;
    if (close(epollfd) == -1)
        std::cerr << "Failed to close epoll file descriptor" << std::endl;
    if (close(epollfd) == -1)
        std::cerr << "Failed to close epoll file descriptor" << std::endl;
    if (close(listen_sock) == -1)
        std::cerr << "Failed to close listen socket" << std::endl;

    std::cout << "[Server] Destructor is called" << std::endl;
}

void Server::setnonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        (perror("fcntl"), exit(EXIT_FAILURE));
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        perror("fcntl");
        exit(EXIT_FAILURE);
    }
}

int Server::establishingMultiServer(ConfigData configData) {
    int serverSocket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, getprotobyname("tcp")->p_proto);
    if (serverSocket < 0) {
        perror("Error opening stream socket");
        return EXIT_FAILURE;
    }

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(configData.getPort());
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    int a = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEPORT, &a, sizeof(int)) < 0) {
        perror("setsockopt failed");
        close(serverSocket);
        return EXIT_FAILURE;
    }

    if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1) {
        perror("binding stream socket");
        close(serverSocket);
        return EXIT_FAILURE;
    }

    if (listen(serverSocket, 5) < 0) {
        perror("listen stream socket");
        close(serverSocket);
        return EXIT_FAILURE;
    }

    std::cout << "Server started on port " << configData.getPort() << std::endl;
    return serverSocket;
}

int Server::startMultipleServers(ConfigData configData) {
    int listen_sock = establishingMultiServer(configData);
    if (listen_sock == EXIT_FAILURE) {
        return EXIT_FAILURE;
    }

    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLOUT;
    ev.data.fd = listen_sock;

    if (epoll_ctl(this->epollfd, EPOLL_CTL_ADD, listen_sock, &ev) == -1) {
        perror("Failed to add file descriptor to epoll");
        close(listen_sock);
        return EXIT_FAILURE;
    }

    this->multiServers[listen_sock] = configData;
    this->sockets.push_back(listen_sock);

    return EXIT_SUCCESS;
}