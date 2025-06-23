#pragma once

#include "GESChat.h"

#ifdef SERVER

#include <string>
#include <thread>
#include <winsock2.h>

void broadcast(const char* data, int size);

class User
{
	SOCKET sock;
	mutable std::string name;
	mutable bool active;

public:
	User(SOCKET sock);
	~User();

	void setActive(bool pactive) const;
	bool isActive() const;

	void setName(const char* pname) const;
	const char* getName() const;

	int snd(const char* buf, int size) const;
	int rcv(char* buf, int size) const;

};
bool handshake(const User* user);
void handle(const User* user);

void utpInit(int backlog);
void utpAdd(const User* user);

#endif
