#ifndef WEBSERVEXCEPTION_HPP
#define WEBSERVEXCEPTION_HPP

#include <exception>
#include <string>

class WebservException : public std::exception
{
private:

    std::string _message;
    std::string _context;

public:

    WebservException(const std::string& message, const std::string& context = "")
        : _message("Error: " + message + "\n"), _context(context){}

    virtual ~WebservException() throw() {}
    
    virtual const char* what() const throw()
    {
        return _message.c_str();
    }
    WebservException(const WebservException&);
    WebservException& operator=(const WebservException&);
};
#endif
