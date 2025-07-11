#include "GESChat.h"

#ifdef CLIENT

#include <iostream>
#include <string>
#include <cstring>
#include <thread>
#include <mutex>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "GCProtocol.h"
#include "ConsoleHandler.h"

#pragma comment(lib, "ws2_32.lib")

#define CLIENT_EXIT_CODE "~!"



SOCKET init(PCSTR host, PCSTR port);
void handshake();

void listenConsole();
int sendMessage(const char* message);

void listenServer();
std::string stbStatus2String(const char type);

struct Client
{
	SOCKET sock;
	const char* username;
	bool running;

	std::mutex mtx;

	std::string inputLine = "";

	int snd(const char* buf, int len) const;
	int rcv(char* buf, int len) const;
} THIS_CLIENT{ INVALID_SOCKET, nullptr, false };

#define C_CH_SAVE_AND_PRINT_INPUT ANSI("s") << "\n\r[" << THIS_CLIENT.username << "]: " << THIS_CLIENT.inputLine
#define C_CH_RETURN_BACK_AND_ERASE ANSI("u") << ANSI("J")



int Client::snd(const char* buf, int len) const { return send(sock, buf, len, 0); }
int Client::rcv(char* buf, int len) const { return recv(sock, buf, len, 0); }

void run(int argc, char* argv[])
{
	enableANSI();

	std::cout << "Using GC Protocol Version: " << (int)GC_PROTOCOL_VERSION_MAJOR << "." << (int)GC_PROTOCOL_VERSION_MINOR << std::endl;

	ArgsParser ap{ argc, argv };

	PCSTR host = ap.getOrDefault("-h", LOCALHOST);
	PCSTR port = ap.getOrDefault("-p", DEFAULT_PORT);
	std::cout << "Connecting to server: " << host << ":" << port << std::endl;

	char username[USERNAME_LENGTH];
	memset(username, 0, USERNAME_LENGTH);
	
	std::cout << "Enter your name: ";
	std::cin >> username;
	username[USERNAME_LENGTH - 1] = 0;

	std::cout << ANSI("1F") << ANSI("J");
	std::cout << "Your username: " << username << std::endl;

	SOCKET sock = init(host, port);

	std::cout << "--------------------------------------------" << std::endl;

	THIS_CLIENT.sock = sock;
	THIS_CLIENT.username = username;

	handshake();

	THIS_CLIENT.running = true;

	std::thread serverListener{ listenServer };
	serverListener.detach();

	listenConsole();

	closesocket(sock);
	std::cout << "\n\rClient closed" << std::endl;
	WSACleanup();
}

SOCKET init(PCSTR host, PCSTR port)
{
	int status;

	SOCKET sock = INVALID_SOCKET;

	addrinfo hints{}, * result{};
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	status = getaddrinfo(host, port, &hints, &result);
	if (status) throw std::exception{ "getaddrinfo() failed" };

	sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (sock == INVALID_SOCKET) throw std::exception{ "socket() failed" };

	status = connect(sock, result->ai_addr, (int)result->ai_addrlen);
	if (status == SOCKET_ERROR) throw std::exception{ "Connection failed" };

	freeaddrinfo(result);

	return sock;
}

void handshake()
{
	char hsh[HANDSHAKE_LENGTH_CS];
	memset(hsh, 0, HANDSHAKE_LENGTH_CS);
	hsh[0] = HANDSHAKE;
	hsh[1] = GC_PROTOCOL_VERSION_MAJOR;
	hsh[2] = GC_PROTOCOL_VERSION_MINOR;
	copyarray(THIS_CLIENT.username, hsh, 0, 1 + GC_PROTOCOL_VERSION_LENGTH, strlen(THIS_CLIENT.username));
	hsh[HANDSHAKE_LENGTH_CS - 1] = 0;

	THIS_CLIENT.snd(hsh, HANDSHAKE_LENGTH_CS);

	char hsh_s[HANDSHAKE_LENGTH_SC];
	THIS_CLIENT.rcv(hsh_s, HANDSHAKE_LENGTH_SC);
	if (hsh_s[0] != HANDSHAKE) throw std::exception{ "Invalid server response" };

	char errorCode = hsh_s[1];
	if (errorCode == SERVER_ERROR_TRANSCENDED_BACKLOG)
		throw std::exception{ "Server is full. Try connect later :(" };
	else if (errorCode == SERVER_ERROR_DUPLICATE_USERNAME)
		throw std::exception{ ("User with name '" + std::string{ THIS_CLIENT.username } + "' is already on the server").c_str() };
	else if (errorCode == SERVER_ERROR_INCOMPATABLE_PROTOCOL_VERSION)
	{
		char serverMajor = hsh_s[2], serverMinor = hsh_s[3];
		throw std::exception{ ("Your client uses different GC Protocol version: " +
			std::to_string((int)GC_PROTOCOL_VERSION_MAJOR) + "." + std::to_string((int)GC_PROTOCOL_VERSION_MINOR) + 
			". Server requires: " + std::to_string((int)serverMajor) + "." + std::to_string((int)serverMinor)).c_str() };
	}
	else if (errorCode == SERVER_ERROR_INVALID_USERNAME)
		throw std::exception{ "Inavlid username. Username must match [a-z;A-Z;0-9;_]" };
	else if (hsh_s[1] != SERVER_OK) throw std::exception{ ("Server sent error code: " + std::to_string((int)errorCode)).c_str() };
}

