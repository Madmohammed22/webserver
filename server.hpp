/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mmad <mmad@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/18 03:11:18 by mmad              #+#    #+#             */
/*   Updated: 2025/04/21 16:23:14 by mmad             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP

#include "request.hpp"
#include "globalInclude.hpp"

class CONFIG
{
    int port;
    int maxevents;
    int chunksize;
    int timeout;
    int timeoutms;
    std::string pathc;
    std::string pathe; 
    std::string pathu; 
    std::string test; 


public :
    CONFIG(const CONFIG& conf) {
        this->port = conf.port;
        this->maxevents = conf.maxevents;
        this->chunksize = conf.chunksize;
        this->timeout = conf.timeout;
        this->timeoutms = conf.timeoutms;

        this->pathc = conf.pathc;
        this->pathe = conf.pathe;
        this->pathu = conf.pathu;
        this->test = conf.test;
    }
public:
    void set_port(int port) { this->port = port; };
    void set_maxevents(int maxevents) { this->maxevents = maxevents; };
    void set_chunksize(int chunksize) { this->chunksize = chunksize; };
    void set_timeout(int timeout) { this->timeout = timeout; };
    void set_timeoutms(int timeoutms) { this->timeoutms = timeoutms; };

public:
    void set_pathc(std::string pathc) { this->pathc = pathc; };
    void set_pathe(std::string pathe) { this->pathe = pathe; };
    void set_pathu(std::string pathu) { this->pathu = pathu; };
    void set_test(std::string test) { this->test = test; };
};

class Binary_String;
class Request;

class Server
{

public:
    struct epoll_event ev;
    int listen_sock;
    int epollfd;

public:
    Server();
    Server(const Server &Init);
    Server &operator=(const Server &Init);
    ~Server();

public:
    // Map to keep track of file in for each client
    std::map<int, Request> request;
    std::map<int, struct CONFIG> multiServers;
    int establishingServer();

public:
    int pageNotFound;
    size_t LARGE_FILE_THRESHOLD;

public:
    int startServer();
    bool validateHeader(int fd, FileTransferState &state);
    int handleClientConnections();
    // Methods
    int serve_file_request(int fd, Server *server, std::string request);
    int handle_delete_request(int fd);
    int handlePostRequest(int fd, Server *server, Binary_String request);

    // Functions helper
    bool closeConnection(int fd);
    static bool containsOnlyWhitespace(const std::string &str);
    static std::string trim(std::string str);
    int parsePostRequest(Server *server, int fd, std::string header);
    static std::string getContentType(const std::string &path);
    std::string readFile(const std::string &path);
    int getFileType(std::string path);
    bool canBeOpen(std::string &filePath);
    static std::string parseSpecificRequest(std::string request);
    static std::ifstream::pos_type getFileSize(const std::string &path);
    static std::string getCurrentTimeInGMT();
    std::string key_value_pair_header(std::string request, std::string target_key);
    void key_value_pair_header(int fd, std::string header);
    std::pair<Binary_String, Binary_String> ft_parseRequest_binary(Binary_String header);
    void printfContentHeader(Server *server, int fd);
    static bool searchOnSpecificFile(std::string path, std::string fileTarget);

    // Response headers
    static std::string createNotFoundResponse(std::string contentType, size_t contentLength);
    std::string createChunkedHttpResponse(std::string contentType);
    std::string httpResponse(std::string contentType, size_t contentLength);
    static std::string methodNotAllowedResponse(std::string contentType, size_t contentLength);
    int processMethodNotAllowed(int fd, Server *server, std::string request);
    static std::string createUnsupportedMediaResponse(std::string contentType, size_t contentLength);
    static std::string createBadResponse(std::string contentType, size_t contentLength);
    void writeData(Server *server, Binary_String &chunk, int fd);
    std::string goneHttpResponse(std::string contentType, size_t contentLength);
    std::string deleteHttpResponse(Server *server);
    std::string createTimeoutResponse(std::string contentType, size_t contentLength);
    int getSpecificRespond(int fd, Server *server, std::string file, std::string (*f)(std::string, size_t));
    std::pair<size_t, std::string> returnTargetFromRequest(std::string header, std::string target);

    // Transfer-Encoding: chunked
    int handleFileRequest(int fd, const std::string &filePath, std::string Connection);
    int continueFileTransfer(int fd, std::string filePath);
    void setnonblocking(int fd);
    static std::map<std::string, std::string> key_value_pair(std::string header);

    int serve_file_request(int fd);
};

template <typename T>
std::pair<T, T> ft_parseRequest_T(int fd, Server *server, T header)
{

    std::pair<T, T> pair_request;
    try
    {
        pair_request.first = header.substr(0, header.find("\r\n\r\n"));
        pair_request.second = header.substr(header.find("\r\n\r\n"), header.length());
    }
    catch (const std::exception &e)
    {
        server->getSpecificRespond(fd, server, "404.html", server->createNotFoundResponse);
    }

    return pair_request;
}

#endif
