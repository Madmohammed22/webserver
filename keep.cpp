// Fix for establishingMultiServer - no changes needed, function looks good

// Fix for startMultipleServers - Logic is inverted for the listen_sock check
int Server::startMultipleServers(ConfigData configData)
{
    int listen_sock = 0;
    int epollfd = 0;
    struct epoll_event ev;

    // Call establishingMultiServer and check if it failed
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

// Modified main function to properly handle multiple server configurations
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
    
    // Initialize epoll once for all servers
    server->epollfd = epoll_create1(0);
    if (server->epollfd == -1)
    {
        std::cerr << "Failed to create epoll file descriptor" << std::endl;
        delete server;
        return EXIT_FAILURE;
    }

    // Start all configured servers
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

    // Main event loop
    while (true)
    {
        if (server->handleClientConnectionsForMultipleServers() == EXIT_FAILURE)
            break;
    }

    delete server;
    return EXIT_SUCCESS;
}

// Fix for handleClientConnectionsForMultipleServers to use the correct config for each connection
int Server::handleClientConnectionsForMultipleServers()
{
    int conn_sock;
    Binary_String holder(CHUNK_SIZE);
    struct epoll_event events[MAX_EVENTS];
    int nfds;

    nfds = epoll_wait(epollfd, events, MAX_EVENTS, TIMEOUTMS);
    if (nfds == -1)
        return std::cerr << "epoll_wait failed" << std::endl, EXIT_FAILURE;
    if (nfds == 0)
        return EXIT_SUCCESS;  // Return success if no events, not 0

    for (int i = 0; i < nfds; ++i)
    {
        int fd = events[i].data.fd;
        
        // Check if this is a listening socket (server socket)
        if (multiServers.find(fd) != multiServers.end())
        {
            // This is a server socket, accept the connection
            ConfigData& serverConfig = multiServers[fd]; // Get the config for this server
            
            conn_sock = accept(fd, NULL, NULL);
            if (conn_sock == -1)
            {
                std::cerr << "accept failed" << std::endl;
                continue;
            }

            setnonblocking(conn_sock);
            
            struct epoll_event ev;
            ev.events = EPOLLIN | EPOLLOUT;
            ev.data.fd = conn_sock;
            
            if (epoll_ctl(epollfd, EPOLL_CTL_ADD, conn_sock, &ev) == -1)
            {
                std::cerr << "epoll_ctl: conn_sock failed" << std::endl;
                close(conn_sock);
                continue;
            }
            
            // Store which server this connection belongs to
            clientToServer[conn_sock] = fd;
            
            // Initialize request for this connection
            request[conn_sock] = Request();
            request[conn_sock].state = FileTransferState();
            request[conn_sock].state.file = new std::ofstream();
            request[conn_sock].state.file->open("TMP", std::ios::binary);
            request[conn_sock].state.file->close();
        }
        else if (events[i].events & EPOLLIN)
        {
            // Handle incoming data
            FileTransferState &state = request[fd].state;
            state.fd = fd;

            ssize_t bytes = recv(fd, &holder[0], holder.length(), 0);
            if (bytes <= 0)
            {
                close(fd);
                request.erase(fd);
                clientToServer.erase(fd); // Clean up the mapping
                continue;
            }

            holder[bytes] = '\0';

            if (!state.headerFlag && !state.isComplete)
            {
                state.buffer.append(holder, 0, bytes);
                validateHeader(fd, state);
            }
            else if (!state.isComplete)
            {
                state.file->write(holder.c_str(), bytes);
                state.bytesReceived += bytes;
                
                if (static_cast<int>(atoi(request[fd].contentLength.c_str())) <= state.bytesReceived)
                {
                    state.file->close();
                    state.isComplete = true;
                }
            }
        }
        else if (events[i].events & EPOLLOUT)
        {
            if (request[fd].state.isComplete)
            {
                // Get the server socket this client is connected to
                int serverSocket = clientToServer[fd];
                // Get the config for that server
                ConfigData& serverConfig = multiServers[serverSocket];
                
                if (request[fd].getMethod() == "GET")
                    serve_file_request(fd, serverConfig);
                else if (request[fd].getMethod() == "DELETE")
                    handle_delete_request(fd, serverConfig); // You might need to update this function to accept config
            }
        }
    }

    return EXIT_SUCCESS;
}

// Add these to your Server class declaration
class Server {
    // ... existing members
private:
    std::map<int, int> clientToServer; // Maps client sockets to their server sockets
    // ... rest of your class
};
