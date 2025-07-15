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


int Server::establishingMultiServer(t_listen listen_)
{
    int serverSocket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, getprotobyname("tcp")->p_proto);
    if (serverSocket < 0)
    {
        perror("Error opening stream socket");
        return EXIT_FAILURE;
    }

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(listen_.port);
    serverAddress.sin_addr.s_addr = inet_addr(listen_.host.c_str()); ;

    int a = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEPORT, &a, sizeof(int)) < 0)
    {
        perror("setsockopt failed");
        close(serverSocket);
        return EXIT_FAILURE;
    }

    if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1)
    {
        perror("binding stream socket");
        close(serverSocket);
        return EXIT_FAILURE;
    }

    if (listen(serverSocket, 5) < 0)
    {
        perror("listen stream socket");
        close(serverSocket);
        return EXIT_FAILURE;
    }

    return serverSocket;
}

int Server::startMultipleServers(t_listen listen)
{
    int listen_sock = establishingMultiServer(listen);
    if (listen_sock == EXIT_FAILURE)
    {
        return EXIT_FAILURE;
    }

    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLOUT;
    ev.data.fd = listen_sock;

    if (epoll_ctl(this->epollfd, EPOLL_CTL_ADD, listen_sock, &ev) == -1)
    {
        perror("Failed to add file descriptor to epoll");
        close(listen_sock);
        return EXIT_FAILURE;
    }

    this->multiServers[listen_sock] = listen;
    this->sockets.push_back(listen_sock);

    return EXIT_SUCCESS;
}

void Server::getListenPairs()
{
    std::vector<ConfigData>::iterator it;
    t_listen newListen;

    for (it = configData.begin(); it != configData.end(); ++it)
    {
        newListen.host = it->getHost();
        newListen.port = it->getPort();
        std::vector<t_listen>::iterator itListen;
        for (itListen = listenVec.begin(); itListen != listenVec.end(); itListen++)
        {
            if (it->getHost() == itListen->host && it->getPort() == itListen->port)
                break;
        }
        if (itListen == listenVec.end())
            listenVec.push_back(newListen);
    }
    
    std::vector<t_listen>::iterator itListen;
    for (itListen = listenVec.begin(); itListen != listenVec.end(); itListen++)
    {
        std::cout << "The host " << itListen->host << " the port " << itListen->port << "" <<  std::endl;
    }
    std::cout << "\n";
}

ConfigData Server::getConfigForRequest(t_listen listen, std::string serverName)
{
    std::vector<ConfigData> configList;
    std::vector<ConfigData>::iterator it;

    for (it = configData.begin(); it != configData.end(); ++it)
    {
        if (it->getHost() == listen.host && it->getPort() == listen.port)
            configList.push_back(*it);
    }

    if (serverName.empty())
        return configList[0];

    for (it = configList.begin(); it != configList.end(); ++it)
    {
        if (strcasecmp(it->getServerName().c_str(), serverName.c_str()) == 0)
            return *it;
    }
    return configList[0];
}

void Server::writePostDataToCgi(Request& req)
{
    req.multp.file = new std::ifstream(req.state.fileName.c_str(), std::ios::binary);
    if (!req.multp.file->is_open())
    {
        req.code = 500;
        cleanupResources(req);
        return ;
    }

    char buffer[CHUNK_SIZE];
    while(true)
    {
      req.multp.file->read(buffer, CHUNK_SIZE);

      int bytesRead = req.multp.file->gcount();

      if (bytesRead > 0)
      {
         int bytesSent = write(req.cgi.fdIn, buffer, bytesRead);
         if (bytesSent < 0)
         {
           std::cerr << "write: fork failed\n";
           req.code = 500;
           cleanupResources(req);
         }
    
         req.multp.readPosition += bytesRead;
         req.cgi.bytesSend += bytesSent;
      }
       
      if (bytesRead == 0 || req.cgi.bytesSend >= static_cast<int>(atoi(req.contentLength.c_str())))
      {
          cleanupResources(req);
          break;
      }
   }
}

void Server::cleanupResources(Request& req)
{
    if (req.multp.file)
    {
        req.multp.file->close();
        remove(req.state.fileName.c_str());
        delete req.multp.file;
    }
}

