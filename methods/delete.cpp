
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
    state.url = Server::parseSpecificRequest(request.header);
    state.offset = 0;
    // state.fileSize = Server::getFileSize(PATHC + state.url);
    state.isComplete = false;
    state.isValidHeader = false;
    state.logFile.insert("");
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
                continue;

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
                    std::cerr << "Error: Unable to remove directory " << entryPath << std::endl;
            }
            else
            {
                if (unlink(entryPath.c_str()) == -1)
                    std::cerr << "Error: Unable to remove file " << entryPath << std::endl;
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

std::string readFiles(std::string path)
{

    std::ifstream infile(path.c_str(), std::ios::binary);
    if (!infile)
        return std::cerr << "Failed to open file:: " << path << std::endl, "";

    std::ostringstream oss;
    oss << infile.rdbuf();
    return oss.str();
}

int Server::deleteTargetUrl(int fd, std::string url, ConfigData configdata, Location location, int state)
{
    if (access(url.c_str(), R_OK | W_OK) == -1)
        return getSpecificRespond(fd, configdata.getErrorPages().find(403)->second, Forbidden);
    if (state == 1)
    {
        if (state == 1 && fetchIndex(location.root + location.path, location.index).empty())
            return getSpecificRespond(fd, configdata.getErrorPages().find(404)->second, createNotFoundResponse);
        if (DELETE(url.c_str()) == -1)
        {
            return request.erase(fd), close(fd), std::cerr << "Failed to delete file or directory: " << url << std::endl, 0;
        }
        // deleteDirectoryContents(location.root + location.path);
    }
    else if (state == 2)
    {
        if (DELETE(url.c_str()) == -1)
            return request.erase(fd), close(fd), std::cerr << "Failed to delete file or directory: " << url << std::endl, 0;
    }
    std::string httpResponse = deleteResponse(this);
    if (send(fd, httpResponse.c_str(), httpResponse.length(), MSG_NOSIGNAL) == -1)
        return std::cerr << "Failed to send HTTP header." << std::endl, request.erase(fd), close(fd), 0;
    // return close(fd), request.erase(fd), 0;
    request[fd].state.last_activity_time = time(NULL);
    return 0;
}

int Server::handle_delete_request(int fd, ConfigData configdata)
{
    std::string Connection = request[fd].connection;
    std::string url = request[fd].state.url;
    Location location = getExactLocationBasedOnUrl(url, configdata);
    if (location.path.empty() == true)
        return getSpecificRespond(fd, configdata.getErrorPages().find(404)->second, createNotFoundResponse);
    if (checkAvailability(fd, location) == false)
        return getSpecificRespond(fd, configdata.getErrorPages().find(405)->second, methodNotAllowedResponse), EXIT_FAILURE;
    int state;
    size_t checkState = 0;
    if ((state = getFileType(location.root + url)) == -1)
    {
        return getSpecificRespond(fd, configdata.getErrorPages().find(404)->second, createNotFoundResponse);
    }

    if (state == 1 && location.index.empty())
    {
        return getSpecificRespond(fd, configdata.getErrorPages().find(404)->second, createNotFoundResponse);
    }
    if (canBeOpen(fd, url, location, checkState, configdata))
    {
        return request[fd].state.logFile.insert(url), checkState = 0, deleteTargetUrl(fd, url, configdata, location, state);
    }
    else
    {
        if (checkState == 301)
        {
            std::string httpRespons = MovedPermanently(getContentType(url), location.path);
            if (send(fd, httpRespons.c_str(), httpRespons.length(), MSG_NOSIGNAL) == -1)
                return std::cerr << "Failed to send HTTP header." << std::endl, EXIT_FAILURE;
            return 0;
        }
        if (location.path == "/")
        {
            if (checkState == 404)
                url = location.path;
            if (canBeOpen(fd, url, location, checkState, configdata))
            {
                return checkState = 0, deleteTargetUrl(fd, url, configdata, location, state);
            }
            return checkState = 0, getSpecificRespond(fd, configdata.getErrorPages().find(404)->second, createNotFoundResponse);
        }
    }

    return 0;
}
