#ifndef REQUEST_HPP
#define REQUEST_HPP

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
#include "Binary_String.hpp"

struct FileTransferState
{
    time_t last_activity_time;
    std::string filePath;
    size_t offset;
    size_t endOffset;
    size_t fileSize;
    bool isComplete;
    bool isCompleteShortFile;
    int socket;
    bool isValidHeader;
    int fd;
    Binary_String header;
    int headerFlag;
    std::ofstream *file;
    int bytesReceived;
    Binary_String buffer;
    int flag;
    std::map<std::string, std::string> mapOnHeader;
    Multipart multp;
    std::string typeOfConnection;
    std::set<std::string> knownPaths;
    FileTransferState() : offset(0), fileSize(0), isComplete(false), headerFlag(false), file(NULL) {}
    ~FileTransferState()
    {
        if (file)
        {
            file->close();
            delete file;
        }
    }
};

class Request
{

private:
    std::string path;
    std::string host;
    std::string connection;
    std::string transferEncoding;
    std::string contentLength;

public:
    std::map<int, FileTransferState> fileTransfers;

public:
    void setPath(std::string input) { this->path; };
    void setHost(std::string input) { this->host; };
    void setConnection(std::string input) { this->connection; };
    void setTransferEncoding(std::string input) { this->transferEncoding; };
    void setContentLength(std::string input) { this->contentLength; };
};

class RequstBuilder
{

public:
    virtual void BuildPath() = 0;
    virtual void BuildHost() = 0;
    virtual void BuildConnection() = 0;
    virtual void BuildTransferEncoding() = 0;
    virtual void BuildContentLength() = 0;

public:
    virtual Request getRequest() const = 0;
};

class Get : public RequstBuilder
{
private:
    std::string header;
    int fd;
public:
    Request request;

public:
    void BuildPath() override;
    void BuildHost() override;
    void BuildConnection() override;
    void BuildTransferEncoding() override;
    void BuildContentLength() override;
};

class Post : public RequstBuilder
{
private:
    std::string header;
    int fd;

public:
    Request request;

public:
    void BuildPath() override;
    void BuildHost() override;
    void BuildConnection() override;
    void BuildTransferEncoding() override;
    void BuildContentLength() override;
};

class Delete : public RequstBuilder
{
private:
    std::string header;
    int fd;

public:
    Request request;

public:
    void BuildPath() override;
    void BuildHost() override;
    void BuildConnection() override;
    void BuildTransferEncoding() override;
    void BuildContentLength() override;
};

class Build
{
public:
    void buildRequest(RequstBuilder &requstBuilder)
    {
        requstBuilder.BuildPath();
        requstBuilder.BuildHost();
        requstBuilder.BuildConnection();
        requstBuilder.BuildTransferEncoding();
        requstBuilder.BuildContentLength();
    }
};

#endif