#pragma once
#include <WinSock2.h>
#include <windows.h>

//WSA -> Windows Sockets API

class WSAInitializer
{
public:
	WSAInitializer();
	~WSAInitializer();
};
