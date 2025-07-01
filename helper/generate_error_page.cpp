#include "../server.hpp"

retfun Server::errorFunction(int errorCode)
{
  if (errorCode == 400)
    return (createBadRequest);
  else if (errorCode == 501)
    return (notImplemented);
  else if (errorCode == 403)
    return (forbidden);
  else if (errorCode == 404)
    return (createNotFoundResponse);
  else if (errorCode == 405)
    return (methodNotAllowedResponse);
  else if (errorCode == 413)
    return (payloadTooLarge);
  else if (errorCode == 500)
    return (internalServerError);
  else if (errorCode == 504)
    return (gatewayTimeout);
  return (internalServerError);
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

void Server::getResponse(int fd, int code)
{
    std::string file;


    if (request[fd].serverConfig.getErrorPages().find(code) == 
          request[fd].serverConfig.getErrorPages().end())
      file = request[fd].serverConfig.getErrorPages().find(code)->second;
    else
      file = "";
    getSpecificRespond(fd, file, errorFunction(code), code);
  
}

int Server::getSpecificRespond(int fd, std::string file, std::string (*f)(std::string, size_t), int code)
{
    std::string content;

    if (!file.empty())
      content = readFile(file);
    if (content.empty() || file.empty())
      content = getErrorPage(code);   
    
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

