#include <WinSock2.h>
#include <ws2tcpip.h>
#include <Windows.h>
#include <sstream>
#include <iostream>
#include <ctime>
#include <vector>
#include <map>
#include <fstream>

#include "TrigUtil.h"

#define DEFAULT_BUFFLEN 1024
#define RECV_BUFLEN 4
#define EVENT_HANDLED true
#define EVENT_NOT_HANDLED false
#define DEFAULT_PORT "20"
#define MAX_CONNECTIONS 30

struct SOCK_DATA {
	SOCKET clientSocket;
	std::vector<char> data;
	float functionData[4];
};

std::vector<SOCK_DATA> connectedSockets(MAX_CONNECTIONS);
std::vector<DWORD> dwThreadIdArray(MAX_CONNECTIONS);
std::vector<HANDLE> hThreadArray(MAX_CONNECTIONS);
std::ofstream fsDataStorage("Storage.dat", std::ofstream::app);


struct addrinfo *result = NULL;
SOCKET ServerSock;

SOCKET setupServerSock(addrinfo ** addressInformation) {
	WSADATA wsaData;
	int ires;
	ires = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (ires) {
		std::cout << "WSAStartup failed: " << ires << std::endl;
		return INVALID_SOCKET;
	}

	struct addrinfo hints;
	ZeroMemory(&hints, sizeof(addrinfo));

	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	ires = getaddrinfo(NULL, DEFAULT_PORT, &hints, addressInformation);

	if (ires) {
		std::cout << "getaddrinfo failed: " << ires << std::endl;
		WSACleanup();
		return INVALID_SOCKET;
	}

	SOCKET ServerSock = INVALID_SOCKET;

	addrinfo * ptr = *addressInformation;

	ServerSock = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

	if (ServerSock == INVALID_SOCKET) {
		std::cout << "Error instantiating socket...: " << WSAGetLastError() << std::endl;
		freeaddrinfo(ptr);
		WSACleanup();

		return INVALID_SOCKET;
	}

	return ServerSock;

}

addrinfo * bindSocket(SOCKET & ServerSock, addrinfo * serverInfo) {
	//struct sockaddr_in cliaddr;
	//ZeroMemory(&cliaddr, sizeof(cliaddr));
	//cliaddr.sin_family = AF_INET;
	//cliaddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	//cliaddr.sin_port = htons(20);

	//struct in_addr v;

	//struct sockaddr clientAddr;
	//clientAddr.sa_family = AF_INET;
	
	//struct sockaddr_in sInfo = *((sockaddr_in*)(serverInfo->ai_addr));

	int ires = SOCKET_ERROR;
	addrinfo * ptr = serverInfo;
	do {
		ires = bind(ServerSock, ptr->ai_addr, ptr->ai_addrlen);
			ptr = ptr->ai_next;
	} while (ires == SOCKET_ERROR && ptr);
	
	
	if (ires == SOCKET_ERROR) {
		std::cout << "Socket Binding Failed: " << WSAGetLastError() << std::endl;
		freeaddrinfo(serverInfo);
		closesocket(ServerSock);
		WSACleanup();
		return serverInfo;
	}
	freeaddrinfo(serverInfo);
	return NULL;
}


void writeSocketFindings(SOCK_DATA sdData) {
	fsDataStorage << (short)sdData.data[0];
	fsDataStorage << ",";
	fsDataStorage << (short)sdData.data[1];
	fsDataStorage << ",";
	fsDataStorage << (short)sdData.data[2];
	fsDataStorage << " ";

	fsDataStorage << sdData.functionData[0];
	fsDataStorage << ",";
	fsDataStorage << sdData.functionData[1];
	fsDataStorage << ",";
	fsDataStorage << sdData.functionData[2];
	fsDataStorage << ",";
	fsDataStorage << sdData.functionData[3];
	fsDataStorage << std::endl;
}

BOOL WINAPI ControlEventHandler(DWORD dwCtrlType) {
	if (dwCtrlType == CTRL_C_EVENT) {
		for (unsigned int i = 0; i < hThreadArray.size(); i++) {
			HANDLE x = hThreadArray[i];
			DWORD exitCode = 0;
			if (!x)
				continue;
			bool res = GetExitCodeThread(x, &exitCode);

			if (res)
				if (exitCode == STILL_ACTIVE) {
					TerminateThread(x, 1);
					CloseHandle(x);
					closesocket(connectedSockets[i].clientSocket);
				}
				else {
					if (!exitCode)
						writeSocketFindings(connectedSockets[i]);
				}
		}

		closesocket(ServerSock);
		if (result)
			freeaddrinfo(result);
		WSACleanup();
		return EVENT_HANDLED;
	}
	return EVENT_NOT_HANDLED;
}

