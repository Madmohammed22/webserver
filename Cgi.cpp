#include "Cgi.hpp"
#include "request.hpp"
#include "server.hpp"
#include "ConfigData.hpp"
#include "helper/utils.hpp"
 
Cgi::Cgi() :_env(NULL), _argv(NULL), _isCgi(false), _pid(-1), cgiState(CGI_NOT_STARTED)
              , fdIn(-1), bytesSend(0)
{
  if (fdIn != -1)
    close(fdIn);
}

Cgi::~Cgi()
{
  int i;

  i = 0;
  if (_env && *_env)
  {
    while (_env[i])
    {
      free(_env[i]);
      i++;
    }
    free(_env);
  }
  if (_argv)
  {
    free(_argv[0]);
    free(_argv[1]);
    free(_argv);
  }
}

std::string Cgi::getFileExtension(std::string& path, std::string &url)
{
    std::string path_tmp;
    size_t end;
    
    if (!(path.find(".") != std::string::npos))
      return ("");
    path_tmp = path.substr(path.find("."));
    size_t firstSlash = path_tmp.find('/');
    end = firstSlash;
    if (!(firstSlash != std::string::npos))
      end = path_tmp.length();
    url = path.substr(0, end + path.find("."));
    return (path_tmp.substr(0, end));
}

void Cgi::parseCgi(Request &req)
{
  std::string path = req.state.url;
  std::string url;

  size_t quePos = path.find('?');

  if (quePos != std::string::npos)
  {
    setQuery(path.substr(quePos + 1));
    path = path.substr(0, quePos);
  }
  else
    setQuery("");

  if (path == "/cgi-bin")
    _path = path + "/" + req.location.index[0];
  else if (path == "/cgi-bin/")
    _path = path + req.location.index[0];
  else 
    _path = path;

  if (_path[0] == '/')
    _path.erase(0, 1);
   if (getFileType(_path) != 2)
  {
    req.code = 404;
    std::cerr << "CGI: Not a file\n";
    return ;
  }

  if (!(req.location.cgi.find(getFileExtension(_path, url)) != req.location.cgi.end()))
  {
      req.code = 403;
      std::cerr << "CGI: extension not supported\n";
      return ;
  }

  if (access(url.c_str(), R_OK | X_OK ) == -1)
  {
    req.code = 403;
    std::cerr << "CGI: access denied\n";
    return ;
  }

  if (req.getMethod() == "DELETE")
  {
    std::cerr << "CGI: unsupported method\n";
    req.code = 405;
    return ;
  }
  _isCgi = true;
}



std::string Cgi::getPathInfo(std::string &path, std::string &ext)
{
  int pathStart;
  std::string PathInfo;
  
  pathStart = path.find(ext);
  PathInfo = path.substr(pathStart + ext.length());
  return (PathInfo);
}

void Cgi::setEnv(Request &req)
{
  size_t allocSize = 11;
  std::string scriptName;
  std::string ext;
  std::string url;
  std::string query;
  stringstream stream;
  std::string port;
  int i = 0;
  
  stream << req.serverConfig.getPort();
  stream >> port;
  ext = getFileExtension(_path, url);
  _env = (char **)calloc(allocSize + 1, sizeof(char *));
  _env[i++] = strdup(("PATH_INFO" + getPathInfo(_path, ext)).c_str());
  _env[i++] = strdup(("REQUEST_METHOD=" + req.getMethod()).c_str());
  int start = _path.rfind("/");
  scriptName = _path.substr(start + 1, url.length());
  _env[i++] = strdup(("SCRIPT_FILENAME=" + scriptName).c_str());
  _env[i++] = strdup(("QUERY_STRING=" + _query).c_str());
  _env[i++] = strdup(("SERVER_NAME=" + req.getHost()).c_str());
  _env[i++] = strdup(("HTTP_COOKIE=" + req.Cookie).c_str());
  _env[i++] = strdup(("SERVER_PORT" + port).c_str());
  _env[i++] = strdup(("CONTENT_TYPE=" + req.getContentType()).c_str());
  _env[i++] = strdup(("CONTENT_LENGTH=" + req.getContentLength()).c_str());
  _env[i] = NULL;

  _argv = (char **)malloc(sizeof(char *) * 3);
  _argv[0] = strdup(req.location.cgi[ext].c_str());
  _argv[1] = strdup(url.c_str());
  _argv[2] = NULL;
}

void Cgi::runCgi(Server &serv, Request &req)
{

  fileNameOut = createTempFile();

  if (req.getMethod() == "POST" && req.getContentLength() != "0")
  {
    fileNameIn = createTempFile();
    fdIn = open(fileNameIn.c_str(), O_RDWR | O_CREAT, 0644);  
    serv.writePostDataToCgi(req);
    lseek(fdIn, 0, SEEK_SET);
  }

  req.state.last_activity_time = time(NULL);
  int fdOut = open(fileNameOut.c_str(), O_WRONLY | O_CREAT, 0644);  
  _pid = fork();
  if ( _pid < 0 )
  {
    req.code = 500;
    std::cerr << "CGI: fork failed\n";
    return ;
  }

  if ( _pid == 0 )
  {
    dup2(fdOut, STDOUT_FILENO);
    close(fdOut);
    if (req.getMethod() == "POST" && fdIn != -1)
    {
      dup2(fdIn, STDIN_FILENO);
      close(fdIn);
    }
    else 
    {
     int devNull = open("/dev/null", O_RDONLY);
     dup2(devNull, STDIN_FILENO);
     close(devNull);
    }
    if ( execve(_argv[0], _argv, _env) == -1 )
       exit(1);
  }

  close(fdOut);
}

bool Cgi::getIsCgi()
{
  return (_isCgi);
}

std::string Cgi::getQuery()
{
  return (_query);
}

void Cgi::setQuery(std::string query)
{
  _query = query;
}

int Cgi::getStatus()
{
  return (_status);
}

void Cgi::setStatus(int status)
{
  _status = status;
}

pid_t Cgi::getPid()
{
  return (_pid);
}

