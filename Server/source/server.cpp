#include "server.hpp"
#include <iostream>
#include <string>
#include <string.h>
#include <thread>
#include <exception>

Server::Server()
{
	_serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (_serverSocket == INVALID_SOCKET)
		throw std::exception(__FUNCTION__ "-socket");
}

Server::~Server()
{
	try
	{
		closesocket(_serverSocket);
		connectedClients.clear();
	}
	catch(...) {}
}

void Server::serve(int port)
{
	std::unique_lock<std::mutex> lockScreen(screenProtection, std::defer_lock);
	struct sockaddr_in sa = { 0 };

	sa.sin_port = htons(port); // convert number in host byte order to network byte order
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = INADDR_ANY;
	//connect between the IP and PORT
	if (bind(_serverSocket, (struct sockaddr*)&sa, sizeof(sa)) == SOCKET_ERROR)
		throw std::exception(__FUNCTION__ "- bind");

	if (listen(_serverSocket, SOMAXCONN) == SOCKET_ERROR)
		throw std::exception(__FUNCTION__ "- listen");
	
	std::cout << "Listening on port " << port << "." << std::endl;
	while (true)
	{
		lockScreen.lock();
		std::cout << "Waiting for client connection request..." << std::endl;
		lockScreen.unlock();
		acceptClient();
	}
}

void Server::acceptClient()
{
	SOCKET client_socket = accept(_serverSocket, NULL, NULL);
	if (client_socket == INVALID_SOCKET)
		throw std::exception(__FUNCTION__);
	
	{
		std::unique_lock<std::mutex> lock(screenProtection);
		std::cout << "Client " << client_socket << " accepted. Server and client can speak." << std::endl;
	}

	// the fucntion that handle the conversation between client and server
	std::thread clientHandlerThread(&Server::clientHandler, this, client_socket);
	clientHandlerThread.detach();
}

