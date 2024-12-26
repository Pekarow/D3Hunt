#define WIN32_LEAN_AND_MEAN

//#include <windows.h>
//#include <winsock2.h>
//
//#include <stdlib.h>
#include <stdio.h>
//#include <string>
//
//// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
//#pragma comment (lib, "Ws2_32.lib")
//#pragma comment (lib, "Mswsock.lib")
//#pragma comment (lib, "AdvApi32.lib")


#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "5000"

#include "ClientSocket.h"
#include <WinSock2.h>
#include <ws2tcpip.h>
//#include "DofusDBUtils.h"

ClientSocket::ClientSocket()
{
	mConnectSocket = INVALID_SOCKET;
	WSADATA wsaData;
	struct addrinfo hints, * result, * ptr;


	int iResult;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		return;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Resolve the server address and port
	iResult = getaddrinfo("", DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return;
	}

	// Attempt to connect to an address until one succeeds
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

		// Create a SOCKET for connecting to server
		mConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
			ptr->ai_protocol);
		if (mConnectSocket == INVALID_SOCKET) {
			printf("socket failed with error: %ld\n", WSAGetLastError());
			WSACleanup();
			return;
		}

		// Connect to server.
		iResult = connect(mConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			closesocket(mConnectSocket);
			mConnectSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}
	freeaddrinfo(result);


}

ClientSocket::~ClientSocket()
{
	if (!isValid())
		return;
	int iResult;

	// shutdown the connection since no more data will be sent
	iResult = shutdown(mConnectSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(mConnectSocket);
		WSACleanup();
		return;
	}

	// cleanup
	closesocket(mConnectSocket);
	WSACleanup();
}

bool ClientSocket::isValid()
{
	return mConnectSocket != INVALID_SOCKET;
}

bool ClientSocket::send(const char* content)
{

	int iResult;

	// Send an initial buffer
	iResult = ::send(mConnectSocket, content, (int)strlen(content), 0);
	if (iResult == SOCKET_ERROR) {
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(mConnectSocket);
		WSACleanup();
		return 1;
	}

	printf("Bytes Sent: %ld\n", iResult);
	return false;
}

void ClientSocket::receive(char** out, int& size)
{
	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN;
	int iResult;
	// Receive until the peer closes the connection
	do {

		iResult = recv(mConnectSocket, recvbuf, recvbuflen, 0);
		if (iResult > 0)
		{
			printf("Bytes received: %d\n", iResult);
			size = iResult;
			break;
		}
		else if (iResult == 0)
			printf("Connection closed\n");
		else
			printf("recv failed with error: %d\n", WSAGetLastError());

	} while (iResult > 0);
	(*out) = new char[size + 1];
	for (int i = 0; i < size; i++)
	{
		(*out)[i] = recvbuf[i];
	}
	(*out)[size] = '\0';
}


//
//int main()
//{
//	DofusDB::DBInfos infos{ 1,1,2, 0,"test" };
//	ClientSocket sock;
//	if (!sock.isValid())
//		return -1;
//	sock.send(DofusDB::DBInfos2json(infos).c_str());
//	int size;
//	char* rec = nullptr;
//	sock.receive(&rec, size);
//	std::cout << std::string(rec) << std::endl;
//	printf("%s\n", rec);
//
//	// Ask server to close
//	infos.exit = 1;
//	sock.send(DofusDB::DBInfos2json(infos).c_str());
//	delete rec;
//
//	return 0;
//}

