#include "server.hpp"

int Server::handleClientConnections()
{
    int conn_sock;
    Binary_String holder(CHUNK_SIZE);
    struct epoll_event events[MAX_EVENTS];
    int nfds;

    nfds = epoll_wait(epollfd, events, MAX_EVENTS, TIMEOUTMS);
    if (nfds == -1)
        return std::cerr << "epoll_wait failed" << std::endl, EXIT_FAILURE;
    if (nfds == 0)
        return 0;

    for (int i = 0; i < nfds; ++i)
    {
        int fd = events[i].data.fd;

        if (fd == listen_sock) 
        {
            conn_sock = accept(listen_sock, NULL, NULL);
            if (conn_sock == -1)
            {
                std::cerr << "accept failed" << std::endl;
                continue;
            }

            setnonblocking(conn_sock); 
            ev.events = EPOLLIN | EPOLLOUT;
            ev.data.fd = conn_sock;
            if (epoll_ctl(epollfd, EPOLL_CTL_ADD, conn_sock, &ev) == -1)
            {
                std::cerr << "epoll_ctl: conn_sock failed" << std::endl;
                close(conn_sock);
                continue;
            }
            request[conn_sock] = Request();
            request[conn_sock].state = FileTransferState();
            request[conn_sock].state.file = new std::ofstream();
            request[conn_sock].state.file->open("TMP", std::ios::binary);
            request[conn_sock].state.file->close();
        }
        else if (events[i].events & EPOLLIN)
        {
            FileTransferState &state = request[fd].state;
            state.fd = fd;
            ssize_t bytes = recv(fd, &holder[0], holder.length(), 0);

            if (bytes <= 0)
            {
                close(fd);   
                request.erase(fd);
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
                if (request[fd].getMethod() == "GET")
                    serve_file_request(fd, configData[0]);
                else if (request[fd].getMethod() == "DELETE")
                    handle_delete_request(fd);
            }
        }
    }
    return EXIT_SUCCESS;
}