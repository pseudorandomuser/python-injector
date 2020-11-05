#include <string>
#include <stdio.h>
#include <algorithm>

#include "stdafx.h"
#include "windows.h"

using namespace std;

HINSTANCE handle;
HWND submitButton;
HWND editBox;

LRESULT CALLBACK DLLWindowProc(
	HWND,
	UINT,
	WPARAM,
	LPARAM
);

int callback(void* code) {
	return PyRun_SimpleStringFlags((char*)code, NULL);
}

void removeCarriage(char* str) {
	int length = strlen(str);
	for (int i = 0; i < length; i++) {
		if (str[i] == '\r') {
			for (int j = i; j < length; j++) {
				str[j] = str[j + 1];
				if (str[j] == '\0') {
					break;
				}
			}
			i--;
		}
	}
}

BOOL RegisterDLLWindowClass(wchar_t szClassName[]) {
	WNDCLASSEX			wclass;
	wclass.hInstance =		handle;
	wclass.lpszClassName =		(LPCSTR)szClassName;
	wclass.lpfnWndProc =		DLLWindowProc;
	wclass.style =			CS_DBLCLKS;
	wclass.cbSize =			sizeof(WNDCLASSEX);
	wclass.hIcon =			LoadIcon(NULL, IDI_APPLICATION);
	wclass.hIconSm =		LoadIcon(NULL, IDI_APPLICATION);
	wclass.hCursor =		LoadCursor(NULL, IDC_ARROW);
	wclass.lpszMenuName =		NULL;
	wclass.cbClsExtra =		0;
	wclass.cbWndExtra =		0;
	wclass.hbrBackground =		(HBRUSH)(COLOR_WINDOW+1);

	if (!RegisterClassEx(&wclass)) {
		return 0;
	}
}

DWORD WINAPI ThreadProc() {
	MSG messages;
	RegisterDLLWindowClass(reinterpret_cast<wchar_t *>("InjectedDLLWindowClass"));

	HWND hwnd = CreateWindowEx(
		0,						// Ex Style
		(LPCSTR)"InjectedDLLWindowClass",		// Class Name
		(LPCSTR)"Python Injector",			// Window Title Name
		WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,	// DW Style
		CW_USEDEFAULT,					// Position X CW_USEDEFAULT
		CW_USEDEFAULT,					// Position Y CW_USEDEFAULT
		506,						// Window Width
		528,						// Window Height
		NULL,						// Window Parent
		NULL,						// Window Menu
		handle,						// HINSTANCE
		NULL
	);

	ShowWindow(hwnd, SW_SHOWNORMAL);

	while (GetMessage(&messages, NULL, 0, 0)) {
		TranslateMessage(&messages);
		DispatchMessage(&messages);
	}

	return 1;
}

WNDPROC wndButtonProc;
LRESULT CALLBACK ButtonProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
		case WM_LBUTTONUP:
			int length = GetWindowTextLength(editBox) + 1;
			char* buffer = calloc(length, sizeof(char));
			SendMessage(editBox, WM_GETTEXT, (WPARAM)length, (LPARAM)buffer);
			removeCarriage(buffer);
			Py_AddPendingCall(callback, (void*)buffer);
			MessageBox(NULL, (LPCSTR)buffer, (LPCSTR)"", MB_OK);
			break;
	}
	return CallWindowProc(wndButtonProc, hwnd, message, wParam, lParam);
}

LRESULT CALLBACK DLLWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
	int result;
	switch (message) {
		case WM_COMMAND:
			break;
		case WM_CREATE:
			submitButton = CreateWindowEx(
				NULL, "Button", "Submit Code",
				WS_BORDER | WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
				0, 250, 500, 250,
				hwnd, NULL, handle, ButtonProc
			);
			wndButtonProc = (WNDPROC)SetWindowLong(submitButton, GWL_WNDPROC, (LONG)ButtonProc);
			editBox = CreateWindowEx(
				NULL, "Edit", "",
				WS_CHILD | WS_VISIBLE | WS_HSCROLL |
				WS_VSCROLL | ES_LEFT | ES_MULTILINE |
				ES_AUTOHSCROLL | ES_AUTOVSCROLL,
				0, 0, 500, 250,
				hwnd, NULL, handle, NULL
			);
			SendMessage(editBox, EM_LIMITTEXT, WPARAM(2147483646), 0);
			break;
		case WM_SETFOCUS:
			SetFocus(editBox);
			break;
		case WM_CLOSE:
			result = MessageBox(NULL, (LPCSTR)("Do you really want to exit?\r\n"
				"(You can not re-open this window unless you restart the process)"),
				(LPCSTR)"Exit Python Injector", MB_ICONQUESTION | MB_YESNO);
			if (result == IDYES) {
				DestroyWindow(hwnd);
			}
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hwnd, message, wParam, lParam);
	}
	return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
	if(ul_reason_for_call==DLL_PROCESS_ATTACH) {
		handle = hModule;
		CreateThread(0, NULL, (LPTHREAD_START_ROUTINE)ThreadProc, NULL, NULL, NULL);
	}
	return TRUE;
}
