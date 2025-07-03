#include "GESChat.h"
#include "ConsoleHandler.h"

#include <string>
#include <iostream>

//Defined in GESChat.cpp
int startupWSA();

int main(int argc, char* argv[]) {
	try
	{
		if (startupWSA()) throw std::exception{ "WSA Startup failed" };
		run(argc, argv);
	}
	catch (const std::exception& e)
	{
		std::cerr << ANSI("31m") << e.what() << ANSI("0m") << std::endl << std::endl;
		std::system("pause");
	}
    return 0;
}
