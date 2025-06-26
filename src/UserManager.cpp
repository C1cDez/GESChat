#include "UserManager.h"

#include "GESChat.h"

#ifdef SERVER

#include "GCProtocol.h"

#include <iostream>
#include <windows.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")



User::User(SOCKET sock) : sock(sock), name{ "" }, active{ false } {}
User::~User() { closesocket(sock); }

void User::setActive(bool pactive) const { active = pactive; }
bool User::isActive() const { return active; }
void User::setName(const char* pname) const { name = std::string{ pname }; };
const char* User::getName() const { return name.c_str(); }

int User::snd(const char* buf, int size) const { return send(sock, buf, size, 0); }
int User::rcv(char* buf, int size) const { return recv(sock, buf, size, 0); }


void broadcastMessage(const User* author, const char* message);
void broadcastSTB(unsigned char code, const User* user);

void forwardPrivateMessage(const User* sender, const char* receiver, const char* message);


struct UserThread
{
	const User* user;
	std::thread* thread;
};
struct UserThreadPool
{
	UserThread* userthreads;
	size_t size;
	size_t current;
} UTP;

bool utpFull();
size_t utpFindUserByName(const char* username);
void utpUpdateCurrent();
void utpClear();



bool handshake(const User* user)
{
	char hsh_c[HANDSHAKE_LENGTH_CS];
	user->rcv(hsh_c, HANDSHAKE_LENGTH_CS);
	const char* username = hsh_c + 1 + GC_PROTOCOL_VERSION_LENGTH;

	char hsh[HANDSHAKE_LENGTH_SC];
	hsh[0] = HANDSHAKE;
	
	bool flag = true;
	if (hsh_c[0] != HANDSHAKE)
	{
		hsh[1] = INVALID_DATA;
		LOG("Client sent invalid data");
	}
	else if (utpFull())
	{
		hsh[1] = SERVER_ERROR_TRANSCENDED_BACKLOG;
	}
	else if (compareGCProtocolVersion(hsh_c[1], hsh_c[2]))
	{
		LOG("Client sent incompatable protocol version");
		hsh[1] = SERVER_ERROR_INCOMPATABLE_PROTOCOL_VERSION;
		hsh[2] = GC_PROTOCOL_VERSION_MAJOR;
		hsh[3] = GC_PROTOCOL_VERSION_MINOR;
	}
	else if (!isValidUsername(username))
	{
		hsh[1] = SERVER_ERROR_INVALID_USERNAME;
	}
	else if (utpFindUserByName(username) < UTP.size)
	{
		hsh[1] = SERVER_ERROR_DUPLICATE_USERNAME;
	}
	else
	{
		hsh[1] = SERVER_OK;
		flag = false;
	}
	
	user->snd(hsh, HANDSHAKE_LENGTH_SC);

	if (!flag)
	{
		user->setName(username);
		LOG(username << " joined");
	}

	return flag;
}

void handle(const User* user)
{
	int status = 0;

	broadcastSTB(SERVER_TECHNICAL_BROADCASTING_STATUS_USER_JOINED, user);

	while (true)
	{
		char buffer[MAX_SERVER_POSSIBLE_INPUT];
		status = user->rcv(buffer, MAX_SERVER_POSSIBLE_INPUT);
		if (status == SOCKET_ERROR)
		{
			user->setActive(false);
			break;
		}

		char code = buffer[0];
		
		if (code == SENDING_MESSAGE)
		{
			const char* message = buffer + 1;
			LOG(ANSI("2m") << "<" << user->getName() << "> " << message << ANSI("0m"));
			broadcastMessage(user, message);
		}
		else if (code == SENDING_PRIVATE_MESSAGE)
		{
			const char* toUser = buffer + 1;
			const char* message = buffer + 1 + USERNAME_LENGTH;
			LOG(ANSI("2m") << "<" << user->getName() << " -> " << toUser << "> " << message << ANSI("0m"));
			forwardPrivateMessage(user, toUser, message);
		}
		else LOG("Client sent invalid data");
	}

	LOG("Client left (" << user->getName() << ")");

	broadcastSTB(SERVER_TECHNICAL_BROADCASTING_STATUS_USER_LEFT, user);
}


