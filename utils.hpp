#ifndef UTILS_HPP
#define UTILS_HPP

#include <iostream>
#include <vector>
#include <cstdlib>


int getFileType(std::string path);
int ft_strlen(char *str);
int	ft_atoi(const char *nptr);
std::string trim(const std::string& str, const std::string& chars_to_trim);
std::string& rtrim(std::string& str);
std::string rtrim_copy(std::string str);
int countIndent(const std::string& line);
std::string trim(const std::string& str);
std::vector<std::string> split(const std::string& str, char delimiter);

#endif
