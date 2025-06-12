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

#include "ConfigData.hpp"
#include "request.hpp"
#include "globalInclude.hpp"
using namespace std;

// #include <curl/curl.h>

typedef struct s_listen
{
    int port;
    std::string host;
} t_listen;

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

public:
    CONFIG(const CONFIG &conf)
    {
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
    std::vector<ConfigData> configData;
    int epollfd;

public:
    Server();
    Server(const Server &Init);
    Server &operator=(const Server &Init);
    ~Server();

public:
    int flag;

    // Map to keep track of file in for each client
    std::map<int, Request> request;
    std::vector<s_listen> listenVec;
    std::map<int, t_listen> multiServers;
    std::map<int, int> clientToServer;
    std::vector<int> sockets;
    struct epoll_event events[MAX_EVENTS];

    int establishingServer();

public:
    int pageNotFound;
    size_t LARGE_FILE_THRESHOLD;

public:
    void getListenPairs();
    ConfigData getConfigForRequest(t_listen listen, std::string host);
    std::string returnFilePath(std::string &path, ConfigData configIndex);
    void handleNewConnection(int fd);
    void handleClientData(int fd);
    void handleClientOutput(int fd);
    int establishingMultiServer(t_listen configData);
    int handleClientConnectionsForMultipleServers();
    int startMultipleServers(t_listen configData);
    int startServer();
    bool validateHeader(int fd, FileTransferState &state, ConfigData serverConfig);
    int handleClientConnections();
    // Methods
    int serve_file_request(int fd, ConfigData configIndex);
    int handle_delete_request(int fd, ConfigData configIndex);


    // Post method
    
    void handlePostRequest(int fd);
    Binary_String readFileChunk_post(int fd);
    void createFileName(std::string line, int fd);
    void writeData(Binary_String& chunk, int fd);
    int parsePostRequest(int fd, ConfigData& configIndex);

    // Functions helper
    static int t_stat_wait(std::string path);
    static bool timedFunction(int timeoutSeconds, time_t startTime);
    static Location getExactLocationBasedOnUrlContainer(std::string target, ConfigData configIndex);
    int sendFinalReques(int fd, std::string filePath, ConfigData configIndex,  Location location, size_t checkState);
    int helper(int fd, std::string &filePath, ConfigData configIndex,  Location location);
    static std::string fetchIndex(std::string root, std::vector<std::string> indexFile);
    static bool check(std::string filePath);
    static std::string redundantSlash(std::string url);
    int deleteTargetUrl(int fd, std::string filePath, ConfigData configIndex, Location location, int state);
    int handle_delete_request___(int fd, ConfigData configIndex);
    std::string listDirectory(const std::string &dir_path, const std::string &fileName, std::string& mime);
    int t_stat(std::string path, Location location);
    static void addSlashBasedOnMethod(std::string &target, std::string method);
    static std::string Forbidden(std::string contentType, size_t contentLength);
    Location getExactLocationBasedOnUrl(std::string target, ConfigData configIndex);
    bool checkAvailability(int fd, Location location);
    void reWrite(std::string &filePath, ConfigData configData);
    bool closeConnection(int fd);
    static bool containsOnlyWhitespace(const std::string &str);
    static std::string trim(std::string str);
    int parsePostRequest(Server *server, int fd, std::string header);
    static std::string getContentType(const std::string &path);
    std::string readFile(std::string path);
    int getFileType(std::string path);
    static Location getLocation_adder1(std::string targetLocation, ConfigData configIndex);
    bool canBeOpen(int fd, std::string &filePath, Location location, size_t &checkState);
    static std::string parseSpecificRequest(std::string request);
    static std::ifstream::pos_type getFileSize(const std::string &path);
    static std::string getCurrentTimeInGMT();
    std::string key_value_pair_header(std::string request, std::string target_key);
    void key_value_pair_header(int fd, std::string header);
    std::pair<Binary_String, Binary_String> ft_parseRequest_binary(Binary_String header);
    void printfContentHeader(Server *server, int fd);
    static bool searchOnSpecificFile(std::string path, std::string fileTarget);
    // std::string movedPermanently(std::string contentType, size_t contentLength);

    // Response headers
    static std::string createNotFoundResponse(std::string contentType, size_t contentLength);
    std::string createChunkedHttpResponse(std::string contentType);
    static std::string httpResponse(std::string contentType, size_t contentLength);
    static std::string methodNotAllowedResponse(std::string contentType, size_t contentLength);
    int processMethodNotAllowed(int fd, Server *server, std::string request);
    static std::string createUnsupportedMediaResponse(std::string contentType, size_t contentLength);
    static std::string createBadRequest(std::string contentType, size_t contentLength);
    static std::string goneHttpResponse(std::string contentType, size_t contentLength);
    std::string deleteResponse(Server *server);
    static std::string createTimeoutResponse(std::string contentType, size_t contentLength);
    int getSpecificRespond(int fd, std::string file, std::string (*f)(std::string, size_t));
    std::pair<size_t, std::string> returnTargetFromRequest(std::string header, std::string target);

    // Transfer-Encoding: chunked
    int handleFileRequest(int fd, const std::string &filePath, std::string Connection, Location configIndex);
    int continueFileTransfer(int fd, std::string filePath, Location configIndex);
    void setnonblocking(int fd);
    static std::map<std::string, std::string> key_value_pair(std::string header);

public:
    std::string MovedPermanently(std::string contentType, std::string location);
};

template <typename T>
std::pair<T, T> ft_parseRequest_T(int fd, Server* server, T header, ConfigData serverConfig)
{

    std::pair<T, T> pair_request;
    try
    {
        pair_request.first = header.substr(0, header.find("\r\n\r\n"));
        pair_request.second = header.substr(header.find("\r\n\r\n"), header.length());
    }
    catch (const std::exception &e)
    {
        server->getSpecificRespond(fd, serverConfig.getErrorPages().find(400)->second, server->createBadRequest);
    }

    return pair_request;
}

#endif
