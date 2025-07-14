#ifndef BINARY_STRING_HPP
#define BINARY_STRING_HPP

#include "server.hpp"

class binaryString
{
    private:

        std::vector <uint8_t > buffer;
        static const size_t npos = -1;

    public:
        
    // constructors and destructors
    
        binaryString(const char* str, size_t n);
        binaryString();
        binaryString(const binaryString& other);
        binaryString(size_t n);
        ~binaryString();

    // string binary manipulation
    
        size_t find(const char* s, size_t pos = 0) const;
        size_t find(const std::string& s, size_t pos = 0) const;
        binaryString substr(size_t pos, size_t n) const;
        binaryString& append(const char* str, size_t subpos, size_t sublen);
        binaryString& append(const std::string& str, size_t subpos, size_t sublen);
        binaryString& append(const binaryString& str, size_t subpos, size_t sublen);
        void clear();
        std::string to_string() const;
        const char* c_str() const;
        size_t length() const;
        uint8_t operator[](size_t i) const;
        uint8_t& operator[](size_t i);
        uint8_t* data();
        bool empty() const;
        std::vector <uint8_t >::iterator begin();
        std::vector <uint8_t >::iterator end();
        void push_back(uint8_t c);
        binaryString operator+(const binaryString& other) const;
        binaryString operator+=(const binaryString& other);
        bool operator==(const binaryString& other) const;
        bool operator!=(const binaryString& other) const;

};

std::ostream& operator<<(std::ostream& os, const binaryString& buffer);

#endif
