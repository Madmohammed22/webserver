#include "../ConfigParsing.hpp"
#include <string>
#include <cctype>
#include <sstream>

std::string& rtrim(std::string& str)
{
    size_t end = str.find_last_not_of(" \t\n\r\f\v");
    
    if (end == std::string::npos)
        str.clear();
    else
        str.erase(end + 1);
    return str;
}

std::string rtrim_copy(std::string str)
{
    rtrim(str);
    return str;
}


int countIndent(const std::string& line)
{
    size_t indent = 0;
    while (indent <= line.size() && (line[indent] == ' ' || line[indent] == '\t'))
    {
        indent++;
    }
    return indent;
}

std::string trim(const std::string& str)
{
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos)
        return ("");
    size_t end = str.find_last_not_of(" \t\n\r");
    return (str.substr(start, end - start + 1));
}

std::vector<std::string> split(const std::string& str, char delimiter)
{
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(str);
    
    while (std::getline(tokenStream, token, delimiter))
    {
        token = trim(token);
        if (!token.empty())
        {
            tokens.push_back(token);
        }
    }
    return tokens;
}

std::string trim(const std::string& str, const std::string& chars_to_trim)
{
    size_t start = str.find_first_not_of(chars_to_trim);
    if (start == std::string::npos)
        return "";
    
    size_t end = str.find_last_not_of(chars_to_trim);
    return str.substr(start, end - start + 1);
}
