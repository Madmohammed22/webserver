#include "server.hpp"

void Server::handleNewConnection(int fd)
{
    int conn_sock = accept(fd, NULL, NULL);
    if (conn_sock == -1)
    {
        std::cerr << "accept failed" << std::endl;
        return;
    }

    setnonblocking(conn_sock);

    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLOUT;
    ev.data.fd = conn_sock;

    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, conn_sock, &ev) == -1)
    {
        std::cerr << "epoll_ctl: conn_sock failed" << std::endl;
        close(conn_sock);
        return;
    }

    clientToServer[conn_sock] = fd;
    request[conn_sock] = Request();
    request[conn_sock].state.last_activity_time = time(NULL);
    request[conn_sock].state = FileTransferState();
    request[conn_sock].state.file = new std::ofstream();
    request[conn_sock].state.file->open("TMP", std::ios::binary);
}

void Server::handleClientData(int fd)
{
    FileTransferState &state = request[fd].state;
    state.fd = fd;

    Binary_String holder(CHUNK_SIZE + 1);
    ssize_t bytes = recv(fd, &holder[0], holder.length() - 1, 0);
    if (bytes <= 0)
    {
        close(fd);
        request.erase(fd);
        clientToServer.erase(fd);
        return;
    }

    holder[bytes] = '\0';

    if (!state.headerFlag && !state.isComplete)
    {
        int serverSocket = clientToServer[fd];
        ConfigData serverConfig = getConfigForRequest(multiServers[serverSocket], request[fd].getHost());
        state.buffer.append(holder, 0, bytes);
        if (validateHeader(fd, state, serverConfig) == false)
        {
            getSpecificRespond(fd, serverConfig.getErrorPages().find(400)->second, createBadRequest);
        }
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

void Server::handleClientOutput(int fd)
{
    if (request[fd].state.isComplete)
    {
        int serverSocket = clientToServer[fd];
        ConfigData serverConfig = getConfigForRequest(multiServers[serverSocket], request[fd].getHost());
        if (request[fd].getMethod() == "GET")
        {
            std::cout << "-------( REQUEST PARSED )-------\n\n";
            std::cout << request[fd].header << std::endl;
            std::cout << "-------( END OF REQUEST )-------\n\n\n";
            int state = serve_file_request(fd, serverConfig);
            if (state == 310)
            {
                close(fd);
                request.erase(fd);
                return;
            }
            return;
        }
        else if (request[fd].getMethod() == "DELETE")
        {
            std::cout << "-------( REQUEST PARSED )-------\n\n";
            std::cout << request[fd].header << std::endl;
            std::cout << "-------( END OF REQUEST )-------\n\n\n";
            handle_delete_request(fd, serverConfig);
        }
        else if (request[fd].getMethod() == "POST")
        {
            //[soukaina] here i should check for the post if an error responce is sent
            if (request[fd].state.PostHeaderIsValid == false && parsePostRequest(fd, serverConfig) != 0)
                request[fd].state.PostHeaderIsValid = true;
            handlePostRequest(fd);
        }
    }
}

int Server::handleClientConnectionsForMultipleServers()
{
    struct epoll_event events[MAX_EVENTS];
    int nfds = epoll_wait(epollfd, events, MAX_EVENTS, TIMEOUTMS);

    for (int i = 0; i < nfds; ++i)
    {
        int fd = events[i].data.fd;

        if (multiServers.find(fd) != multiServers.end())
        {
            handleNewConnection(fd);
        }
        else if (events[i].events & EPOLLIN)
        {
            handleClientData(fd);
        }
        else if (events[i].events & EPOLLOUT)
        {
            handleClientOutput(fd);
        }
    }
    return EXIT_SUCCESS;
}
