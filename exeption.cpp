
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


// Online C++ compiler to run C++ program online
#include <iostream>

void runSecondMethod(){
    std::cout << "Do something" << std::endl;
} 
void runFirstMethod() throw(std::exception) {  // C++98 style, means may throw std::exception or derived
    std::string test = "this is just a test";
    std::string result = test.substr(0, test.find("00000"));
    std::cout << result;
}
try
{
    /* code */
}
catch(const std::exception& e)
{
    std::ru
}

int main() {
    try
    {
        runFirstMethod();
    }
    catch(const std::exception& e)
    {
        exit(0);
        std::cerr << e.what() << '\n';
    }
    
    return 0;
}