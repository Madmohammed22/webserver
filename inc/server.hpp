#ifndef SERVER_HPP
#define SERVER_HPP


#include <time.h>
#include <bits/types.h>
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
#include <cstdlib>


#define ERROR404 404
#define ERROR405 405
#define SUCCESS 200

#define PORT 8080 
#define MAX_EVENTS 1024
#define CHUNK_SIZE 1024    
#define TIMEOUT 4
#define TIMEOUTMS 30000
#define PATHC "root/content/"
#define PATHE "root/error/" 
#define PATHU "root/UPLOAD"
#define STATIC "root/static/"

class Server
{
public:
    Server();
    Server(const Server& Init);
    Server& operator=(const Server& Init);
    ~Server();

    // init the Http server and run it
    void initServer();
    int establishingServer();
    int handleClientConnections(int listen_sock, struct epoll_event &ev, sockaddr_in &clientAddress, int epollfd, socklen_t &clientLen);
};

#endif
