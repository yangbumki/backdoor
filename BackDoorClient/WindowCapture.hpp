#ifndef __WINDOW_CAPTURE_HPP_
#define __WINDOW_CAPTURE_HPP_

#include <Windows.h>
#include <iostream>

class WindowCapture
{
private:
	HANDLE		wndThread;
	HINSTANCE	hInstance;
	HWND		hwnd;
	HDC			hdc, dstHdc;
	WNDCLASS	wndClass;
	MSG			msg;

	int width;
	int height;

	static DWORD WINAPI WndThread(void* arg) {
		WindowCapture* wc = (WindowCapture*)arg;

		while (GetMessage(&wc->msg, NULL, 0, 0) != 0) {
			TranslateMessage(&wc->msg);
			DispatchMessage(&wc->msg);
		}

		printf("goodbye Thread :) \n");
		return 0;
	};

	static LRESULT CALLBACK WndProc(HWND hwnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
	{
		CREATESTRUCT* cs = (CREATESTRUCT*)lParam;
		switch (iMessage)
		{
		case WM_CREATE:
			return 0;

		case WM_PAINT:
			PAINTSTRUCT ps;
			BeginPaint(hwnd, &ps);
			EndPaint(hwnd, &ps);
			return 0;

		case WM_DESTROY:
			PostQuitMessage(0);

		default:
			return DefWindowProc(hwnd, iMessage, wParam, lParam);

		}
	};


public:
	WindowCapture(int width, int height, HDC dstHdc)
	{
		this->dstHdc = dstHdc;
		this->width = width;
		this->height = height;

		hInstance = GetModuleHandle(NULL);

		wndClass.cbClsExtra = 0;
		wndClass.cbWndExtra = 0;
		wndClass.hInstance = hInstance;
		wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
		wndClass.hIcon = LoadIcon(NULL, IDC_ICON);
		wndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
		wndClass.lpszClassName = L"CapTure";
		wndClass.lpszMenuName = NULL;
		wndClass.style = CS_HREDRAW | CS_VREDRAW;
		wndClass.lpfnWndProc = WndProc;

		RegisterClass(&wndClass);
		hwnd = CreateWindowEx(0, wndClass.lpszClassName, wndClass.lpszClassName, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, NULL, NULL, hInstance, NULL);
		ShowWindow(hwnd, SW_SHOWDEFAULT);

		UpdateWindow(hwnd);

		//wndThread = CreateThread(NULL, 0, WndThread, this, 0, NULL);

		while (GetMessage(&msg, NULL, 0, 0) != 0) {
			ShowScreen();
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	};

	void ShowScreen() {
		hdc = GetDC(hwnd);
		int srcWidth = GetDeviceCaps(dstHdc, HORZRES);
		int srcHeight = GetDeviceCaps(dstHdc, VERTRES);

		StretchBlt(this->hdc, 0, 0, width, height, dstHdc, 0, 0, srcWidth, srcHeight, SRCCOPY);

		ReleaseDC(this->hwnd, this->hdc);
	}
};

#endif