DWORD WINAPI handleConnection(LPVOID param) {
	SOCK_DATA * sock_data = (SOCK_DATA*)param;
	SOCKET &clientSocket = sock_data->clientSocket;
	std::vector<char> &data = sock_data->data;


	std::string currData;
	std::string leftover;

	//Send greeting message

	std::stringstream greetingbuilder;

	greetingbuilder << "Thank you for your help today, here are your instructions:" << std::endl;
	greetingbuilder << "Build a cos function that will fit the data that will be given" << std::endl;
	greetingbuilder << "Please make sure it is in the form acos(rT * (n % m))" << std::endl;
	greetingbuilder << "Where r is the angle, in radians, floating point" << std::endl;
	greetingbuilder << "T is a step or phase shift variable, floating point" << std::endl;
	greetingbuilder << "a is the amplitude of the cos function, floating point" << std::endl;
	greetingbuilder << "and m is a positive, whole integer." << std::endl;
	greetingbuilder << "NOTES: n is the index of the data point, and does not need to be entered manually" << std::endl;
	greetingbuilder << "Please give all floating point values to 10 sig-figs" << std::endl;

	std::string greeting;
	while (true) {
		char temp = greetingbuilder.get();
		if (greetingbuilder.eof())
			break;
		greeting.append(1, temp);
	}

	sendMessage(&greeting[0], greeting.size(), clientSocket);
	greeting = "\n\nHere is the data:\n";
	sendMessage(&greeting[0], greeting.size(), clientSocket);
	sendMessage(&data[0], data.size(), clientSocket);
	greeting = "\n\nPlease enter the value of a for your solution:\n";
	sendMessage(&greeting[0], greeting.size(), clientSocket);

	float a, theta, T, m;
	try {
		recvMessage(clientSocket, currData, leftover);
		a = makeFloat(&currData[0]);

		greeting = "\n\nPlease enter the value of r for your solution:\n";
		sendMessage(&greeting[0], greeting.size(), clientSocket);
		recvMessage(clientSocket, currData, leftover);
		theta = makeFloat(&currData[0]);

		greeting = "\n\nPlease enter the value of T for your solution:\n";
		sendMessage(&greeting[0], greeting.size(), clientSocket);
		recvMessage(clientSocket, currData, leftover);
		T = makeFloat(&currData[0]);

		greeting = "\n\nPlease enter the value of m for your solution:\n";
		sendMessage(&greeting[0], greeting.size(), clientSocket);
		recvMessage(clientSocket, currData, leftover);
		m = makeFloat(&currData[0]);

	}
	catch (int param) {
		if (param == 0x404)
		{
			closesocket(clientSocket);
			return 1;
		}
	}

	sock_data->functionData[0] = a;
	sock_data->functionData[1] = theta;
	sock_data->functionData[2] = T;
	sock_data->functionData[3] = m;

	closesocket(clientSocket);
	return 0;
}

int main()
{
	SetConsoleCtrlHandler(ControlEventHandler, true);
	srand((unsigned int)time(NULL));
	std::vector<std::vector<char>> dataStorage;
	loadFile(dataStorage);

	std::map<std::vector<char>, bool> dataMapping;

	for (std::vector<char> x : dataStorage) {
		dataMapping[x] = true;
	}
	dataStorage.clear(); //Hopefully this will clean up some data in the heap right away rather than later.


	ServerSock = setupServerSock(&result);

	result = bindSocket(ServerSock, result);
	while (ServerSock != INVALID_SOCKET) {
		for (unsigned int i = 0; i < hThreadArray.size(); i++) {
			HANDLE x = hThreadArray[i];
			if (!x)
				continue;
			DWORD exitCode = 0;
			bool res = GetExitCodeThread(x, &exitCode);
			if (res)
				if (exitCode != STILL_ACTIVE) {
					if (exitCode != 0)
						closesocket(connectedSockets[i].clientSocket);
					else {
						writeSocketFindings(connectedSockets[i]);
						dataMapping[connectedSockets[i].data] = true;
					}
					
					hThreadArray[i] = NULL;
					connectedSockets[i] = {};
					dwThreadIdArray[i] = NULL;
					continue;
				}
		}

		if (listen(ServerSock, MAX_CONNECTIONS) == SOCKET_ERROR) {
			std::cout << "Error occured while setting up listening capability of socket: " << WSAGetLastError() << std::endl;
			closesocket(ServerSock);
			WSACleanup();
			return 1;
		}

		SOCKET ClientSocket = accept(ServerSock, NULL, NULL);
		if (ClientSocket == INVALID_SOCKET) {
			std::cout << "Accept failed: " << WSAGetLastError() << std::endl;
			break;
		}

		std::vector<char> b;

		do {
			unsigned char data[3];
			data[0] = (char)floor(((double)rand() / RAND_MAX) * 256);
			data[1] = (char)floor(((double)rand() / RAND_MAX) * 256);
			data[2] = (char)floor(((double)rand() / RAND_MAX) * 256);

			double average = 0.0;
			average = (data[0] + data[1] + data[2]) / (float)3;

			b = std::vector<char>{ (char)(data[0] - average), (char)(data[1] - average), (char)(data[2] - average) };


		} while (dataMapping[b]);

		//Receive ID Number (1st byte signifies length of rest)
		/*
		std::string id;
		recvMessage(ClientSocket, id);

		*/

		//TODO: Send Customized Greeting

		SOCK_DATA dat = { ClientSocket, b };
		int iSockIndex = -1;
		for (unsigned int i = 0; i < hThreadArray.size(); i++) {
			if (!hThreadArray[i]) {
				iSockIndex = i;
				break;
			}
		}
		
		
		connectedSockets[iSockIndex] = dat;
		
		hThreadArray[iSockIndex] = 
			CreateThread(
				NULL,
				0,
				handleConnection,
				&connectedSockets[iSockIndex],
				0,
				&dwThreadIdArray[iSockIndex]
			);
		//handleConnection(&connectedSockets[connectedSockets.size()-1]);
		if (hThreadArray[iSockIndex] == NULL) {
			std::cerr << "Error creating thread" << std::endl;
			exit(3);
		}
	}



	closesocket(ServerSock);
	if (result)
		freeaddrinfo(result);
	WSACleanup();
	return 0;
}