#include "../server.hpp"


void deleteDirectoryContents(const std::string& dir)
{
    DIR* dp = opendir(dir.c_str());
    if (dp == NULL) {
        std::cerr << "Error: Unable to open directory " << dir << std::endl;
        return;
    }

    try
    {
        struct dirent* entry;
        while ((entry = readdir(dp)) != NULL) {
            // Skip the special entries "." and ".."
            if (std::string(entry->d_name) == "." || std::string(entry->d_name) == "..") {
                continue;
            }

            struct stat entryStat;
            std::string entryPath = dir + "/" + entry->d_name;
            if (stat(entryPath.c_str(), &entryStat) == -1) {
                std::cerr << "Error: Unable to stat " << entryPath << std::endl;
                continue;
            }

            if (S_ISDIR(entryStat.st_mode)) {
                // If it's a directory, use recursive removal
                if (rmdir(entryPath.c_str()) == -1) {
                    std::cerr << "Error: Unable to remove directory " << entryPath << std::endl;
                }
            } else {
                // If it's a file, remove it
                if (unlink(entryPath.c_str()) == -1) {
                    std::cerr << "Error: Unable to remove file " << entryPath << std::endl;
                }
            }
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    closedir(dp);
    rmdir(dir.c_str());
}

int DELETE(std::string request){    
    const char* filename = request.c_str();
    if (unlink(filename) == -1) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

// Improved searchOnPath function
bool searchOnPath(std::vector<std::string>& nodePath, const std::string& filePath) {
    // Iterate through the nodePath vector
    std::vector<std::string>::iterator begin = nodePath.begin();
    while (begin != nodePath.end()) {
        if (*begin == filePath) {
            return true;
        }
    }
    return false;
}

int Server::handle_delete_request(int fd, Server *server, std::string request) {
    std::cout << "-------( REQUEST PARSED )-------\n\n";
    std::cout << request << std::endl;
    std::cout << "-------( END OF REQUEST )-------\n\n";
    
    std::string filePath = server->parseRequest(request, server);
    if (server->canBeOpen(filePath)) {
        if (filePath.at(0) != '/')
            filePath = "/" + filePath;
        if (server->getFileType(filePath) == 1){
            deleteDirectoryContents(filePath.c_str());
        }
        
        if (DELETE(filePath) == -1) {
            return server->fileTransfers.erase(fd), close(fd), std::cerr << "Failed to delete file or directory: " << filePath << std::endl, 0;
        }
        std::string httpResponse = server->deleteHttpResponse(server);
        if (send(fd, httpResponse.c_str(), httpResponse.length(), MSG_NOSIGNAL) == -1) {
            std::cerr << "Failed to send HTTP header." << std::endl;
            return server->fileTransfers.erase(fd), close(fd), 0;
        }
        return server->fileTransfers.erase(fd), close(fd), 0;
    }
    return getSpecificRespond(fd, server, "404.html", server->createNotFoundResponse);
}