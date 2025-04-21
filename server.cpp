/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mmad <mmad@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/18 03:11:14 by mmad              #+#    #+#             */
/*   Updated: 2025/04/21 16:14:23 by mmad             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server.hpp"

Server::Server()
{
    this->pageNotFound = 0;
    this->LARGE_FILE_THRESHOLD = 1024 * 1024;
}

Server::Server(const Server &Init)
{
    this->pageNotFound = Init.pageNotFound;
    this->LARGE_FILE_THRESHOLD = Init.LARGE_FILE_THRESHOLD;
}

Server &Server::operator=(const Server &Init)
{
    if (this == &Init)
        return *this;
    return *this;
}

Server::~Server()
{
    std::cout << "[Server] Destructor is called" << std::endl;
}

std::string trim(std::string str)
{
    str.erase(str.find_last_not_of(' ') + 1);
    str.erase(0, str.find_first_not_of(' '));
    return str;
}

void Server::key_value_pair_header(int fd, Server *server, std::string header)
{
    std::map<std::string, std::string> mapv = server->fileTransfers[fd].mapOnHeader;
    size_t j = 0;
    for (size_t i = 0; i < header.length(); i++)
    {
        std::string result;
        if (static_cast<unsigned char>(header.at(i)) == 10)
        {
            result = header.substr(j, i - j);
            if (!result.empty())
            {
                mapv.insert(std::pair<std::string, std::string>(trim(result.substr(0, result.find(" "))), trim(result.substr(result.find(" "), result.length()))));
            }
            j = i + 1;
        }
    }
    std::string result = header.substr(j, header.length());
    mapv.insert(std::pair<std::string, std::string>(trim(result.substr(0, result.find(" "))), trim(result.substr(result.find(" "), result.length()))));
    server->fileTransfers[fd].mapOnHeader = mapv;
}

int add_time_out_for_client(int fd, std::map<int, Client> &client)
{
    Client state = client[fd];
    if (state.isComplete == true)
    {
        time_t current_time = time(NULL);
        return close(fd), 0;
        if (current_time - state.current_time > TIMEOUT)
        {
            std::cerr << "Client " << fd << " timed out." << std::ends;
            state.isComplete = false;
            state.current_time = time(NULL);
        }
        else
        {
            std::cout << "ERROR" << std::endl;
            return close(fd), 0;
        }
    }
    return 0;
}

void initializeFileTransfers(Server *server, int fd, Binary_String &request)
{
    FileTransferState state;

    state.multp.containHeader = true;
    state.filePath = server->parseSpecificRequest(request.to_string(), server);
    state.fileSize = server->getFileSize(state.filePath);
    state.offset = 0;
    state.isComplete = false;
    state.multp.flag = true;
    server->fileTransfers[fd] = state;
}

int handleClientConnections(Server *server, int listen_sock, struct epoll_event &ev,
    sockaddr_in &clientAddress, int epollfd, socklen_t &clientLen, std::map<int, Binary_String> &send_buffers) throw(std::exception)
{
    int conn_sock;
    Binary_String holder(CHUNK_SIZE);
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
            int fd = events[i].data.fd;
            int bytes = recv(fd, &holder[0], holder.length(), 0);

            if (bytes < 0)
                return server->fileTransfers.erase(fd), close(fd), 0;
            if (bytes == 0)
            {
                if (server->fileTransfers.find(fd) != server->fileTransfers.end())
                    server->fileTransfers.erase(fd), close(fd);
                continue;
            }
            else
            {
                holder[bytes] = '\0';
                send_buffers[fd] = holder;
            }
        }
        else if (events[i].events & EPOLLOUT)
        {
            int fd = events[i].data.fd;
            if (server->fileTransfers.find(fd) != server->fileTransfers.end())
            {
                server->key_value_pair_header(fd, server, ft_parseRequest_T(fd, server, send_buffers[fd].to_string()).first);
                if (server->continueFileTransfer(fd, server, server->fileTransfers[fd].mapOnHeader.find("Connection:")->second) == -1)
                    return std::cerr << "Failed to continue file transfer" << std::endl, close(fd), 0;
                continue;
            }
            request = send_buffers[fd].to_string();
            if (request.find("DELETE") != std::string::npos)
            {
                server->handle_delete_request(fd, server, request);
            }
            else if (request.find("GET") != std::string::npos)
            {
                server->serve_file_request(fd, server, request, client);
            }
            else if (request.find("PUT") != std::string::npos || request.find("PATCH") != std::string::npos || request.find("HEAD") != std::string::npos || request.find("OPTIONS") != std::string::npos)
            {
                server->processMethodNotAllowed(fd, server, request);
            }
            if (server->fileTransfers.find(fd) == server->fileTransfers.end())
                send_buffers.erase(fd);
        }
    }
    return EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
    (void)argc, (void)argv;
    Server *server = new Server();

    sockaddr_in clientAddress;
    socklen_t clientLen = sizeof(clientAddress);
    int listen_sock, epollfd;
    struct epoll_event ev;

    if ((listen_sock = server->establishingServer()) == EXIT_FAILURE)
        return delete server, EXIT_FAILURE;

    std::cout << "Server is listening\n"
              << std::endl;
    if ((epollfd = epoll_create1(0)) == -1)
    {
        return std::cout << "Failed to create epoll file descriptor" << std::endl,
               close(listen_sock), delete server, EXIT_FAILURE;
    }

    ev.events = EPOLLIN | EPOLLOUT;
    ev.data.fd = listen_sock;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listen_sock, &ev) == -1)
    {
        return std::cerr << "Failed to add file descriptor to epoll" << std::endl,
               close(listen_sock), close(epollfd), delete server, EXIT_FAILURE;
    }

    std::map<int, Binary_String> send_buffers;
    while (true)
    {
        try
        {
            handleClientConnections(server, listen_sock, ev, clientAddress, epollfd, clientLen, send_buffers);
        }
        catch (std::exception &e)
        {
            break;
        }
    }

    for (std::map<int, FileTransferState>::iterator it = server->fileTransfers.begin(); it != server->fileTransfers.end(); ++it)
        close(it->first);
    server->fileTransfers.clear();
    close(listen_sock);
    if (close(epollfd) == -1)
        return std::cerr << "Failed to close epoll file descriptor" << std::endl, delete server, EXIT_FAILURE;
    delete server;    
    return EXIT_SUCCESS;
}
