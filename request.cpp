
#include "server.hpp"
#include "request.hpp"
#include "helper/utils.hpp"

void Build::requestBuilder(RequstBuilder &requstBuilder)
{
    requstBuilder.buildConnection();
    requstBuilder.buildMethod();
    requstBuilder.buildCookie();
    requstBuilder.buildFileTransfers();
    requstBuilder.buildTransferEncoding();
    requstBuilder.buildContentLength();
    requstBuilder.buildContentType();
    requstBuilder.buildAccept();
    requstBuilder.buildHost();
}

std::pair<bool, int> Build::chainOfResponsibility(RequstBuilder &requstBuilder)
{
    std::pair<bool, int> return_pair;
    return_pair.first = true;
    return_pair.second = 0;
    CookieValidator cookieValidator; 
    ContentLengthValidator contentLengthValidator;
    TransferEncodingValidator transferEncodingValidator;
    ConnectionValidator connectionValidator;
    FileTransferValidator fileTransferValidator;
    ContentTypeValidator contentTypeValidator;

    std::vector<HeaderValidator *> validators;
    validators.push_back(&cookieValidator);
    validators.push_back(&fileTransferValidator);
    validators.push_back(&connectionValidator);
    validators.push_back(&transferEncodingValidator);
    validators.push_back(&contentLengthValidator);
    validators.push_back(&contentTypeValidator);
    for (std::vector<HeaderValidator *>::iterator it = validators.begin(); it != validators.end(); ++it)
    {
        if (!(*it)->validate(requstBuilder))
        {
            return_pair.first = false;
            return_pair.second = -1;
            return return_pair;
        }
    }
    return return_pair;
}

bool FileTransferValidator::validate(RequstBuilder &builder)
{
    std::string method = builder.getRequest().getMethod();
    if (method == "GET" || method == "POST" || method == "DELETE")
    {
        std::string outOfRange = builder.getRequest().state.url;
        if (resolveUrl(outOfRange).empty() )
        {
            return false;
        }
    }
    return true;
}

bool TransferEncodingValidator::validate(RequstBuilder &builder)
{
    if (builder.getRequest().getMethod() == "GET")
    {
        return builder.getRequest().getTransferEncoding() == "undefined";
    }
    else if (builder.getRequest().getMethod() == "POST")
    {
        return builder.getRequest().getTransferEncoding() == "undefined";
    }
    else if (builder.getRequest().getMethod() == "DELETE")
    {
        return builder.getRequest().getTransferEncoding() == "undefined";
    }
    return true;
}

bool ContentTypeValidator::validate(RequstBuilder &builder)
{
    if (builder.getRequest().getMethod() == "GET")
    {
        return builder.getRequest().getContentType() == "undefined";
    }
    else if (builder.getRequest().getMethod() == "POST")
    {
        // return true;
        if (builder.getRequest().getContentLength() == "0"){
            return true;
        }
        return builder.getRequest().getContentType().find("multipart/form-data") != std::string::npos;
    }
    else if (builder.getRequest().getMethod() == "DELETE")
    {
        return builder.getRequest().getContentType() == "undefined";
    }
    return true;
}

bool ContentLengthValidator::validate(RequstBuilder &builder)
{
    if (builder.getRequest().getMethod() == "GET")
    {
        return builder.getRequest().getContentLength() == "undefined";
    }
    else if (builder.getRequest().getMethod() == "POST")
    {
        if (builder.getRequest().getContentLength().empty() || builder.getRequest().getContentLength() == "undefined")
        {
            return false;
        }
        bool check = true;
        std::string str = builder.getRequest().getContentLength();
        for (size_t i = 0; i < str.length(); i++)
        {
            if (!isdigit(str.at(i)))
                check = false;
        }
        return check;
    }
    else if (builder.getRequest().getMethod() == "DELETE")
    {
        return builder.getRequest().getContentLength() == "undefined";
    }
    return false;
}

bool ConnectionValidator::validate(RequstBuilder &builder)
{
    bool check = builder.getRequest().getConnection() == "close" || builder.getRequest().getConnection() == "keep-alive" || builder.getRequest().getConnection() == "undefined" || !builder.getRequest().getConnection().empty();

    return check;
}

bool CookieValidator::validate(RequstBuilder &builder){
    (void)builder;
    return true;
}

