#include "GESChat.h"

#include <iostream>
#include <cctype>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "GCProtocol.h"

#pragma comment(lib, "ws2_32.lib")

int startupWSA()
{
	WSAData wsa;
	return WSAStartup(MAKEWORD(2, 2), &wsa);
}

const char* ArgsParser::getOrDefault(const char* key, const char* def) const
{
    for (size_t i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], key)) continue;
        return i >= argc - 1 ? def : argv[i + 1];
    }
    return def;
}

void copyarray(const char* source, char* dest, size_t soff, size_t doff, size_t len)
{
    for (size_t i = 0; i < len; i++) dest[doff + i] = source[soff + i];
}

int compareGCProtocolVersion(unsigned char major, unsigned char minor)
{
    short thisVersion = 256 * GC_PROTOCOL_VERSION_MAJOR + GC_PROTOCOL_VERSION_MINOR;
    short clientVersion = 256 * major + minor;
    return clientVersion - thisVersion;
}

bool isValidUsername(const char* username)
{
    for (size_t i = 0; i < USERNAME_LENGTH; i++)
    {
        char c = username[i];
        if (c && !isalnum(c) && c != '_') return false;
    }
    return true;
}

void enableANSI()
{
    SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
}
void removeANSIEscapeSequences(char* data, size_t size)
{
    for (size_t i = 0; i < size; i++)
    {
        if (data[i] == '\x1b') data[i] = '~';
    }
}

std::string stbStatus2String(const char type)
{
    switch (type)
    {
    case SERVER_TECHNICAL_BROADCASTING_STATUS_USER_JOINED: return "joined";
    case SERVER_TECHNICAL_BROADCASTING_STATUS_USER_LEFT: return "left";
    default: return "";
    }
}
