#pragma once
#include <WinSock2.h>
#include <windows.h>
#include <string>

class Client
{
public:
	Client();
	~Client();
	void connectToServer(std::string, int);
	void startConversation();
	void sendMsgToServer();
	void receiveMsgFromServer();

private:
	SOCKET _clientSocket;
};