
#include "../server.hpp"

void DELETE::includeBuild(std::string target, std::string &metaData, int pick)
{
    std::map<std::string, std::string>::iterator it = request.keys.find(target);
    if (it != request.keys.end())
    {
        if (Server::containsOnlyWhitespace(it->second) == false)
        {
            if (pick == 1)
                metaData = it->first;
            else
                metaData = it->second;
        }
        else
            metaData = "empty";
        return;
    }
    metaData = "undefined";
}

void DELETE::buildFileTransfers()
{
    FileTransferState &state = request.state;
    state.filePath = Server::parseSpecificRequest(request.header);
    state.offset = 0;
    // state.fileSize = Server::getFileSize(PATHC + state.filePath);
    state.isComplete = false;
    state.isValidHeader = false;
    state.logFile.push_back("");
}

void deleteDirectoryContents(const std::string &dir)
{
    DIR *dp = opendir(dir.c_str());
    if (dp == NULL)
    {
        std::cerr << "Error: Unable to open directory " << dir << std::endl;
        return;
    }

    try
    {
        struct dirent *entry;
        while ((entry = readdir(dp)) != NULL)
        {
            if (std::string(entry->d_name) == "." || std::string(entry->d_name) == "..")
            {
                continue;
            }

            struct stat entryStat;
            std::string entryPath = dir + "/" + entry->d_name;
            if (stat(entryPath.c_str(), &entryStat) == -1)
            {
                std::cerr << "Error: Unable to stat " << entryPath << std::endl;
                continue;
            }

            if (S_ISDIR(entryStat.st_mode))
            {
                if (rmdir(entryPath.c_str()) == -1)
                {
                    std::cerr << "Error: Unable to remove directory " << entryPath << std::endl;
                }
            }
            else
            {
                if (unlink(entryPath.c_str()) == -1)
                {
                    std::cerr << "Error: Unable to remove file " << entryPath << std::endl;
                }
            }
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    closedir(dp);
    rmdir(dir.c_str());
}

int DELETE(std::string request)
{
    const char *filename = request.c_str();

    std::cout << "DELETE: " << filename << std::endl;
    return unlink(filename) == -1 ? EXIT_FAILURE : EXIT_SUCCESS;
}

std::string readFiles(std::string path)
{

    std::ifstream infile(path.c_str(), std::ios::binary);
    if (!infile)
        return std::cerr << "Failed to open file:: " << path << std::endl, "";

    std::ostringstream oss;
    oss << infile.rdbuf();
    return oss.str();
}

int Server::handle_delete_request___(int fd, ConfigData configIndex)
{

    std::string filePath = request[fd].state.filePath;
    Location location = getExactLocationBasedOnUrl(filePath, configIndex, addSlashBasedOnMethod).first;
    std::cout << "url :" << filePath << ", location: " << location.path << std::endl;
    size_t checkState = 0;
    if (canBeOpen(fd, filePath, location, checkState))
    {
        checkState = 0;
        if (getFileType(location.path) == 1)
            deleteDirectoryContents(filePath.c_str());
        if (access(location.path.c_str(), X_OK | R_OK | W_OK) == -1)
            return getSpecificRespond(fd, configIndex.getErrorPages().find(403)->second, Forbidden);

        if (DELETE(filePath) == -1)
            return request.erase(fd), close(fd), std::cerr << "Failed to delete file or directory: " << filePath << std::endl, 0;
        request[fd].state.logFile.push_back(location.path);
        std::string httpResponse = deleteResponse(this);
        if (send(fd, httpResponse.c_str(), httpResponse.length(), MSG_NOSIGNAL) == -1)
            return std::cerr << "Failed to send HTTP header." << std::endl, request.erase(fd), close(fd), 0;
        return 0;
    }
    checkState = 0;
    std::vector<std::string>::iterator it = find(request[fd].state.logFile.begin(), request[fd].state.logFile.end(), location.path);
    if (it != request[fd].state.logFile.end())
    {
        return getSpecificRespond(fd, configIndex.getErrorPages().find(410)->second, goneHttpResponse);
    }
    return getSpecificRespond(fd, configIndex.getErrorPages().find(404)->second, createNotFoundResponse);
}
int Server::handle_delete_request(int fd, ConfigData configIndex)
{
    std::string Connection = request[fd].connection;
    std::string filePath = request[fd].state.filePath;
    Location location = getExactLocationBasedOnUrl(filePath, configIndex, addSlashBasedOnMethod).first;
    int state = getFileType(location.root +  filePath);
    state == 1 && filePath.at(filePath.size() - 1) != '/' ? filePath = location.path : filePath;
    size_t checkState = 0;
    if (canBeOpen(fd, filePath, location, checkState))
    {
        std::cout << "filepath: " << filePath << std::endl;
        if (access(filePath.c_str(), R_OK | W_OK) == -1)
            return getSpecificRespond(fd, configIndex.getErrorPages().find(403)->second, Forbidden);
        if (state == 1){
            std::cout << location.root + location.path << std::endl;
            deleteDirectoryContents(location.root + location.path);
        }
        else if (state == 2)
        {
            if (DELETE(filePath.c_str()) == -1)
                return request.erase(fd), close(fd), std::cerr << "Failed to delete file or directory: " << filePath << std::endl, 0;
            
        }

        std::string httpResponse = deleteResponse(this);
        if (send(fd, httpResponse.c_str(), httpResponse.length(), MSG_NOSIGNAL) == -1)
            return std::cerr << "Failed to send HTTP header." << std::endl, request.erase(fd), close(fd), 0;
        return close(fd), request.erase(fd), 0;
    }
    else{
        if (checkState == 301 ){
            filePath = location.redirect;
            
            std::cout << filePath << std::endl;
        }
        return getSpecificRespond(fd, configIndex.getErrorPages().find(404)->second, createNotFoundResponse);
    }

    close(fd);
    request.erase(fd);
    return 0;
}