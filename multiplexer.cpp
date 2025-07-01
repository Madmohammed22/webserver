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

        if ((state.bytesReceived = bytes) && validateHeader(fd, state, holder) == false)
        {
            ConfigData serverConfig = getConfigForRequest(multiServers[serverSocket], "");
  
            getResponse(fd, request[fd].code);
            close(fd);
            request.erase(fd);
            return ;
        }
        if (state.isComplete == true && request[fd].cgi.getIsCgi() == true && request[fd].cgi.cgiState == CGI_NOT_STARTED)
        {
            request[fd].cgi.runCgi(*this, request[fd]);
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
            state.file->close();
        }
        if (state.isComplete && request[fd].cgi.getIsCgi() == true && request[fd].cgi.cgiState == CGI_NOT_STARTED)
        {
            request[fd].cgi.runCgi(*this, request[fd]);
            if (request[fd].code != 200)
            {
              //[soukaina] i think it should be in the epollout
              getResponse(fd, request[fd].code);
              close(fd);
              request.erase(fd);
              return ;
            }
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
    
    if (req.cgi.cgiState == CGI_RUNNING || req.cgi.cgiState == CGI_COMPLETE)    
        getCgiResponse(req, fd);
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
        /*std::cout << "-------( REQUEST PARSED )-------\n\n";*/
        /*std::cout << req.header << std::endl;*/
        /*std::cout << "-------( END OF REQUEST )-------\n\n\n";*/
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
        if (req.state.PostHeaderIsValid == false && parsePostRequest(fd, serverConfig, req) != 0)
        {
            if (req.code != 200)
            {
              getResponse(fd, req.code);
              close(fd);
              request.erase(fd);
              return ;
            }
            req.state.PostHeaderIsValid = true;
        }

        if (req.getContentLength() == "0")
        {
            std::string httpRespons;
            
            if (req.state.file->is_open())
            {
              req.state.file->close();
              delete req.state.file;
              remove(req.state.fileName.c_str());
            }
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
        handlePostRequest(fd);
    }
}

void Server::sendCgiResponse(Request &req, int fd)
{
  std::ostringstream oss;
  std::string httpRespons;
  
  httpRespons = req.cgi.CgiBodyResponse;

  if (!(httpRespons.find("HTTP/1.1") != std::string::npos))
  {
		httpRespons.insert(0, "HTTP/1.1 200 OK\r\n");
  
  }
  int faild = send(fd, httpRespons.c_str(), httpRespons.length(), MSG_NOSIGNAL);

  if (faild == -1)
  {
    //[soukaina] i have to send 500

    close(fd), request.erase(fd);
    return ;
  }
  if (request[fd].getConnection() == "close" || request[fd].getConnection().empty())
      close(fd), request.erase(fd);
  request[fd].cgi.cgiState = CGI_COMPLETE;
}

void Server::getCgiResponse(Request &req, int fd)
{
    int status;
    int pid = waitpid(req.cgi.getPid(), &status, WNOHANG);

    if (request[fd].cgi.cgiState == CGI_COMPLETE
        || request[fd].cgi.cgiState == CGI_RUNNING)
    {
      if (timedFunction(TIMEOUTREDIRACTION, request[fd].state.last_activity_time) == false)
      {
        if (request[fd].cgi.cgiState == CGI_RUNNING)
          getResponse(fd, 504);
        close(fd);
        request.erase(fd);
      }
    }

    if (pid == req.cgi.getPid())
    {
        if (WEXITSTATUS(status) != 0)
        {
            req.code = 500;
            return ;
        }
    
        // adding specific error response here
        int fde = open(req.cgi.fileNameOut.c_str(), O_RDONLY, 0644);
        if (fde < 0)
        {
            req.code = 500;
            // adding specific error response here
            return;
        }


        int totalBytes = 0;
        char buffer[CHUNK_SIZE];
          
        request[fd].state.last_activity_time = time(NULL);
        while (true)
        {
            int readBytes = read(fde, buffer, CHUNK_SIZE);
            
            if (readBytes < 0)
            {
                req.code = 500;
                close(fde);
                remove(req.cgi.fileNameOut.c_str());
                return;
            }
            if (readBytes == 0)
            {
                close(fde);
                remove(req.cgi.fileNameOut.c_str());
                break;
            }
            req.cgi.CgiBodyResponse.append(buffer, readBytes);
            totalBytes += readBytes;
            /*if (timedFunction(TIMEOUTREDIRACTION, request[fd].state.last_activity_time) == false)*/
            /*{*/
            /*  close(fd);*/
            /*  request.erase(fd);*/
            /*}*/
        }
        sendCgiResponse(req, fd);
        request[fd].state.last_activity_time = time(NULL);
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