void utpInit(int backlog)
{
	if (UTP.size) return;
	UTP.size = (size_t) backlog;
	UTP.userthreads = new UserThread[UTP.size];
	for (size_t i = 0; i < UTP.size; i++) UTP.userthreads[i] = UserThread{ nullptr, nullptr };
	UTP.current = 0;
}

bool utpFull() { return UTP.current >= UTP.size; }
void utpAdd(const User* user)
{
	user->setActive(true);
	std::thread* thread = new std::thread{ handle, user };
	UserThread ut{ user, thread };
	UTP.userthreads[UTP.current] = ut;
	utpUpdateCurrent();
	utpClear();
}


size_t utpFindUserByName(const char* username)
{
	for (size_t i = 0; i < UTP.size; i++)
	{
		const User* user = UTP.userthreads[i].user;
		if (user && user->isActive() && !strcmp(user->getName(), username)) return i;
	}
	return UTP.size;
}


void utpUpdateCurrent()
{
	int _c = 0;
	while (_c < UTP.size)
	{
		const User* user = UTP.userthreads[_c].user;
		if (!user || !user->isActive()) break;
		_c++;
	}
	UTP.current = _c;
}

void utpClear()
{
	for (size_t i = 0; i < UTP.size; i++)
	{
		UserThread& ut = UTP.userthreads[i];
		if (ut.user && !ut.user->isActive())
		{
			ut.thread->join();
			delete ut.thread; ut.thread = nullptr;
			delete ut.user; ut.user = nullptr;
		}
	}
}


void broadcast(const char* packet, int size)
{
	for (size_t i = 0; i < UTP.size; i++)
	{
		const User* user = UTP.userthreads[i].user;
		if (user && user->isActive()) user->snd(packet, size);
	}
}

void broadcastSTB(unsigned char code, const User* user)
{
	char stb[SERVER_TECHNICAL_BROADCASTING_LENGTH];
	stb[0] = SERVER_TECHNICAL_BROADCASTING;
	stb[1] = code;
	copyarray(user->getName(), stb, 0, 1 + 1, USERNAME_LENGTH);

	broadcast(stb, SERVER_TECHNICAL_BROADCASTING_LENGTH);
}
void broadcastMessage(const User* author, const char* message)
{
	char packet[BROADCASTING_MESSAGE_LENGTH];
	memset(packet, 0, BROADCASTING_MESSAGE_LENGTH);
	packet[0] = BROADCASTING_MESSAGE;
	copyarray(author->getName(), packet, 0, 1, USERNAME_LENGTH);
	copyarray(message, packet, 0, 1 + USERNAME_LENGTH, strlen(message));

	broadcast(packet, BROADCASTING_MESSAGE_LENGTH);
}

void forwardPrivateMessage(const User* sender, const char* receiverName, const char* message)
{
	char packet[FORWARDING_PRIVATE_MESSAGE_LENGTH];
	memset(packet, 0, FORWARDING_PRIVATE_MESSAGE_LENGTH);
	packet[0] = FORWARDING_PRIVATE_MESSAGE;
	
	size_t receiverId = utpFindUserByName(receiverName);
	if (receiverId < UTP.size)
	{
		packet[1] = FORWARDING_PRIVATE_MESSAGE_STATUS_OK;
		copyarray(sender->getName(), packet, 0, 1 + 1, USERNAME_LENGTH - 1);
		copyarray(receiverName, packet, 0, 1 + 1 + USERNAME_LENGTH, USERNAME_LENGTH - 1);
		copyarray(message, packet, 0, 1 + 1 + USERNAME_LENGTH + USERNAME_LENGTH, strlen(message));

		const User* receiver = UTP.userthreads[receiverId].user;
		receiver->snd(packet, FORWARDING_PRIVATE_MESSAGE_LENGTH);
		sender->snd(packet, FORWARDING_PRIVATE_MESSAGE_LENGTH);
	}
	else
	{
		packet[1] = FORWARDING_PRIVATE_MESSAGE_STATUS_USER_NOT_FOUND;
		copyarray(receiverName, packet, 0, 1 + 1, USERNAME_LENGTH - 1);
		LOG("PM receiver '" << receiverName << "' wasnt found. Sending back error");
		sender->snd(packet, 1 + 1 + USERNAME_LENGTH);
	}
}

#endif
