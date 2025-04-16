/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mmad <mmad@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/18 03:11:14 by mmad              #+#    #+#             */
/*   Updated: 2025/04/16 16:38:21 by mmad             ###   ########.fr       */
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

std::string Server::key_value_pair_header(std::string request, std::string target_key)
{
    std::map<std::string, std::string> mapv;
    request.erase(std::remove(request.begin(), request.end(), '\r'), request.end());
    size_t j = 0;
    for (size_t i = 0; i < request.length(); i++)
    {
        std::string result;
        if (static_cast<unsigned char>(request.at(i)) == 10)
        {
            result = request.substr(j, i - j);
            if (!result.empty())
            {
                mapv.insert(std::pair<std::string, std::string>(result.substr(0, result.find(" ")), result.substr(result.find(" "), result.length())));
            }
            j = i + 1;
        }
    }
    std::map<std::string, std::string>::iterator it = mapv.find(target_key);
    if (it != mapv.end())
        return it->second;
    return "";
}

int returnTimeoutRequest(int fd, Server *server)
{
    std::string path1 = PATHE;
    std::string path2 = "408.html";
    std::string new_path = path1 + path2;
    std::string content = server->readFile(new_path);
    std::string httpResponse = server->createTimeoutResponse(server->getContentType(new_path), content.length());
    if (!httpResponse.empty())
    {
        if (send(fd, httpResponse.c_str(), httpResponse.length(), MSG_NOSIGNAL) == -1)
        {
            return std::cerr << "Failed to send error response header" << std::endl, server->fileTransfers.erase(fd), close(fd), 0;
        }

        if (send(fd, content.c_str(), content.length(), MSG_NOSIGNAL) == -1)
            return std::cerr << "Failed to send error content" << std::endl, server->fileTransfers.erase(fd), close(fd), 0;

        if (send(fd, "\r\n\r\n", 2, MSG_NOSIGNAL) == -1)
            return server->fileTransfers.erase(fd), close(fd), 0;
    }

    return 0;
}

int handleClientConnections(Server *server, int listen_sock, struct epoll_event &ev, sockaddr_in &clientAddress, int epollfd, socklen_t &clientLen, std::map<int, Binary_String >& send_buffers)
{
    int conn_sock;
    std::vector<unsigned char> buffer(CHUNK_SIZE);
    Binary_String holder;
    std::string request;
    struct epoll_event events[MAX_EVENTS];
    int nfds;

    if ((nfds = epoll_wait(epollfd, events, MAX_EVENTS, TIMEOUTMS)) == -1)
        return std::cerr << "epoll_wait" << std::endl, EXIT_FAILURE;
    if (nfds == 0)
    {
        return 0;
    }
    for (int i = 0; i < nfds; ++i)
    {
        if (events[i].data.fd == listen_sock)
        {
            conn_sock = accept(listen_sock, (struct sockaddr *)&clientAddress, &clientLen);
            if (conn_sock == -1)
                return std::cerr << "accept" << std::endl, EXIT_FAILURE;
            server->setnonblocking(conn_sock);
            ev.events = EPOLLIN | EPOLLOUT;
            ev.data.fd = conn_sock;
            if (epoll_ctl(epollfd, EPOLL_CTL_ADD, conn_sock, &ev) == -1)
                return std::cerr << "epoll_ctl: conn_sock" << std::endl, EXIT_FAILURE;
        }

        else if (events[i].events & EPOLLIN)
        {
            int bytes = recv(events[i].data.fd, &buffer[0], buffer.size(), 0);
            if (bytes < 0)
            {
                return server->fileTransfers.erase(events[i].data.fd), close(events[i].data.fd), 0;
            }
            if (bytes == 0)
            {
                if (server->fileTransfers.find(events[i].data.fd) != server->fileTransfers.end())
                    server->fileTransfers.erase(events[i].data.fd);
                continue;
            }
            else
            {
                buffer[bytes] = '\0';
                send_buffers[events[i].data.fd] = holder.append(reinterpret_cast<char*> (&buffer[0]), 0, bytes);
                std::cout << holder.to_string() << std::ends;
            }
        }
        else if (events[i].events & EPOLLOUT)
        {
            if (server->fileTransfers.find(events[i].data.fd) != server->fileTransfers.end())
            {
                request = send_buffers[events[i].data.fd].to_string();
                std::pair<std::string, std::string> pair_request = server->ft_parseRequest(request);
                std::string Connection = server->key_value_pair_header(pair_request.first, "Connection:");
                if (server->continueFileTransfer(events[i].data.fd, server, Connection) == -1)
                return std::cerr << "Failed to continue file transfer" << std::endl, 0;
                continue;
            }
            request = send_buffers[events[i].data.fd].to_string();
            if (request.empty())
                continue;
            if (request.find("DELETE") != std::string::npos)
            {
                server->handle_delete_request(events[i].data.fd, server, request);
            }
            else if (request.find("GET") != std::string::npos)
            {
                server->serve_file_request(events[i].data.fd, server, request);
            }
            else if (request.find("PUT") != std::string::npos || request.find("PATCH") != std::string::npos || request.find("HEAD") != std::string::npos || request.find("OPTIONS") != std::string::npos)
            {
                server->processMethodNotAllowed(events[i].data.fd, server, request);
            }

            if (server->fileTransfers.find(events[i].data.fd) == server->fileTransfers.end())
                send_buffers.erase(events[i].data.fd);
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

    std::cout << "Server is listening" << std::endl;
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

    // std::map<int, std::vector<unsigned char> > send_buffers;
    std::map<int, Binary_String > send_buffers;
    while (true)
    {
        if (handleClientConnections(server, listen_sock, ev, clientAddress, epollfd, clientLen, send_buffers) == EXIT_FAILURE)
            break;
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
