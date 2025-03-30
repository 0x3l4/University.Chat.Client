#pragma once

#include "resource.h"
#include <string>

#define MAX_LOADSTRING 100
#define TIME_QUANT 5000
#define BUFSIZE 512
#define BASIC_MARGIN 10
#define WINDOW_X 400
#define WINDOW_Y 400

enum {
	IDC_EDIT = 201,
	IDC_BTN_CONNECT = 202,
	IDC_BTN_DISCONNECT = 203,
	IDC_BTN_EXIT = 206,
	IDC_EDIT_MSG = 204,
	IDC_MNPLY_CHECK = 205
};

// ���������� ����������:
HINSTANCE hInst;                                // ������� ���������
WCHAR szTitle[MAX_LOADSTRING];                  // ����� ������ ���������
WCHAR szWindowClass[MAX_LOADSTRING];            // ��� ������ �������� ����
HANDLE hClientThread = NULL;
HANDLE hMutex;
HWND hEdit, hButtonConnect, hButtonDisconnect, hButtonExit, hEditMsg, hChkMonopoly;

volatile bool stopClientThread = false;
volatile bool isThisClientWantExclusive = false;
volatile bool isThisClientBlocked = false;

// ��������� ���������� �������, ���������� � ���� ������ ����:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
DWORD WINAPI		ClientSendThread(LPVOID);

void				MessageCicleElement(HANDLE);
void				OnCreate(HWND, LPARAM, HFONT);
void				OnConnect();
void				OnDisconnect();
void				LogStringCreator(const wchar_t*, const wchar_t*);
void				PrintMessage(const wchar_t*);
std::wstring		GetCurrentTimeString();