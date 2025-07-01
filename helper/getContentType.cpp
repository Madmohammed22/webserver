#include "../server.hpp"

std::string Server::getContentType(const std::string &path)
{
    std::map<std::string, std::string> extensionToType;
    extensionToType.insert(std::make_pair(std::string(".html"), std::string("text/html")));
    extensionToType.insert(std::make_pair(std::string(".css"), std::string("text/css")));
    extensionToType.insert(std::make_pair(std::string(".txt"), std::string("text/txt")));
    extensionToType.insert(std::make_pair(std::string(".js"), std::string("application/javascript")));
    extensionToType.insert(std::make_pair(std::string(".json"), std::string("application/json")));
    extensionToType.insert(std::make_pair(std::string(".cpp"), std::string("application/cpp")));
    extensionToType.insert(std::make_pair(std::string(".xml"), std::string("application/xml")));
    extensionToType.insert(std::make_pair(std::string(".mp4"), std::string("video/mp4")));
    extensionToType.insert(std::make_pair(std::string(".mp3"), std::string("audio/mpeg")));
    extensionToType.insert(std::make_pair(std::string(".wav"), std::string("audio/wav")));
    extensionToType.insert(std::make_pair(std::string(".pdf"), std::string("text/pdf")));
    extensionToType.insert(std::make_pair(std::string(".ogg"), std::string("audio/ogg")));
    extensionToType.insert(std::make_pair(std::string(".png"), std::string("image/png")));
    extensionToType.insert(std::make_pair(std::string(".jpg"), std::string("image/jpeg")));
    extensionToType.insert(std::make_pair(std::string(".jpeg"), std::string("image/jpeg")));
    extensionToType.insert(std::make_pair(std::string(".gif"), std::string("image/gif")));
    extensionToType.insert(std::make_pair(std::string(".svg"), std::string("image/svg+xml")));
    extensionToType.insert(std::make_pair(std::string(".ico"), std::string("image/x-icon")));

    size_t dotPos = path.rfind('.');
    if (dotPos != std::string::npos)
    {
        std::string extension = path.substr(dotPos);
        std::map<std::string, std::string>::const_iterator it = extensionToType.find(extension);
        if (it != extensionToType.end())
            return it->second;
    }
    return "application/octet-stream";
}

std::string Server::getCurrentTimeInGMT()
{
    time_t t = time(0);
    tm *time_struct = gmtime(&t); // Use gmtime to get UTC time

    char buffer[100];
    strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", time_struct);
    std::string date = buffer;
    return date;
}

int Server::getFileType(std::string path)
{
    struct stat s;
    if (stat(path.c_str(), &s) == 0)
    {
        if (s.st_mode & S_IFDIR) // dir
            return 1;
        if (s.st_mode & S_IFREG) // file
            return 2;
    }
    return -1;
}

std::string Server::createChunkedHttpResponse(std::string contentType)
{
    std::ostringstream oss;
    oss << "HTTP/1.1 200 OK\r\n"
        << "Transfer-Encoding: chunked\r\n"
        << "Content-Type: " << contentType + "; charset=utf-8" << "\r\n"
        << "Last-Modified: " << getCurrentTimeInGMT() << "\r\n\r\n";
    return oss.str();
}

std::string Server::httpResponse(std::string contentType, size_t contentLength)
{
    std::ostringstream oss;
    oss << "HTTP/1.1 200 OK\r\n"
        << "Content-Type: " << contentType + "; charset=utf-8" << "\r\n"
        << "Last-Modified: " << getCurrentTimeInGMT() << "\r\n"
        << "Content-Length: " << contentLength << "\r\n\r\n";
    return oss.str();
}

std::string Server::httpResponseIncludeCookie(std::string contentType, size_t contentLength, std::string setCookie)
{
    std::ostringstream oss;
    oss << "HTTP/1.1 200 OK\r\n"
        << "Content-Type: " << contentType + "; charset=utf-8" << "\r\n"
        << "Last-Modified: " << getCurrentTimeInGMT() << "\r\n"
        << "setCookie: " << setCookie << "\r\n"
        << "Content-Length: " << contentLength << "\r\n\r\n";
    return oss.str();
}

std::string Server::Forbidden(std::string contentType, size_t contentLength)
{
    std::ostringstream oss;
    oss << "HTTP/1.1 403 OK\r\n"
        << "Content-Type: " << contentType + "; charset=utf-8" << "\r\n"
        << "Last-Modified: " << getCurrentTimeInGMT() << "\r\n"
        << "Content-Length: " << contentLength << "\r\n\r\n";
    return oss.str();
}

