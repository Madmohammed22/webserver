#include "../inc/server.hpp"

void Server::initServer()
{
    sockaddr_in clientAddress;
    socklen_t clientLen = sizeof(clientAddress);
    int listen_sock, epollfd;
    struct epoll_event ev;

    if ((listen_sock = establishingServer()) == EXIT_FAILURE)
         exit(EXIT_FAILURE);

    if ((epollfd = epoll_create1(0)) == -1)
    {
        std::cout << "Failed to create epoll file descriptor" << std::endl,
        close(listen_sock);
        exit (EXIT_FAILURE);
    }

    ev.events = EPOLLIN | EPOLLOUT;
    ev.data.fd = listen_sock;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listen_sock, &ev) == -1)
    {
        std::cerr << "Failed to add file descriptor to epoll" << std::endl,
        close(listen_sock);
        close(epollfd);
        exit(EXIT_FAILURE);
    }

    while (true)
    {
        if (handleClientConnections(listen_sock, ev, clientAddress, epollfd, clientLen) == EXIT_FAILURE)
            break;
    }

    /*for (std::map<int, FileTransferState>::iterator it = server->fileTransfers.begin(); it != server->fileTransfers.end(); ++it)*/
    /*    close(it->first);*/
    /*server->fileTransfers.clear();*/
    /*close(listen_sock);*/
    /*if (close(epollfd) == -1)*/
    /*    return std::cerr << "Failed to close epoll file descriptor" << std::endl, delete server, EXIT_FAILURE;*/
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


int Server::handleClientConnections(int listen_sock, struct epoll_event &ev, sockaddr_in &clientAddress, int epollfd, socklen_t &clientLen)
{
    int conn_sock;
    binaryString holder(CHUNK_SIZE);
    std::string request;
    struct epoll_event events[MAX_EVENTS];
    int nfds;

    if ((nfds = epoll_wait(epollfd, events, MAX_EVENTS, TIMEOUTMS)) == -1)
        return std::cerr << "epoll_wait" << std::endl, EXIT_FAILURE;
    if (nfds == 0)
        return 0;
    std::map<int, Client> client;
    for (int i = 0; i < nfds; ++i)
    {
        if (events[i].data.fd == listen_sock)
        {
            conn_sock = accept(listen_sock, (struct sockaddr *)&clientAddress, &clientLen);
            if (conn_sock == -1)
                return std::cerr << "accept" << std::endl, close(conn_sock), 0;

            server->setnonblocking(conn_sock);
            ev.events = EPOLLIN | EPOLLOUT;
            ev.data.fd = conn_sock;            
            if (epoll_ctl(epollfd, EPOLL_CTL_ADD, conn_sock, &ev) == -1)
                return std::cerr << "epoll_ctl: conn_sock" << std::endl, EXIT_FAILURE;
        }

        else if (events[i].events & EPOLLIN)
        {
            int bytes = recv(events[i].data.fd, &holder[0], holder.length(), 0);

            if (bytes < 0)
            {
                return server->fileTransfers.erase(events[i].data.fd), close(events[i].data.fd), 0;
            }
            if (bytes == 0)
            {
                /*if (server->fileTransfers.find(events[i].data.fd) != server->fileTransfers.end())*/
                /*{*/
                    std::cout << "i have been here\n\n";
                    /*server->fileTransfers.erase(events[i].data.fd);*/
                /*}*/
                continue;
            }
            else
            {
                holder[bytes] = '\0';
                send_buffers[events[i].data.fd] = holder;
                /*for (int j = 0; j < bytes; j++)*/
                /*{*/
                /*  std::cout << holder[j] << std::ends;*/
                /*}*/
                bool isPostRequest = holder.to_string().find("POST") != std::string::npos;
                if (!isPostRequest)
                    std::cout << "true" << std::endl;
                bool isExistingTransfer = server->fileTransfers.find(events[i].data.fd) != server->fileTransfers.end();
                if (isPostRequest || isExistingTransfer)
                {
                    if (isPostRequest && !isExistingTransfer)
                    {
                        std::fstream* file = new std::fstream();
                        file->open("TMP", std::ios::out | std::ios::binary | std::ios::trunc);
                        if (!file->is_open())
                        {
                            std::cerr << "Failed to open file TMP\n";
                            delete file;
                            continue;
                        }
                        server->fileTransfers[events[i].data.fd].file = file;
                        server->fileTransfers[events[i].data.fd].file->write(holder.c_str(), bytes);
                        if (server->fileTransfers[events[i].data.fd].file->fail()) 
                        {
                          std::cerr << "File write failed\n";
                          server->fileTransfers.erase(events[i].data.fd);
                          continue;
                        }
                    }

                    if (isExistingTransfer && server->fileTransfers[events[i].data.fd].file)
                    {
                        server->fileTransfers[events[i].data.fd].file->write(holder.c_str(), bytes);
                        if (server->fileTransfers[events[i].data.fd].file->fail()) 
                        {
                          std::cerr << "File write failed\n";
                          server->fileTransfers.erase(events[i].data.fd);
                          continue;
                        }
                        std::cout << "``````````````" << bytes << "`````````````````````" << std::endl;
                        if (bytes < CHUNK_SIZE)
                        {
                            std::cout << "we have reached the final chunk!!";
                           server->fileTransfers[events[i].data.fd].file->close();
                           server->fileTransfers[events[i].data.fd].multp.flag = true;
                        }
                        server->fileTransfers[events[i].data.fd].file->flush();
                    }
                }
            }
        }
        else if (events[i].events & EPOLLOUT)
        {
            if (server->fileTransfers.find(events[i].data.fd) != server->fileTransfers.end() && server->fileTransfers[events[i].data.fd].multp.flag == true)
            {
                std::cout << "i was heregcc once\n";
                server->handlePostRequest(events[i].data.fd, server); 
            }
            /*if (server->fileTransfers.find(events[i].data.fd) != server->fileTransfers.end())*/
            /*{*/
            /*    request = send_buffers[events[i].data.fd].to_string();*/
            /*    std::pair<std::string, std::string> pair_request = ft_parseRequest_T(events[i].data.fd, server, request);*/
            /*    server->key_value_pair_header(events[i].data.fd, server, ft_parseRequest_T(events[i].data.fd, server, request).first);*/
            /*    std::string Connection = server->fileTransfers[events[i].data.fd].mapOnHeader.find("Connection:")->second;*/
            /*    if (server->continueFileTransfer(events[i].data.fd, server, Connection) == -1)*/
            /*        return std::cerr << "Failed to continue file transfer" << std::endl, close(events[i].data.fd), 0;*/
            /*    continue;*/
            /*}*/
            request = send_buffers[events[i].data.fd].to_string();
            
            if (request.find("DELETE") != std::string::npos)
            {
                server->handle_delete_request(events[i].data.fd, server, request);
            }
            else if (request.find("GET") != std::string::npos)
            {
                std::cout << "i was here\n";
                server->key_value_pair_header(events[i].data.fd, server, ft_parseRequest_T(events[i].data.fd, server, request).first);
                server->serve_file_request(events[i].data.fd, server, request, client);
            }
            else if (request.find("PUT") != std::string::npos || request.find("PATCH") != std::string::npos || request.find("HEAD") != std::string::npos || request.find("OPTIONS") != std::string::npos)
            {
                server->processMethodNotAllowed(events[i].data.fd, server, request);
            }
            /*if (server->fileTransfers.find(events[i].data.fd) == server->fileTransfers.end())*/
            /*{*/
            /*    send_buffers.erase(events[i].data.fd);*/
            /*}*/
        }
    }
    return EXIT_SUCCESS;
}

