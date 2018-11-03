#pragma once

#include <vector>
#include <string>
#include <WinSock2.h>
#include <ws2tcpip.h>

void loadFile(std::vector<std::vector<char>> & dataStorage);
float makeFloat(char *data);
long long makeLongLong(char *data);
void sendMessage(const char * message, size_t size_msg, SOCKET &sock);
void recvMessage(SOCKET & sock, std::string & storage, std::string &leftover);


