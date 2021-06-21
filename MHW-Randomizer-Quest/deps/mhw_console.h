#pragma once
#include <WS2tcpip.h>

#pragma comment (lib, "ws2_32.lib")

class mhw_console
{
};

extern SOCKET s;
extern bool ConsoleEnable;

bool TCPConnect();
void LognSend(std::string str);