#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "client.hpp"
#include <exception>
#include <string>
#include <iostream>
#include <thread>

Client::Client()
{
	_clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (_clientSocket == INVALID_SOCKET)
		throw std::exception(__FUNCTION__ "- socket");
}

Client::~Client()
{
	try
	{
		closesocket(_clientSocket);
	}
	catch(...) {}
}


void Client::connectToServer(std::string serverIP, int port)
{
	struct sockaddr_in sa = { 0 };

	sa.sin_port = htons(port);
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = inet_addr(serverIP.c_str());

	int status = connect(_clientSocket, (struct sockaddr*)&sa, sizeof(sa));
	if (status == INVALID_SOCKET)
		throw std::exception("Can't connect to server.");
}


void Client::startConversation()
{
	std::string s, strConverting, lengthMsgString;
	char getLengthMsg[5] = {0};
	char* exactlyArray;
	int lengthMsgInt;
	int getBytes, dataSent;

	recv(_clientSocket, getLengthMsg, 4, 0);
	getLengthMsg[4] = 0;
	strConverting = getLengthMsg; // convert the array of chars to cpp string
	lengthMsgInt = std::stoi(strConverting);
	exactlyArray = new char[lengthMsgInt + 1];

	recv(_clientSocket, exactlyArray, lengthMsgInt, 0);
	exactlyArray[lengthMsgInt] = 0; // putting null at the end of the message
	std::cout << exactlyArray << std::endl; // the server message
	delete[]exactlyArray;
	memset(getLengthMsg, '\0', sizeof(getLengthMsg));

	std::cout << "Enter your name here: ";
	std::getline(std::cin, s); // get all the line, with spaces
	lengthMsgString = std::to_string(s.length());
	std::copy(lengthMsgString.begin(), lengthMsgString.end(), getLengthMsg);
	//send to server the name of client
	send(_clientSocket, getLengthMsg, 4, 0); // the length of message
	send(_clientSocket, s.c_str(), s.size(), 0); // the message
	
	if (s != "bye")
	{
		std::thread sendToServer(&Client::sendMsgToServer, this);
		std::thread recevieFromServer(&Client::receiveMsgFromServer, this);
		sendToServer.join();
		recevieFromServer.join();
	}
	else
	{
		char endConversation[4] = { 0 };
		recv(_clientSocket, endConversation, 4, 0);
		recv(_clientSocket, endConversation, 3, 0);
		std::cout << "From client side: You didn't enter your name as expected... " << endConversation  << "!" << std::endl;
	}
}


// the function sends the messages to server
void Client::sendMsgToServer()
{
	std::string msgToSend, strConverting, lengthMsgString;
	char getLengthMsg[5] = { 0 };

	std::getline(std::cin, msgToSend); // get all the line, with spaces
	lengthMsgString = std::to_string(msgToSend.length()); // the length of the message
	while (msgToSend != "bye")
	{
		std::copy(lengthMsgString.begin(), lengthMsgString.end(), getLengthMsg);

		//send to server the length of the message, after it the message itself
		send(_clientSocket, getLengthMsg, 4, 0);
		send(_clientSocket, msgToSend.c_str(), msgToSend.size(), 0);
		
		std::getline(std::cin, msgToSend); // get all the line, with spaces
		lengthMsgString = std::to_string(msgToSend.length());
		memset(getLengthMsg, '\0', sizeof(getLengthMsg));
	}


	// end the conversation with the server
	std::string byeLengthMsg = "3";
	std::copy(byeLengthMsg.begin(), byeLengthMsg.end(), getLengthMsg);

	send(_clientSocket, getLengthMsg, 4, 0);
	send(_clientSocket, msgToSend.c_str(), msgToSend.size(), 0);
}


// the function received messages from the server
void Client::receiveMsgFromServer()
{
	std::string msgFromServer, strConverting, lengthMsgString;
	char getLengthMsg[5] = { 0 };
	char* exactlyArray;
	int lengthMsgInt;
	while (msgFromServer != "bye")
	{
		recv(_clientSocket, getLengthMsg, 4, 0);
		getLengthMsg[4] = 0;
		strConverting = getLengthMsg; // convert the array of chars to cpp string

		lengthMsgInt = std::stoi(strConverting);
		exactlyArray = new char[lengthMsgInt + 1];

		recv(_clientSocket, exactlyArray, lengthMsgInt, 0);
		exactlyArray[lengthMsgInt] = 0; // putting null at the end
		msgFromServer = exactlyArray; // convert the array of chars to cpp string
		if (msgFromServer != "bye")
			std::cout << msgFromServer << std::endl;
		delete[]exactlyArray;
		memset(getLengthMsg, '\0', sizeof(getLengthMsg));
	}
	std::cout << "From server & client sides: You have successfully left the group!\nGood bye!" << std::endl;
}
