#ifndef GLOBALINCLUDE_HPP
#define GLOBALINCLUDE_HPP

#include "request.hpp"

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
#include <dirent.h>

#include "Binary_String.hpp"

#define ERROR404 404
#define ERROR405 405
#define SUCCESS 200

#define PORT 8080
#define MAX_EVENTS 1024
#define CHUNK_SIZE 1024
#define TIMEOUT 600
#define TIMEOUTMS 30000
#define MAXURI 1000
#define PATHC "/root/content/"
#define PATHE "root/error/"
#define PATHU "root/UPLOAD"
#define STATIC "root/static/"
#define TEST "root/test/"


struct Multipart
{
    bool flag;
    bool containHeader;
    bool isInHeader;
    std::string partialHeaderBuffer;
    std::string boundary;
    int readPosition;
    std::vector<std::ofstream*> outFiles;
    int currentFileIndex;
    std::string currentFileName;
    int currentFd;
    std::ifstream *file;
    Multipart() : flag(false), isInHeader(true), readPosition(0), currentFileIndex(0){}
};

#endif