bool   IsUnreservedChar(char c)
{
    return ( isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~');
}

bool isValidHeaderKeyChar(char c)
{
    return (isalnum(c) || c == '-' || c == '_');
}

bool isValidHeaderValueChar(char c)
{
    std::string specialChars = "_ :;.,\\/\"'?!(){}[]@<>=-+*#$&`|~^%";
    
    return (isalnum(c) || specialChars.find(c) != std::string::npos);
}

bool IsReservedChar(char c)
{
    std::string reserved_chars = "!*'();:@&=+$,/?#[]";

    return (reserved_chars.find(c) != std::string::npos);
}

int    Request::checkHeaderSyntax(Binary_String buffer)
{
    int i = 0;

    while (buffer[i])
    {
        if (_parsingState == START)
        {
            if (buffer[i] == 'G')
                method = "ET";
            else if (buffer[i] == 'P')
                method = "OST";
            else if (buffer[i] == 'D')
                method = "ELETE";
            else
            {
                code = 501;
                _parsingState = ERROR;
                break ;
            }
            _parsingState = Method;                   
        }
        else if (_parsingState == Method)
        {
            if (!method.length())
            {
                _parsingState = FirstSpace;
                continue ;
            }
            else
            {
                if (method[0] == buffer[i])
                    method.erase(0, 1);
                else
                {
                    code = 400;
                    _parsingState = ERROR;
                    break;
                }
            }
        }
        else if (_parsingState == FirstSpace)
        {
            if (buffer[i] != ' ')
            {
                code = 400;
                _parsingState = ERROR;
                break;
            }
            _parsingState = SlashURI; 
        }
        else if (_parsingState == SlashURI)
        {
            if (buffer[i] != '/')
            {
                code = 400;
                _parsingState = ERROR;
                break;
            }
            _parsingState = URIBody;
        }
        else if (_parsingState == URIBody)
        {
            if (buffer[i] == ' ')
            {
                _parsingState = StringH;
            }
            if (!IsReservedChar(buffer[i]) == false && !IsUnreservedChar(buffer[i]) == false)
            {
                code = 400;
                _parsingState = ERROR;
                break;
            }
            _urlLength++;
            if (_urlLength > MAX_URI_SIZE)
            {
                code = 400;
                _parsingState = ERROR;
                break;
            }
        }
        else if (_parsingState == StringH)
        {
                if (buffer[i] != 'H')
                {
                    code = 400;
                    _parsingState = ERROR;
                    break;
                }
                _parsingState = StringHT;
        }
        else if (_parsingState == StringHT)
        {
                if (buffer[i] != 'T')
                {
                    code = 400;
                    _parsingState = ERROR;
                    break;
                }
                _parsingState = StringHTT;
        }
        else if (_parsingState == StringHTT)
        {
                if (buffer[i] != 'T')
                {
                    code = 400;
                    _parsingState = ERROR;
                    break;
                }
                _parsingState = StringHTTP;
        }
        else if (_parsingState == StringHTTP)
        {
                if (buffer[i] != 'P')
                {
                    code = 400;
                    _parsingState = ERROR;
                    break;
                }
                _parsingState = StringHTTPSlash;
        }
        else if (_parsingState == StringHTTPSlash)
        {
                if (buffer[i] != '/')
                {
                    code = 400;
                    _parsingState = ERROR;
                    break;
                }
                _parsingState = StringHTTPSlashN;
        }
        else if (_parsingState == StringHTTPSlashN)
        {
                if (!isdigit(buffer[i]))
                {
                    code = 400;
                    _parsingState = ERROR;
                    break;
                }
                _parsingState = StringHTTPSlashNDot;
        }
        else if (_parsingState == StringHTTPSlashNDot)
        {
                if (buffer[i] != '.')
                {
                    code = 400;
                    _parsingState = ERROR;
                    break;
                }
                _parsingState = StringHTTPSlashNDotN;
        }
        else if (_parsingState == StringHTTPSlashNDotN)
        {
                if (!isdigit(buffer[i]))
                {
                    code = 400;
                    _parsingState = ERROR;
                    break;
                }
                _parsingState = FirstSlashR;
        }
        else if (_parsingState == FirstSlashR)
        {
                if (buffer[i] != '\r')
                {
                    code = 400;
                    _parsingState = ERROR;
                    break;
                }
                _parsingState = FirstSlashN;
        }
        else if (_parsingState == FirstSlashN)
        {
                if (buffer[i] != '\n')
                {
                    code = 400;
                    _parsingState = ERROR;
                    break;
                }
                _parsingState = HeaderKey;
        }
        else if (_parsingState == HeaderKey)
        {
            if (buffer[i] == '\r')
                _parsingState = TERMINATOR;
            else if (buffer[i] == '\n')
            {
                _keyLength = 0;
            }
            else if (buffer[i] == ':')
            {
                _parsingState = HeaderValue;
                state.header += buffer[i];
                i++;
            }
            else if (!isValidHeaderKeyChar(buffer[i]))
            {
                code = 400;
                _parsingState = ERROR;
                break;
            }
            _keyLength++;
            if (_keyLength > MAX_HEADER_KEY_SIZE)
            {
                code = 400;
                _parsingState = ERROR;
                break;
            }
        }
        else if (_parsingState == HeaderValue)
        {
            if (buffer[i] == '\r')
            {
                _valueLength = 0;
                _parsingState = HeaderKey;
            }
            else if (!isValidHeaderValueChar(buffer[i]))
            {
                code = 400;
                _parsingState = ERROR;
                break;
            }
            _valueLength++;
            if (_valueLength > MAX_HEADER_VALUE_SIZE)
            {
                code = 400;
                _parsingState = ERROR;
                break;
            }
        }
        else if (_parsingState == TERMINATOR)
        {
            if (buffer[i] != '\n')
            {
                code = 400;
                break;
            }
            state.header += buffer[i];
            state.header += '\0';
            if (buffer[i + 1] != '\0')
                bodyStart = i + 1;
            _parsingState = END;
            return code;
        }
        std::cout << _parsingState << std::endl;
        state.header += buffer[i];
        i++;
    }
    return code;
}
