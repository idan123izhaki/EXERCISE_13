#pragma once
#include <WinSock2.h>
#include <windows.h>
#include <mutex>
#include <map>

class Server
{
public:
	Server();
	~Server();
	void serve(int port);

private:
	void acceptClient();
	void clientHandler(SOCKET);
	std::mutex mapProtection, screenProtection;
	std::map<int, std::string> connectedClients;
	SOCKET _serverSocket;
};