#pragma once

#define WIN32_LEAN_AND_MEAN

//replce with CLIENT or SERVER to turn on selected 
#define NEITHER_SERVER_NOR_CLIENT
void run(int argc, char* argv[]);

#define LOCALHOST "127.0.0.1"
#define DEFAULT_PORT "1488"
#define MAX_BACKLOG "128"

#define CLIENT_EXIT_CODE "~!"

#define ANSI(code) "\x1b[" << code

#ifdef SERVER
	#define LOG(data) std::cout << data << std::endl
#endif

struct ArgsParser
{
	int argc;
	char** argv;

	const char* getOrDefault(const char* key, const char* def) const;
};


void copyarray(const char* source, char* dest, size_t soff, size_t doff, size_t len);