void listenServer()
{
	int status = 0;

	while (true)
	{
		if (!THIS_CLIENT.running) break;

		char buffer[MAX_CLIENT_POSSIBLE_INPUT];
		status = THIS_CLIENT.rcv(buffer, MAX_CLIENT_POSSIBLE_INPUT);
		if (status == SOCKET_ERROR)
		{
			THIS_CLIENT.running = false;
			break;
		}

		char code = buffer[0];

		if (code == BROADCASTING_MESSAGE)
		{
			const char* name = buffer + 1;
			const char* message = buffer + 1 + USERNAME_LENGTH;

			CH_MTXLCK_PRINT( THIS_CLIENT.mtx,
				C_CH_RETURN_BACK_AND_ERASE << 
				"<" << name << "> " << message << "\n\r" <<
				C_CH_SAVE_AND_PRINT_INPUT
			);
		}
		else if (code == SERVER_TECHNICAL_BROADCASTING)
		{
			const char* name = buffer + 1 + 1;

			CH_MTXLCK_PRINT(THIS_CLIENT.mtx,
				C_CH_RETURN_BACK_AND_ERASE << ANSI("3m") << ANSI("33;1m") <<
				"[SERVER]: " << name << " " << stbStatus2String(buffer[1]) <<
				ANSI("0m") << "\n\r" <<
				C_CH_SAVE_AND_PRINT_INPUT
			);
		}
		else if (code == FORWARDING_PRIVATE_MESSAGE)
		{
			char type = buffer[1];
			if (type == FORWARDING_PRIVATE_MESSAGE_STATUS_OK)
			{
				const char* sender = buffer + 1 + 1;
				const char* receiver = buffer + 1 + 1 + USERNAME_LENGTH;
				const char* message = buffer + 1 + 1 + USERNAME_LENGTH + USERNAME_LENGTH;

				CH_MTXLCK_PRINT( THIS_CLIENT.mtx,
					C_CH_RETURN_BACK_AND_ERASE << ANSI("3m") << 
					"<" << sender << " -> " << receiver << "> " << message << 
					ANSI("0m") << "\n\r" <<
					C_CH_SAVE_AND_PRINT_INPUT
				);
			}
			else if (type == FORWARDING_PRIVATE_MESSAGE_STATUS_USER_NOT_FOUND)
			{
				const char* misspelledName = buffer + 1 + 1;

				CH_MTXLCK_PRINT( THIS_CLIENT.mtx,
					C_CH_RETURN_BACK_AND_ERASE << ANSI("3m") << ANSI("31m") <<
					"[SERVER]: " << "User '" << misspelledName << "' not found" << 
					ANSI("0m") << "\n\r" <<
					C_CH_SAVE_AND_PRINT_INPUT
				);
			}
		}
	}
}

void listenConsole()
{
	CH_MTXLCK_PRINT(THIS_CLIENT.mtx, ANSI("s"));

	char ch;
	getchar();     //Skip first \n from gathering name
	int status = 0;

	while (true)
	{
		if (!THIS_CLIENT.running) break;

		ch = getchar();
		if (ch == '\b')
		{
			if (!THIS_CLIENT.inputLine.empty())
			{
				THIS_CLIENT.inputLine.pop_back();
				std::cout << "\b \b";
			}
		}
		else if (ch == '\n')
		{
			if (THIS_CLIENT.inputLine == CLIENT_EXIT_CODE)
			{
				THIS_CLIENT.running = false;
				break;
			}

			if (THIS_CLIENT.inputLine.empty())
			{
				CH_MTXLCK_PRINT(THIS_CLIENT.mtx,
					ANSI("2F") << ANSI("J") << C_CH_SAVE_AND_PRINT_INPUT
				);
				continue;
			}

			status = sendMessage(THIS_CLIENT.inputLine.c_str());

			if (status == SOCKET_ERROR)
			{
				THIS_CLIENT.running = false;
				throw std::exception{ "Server closed connection" };
			}

			THIS_CLIENT.inputLine.clear();
		}
		else THIS_CLIENT.inputLine += ch;
	}
}

//Cryptography? - Brooooo, never heard about that
int sendMessage(const char* message)
{
	char msg[SENDING_MESSAGE_LENGTH];
	memset(msg, 0, SENDING_MESSAGE_LENGTH);
	msg[0] = SENDING_MESSAGE;
	copyarray(message, msg, 0, 1, std::min<size_t>(strlen(message), MESSAGE_LENGTH - 1));
	msg[SENDING_MESSAGE_LENGTH - 1] = 0;

	return THIS_CLIENT.snd(msg, SENDING_MESSAGE_LENGTH);
}

#endif
