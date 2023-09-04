#pragma comment (lib, "ws2_32.lib")

#include "WSAinitializer.hpp"
#include "server.hpp"
#include <iostream>
#include <exception>

int main()
{

	try
	{
		WSAInitializer wsaInit;
		Server myServer;
		myServer.serve(8876);
	}
	catch (std::exception& e)
	{
		std::cout << "Error occured: " << e.what() << std::endl;
	}
	system("PAUSE");

	return 0;
}