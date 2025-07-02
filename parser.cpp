#include "server.hpp"
#include "Cgi.hpp"
#include "request.hpp"
#include "helper/utils.hpp"

std::string concatenate(std::vector<std::string> &vec)
{
  std::string result;
  std::vector<std::string>::iterator it;

  for (it = vec.begin(); it != vec.end(); it++)
  {
    result += '/' + *it;
  }
  return result;
}

std::string resolveUrl(std::string &url)
{
  std::vector<std::string> splitBySlash;
  std::vector<std::string> currentUrl;
  std::string result;

  splitBySlash = split(url, '/');
  std::vector<std::string>::iterator it;
  
  for (it = splitBySlash.begin(); it != splitBySlash.end(); it++)
  {
    if (*it == ".." && !currentUrl.empty())
      currentUrl.pop_back();
    else if (*it == ".." && currentUrl.empty())
      return ("");
    else if (*it != ".")
      currentUrl.push_back(*it);
  }

  if (currentUrl.empty())
    return ("/");
  result = concatenate(currentUrl);
  if (url[url.length() - 1] == '/')
    return (result + "/");
  return (result);
}

template <typename T>

bool header_parser(T method, Request &request, std::string header, std::map<std::string, std::string> tmpMap)
{
  Build build;
  std::ofstream *file = request.state.file;
  Request methaData(header, tmpMap);
  method = T(methaData);
  build.requestBuilder(method);
  if (!build.chainOfResponsibility(method).first)
  {
    return false;
  }

  //[soukaina] can you just assign methaData to request ???

  methaData = method.getRequest();
  request.method = methaData.getMethod();
  request.connection = methaData.getConnection();
  request.transferEncoding = methaData.getTransferEncoding();
  request.contentLength = methaData.getContentLength();
  request.ContentType = methaData.getContentType();
  request.accept = methaData.getAccept();
  request.host = methaData.getHost();
  request.header = header;
  request.Cookie = methaData.getCookie();
  request.state = methaData.getFileTransfers();
  request.state.file = file;
  return true;
}

bool Server::validateHeader(int fd, FileTransferState &state, Binary_String holder)
{
  std::map<std::string, std::string> tmpMap;
  static size_t backup;
  std::string fileName_backup;
  ConfigData serverConfig;
  Request &req = request[fd];

  req.checkHeaderSyntax(holder);

  if (req.getParsingState() == ERROR)
    return (false);
  if (req.getParsingState() == END)
  {
    backup = state.bytesReceived;
    fileName_backup = state.fileName;
    backup = backup - req.bodyStart;

    serverConfig = getConfigForRequest(multiServers[clientToServer[fd]], req.getHost());
    request[fd].serverConfig = serverConfig;
    state.headerFlag = false;
    tmpMap = key_value_pair(ft_parseRequest_T(fd, this, state.header).first);

    if (!(state.header.find("POST") != std::string::npos))
    {
      req.state.file->close();
      delete req.state.file;
      remove(req.state.fileName.c_str());
    }

    if (state.header.find("GET") != std::string::npos)
    {
      GET get;
      if (header_parser(get, req, state.header, tmpMap) == false)
      {
        // [soukaina] bad request
        // [mmad] bad request
        req.code = 404;
        return false;
      }
      req.state.isComplete = true;
    }
    else if (state.header.find("POST") != std::string::npos)
    {
      POST post;

      if (header_parser(post, req, state.header, tmpMap) == false)
      {
        return false;
      }
      req.state.last_activity_time = time(NULL);
    }
    else if (state.header.find("DELETE") != std::string::npos)
    {
      DELETE delete_;
      
      if (header_parser(delete_, req, state.header, tmpMap) == false)
        return false;
        
      req.state.isComplete = true;
    }
    req.location = getExactLocationBasedOnUrl(state.url, req.serverConfig);
    if (req.contentLength == "0")
    {
      req.state.file->close();
      delete req.state.file;
      remove(fileName_backup.c_str());
      state.isComplete = true;
    }

    if (request[fd].state.url.find("/cgi-bin") != std::string::npos)
    {
      request[fd].cgi.parseCgi(request[fd]);
      if (request[fd].code != 200)
        return (false);
      request[fd].cgi.setEnv(request[fd]);
    }

    if (req.getMethod() == "POST" && req.bodyStart)
    { 
      Binary_String body = holder.substr(req.bodyStart, backup);
      
      state.file->write(body.c_str(), backup); //----
      if (static_cast<int>(atoi(req.contentLength.c_str())) <= (int)backup)
      {
        state.isComplete = true;
        state.file->close();
      }
    }

    state.fileName = fileName_backup;
    state.bytesReceived = backup;
    
    if (req.cgi.getIsCgi() == false && req.getContentLength() == "0"
        && req.getMethod() == "POST")
    {
      req.state.last_activity_time = time(NULL);
      req.state.isComplete = true;
      return true;
    }
    state.buffer.clear();
  }
  return true;
}