std::string Server::createNotFoundResponse(std::string contentType, size_t contentLength)
{
    std::ostringstream oss;
    oss << "HTTP/1.1 404 Not Found\r\n"
        << "Content-Type: " << contentType + "; charset=utf-8" << "\r\n"
        << "Last-Modified: " << getCurrentTimeInGMT() << "\r\n"
        << "Content-Length: " << contentLength << "\r\n\r\n";

    return oss.str();
}

std::string Server::createUnsupportedMediaResponse(std::string contentType, size_t contentLength)
{
    std::ostringstream oss;
    oss << "HTTP/1.1 415 Unsupported Media Type\r\n"
        << "Content-Type: " << contentType + "; charset=utf-8" << "\r\n"
        << "Last-Modified: " << getCurrentTimeInGMT() << "\r\n"
        << "Content-Length: " << contentLength << "\r\n\r\n";

    return oss.str();
}

std::string getCurrentTimeInGMT1()
{
    time_t t = time(0);
    tm *time_struct = gmtime(&t); // Use gmtime to get UTC time

    char buffer[100];
    strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", time_struct);
    std::string date = buffer;
    return date;
}

std::string Server::createBadRequest(std::string contentType, size_t contentLength)
{
    std::ostringstream oss;
    oss << "HTTP/1.1 400 Bad Request\r\n"
        << "Content-Type: " << contentType + "; charset=utf-8" << "\r\n"
        << "Last-Modified: " << getCurrentTimeInGMT1() << "\r\n"
        << "Content-Length: " << contentLength << "\r\n\r\n";

    return oss.str();
}

std::string Server::goneHttpResponse(std::string contentType, size_t contentLength)
{
    std::ostringstream oss;
    oss << "HTTP/1.1 410 Gone\r\n"
        << "Content-Type: " << contentType + "; charset=utf-8" << "\r\n"
        << "Last-Modified: " << getCurrentTimeInGMT1() << "\r\n"
        << "Content-Length: " << contentLength << "\r\n\r\n";
    return oss.str();
}

std::string Server::notImplemented(std::string contentType, size_t contentLength)
{
    std::ostringstream oss;
    oss << "HTTP/1.1 501 Not Implemented\r\n"
        << "Content-Type: " << contentType + "; charset=utf-8" << "\r\n"
        << "Last-Modified: " << getCurrentTimeInGMT1() << "\r\n"
        << "Content-Length: " << contentLength << "\r\n\r\n";
    return oss.str();
}

std::string Server::internalServerError(std::string contentType, size_t contentLength)
{
    std::ostringstream oss;
    oss << "HTTP/1.1 500 Internal Server Error\r\n" 
        << "Content-Type: " << contentType + "; charset=utf-8" << "\r\n"
        << "Last-Modified: " << getCurrentTimeInGMT1() << "\r\n"
        << "Content-Length: " << contentLength << "\r\n\r\n";
    return oss.str();
}

std::string Server::payloadTooLarge(std::string contentType, size_t contentLength)
{
    std::ostringstream oss;
    oss << "HTTP/1.1 413 Payload Too Large\r\n"
        << "Content-Type: " << contentType + "; charset=utf-8" << "\r\n"
        << "Last-Modified: " << getCurrentTimeInGMT1() << "\r\n"
        << "Content-Length: " << contentLength << "\r\n\r\n";
    return oss.str();
}

std::string Server::methodNotAllowedResponse(std::string contentType, size_t contentLength)
{
    (void)contentLength;
    std::ostringstream oss;
    oss << "HTTP/1.1 405 Method Not Allowed\r\n"
        << "Content-Type: " << contentType + "; charset=utf-8" << "\r\n"
        << "Allow: GET, POST, DELETE\r\n\r\n";
    // << "Content-Length: " << contentLength << "\r\n\r\n";

    return oss.str();
}

std::string Server::createTimeoutResponse(std::string contentType, size_t contentLength)
{
    std::ostringstream oss;
    oss << "HTTP/1.1 408 Request Timeout\r\n"
        << "Content-Type: " << contentType + "; charset=utf-8" << "\r\n"
        << "Last-Modified: " << getCurrentTimeInGMT() << "\r\n"
        << "Content-Length: " << contentLength << "\r\n\r\n";

    return oss.str();
}

std::string Server::deleteResponse(Server *server)
{
    std::ostringstream oss;
    oss << "HTTP/1.1 204 No Content\r\n"
        << "Last-Modified: " << server->getCurrentTimeInGMT() << "\r\n\r\n";
    return oss.str();
}

