#include "./server.hpp"
#include "./ConfigParsing.hpp"

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

    server->epollfd = epoll_create1(0);
    if (server->epollfd == -1) {
        perror("Failed to create epoll file descriptor");
        delete server;
        return EXIT_FAILURE;
    }

    for (size_t i = 0; i < server->configData.size(); i++) {
        if (server->startMultipleServers(server->configData[i]) == EXIT_FAILURE) {
            std::cerr << "Failed to start server on port " << server->configData[i].getPort() << std::endl;
            delete server;
            return EXIT_FAILURE;
        }
    }

    while (true) {
        struct epoll_event events[MAX_EVENTS];
        int nfds = epoll_wait(server->epollfd, events, MAX_EVENTS, TIMEOUTMS);
        if (nfds == -1) {
            perror("epoll_wait failed");
            break;
        }

        if (nfds == 0) {
            continue;
        }

        for (int i = 0; i < nfds; ++i) {
            int fd = events[i].data.fd;

            if (server->multiServers.find(fd) != server->multiServers.end()) {
                // Handle new connection
                int conn_sock = accept(fd, NULL, NULL);
                if (conn_sock == -1) {
                    perror("accept failed");
                    continue;
                }

                setnonblocking(conn_sock);

                struct epoll_event ev;
                ev.events = EPOLLIN | EPOLLOUT;
                ev.data.fd = conn_sock;

                if (epoll_ctl(server->epollfd, EPOLL_CTL_ADD, conn_sock, &ev) == -1) {
                    perror("epoll_ctl: conn_sock failed");
                    close(conn_sock);
                    continue;
                }

                server->clientToServer[conn_sock] = fd;
                server->request[conn_sock] = Request();
                server->request[conn_sock].state = FileTransferState();
                server->request[conn_sock].state.file = new std::ofstream();
                server->request[conn_sock].state.file->open("TMP", std::ios::binary);
                server->request[conn_sock].state.file->close();
            } else if (events[i].events & EPOLLIN) {
                // Handle client data
                FileTransferState &state = server->request[fd].state;
                state.fd = fd;

                Binary_String holder(CHUNK_SIZE);
                ssize_t bytes = recv(fd, &holder[0], holder.length(), 0);
                if (bytes <= 0) {
                    close(fd);
                    server->request.erase(fd);
                    server->clientToServer.erase(fd);
                    continue;
                }

                holder[bytes] = '\0';

                if (!state.headerFlag && !state.isComplete) {
                    state.buffer.append(holder, 0, bytes);
                    validateHeader(fd, state);
                } else if (!state.isComplete) {
                    state.file->write(holder.c_str(), bytes);
                    state.bytesReceived += bytes;

                    if (static_cast<size_t>(atoi(server->request[fd].contentLength.c_str())) <= state.bytesReceived) {
                        state.file->close();
                        state.isComplete = true;
                    }
                }
            } else if (events[i].events & EPOLLOUT) {
                // Handle client output
                if (server->request[fd].state.isComplete) {
                    int serverSocket = server->clientToServer[fd];
                    ConfigData& serverConfig = server->multiServers[serverSocket];

                    if (server->request[fd].getMethod() == "GET")
                        serve_file_request(fd, serverConfig);
                    else if (server->request[fd].getMethod() == "DELETE")
                        handle_delete_request(fd);
                }
            } else {
                std::cerr << "Unknown event" << std::endl;
            }
        }
    }

    delete server;
    return EXIT_SUCCESS;
}