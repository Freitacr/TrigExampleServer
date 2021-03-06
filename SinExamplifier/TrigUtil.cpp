// SinExamplifier.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#define DEFAULT_BUFFLEN 1024
#define MAX_SUPPORTED_SIZE 2000

#include <WinSock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <map>
#include <ctime>
#include "TrigUtil.h"

float makeFloat(char *data) {
	float ret;
	memcpy_s(&ret, sizeof(float), (void*)data, sizeof(float));
	return ret;
}

long long makeLongLong(char *data) {
	long long ret;
	memcpy_s(&ret, sizeof(long long), data, sizeof(long long));
	return ret;
}

size_t getSize(char *data) {
	size_t ret;
	memcpy_s(&ret, sizeof(size_t), data, sizeof(size_t));
	return ret;
}

void getLineFromStream(std::string & store, std::ifstream & stream) {
	char temp = 0;
	while (true) {
		temp = (char)stream.get();
		if (stream.eof() || temp == '\n' || stream.bad())
			return;

		const char toAdd = temp;
		store.append(1, toAdd);
	}
}

void loadFile(std::vector<std::vector<char>> & dataStorage) {
	std::ifstream a("storage.dat");
	if (!a.is_open())
		return;
	std::string line;
	while (!a.eof()) {
		getLineFromStream(line, a);
		std::string exampleDef;
		if (line.empty())
			break;
		exampleDef = line.substr(0, line.find(' '));
		std::stringstream ssBuilder;
		ssBuilder << exampleDef;
		int dataSize = 0;
		for (char i : exampleDef) {
			if (i == ',')
				dataSize++;
		}
		std::vector<char> b;
		for (int i = 0; i < dataSize+1; i++) {
			short iVal;
			char cEaten;
			ssBuilder >> iVal;
			ssBuilder >> cEaten;
			b.push_back((char)iVal);
		}
		dataStorage.push_back(b);
		line.clear();
	}
}


void sendMessage(const char * message, size_t size_msg, SOCKET &sock) {
	unsigned char size[] = { 0, 0, 0, 0 };
	memcpy_s(size, sizeof(size_t), &size_msg, sizeof(size_t));
	send(sock, (char*)size, sizeof(size_t), 0);
	send(sock, message, size_msg, 0);
}

void recvMessageFromSize(SOCKET & sock, std::string & storage, size_t size) {
	char sRecvBuff[DEFAULT_BUFFLEN];
	ZeroMemory(sRecvBuff, DEFAULT_BUFFLEN * sizeof(char));
	size_t lRecvRes = 1;

	while (lRecvRes > 0 && storage.size() < size) {
		lRecvRes = recv(sock, sRecvBuff, DEFAULT_BUFFLEN, 0);
		if (lRecvRes < 0 || storage.size() + lRecvRes > storage.max_size())
		{
			lRecvRes = 0;
			break;
		}
		storage.append(sRecvBuff, lRecvRes);
	}

	if (lRecvRes == 0) {
		throw 0x404;
	}
}

void recvMessage(SOCKET & sock, std::string & storage, std::string &leftover) {
	storage.clear();
	size_t lMessageSize = 0;
	if (!leftover.empty()) {
		lMessageSize = getSize(&leftover[0]);
		size_t adjustedSize = (leftover.size() - sizeof(size_t));
		leftover.erase(leftover.begin(), leftover.begin() + sizeof(size_t));
		if (adjustedSize < lMessageSize) {
			//Get rest of the message
			recvMessageFromSize(sock, leftover, lMessageSize);
		}
		storage.append(leftover);
		leftover.erase(leftover.begin(), leftover.begin() + lMessageSize);
		storage.erase(storage.begin() + (lMessageSize), storage.end());
		return;
	}
	
	char sRecvBuff[DEFAULT_BUFFLEN];
	ZeroMemory(sRecvBuff, DEFAULT_BUFFLEN * sizeof(char));
	long long lReceivedBytes = storage.size();
	
	size_t lRecvRes = 1;

	while (lRecvRes > 0 && lReceivedBytes < 8)
	{
		lRecvRes = recv(sock, sRecvBuff, DEFAULT_BUFFLEN, 0);
		if (lRecvRes < 0 || lRecvRes > MAX_SUPPORTED_SIZE)
		{
			lRecvRes = 0;
			break;
		}
		storage.append(sRecvBuff, lRecvRes);
		lReceivedBytes += lRecvRes;
	}

	if (lRecvRes == 0) {
		throw 0x404;
	}
	

	//At this point it is now known that we have received 8 bytes of data.
	//Translate this into a size_t and use as the size to check against to see if the full message has been sent.
	lMessageSize = getSize(&storage[0]);
	storage.erase(storage.begin(), storage.begin() + sizeof(size_t));
	
	if (storage.size() < lMessageSize) {
		try {
			recvMessageFromSize(sock, storage, lMessageSize);
		}
		catch (int param) {
			if (param == 0x404)
				throw param;
		}
	}

	if (lMessageSize ^ lReceivedBytes) {
		leftover.append(storage);
		leftover.erase(leftover.begin(), leftover.begin() + lMessageSize);
		storage.erase(storage.begin() + (lMessageSize), storage.end());
		return;
	}
}