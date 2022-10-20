
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <Windows.h>
#include <WinSock2.h>
#include <iostream>
#include <sqlite3.h>
#include <WS2tcpip.h>
#include "utils.hpp"
#include "network.hpp"
#include "packet.hpp"
#include "table.hpp"
#include "handler.hpp"

#pragma comment(lib, "Ws2_32.lib")


#define BUFLEN 8192 // == 1024*8 -> 8 KiB
sockaddr_in address;

SOCKET server_setup(std::map<string, string> varMapping);

handler_info handlers[MAX_HANDLERS];

int main() {
	string test = readFromFile();

	if (test == "") {
		ExitProcess(1);
	}
	
	std::map<string,string> varMapping = parseFromFile(test);
	
	if (varMapping.size() == 0) {
		ExitProcess(1);
	}
	SOCKET serverSocket = server_setup(varMapping);
	
	if (serverSocket == INVALID_SOCKET) {
		ExitProcess(1);
	}
	/*
	{\r\n
		Op_Code:%(numeric->constant)\r\n
		Packet_Serial_Num:%(numeric)\r\n
		Next_Packet_Len:%(numeric)\r\n
		Transmition_Type:%(numeric->constant)\r\n
		Database:%(string)\r\n
		Table_Name: %(string)\r\n
	}
*/
	SOCKET cl = INVALID_SOCKET;
	sockaddr_in clAddress;
	memset(&clAddress, 0, sizeof(clAddress));
	char buffer[BUFLEN];
	ZeroMemory(buffer, BUFLEN);

	if (setup_handlers(handlers, serverSocket) != 0) {
		cout << "No memory for thread creation\n";
		ExitProcess(1);
	}

	
	//------------------------------------------------------ test section -------------------------------------------------------------------

	//Actual data packet:
/*
	{\r\n
		%(Column_Name_Placeholder_#1):(%(Column_Data_#1)|%(Column_Data_#2)|%(Column_Data_#N))\r\n
		%(Column_Name_Placeholder_#2):(%(Column_Data_#1)|%(Column_Data_#2)|%(Column_Data_#N))\r\n
		%(Column_Name_Placeholder_#N):(%(Column_Data_#1)|%(Column_Data_#2)|%(Column_Data_#N))\r\n
	}
*/
	 char dataPacket[] = "{\r\nName:timmy|johnny|kim\r\nAge:16|17|18\r\nClass:Computer science|Engineering|Art\r\n}";
	 char headerPacket[] = "{\r\nOp_Code:100\r\nPacket_Serial_Num:12345\r\nNext_Packet_Len:673423\r\nTransmition_Type:1\r\nDatabase:test.db\r\nTable_Name:students\r\n}";
	 char tu[] = "Students";

	 //Input check to test against inconsistencies in the header packet format.
	 //Correct format but incorrect info will be handled appropriately
	 if (packet::recieveHeaderPacket(headerPacket, handlers[0].handlerInput.t, handlers[0].handlerInput.p) == 0) {
		 // Do something
		 cout << "incorrect format\n";
	}
	else {

		 ResumeThread(handlers[0].hHandler);

	}
	
	
	
	
	/*
		HANDLE CreateThread(
  [in, optional]  LPSECURITY_ATTRIBUTES   lpThreadAttributes,
  [in]            SIZE_T                  dwStackSize,
  [in]            LPTHREAD_START_ROUTINE  lpStartAddress,
  [in, optional]  __drv_aliasesMem LPVOID lpParameter,
  [in]            DWORD                   dwCreationFlags,
  [out, optional] LPDWORD                 lpThreadId
);
	*/


	
	//------------------------------------------------------ test section -------------------------------------------------------------------

	//	//	Operations order for the server:
	//	/*	1) Accept a connection
	//	*	2) Once a connection is accepted, call a recv method to recieve the header
	//	*	3) Once the header was recieved in a buffer, call the method of the packet class 'recieveHeaderPacket' to parse it for: 1) packet_serial_num, 2) next_packet_len, 3) transmition_type, 4) table_name
	//	*	4) Save the details in a data structure to pass it to the sqlite3 handler thread
	//	*	5) The sqlite3 handler thread will generally use only: 1) transmition_type(requesting a table/writing to a table), 2) table_name(requested table/"dirty" table)
	//	*	*6) At operation 4) the server will return to the listening state and from there the sqlite3 handler thread will determine the number of bytes of the packet for the header
	//	*	7) Once the sqlite3 handler thread determined the number of bytes, the header could be sent with the client's request packet serial number and after that the data packet
	//	*	8) The sqlite3 handler thread will return to sleep until the main thread will wake it up
	//	*/
	//}
	
	while (true);

	

	closesocket(serverSocket);
	WSACleanup();
	return 0;
}

SOCKET server_setup(std::map<string,string> varMapping) {

	int PORT = std::stoi(varMapping["PORT"]);

	char HOST[16]; //purely to conform to how the api accepts only char* string types

	ZeroMemory(HOST, sizeof(HOST));
	size_t t = 0;
	for (t = 0; t < varMapping["HOST"].size(); ++t) {
		HOST[t] = varMapping["HOST"][t];
	}
	HOST[t] = 0;

	WSADATA sockInitData;

	if (WSAStartup(MAKEWORD(2, 2), &sockInitData) != 0) {
		std::cout << "Sock init failed with error " << WSAGetLastError() << '\n';
		WSACleanup();
		return INVALID_SOCKET;
	}

	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_port = htons(PORT);
	address.sin_addr.S_un.S_addr = inet_addr(HOST);
	SOCKET serverSocket = INVALID_SOCKET;

	if ((serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET) {
		std::cout << "Socket creation failed with error " << WSAGetLastError() << '\n';
		WSACleanup();
		return INVALID_SOCKET;
	}

	std::cout << "Socket created successfully\n";


	if (bind(serverSocket, (sockaddr*)&address, sizeof(address)) == SOCKET_ERROR) {
		std::cout << "Socket failed to bind with error " << WSAGetLastError() << '\n';
		closesocket(serverSocket);
		WSACleanup();
		return INVALID_SOCKET;
	}
	std::cout << "Socket bound successfully\n";


	if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
		std::cout << "Socket failed to listen with error " << WSAGetLastError() << '\n';
		closesocket(serverSocket);
		WSACleanup();

		return INVALID_SOCKET;
	}
	std::cout << "Socket listening successfully\n";

	return serverSocket;
}

