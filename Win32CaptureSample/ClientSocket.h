// ClientSocket.h�: fichier Include pour les fichiers Include syst�me standard,
// ou les fichiers Include sp�cifiques aux projets.

#pragma once
//#include <WinSock2.h>
//#include "DofusDBUtils.h"

class ClientSocket
{
public:
	ClientSocket();
	~ClientSocket();
	bool isValid();
	bool send(const char* content);
	void receive(char** out, int& size);

private:
	unsigned __int64 mConnectSocket;
};
