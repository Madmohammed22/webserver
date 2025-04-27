#include "../request.hpp"

void Get::BuildPath() {
    std::cout << "-----------------" << std::endl;
    std::cout << this->header << std::endl;
    
    
    // parse Path line in header
    std::cout << "-----------------" << std::endl;


    // initialize final correct data;

    this->request.fileTransfers[this->fd].filePath = "filePath";
}