std::string Server::MovedPermanently(std::string contentType, std::string location)
{
    std::ostringstream oss;
    oss << "HTTP/1.1 301 Moved Permanently\r\n"
        << "Location: " << location << "\r\n"
        << "content-type: " << contentType << "\r\n\r\n";
    return oss.str();
}

std::string Server::listDirectory(const std::string &dir_path, const std::string &url, std::string &mime)
{
    std::string nameFile = "file.html";
    mime = getContentType(nameFile);
    std::ofstream outFile(nameFile.c_str());
    if (!outFile.is_open())
    {
        std::cerr << "Failed to open file:: " << url << std::endl;
        return "";
    }

    outFile << "<!DOCTYPE html>\n"
            << "<html>\n"
            << "<head>\n"
            << "    <title>Directory Listing</title>\n"
            << "</head>\n"
            << "<body>\n"
            << "    <h1>Index of " << url << "</h1>\n"
            << "    <hr>\n"
            << "    <pre>\n";
    DIR *dp = opendir(dir_path.c_str());
    if (dp == NULL)
    {
        std::cerr << "Error: Unable to open directory " << dir_path << std::endl;
        return "";
    }
    struct dirent *entry;

    while ((entry = readdir(dp)) != NULL)
    {
        outFile << "     <a href=\"" << entry->d_name << "\">" << entry->d_name << "</a>\n";
    }

    closedir(dp);
    outFile << "    </pre>\n"
            << "    <hr>\n"
            << "</body>\n"
            << "</html>\n";
    outFile.close();
    std::ostringstream os;
    std::ifstream inFile(nameFile.c_str(), std::ios::binary);
    if (!inFile)
        return "";
    os << inFile.rdbuf();
    inFile.close();
    unlink(nameFile.c_str());
    return os.str();
}

std::string statusCodeString(int statusCode)
{
  switch (statusCode)
  {
      case 400:
        return "Bad Request";
      case 401:
        return "Unauthorized";
      case 403:
        return "Forbidden";
      case 404:
        return "Not Found";
      case 405:
        return "Method Not Allowed";
      case 413:
        return "Payload Too Large";
      case 414:
        return "URI Too Long";
      case 415:
        return "Unsupported Media Type";
      case 500:
        return "Internal Server Error";
      case 501:
        return "Not Implemented";
      case 502:
        return "Bad Gateway";
      case 503:
        return "Service Unavailable";
      case 504:
        return "Gateway Timeout";
      case 505:
        return "HTTP Version Not Supported";
      default:
        return "Internal Server Error";
    }
}

std::string getErrorPage(int errorCode)
{
    std::ostringstream oss;
    
    oss << "<html>\r\n"
        << "<head><title>" << errorCode << " " << statusCodeString(errorCode) << "</title></head>\r\n"
        << "<body>\r\n"
        << "<center><h1>" << errorCode << " " << statusCodeString(errorCode) << "</h1></center>\r\n"
        << "<hr>\r\n"
        << "<center>WebServer</center>\r\n"
        << "</body>\r\n"
        << "</html>";
    
    return oss.str();
}

int Server::getSpecificRespond(int fd, std::string file, std::string (*f)(std::string, size_t))
{
    std::string content;

    if (!file.empty())
      content = readFile(file);
    if (content.empty() || file.empty())
      content = getErrorPage(400);   
        
    std::string httpResponse = f(getContentType(file), content.length());
    try
    {
        if (send(fd, httpResponse.c_str(), httpResponse.length(), MSG_NOSIGNAL) == -1)
            throw std::runtime_error("Failed to send error response header");

        if (send(fd, content.c_str(), content.length(), MSG_NOSIGNAL) == -1)
            throw std::runtime_error("Failed to send error content");

        if (send(fd, "\r\n\r\n", 2, MSG_NOSIGNAL) == -1)
            throw std::runtime_error("Failed to send final CRLF");
        if (request[fd].getConnection() == "close")
            return request.erase(fd), close(fd), 0;
        else
        {
            request[fd].state.isComplete = true;
            close(fd);
            request.erase(fd);
        }
        return 0;
    }
    catch (const std::exception &e)
    {
        return 0;
    }
    return 0;
}

retfun Server::errorFunction(int errorCode)
{
  if (errorCode == 400)
    return (createBadRequest);
  else if (errorCode == 501)
    return (notImplemented);
  else if (errorCode == 404)
    return (createNotFoundResponse);
  else if (errorCode == 405)
    return (methodNotAllowedResponse);
  else if (errorCode == 413)
    return (payloadTooLarge);
  else if (errorCode == 500)
    return (internalServerError);
  return (internalServerError);
}


