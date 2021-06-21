// Stracker's loader.h
#include "pch.h"
#include "loader.h"
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>

using namespace loader;

bool ConsoleEnable;
SOCKET s;


// Log and send message
void LognSend(std::string str)
{
	// send info to loader log
	LOG(INFO) << str;

	// send return message to console for display as message
	// console identify "console" as message return to console
	if (ConsoleEnable)
	{
		str = "console " + str;
		send(s, str.c_str(), str.size(), 0);
	}
}

// Establish connection to console
// referenced from https://www.youtube.com/watch?v=WDn-htpBlnU
bool TCPConnect()
{
	const char* addr = "127.0.0.1";
	WSADATA wsadata;
	WORD ver = MAKEWORD(2, 2);

	int wsOk = WSAStartup(ver, &wsadata);
	if (wsOk != 0)
	{
		LOG(INFO) << "[Randomizer] [Console] Can't Initialize winsock!";
		return false;
	}

	//Get the information needed to finalize a socket
	SOCKADDR_IN target;
	target.sin_family = AF_INET; //Address family internet
	target.sin_port = _WINSOCKAPI_::htons(50382); //Port #50382 you know what this means
	target.sin_addr.s_addr = inet_addr(addr);
	//target.sin_addr.s_addr = INADDR_ANY;

	LOG(INFO) << "[Randomizer] [Console] Create the socket";

	//Create the socket
	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s == INVALID_SOCKET)
	{
		LOG(INFO) << "[Randomizer] [Console] INVALID_SOCKET";
		return false;
	}

	//Try connecting
	int new_sd = connect(s, (SOCKADDR*)&target, sizeof(target));
	if (new_sd == SOCKET_ERROR)
	{
		//Failed to connect
		LOG(INFO) << "[Randomizer] [Console] Failed";
		return false;
	}

	u_long iMode = 1;
	int iResult = ioctlsocket(s, FIONBIO, &iMode);
	if (iResult != NOERROR)
	{
		LOG(INFO) << "[Randomizer] [Console] ioctlsocket fail";
		return false;
	}

	LOG(INFO) << "[Randomizer] [Console] Success";

	fd_set master;
	FD_ZERO(&master);
	FD_SET(s, &master);

	return true;
}