// first 4 bytes- the length of the message
// after it- the message itself
void Server::clientHandler(SOCKET clientSocket)
{
	try
	{
		std::unique_lock<std::mutex> lockScreen(screenProtection, std::defer_lock);
		std::unique_lock<std::mutex> lockMap(mapProtection, std::defer_lock);

		bool insteadName = false;
		std::string s = "Welcome to broadcast chat!\nIf you want to leave the group, you can write \"bye\" anytime.\nwhat is your name? ",
			strConvertor, msgToSend;
		char getLengthMsg[5];

		std::string lengthMsgString = std::to_string(s.length());
		memset(getLengthMsg, '\0', sizeof(getLengthMsg));
		std::copy(lengthMsgString.begin(), lengthMsgString.end(), getLengthMsg);
		//send to client the length of the message, after it the message itself
		send(clientSocket, getLengthMsg, 4, 0);
		send(clientSocket, s.c_str(), s.size(), 0);
		
		lockScreen.lock();
		std::cout << "Server send the first message successfully to client: " << clientSocket << "!" << std::endl;
		lockScreen.unlock();
		
		//receiving the client message
		recv(clientSocket, getLengthMsg, 4, 0);
		getLengthMsg[4] = 0;
		std::string msgConverting(getLengthMsg); // convert the array of chars to cpp string
		int lengthMsgInt = std::stoi(msgConverting); // stoi function converts string to int
		char* exactlyMsg = new char[lengthMsgInt + 1];
		recv(clientSocket, exactlyMsg, lengthMsgInt, 0);

		exactlyMsg[lengthMsgInt] = 0; // putting null at the end of the array
		
		msgConverting = exactlyMsg; // convert the array of chars to cpp string
		delete[]exactlyMsg;

		if (msgConverting != "bye")
		{
			lockScreen.lock();
			std::cout << "Client " << clientSocket << " name is : " << msgConverting << std::endl;
			lockScreen.unlock();

			lockMap.lock();
			connectedClients.insert({ clientSocket, msgConverting });
			std::string joinClientMsg = "\n" + msgConverting + " joined the group.\n";
			lengthMsgString = std::to_string(joinClientMsg.length());
			memset(getLengthMsg, '\0', sizeof(getLengthMsg));
			std::copy(lengthMsgString.begin(), lengthMsgString.end(), getLengthMsg);
			for (const std::pair<int, std::string>& client : connectedClients)
			{
				if (client.first != clientSocket)
				{
					send(client.first, getLengthMsg, 4, 0);
					send(client.first, joinClientMsg.c_str(), joinClientMsg.size(), 0);
				}
			}
			lockMap.unlock();

			lockScreen.lock();
			std::cout << "Connected clients: " << connectedClients.size() << "." << std::endl;
			lockScreen.unlock();

			s = "Hello ";
			s += msgConverting;
			s += ", from server! welcome to this group.\nWhat message would you like to send?\nWrite the message here and after it press enter for sending.\n";
			lengthMsgString = std::to_string(s.length());
			memset(getLengthMsg, '\0', sizeof(getLengthMsg));
			std::copy(lengthMsgString.begin(), lengthMsgString.end(), getLengthMsg);
			//send to client the length of the message, after it the message itself
			send(clientSocket, getLengthMsg, 4, 0);
			send(clientSocket, s.c_str(), s.size(), 0);
		}
		else
		{
			lockScreen.lock();
			std::cout << "Client " << clientSocket << " wrote \"bye\" instead his name..." << std::endl;
			lockScreen.unlock();
			insteadName = true;
		}

		// conversation with the client until he send "bye"
		while (msgConverting != "bye")
		{
			memset(getLengthMsg, '\0', sizeof(getLengthMsg));
			
			recv(clientSocket, getLengthMsg, 4, 0);

			getLengthMsg[4] = 0;
			strConvertor = getLengthMsg; // convert the array of chars to cpp string
			lengthMsgInt = std::stoi(strConvertor);
			exactlyMsg = new char[lengthMsgInt + 1];

			recv(clientSocket, exactlyMsg, lengthMsgInt, 0);

			exactlyMsg[lengthMsgInt] = 0; // putting null at the end of the array
			msgConverting = exactlyMsg;
			delete[]exactlyMsg;
			
			lockScreen.lock();
			std::cout << "Client " << clientSocket << " message is: " << msgConverting << std::endl;
			lockScreen.unlock();
			
			if (msgConverting != "bye")
			{
				lockMap.lock();
				std::string clientName = connectedClients[clientSocket];
				for (const std::pair<int, std::string>& client : connectedClients)
				{
					if (client.first != clientSocket)
					{
						msgToSend = clientName + " send: \"" + msgConverting + "\"";
						lengthMsgString = std::to_string(msgToSend.length());
						memset(getLengthMsg, '\0', sizeof(getLengthMsg));
						std::copy(lengthMsgString.begin(), lengthMsgString.end(), getLengthMsg);

						send(client.first, getLengthMsg, 4, 0);
						send(client.first, msgToSend.c_str(), msgToSend.size(), 0);
					}
				}
				lockMap.unlock();
				
				lockScreen.lock();
				std::cout << "The server send this message: \"" << msgConverting << "\" to everyone!" << std::endl;
				lockScreen.unlock();
				
				msgToSend = connectedClients[clientSocket] + ", Your message has been sent successfully!";
				lengthMsgString = std::to_string(msgToSend.length());
				memset(getLengthMsg, '\0', sizeof(getLengthMsg));
				std::copy(lengthMsgString.begin(), lengthMsgString.end(), getLengthMsg);

				send(clientSocket, getLengthMsg, 4, 0);
				send(clientSocket, msgToSend.c_str(), msgToSend.size(), 0);
			}
		}

		if (!insteadName)
		{
			lockMap.lock();
			std::string exitClientMsg = "\n" + connectedClients[clientSocket] + " left the group.\n";
			lengthMsgString = std::to_string(exitClientMsg.length());
			memset(getLengthMsg, '\0', sizeof(getLengthMsg));
			std::copy(lengthMsgString.begin(), lengthMsgString.end(), getLengthMsg);
			for (const std::pair<int, std::string>& client : connectedClients)
			{
				if (client.first != clientSocket)
				{
					send(client.first, getLengthMsg, 4, 0);
					send(client.first, exitClientMsg.c_str(), exitClientMsg.size(), 0);
				}
			}
			connectedClients.erase(clientSocket);
			lockMap.unlock();
			lockScreen.lock();
			std::cout << "Connected clients: " << connectedClients.size() << "." << std::endl;
			lockScreen.unlock();
		}

		// stop now the "receiveMsgFromServer" function at the client side.
		msgToSend = "bye"; lengthMsgString = "3";
		memset(getLengthMsg, '\0', sizeof(getLengthMsg));
		std::copy(lengthMsgString.begin(), lengthMsgString.end(), getLengthMsg);
		send(clientSocket, getLengthMsg, 4, 0);
		send(clientSocket, msgToSend.c_str(), msgToSend.size(), 0);
		
		lockScreen.lock();
		std::cout << "Client number: " << clientSocket << " disconnect successfully!" << std::endl;
		lockScreen.unlock();
		
		closesocket(clientSocket);
	}
	catch (const std::exception& e)
	{
		closesocket(clientSocket);
	}
}
