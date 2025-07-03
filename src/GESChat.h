#pragma once

#define WIN32_LEAN_AND_MEAN

//replace with CLIENT or SERVER to enable selected mode
#define NEITHER_CLIENT_NOR_SERVER
void run(int argc, char* argv[]);

#define LOCALHOST "127.0.0.1"
#define DEFAULT_PORT "1488"
#define MAX_BACKLOG "128"

struct ArgsParser
{
	int argc;
	char** argv;

	const char* getOrDefault(const char* key, const char* def) const;
};


bool isValidUsername(const char* username);

void copyarray(const char* source, char* dest, size_t soff, size_t doff, size_t len);
