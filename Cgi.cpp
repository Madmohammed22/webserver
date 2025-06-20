#include "Cgi.hpp"
#include "request.hpp"
#include "server.hpp"
#include "ConfigData.hpp"
#include "utils.hpp"
 
Cgi::Cgi() :_env(NULL), _argv(NULL), _isCgi(false), _pid(-1), cgiState(CGI_NOT_STARTED)
{
    _pipeIn[0] = _pipeIn[1] = -1;
    _pipeOut[0] = _pipeOut[1] = -1;
}

Cgi::~Cgi()
{
  int i;

  i = 0;
  if (_env)
  {
    while (_env[i])
    {
      free(_env[i]);
    }
    free(_env);
  }
    if (_argv)
    {
        free(_argv[0]);
        free(_argv[1]);
        free(_argv);
    }
  if (_pipeIn[0] != -1)
    close(_pipeIn[0]);
  if (_pipeIn[1] != -1)
    close(_pipeIn[1]);
  if (_pipeOut[0] != -1)
    close(_pipeOut[0]);
  if (_pipeOut[1] != -1)
    close(_pipeOut[1]);
}

void Cgi::parseCgi(Request &req)
{
  std::string path = req.state.url;

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
   // if (getFileType(_path) != 2)
  // {
  //   req.code = 404;
  //   return ;
  // }

  int ExtensionPos = _path.rfind('.');

  // if the file is not executable ( the nginx default behavior is to send it as a static file)
  if (_path.find(".php", ExtensionPos) != std::string::npos
    && _path.find(".py", ExtensionPos) != std::string::npos)
    return ;
  
  //[soukaina] if the access returns -1 a wierdo prb comes test it later, and check if the file can open first
  if (access(_path.c_str(), R_OK | X_OK ) == -1)
  {
    req.code = 403;
    return ;
  }

  // [soukaina] here i supposed that you already check if the method in the request is allowed in the location
  // if not you should add it
  if (req.getMethod() == "DELETE")
  {
    req.code = 405;
    return ;
  }
  _isCgi = true;
}


void Cgi::setEnv(Request &req)
{
  size_t allocSize = 11;
  std::string scriptName;
  std::string ext;
  std::string query;
  int i = 0;

  int start = _path.rfind("/");
  scriptName = _path.substr(start + 1, _path.length());

  _env = (char **)calloc(allocSize + 1, sizeof(char *));
  _env[i++] = strdup(("REQUEST_METHOD=" + req.getMethod()).c_str());
  _env[i++] = strdup(("SCRIPT_NAME=" + _path).c_str());
  _env[i++] = strdup(("SCRIPT_FILENAME=" + scriptName).c_str());
  _env[i++] = strdup(("QUERY_STRING=" + _query).c_str());
  _env[i++] = strdup(("SERVER_NAME=" + req.getHost()).c_str());
  // [soukaina] just for know but it should be changed  
  _env[i++] = strdup("SERVER_PORT=8080");
  _env[i++] = strdup("SERVER_PROTOCOL=HTTP/1.1");
  _env[i++] = strdup(("CONTENT_TYPE=" + req.getContentType()).c_str());
  _env[i++] = strdup(("CONTENT_LENGTH=" + req.getContentLength()).c_str());
  _env[i] = NULL;

  // i should protect this function
  // ext = _path.substr(_path.rfind("."), _path.length());
  ext = ".py";
  _argv = (char **)malloc(sizeof(char *) * 3);
  // [ soukaina ] before that line i should verify if the binary with the extension is present (this is in the parser)
  _argv[0] = strdup(req.location.cgi[ext].c_str());
  _argv[1] = strdup(this->_path.c_str());
  _argv[2] = NULL;
}

void Cgi::runCgi(Server &serv, int fd, Request &req, ConfigData &serverConfig)
{
  (void) serverConfig;
  (void)fd;
  (void )serv;
  (void) req;

  fileName = "cgiTest";
  int fdOut = open(fileName.c_str(), O_WRONLY | O_CREAT, 0644);  
  _pid = fork();
  if ( _pid < 0)
  {
    req.code = 500;
    return ;
  }
  
  // here we are in the child process
  if (_pid == 0)
  {
    dup2(fdOut, STDOUT_FILENO);
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

