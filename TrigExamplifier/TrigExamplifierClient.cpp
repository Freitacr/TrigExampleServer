// TrigExamplifier.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "../SinExamplifier/TrigUtil.h"
#include <iostream>
#include <WinSock2.h>
#include <ws2tcpip.h>

#define DEFAULT_PORT "20"

SOCKET setupClientSocket(addrinfo ** addressInformation, const char * serverName) {
	WSADATA wsaData;
	int iRes;
	iRes = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iRes) {
		std::cout << "WSAStartup failed: " << iRes << std::endl;
		return INVALID_SOCKET;
	}

	struct addrinfo hints;
	ZeroMemory(&hints, sizeof(addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	iRes = getaddrinfo(serverName, DEFAULT_PORT, &hints, addressInformation);

	if (iRes) {
		std::cout << "getaddrinfo failed: " << iRes << std::endl;
		WSACleanup();
		return INVALID_SOCKET;
	}

	SOCKET ClientSocket = INVALID_SOCKET;

	addrinfo * ptr = *addressInformation;

	ClientSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

	if (ClientSocket == INVALID_SOCKET) {
		std::cout << "Error instantiating socket...: " << WSAGetLastError() << std::endl;
		freeaddrinfo(ptr);
		WSACleanup();

		return INVALID_SOCKET;
	}

	return ClientSocket;

}

void connectSocket(SOCKET &ClientSocket, addrinfo * addressInfo) {
	addrinfo * ptr = addressInfo;
	int iRes = 0;
	do {
		iRes = connect(ClientSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iRes == SOCKET_ERROR)
			ptr = ptr->ai_next;
	} while (iRes == SOCKET_ERROR && ptr);
	if (iRes == SOCKET_ERROR)
	{
		closesocket(ClientSocket);
		ClientSocket = INVALID_SOCKET;
	}
	freeaddrinfo(addressInfo);
}

void communicateWithServer(SOCKET ClientSocket) {
	int greetingCounter = 0;
	std::string message;
	std::string leftover;
	try {
		recvMessage(ClientSocket, message, leftover);
		std::cout << message;

		recvMessage(ClientSocket, message, leftover);
		std::cout << message;

		recvMessage(ClientSocket, message, leftover);
		short data[3] = { message[0], message[1], message[3] };
		std::cout << (short)message[0] << ", " << (short)message[1] << ", " << short(message[2]);

		recvMessage(ClientSocket, message, leftover);
		std::cout << message;

		float a, theta, T, m;

		std::cin >> a;
		sendMessage((char*)&a, sizeof(float), ClientSocket);

		recvMessage(ClientSocket, message, leftover);
		std::cout << message;

		std::cin >> theta;
		sendMessage((char*)&theta, sizeof(float), ClientSocket);

		recvMessage(ClientSocket, message, leftover);
		std::cout << message;
		std::cin >> T;
		sendMessage((char*)&T, sizeof(float), ClientSocket);

		recvMessage(ClientSocket, message, leftover);
		std::cout << message;

		std::cin >> m;
		sendMessage((char*)&m, sizeof(float), ClientSocket);
	}
	catch (int param) {
		if (param == 0x404) {
			std::cerr << "Uh oh... Looks like the server hung up..." << std::endl;
		}
	}

	closesocket(ClientSocket); //Communication finished..
	return;
}

int main(int argc, char ** argv)
{
	std::string serverName;
	if (argc > 1)
		serverName.append(argv[1]);
	else {
		std::cout << "Please enter the ip address of the server you wish to connect to" << std::endl;
		std::cin >> serverName;
	}

	struct addrinfo *addressInfo;
	SOCKET ClientSocket = setupClientSocket(&addressInfo, &serverName[0]);
	connectSocket(ClientSocket, addressInfo);

	if (ClientSocket == INVALID_SOCKET) {
		std::cerr << "Error connecting to server..." << std::endl;
		WSACleanup();
		return 1;
	}
	communicateWithServer(ClientSocket);
	WSACleanup();
	return 0;
}