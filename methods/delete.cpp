#include "../server.hpp"

void DELETE::includeBuild(std::string target, std::string &metaData, int pick)
{
    std::map<std::string, std::string>::iterator it = request.keys.find(target);
    if (it != request.keys.end())
    {
        if (Server::containsOnlyWhitespace(it->second) == false){
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
    state.fileSize = Server::getFileSize(PATHC + state.filePath);
    state.isComplete = false;
    state.isValidHeader = false;
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
    return unlink(filename) == -1 ? EXIT_FAILURE : EXIT_SUCCESS;   
}

int Server::handle_delete_request(int fd)
{
    std::string filePath = request[fd].state.filePath;
    if (canBeOpen(filePath))
    {
        if (filePath.at(0) != '/')
            filePath = "/" + filePath;
        if (getFileType(filePath) == 1)
            deleteDirectoryContents(filePath.c_str());

        if (DELETE(filePath) == -1)
            return request.erase(fd), close(fd), std::cerr << "Failed to delete file or directory: " << filePath << std::endl, 0;
        std::string httpResponse = deleteHttpResponse(this);
        if (send(fd, httpResponse.c_str(), httpResponse.length(), MSG_NOSIGNAL) == -1)
        {
            std::cerr << "Failed to send HTTP header." << std::endl;
            return request.erase(fd), close(fd), 0;
        }
        return request.erase(fd), close(fd), 0;
    }
    return getSpecificRespond(fd, this, "404.html", createNotFoundResponse);
}