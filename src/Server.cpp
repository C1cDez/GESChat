#include "GESChat.h"

#ifdef SERVER

#include <iostream>
#include <string>
#include <thread>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "GCProtocol.h"
#include "ConsoleHandler.h"

#include "UserManager.h"

#pragma comment(lib, "ws2_32.lib")

SOCKET init(PCSTR port, int backlog);
void acceptUsers(SOCKET sock);

void run(int argc, char* argv[])
{
	LOGn("Using GC Protocol Version: " << (int)GC_PROTOCOL_VERSION_MAJOR << "." << (int)GC_PROTOCOL_VERSION_MINOR);

	ArgsParser ap{ argc, argv };

	PCSTR port = ap.getOrDefault("-p", DEFAULT_PORT);
	LOGn("Server port: " << port);
	int backlog = std::stoi(ap.getOrDefault("-b", MAX_BACKLOG));
	LOGn("Server backlog: " << backlog);

	utpInit(backlog);

	SOCKET sock = init(port, backlog);
	LOGn("Server socket: " << sock << std::endl);

	acceptUsers(sock);

	closesocket(sock);
	LOGn("Server closed");
	WSACleanup();
}

SOCKET init(PCSTR port, int backlog)
{
	LOGn("Starting Server...");
	int status;

	SOCKET sock = INVALID_SOCKET;

	addrinfo hints{}, * result{};
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	status = getaddrinfo(NULL, port, &hints, &result);
	if (status) throw std::exception{ "getaddrinfo() failed" };

	sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (sock == INVALID_SOCKET) throw std::exception{ "socket() failed" };

	status = bind(sock, result->ai_addr, (int)result->ai_addrlen);
	if (status == SOCKET_ERROR) throw std::exception{ "bind() failed. Check if no other servers are opened on this port" };

	freeaddrinfo(result);

	status = listen(sock, backlog);
	if (status == SOCKET_ERROR) throw std::exception{ "listen() failed" };

	return sock;
}

void acceptUsers(SOCKET sock)
{
	while (true)
	{
		SOCKET client = accept(sock, NULL, NULL);
		LOGn("Client connected");
		User* user = new User{ client };
		if (handshake(user))
		{
			LOGn("Terminating connection with client");
			user->~User();
		}
		else utpAdd(user);
	}
}

#endif
