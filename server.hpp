/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mmad <mmad@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/18 03:11:18 by mmad              #+#    #+#             */
/*   Updated: 2025/04/16 16:35:59 by mmad             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP

#include <time.h>
#include <bits/types.h>
#include "server.hpp"
#include <unistd.h>
#include <iomanip>  
#include <filesystem>
#include <dirent.h>   
#include <cstring>
#include <sstream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstdlib>
#include <map>
#include <stack>
#include <netdb.h>
#include <arpa/inet.h>
#include <poll.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <sys/stat.h>  
#include <errno.h>     
#include <string.h>
#include <set>
#include <algorithm>

#define ERROR404 404
#define ERROR405 405
#define SUCCESS 200

#define PORT 8080 
#define MAX_EVENTS 10000
#define CHUNK_SIZE 17000    
#define TIMEOUT 60
#define TIMEOUTMS 60000
#define PATHC "root/content/"
#define PATHE "root/error/" 
#define PATHU "root/UPLOAD"

struct Multipart
{
    bool flag;
    bool containHeader;
    std::string boundry;
    std::ofstream *outFile;
    std::string currentFileName;
    int currentFd;
    
    Multipart() : flag(false) ,outFile(NULL) {}
};
// Structure to hold file transfer state
struct FileTransferState {
    time_t last_activity_time;
    std::string filePath;   
    size_t offset;
    size_t endOffset;
    size_t fileSize;
    bool isComplete;
    bool isCompleteShortFile;
    int socket;
    int saveFd;
    int flag;
    Multipart multp;
    std::string typeOfConnection;
    std::set<std::string> knownPaths;
    FileTransferState() : offset(0), fileSize(0), isComplete(false) {}
};

class binary_string
{
    private:
        std::vector <uint8_t > buffer;
        static const size_t npos = -1;
    public:
        binary_string(const char* str, size_t n);
        binary_string();
        binary_string(const binary_string& other);
        binary_string(size_t n);
        ~binary_string();
        size_t find(const char* s, size_t pos = 0) const;
        size_t find(const std::string& s, size_t pos = 0) const;
        binary_string substr(size_t pos, size_t n) const;
        binary_string& append(const char* str, size_t subpos, size_t sublen);
        binary_string& append(const std::string& str, size_t subpos, size_t sublen);
        binary_string& append(const binary_string& str, size_t subpos, size_t sublen);
        void clear();
        std::string to_string() const;
        const char* c_str() const;
        size_t size() const;
        uint8_t operator[](size_t i) const;
        uint8_t& operator[](size_t i);
        uint8_t* data();
        bool empty() const;
        std::vector <uint8_t >::iterator begin();
        std::vector <uint8_t >::iterator end();
        void push_back(uint8_t c);
        binary_string operator+(const binary_string& other) const;
        binary_string operator+=(const binary_string& other);
        bool operator==(const binary_string& other) const;
        bool operator!=(const binary_string& other) const;

};

class Server
{
public:
    Server();
    Server(const Server& Init);
    Server& operator=(const Server& Init);
    ~Server();

public:
    // Map to keep track of file transfers for each client
    std::map<int, FileTransferState> fileTransfers;
    int establishingServer();

public:
    int pageNotFound;
    size_t LARGE_FILE_THRESHOLD;

public:
    // Methods
    int serve_file_request(int fd, Server *server, std::string request);
    int handle_delete_request(int fd, Server *server,std::string request);
    int handle_post_request(int fd, Server *server, binary_string request);
    
    // Functions helper
    std::string getContentType(const std::string &path);
    std::string readFile(const std::string &path);
    int getFileType(std::string path);
    bool canBeOpen(std::string &filePath);
    std::string parseRequest(std::string request, Server *server);
    std::ifstream::pos_type getFileSize(const std::string &path);
    static std::string getCurrentTimeInGMT();
    std::string key_value_pair_header(std::string request, std::string target_key);    
    // Response headers
    static std::string createNotFoundResponse(std::string contentType, size_t contentLength);
    std::string createChunkedHttpResponse(std::string contentType);
    std::string httpResponse(std::string contentType, size_t contentLength);
    static std::string methodNotAllowedResponse(std::string contentType, size_t contentLength);
    int processMethodNotAllowed(int fd, Server *server, std::string request);
    static std::string createBadResponse(std::string contentType, size_t contentLength);
    std::string goneHttpResponse(std::string contentType, size_t contentLength);
    std::string deleteHttpResponse(Server* server);
    std::string createTimeoutResponse(std::string contentType, size_t contentLength);
    int getSpecificRespond(int fd, Server *server, std::string file, std::string (*f)(std::string, size_t));
    std::pair<size_t, std::string> returnTargetFromRequest(std::string header, std::string target);
    std::pair<std::string, std::string> ft_parseRequest(std::string header);
    // Transfer-Encoding: chunked
    int handleFileRequest(int fd, Server *server, const std::string &filePath, std::string Connection);
    int continueFileTransfer(int fd, Server * server, std::string Connection);
    void setnonblocking(int fd);

    int test(int fd, Server *server, std::string Connection);
};


std::ostream& operator<<(std::ostream& os, const binary_string& buffer);
#endif

