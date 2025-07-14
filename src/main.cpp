#include "../inc/server.hpp"

int main(int argc, char **argv)
{
	/*if (argc != 2)*/
	/*{*/
	/*	std::cerr << "Error\n  Usage : ./webserv <config file>" << std::endl;*/
	/*	return EXIT_FAILURE;*/
	/*}*/
    
    Server server;

    /*server.parseConfig();*/
    server.initServer();
    
    return EXIT_SUCCESS;
}

