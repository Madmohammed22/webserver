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

int Server::establishingServer()
{
    int serverSocket = 0;
    serverSocket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, getprotobyname("tcp")->p_proto);
    if (serverSocket < 0)
        return std::cerr << "Error opening stream socket." << std::endl, EXIT_FAILURE;

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT);
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

int Server::startServer()
{
    int listen_sock, epollfd;
    struct epoll_event ev;

    if ((listen_sock = establishingServer()) == EXIT_FAILURE)
        return EXIT_FAILURE;

    std::cout << "Server is listening\n"
              << std::endl;
    if ((epollfd = epoll_create1(0)) == -1)
    {
        return std::cout << "Failed to create epoll file descriptor" << std::endl,
               close(listen_sock), EXIT_FAILURE;
    }

    ev.events = EPOLLIN | EPOLLOUT;
    ev.data.fd = listen_sock;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listen_sock, &ev) == -1)
    {
        return std::cerr << "Failed to add file descriptor to epoll" << std::endl,
               close(listen_sock), close(epollfd), EXIT_FAILURE;
    }
    this->listen_sock = listen_sock;
    this->epollfd = epollfd;
    return EXIT_SUCCESS;
}
