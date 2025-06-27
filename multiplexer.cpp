#include "server.hpp"
#include "helper/utils.hpp"

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
    request[conn_sock].state = FileTransferState();
    request[conn_sock].state.file = new std::ofstream();
    request[conn_sock].state.fileName = createTempFile();
    request[conn_sock].state.file->open(request[conn_sock].state.fileName.c_str(), std::ios::binary);
}

void Server::handleClientData(int fd)
{
    FileTransferState &state = request[fd].state;
    state.fd = fd;

    Binary_String holder(CHUNK_SIZE);
    size_t bytes = recv(fd, &holder[0], CHUNK_SIZE - 1, 0);

    holder[bytes + 1] = '\0';
    if (bytes <= 0)
    {
        close(fd);
        request.erase(fd);
        clientToServer.erase(fd);
        return;
    }

    if (state.headerFlag == true && !state.isComplete)
    {
        int serverSocket = clientToServer[fd];
        // i should rethink about this part cause this part the host is not yet setted
        // so there is a big probability that i am sending an empty string
        /*state.buffer.append(holder, 0, bytes);*/
        if ((state.bytesReceived = bytes) && validateHeader(fd, state, holder) == false)
        {
            // it did broke when it's called with post method
            ConfigData serverConfig = getConfigForRequest(multiServers[serverSocket], "");
            //[soukaina] here i should build the respond error for the code variable that was set by the validate header function
            getSpecificRespond(fd, serverConfig.getErrorPages().find(400)->second, createBadRequest);
        }

        if (state.isComplete && request[fd].cgi.getIsCgi() == true && request[fd].cgi.cgiState == CGI_NOT_STARTED)
        {
            request[fd].cgi.runCgi(*this, fd, request[fd], request[fd].serverConfig);
            request[fd].cgi.cgiState = CGI_RUNNING;
            return;
        }
    }
    else if (!state.isComplete)
    {
        state.file->write(holder.c_str(), bytes);
        state.bytesReceived += bytes;

        if (static_cast<int>(atoi(request[fd].contentLength.c_str())) <= state.bytesReceived)
        {
            state.isComplete = true;
            std::cout << "i was here once \n";
            state.file->close();
        }
        if (state.isComplete && request[fd].cgi.getIsCgi() == true && request[fd].cgi.cgiState == CGI_NOT_STARTED)
        {
            request[fd].cgi.runCgi(*this, fd, request[fd], request[fd].serverConfig);
            request[fd].cgi.cgiState = CGI_RUNNING;
            return;
        }
    }
    holder.clear();
}

void Server::handleClientOutput(int fd)
{
    Request &req = request[fd];

    if (!req.state.isComplete)
        return;

    ConfigData &serverConfig = req.serverConfig;
    //[ soukaina ] here the location is still segfaulting if the post method with data called

    // [soukaina] i have added this line in the parser file after the parsing
    /*request[fd].location = getExactLocationBasedOnUrl(request[fd].state.filePath, serverConfig);*/
    // request[fd].location = serverConfig.getLocations().front();

    if (req.cgi.cgiState == CGI_RUNNING)
        getCgiResponse(req);
    else if (req.getMethod() == "GET")
    {
        // std::cout << "-------( REQUEST PARSED )-------\n\n";
        // std::cout << req.header << std::endl;
        // std::cout << "-------( END OF REQUEST )-------\n\n\n";
        int state = serve_file_request(fd, serverConfig);
        if (state == 310)
        {
            close(fd);
            request.erase(fd);
        }
        if (state == 200 || state == 0)
        {
            if (timedFunction(TIMEOUTREDIRACTION, request[fd].state.last_activity_time) == false)
            {
                close(fd);
                request.erase(fd);
            }
        }
    }
    else if (req.getMethod() == "DELETE")
    {
        std::cout << "-------( REQUEST PARSED )-------\n\n";
        std::cout << req.header << std::endl;
        std::cout << "-------( END OF REQUEST )-------\n\n\n";
        int state = handle_delete_request(fd, serverConfig);
        if (state == 0)
        {
            if (timedFunction(TIMEOUTREDIRACTION, request[fd].state.last_activity_time) == false)
            {
                close(fd);
                request.erase(fd);
            }
        }
    }
    else if (req.getMethod() == "POST")
    {
        if (request[fd].getContentLength() == "0")
        {
            request[fd].state.file->close();
            std::string httpRespons;
            httpRespons = httpResponse(request[fd].ContentType, request[fd].state.fileSize);
            int faild = send(fd, httpRespons.c_str(), httpRespons.length(), MSG_NOSIGNAL);
            if (faild == -1)
            {
                close(fd), request.erase(fd);
            }
            if (request[fd].getConnection() == "close" || request[fd].getConnection().empty())
                request[fd].state.isComplete = true, close(fd), request.erase(fd);
            if (timedFunction(TIMEOUTREDIRACTION, request[fd].state.last_activity_time) == false)
            {
                close(fd);
                request.erase(fd);
            }
            return;
        }
        //[soukaina] here i should check for the post if an error responce is sent
        if (req.state.PostHeaderIsValid == false && parsePostRequest(fd, serverConfig) != 0)
            req.state.PostHeaderIsValid = true;
        handlePostRequest(fd);
    }
}

void Server::getCgiResponse(Request &req)
{
    int status;
    int pid = waitpid(req.cgi.getPid(), &status, WNOHANG);

    if (pid == req.cgi.getPid())
    {
        if (WEXITSTATUS(status) != 0)
            req.code = 502;
        // adding specific error response here

        int fde = open(req.cgi.fileNameOut.c_str(), O_RDONLY, 0644);
        if (fde < 0)
        {
            req.code = 500;
            // adding specific error response here
            return;
        }

        char buffer[CHUNK_SIZE];
        while (true)
        {
            int readBytes = read(fde, buffer, CHUNK_SIZE);

            if (readBytes < 0)
            {
                req.code = 500;
                close(fde);
                return;
            }
            if (readBytes == 0)
            {
                close(fde);
                break;
            }
            req.cgi.CgiBodyResponse.append(buffer, readBytes);
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
