/*
	ShutdownGuard - Prevent Windows shutdown
	Copyright (C) 2009  Stefan Sundin (recover89@gmail.com)
	
	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.
*/

#define UNICODE
#define _UNICODE

#include <stdio.h>
#include <stdlib.h>
#define _WIN32_IE 0x0600
#include <windows.h>
#include <shlwapi.h>
#include <wininet.h>

//App
#define APP_NAME      L"ShutdownGuard"
#define APP_VERSION   "0.3"
#define APP_URL       L"http://shutdownguard.googlecode.com/"
#define APP_UPDATEURL L"http://shutdownguard.googlecode.com/svn/wiki/latest-stable.txt"

//Messages
#define WM_ICONTRAY            WM_USER+1
#define SWM_TOGGLE             WM_APP+1
#define SWM_HIDE               WM_APP+2
#define SWM_AUTOSTART_ON       WM_APP+3
#define SWM_AUTOSTART_OFF      WM_APP+4
#define SWM_AUTOSTART_HIDE_ON  WM_APP+5
#define SWM_AUTOSTART_HIDE_OFF WM_APP+6
#define SWM_SHUTDOWN           WM_APP+7
#define SWM_UPDATE             WM_APP+8
#define SWM_ABOUT              WM_APP+9
#define SWM_EXIT               WM_APP+10

//Balloon stuff missing in MinGW
#define NIIF_USER 4
#define NIN_BALLOONSHOW        WM_USER+2
#define NIN_BALLOONHIDE        WM_USER+3
#define NIN_BALLOONTIMEOUT     WM_USER+4
#define NIN_BALLOONUSERCLICK   WM_USER+5

//Vista shutdown stuff missing in MinGW
HINSTANCE user32=NULL;
BOOL WINAPI (*ShutdownBlockReasonCreate)(HWND, LPCWSTR)=NULL;
BOOL WINAPI (*ShutdownBlockReasonDestroy)(HWND)=NULL;

//Localization
struct strings {
	wchar_t *menu_enable;
	wchar_t *menu_disable;
	wchar_t *menu_hide;
	wchar_t *menu_autostart;
	wchar_t *menu_shutdown;
	wchar_t *menu_update;
	wchar_t *menu_about;
	wchar_t *menu_exit;
	wchar_t *tray_enabled;
	wchar_t *tray_disabled;
	wchar_t *prevent;
	wchar_t *balloon;
	wchar_t *shutdown_ask;
	wchar_t *shutdown_logoff;
	wchar_t *shutdown_shutdown;
	wchar_t *shutdown_nothing;
	wchar_t *update_balloon;
	wchar_t *update_dialog;
	wchar_t *about_title;
	wchar_t *about;
};
#include "localization/strings.h"
struct strings *l10n=&en_US;

//Boring stuff
LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);
HICON icon[2];
NOTIFYICONDATA traydata;
UINT WM_TASKBARCREATED=0;
UINT WM_UPDATESETTINGS=0;
UINT WM_ADDTRAY=0;
UINT WM_HIDETRAY=0;
int tray_added=0;
int hide=0;
int update=0;
struct {
	wchar_t *Prevent;
	int Silent;
	wchar_t *HelpUrl;
	int CheckForUpdate;
} settings={NULL,0,NULL,0};
wchar_t txt[1000];

//Cool stuff
UINT WM_SHUTDOWNBLOCKED=0;
int enabled=1;
int vista=0;

//Error message handling
int showerror=1;

LRESULT CALLBACK ErrorMsgProc(INT nCode, WPARAM wParam, LPARAM lParam) {
	if (nCode == HCBT_ACTIVATE) {
		//Edit the caption of the buttons
		SetDlgItemText((HWND)wParam, IDYES, L"Copy error");
		SetDlgItemText((HWND)wParam, IDNO,  L"OK");
	}
	return 0;
}

void Error(wchar_t *func, wchar_t *info, int errorcode, int line) {
	if (showerror) {
		//Format message
		wchar_t errormsg[100];
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,NULL,errorcode,0,errormsg,sizeof(errormsg)/sizeof(wchar_t),NULL);
		errormsg[wcslen(errormsg)-2]='\0'; //Remove that damn newline at the end of the formatted error message
		swprintf(txt,L"%s failed in file %s, line %d.\nError: %s (%d)\n\n%s", func, TEXT(__FILE__), line, errormsg, errorcode, info);
		//Display message
		HHOOK hhk=SetWindowsHookEx(WH_CBT, &ErrorMsgProc, 0, GetCurrentThreadId());
		int response=MessageBox(NULL, txt, APP_NAME" Error", MB_ICONERROR|MB_YESNO|MB_DEFBUTTON2);
		UnhookWindowsHookEx(hhk);
		if (response == IDYES) {
			//Copy message to clipboard
			OpenClipboard(NULL);
			EmptyClipboard();
			wchar_t *data=LocalAlloc(LMEM_FIXED,(wcslen(txt)+1)*sizeof(wchar_t));
			memcpy(data,txt,(wcslen(txt)+1)*sizeof(wchar_t));
			SetClipboardData(CF_UNICODETEXT,data);
			CloseClipboard();
		}
	}
}

//Check for update
DWORD WINAPI _CheckForUpdate() {
	//Check if we are connected to the internet
	DWORD flags; //Not used
	int tries=0; //Try at least ten times, sleep one second between each attempt
	while (InternetGetConnectedState(&flags,0) == FALSE) {
		tries++;
		Sleep(1000);
		if (tries >= 10) {
			#ifdef DEBUG
			Error(L"InternetGetConnectedState()",L"No internet connection.\nPlease check for update manually at "APP_URL,GetLastError(),__LINE__);
			#endif
			return;
		}
	}
	
	//Open connection
	HINTERNET http, file;
	if ((http=InternetOpen(APP_NAME" - "APP_VERSION,INTERNET_OPEN_TYPE_DIRECT,NULL,NULL,0)) == NULL) {
		#ifdef DEBUG
		Error(L"InternetOpen()",L"Could not establish connection.\nPlease check for update manually at "APP_URL,GetLastError(),__LINE__);
		#endif
		return;
	}
	if ((file=InternetOpenUrl(http,APP_UPDATEURL,NULL,0,INTERNET_FLAG_NO_AUTH|INTERNET_FLAG_NO_AUTO_REDIRECT|INTERNET_FLAG_NO_CACHE_WRITE|INTERNET_FLAG_NO_COOKIES|INTERNET_FLAG_NO_UI,0)) == NULL) {
		#ifdef DEBUG
		Error(L"InternetOpenUrl()",L"Could not establish connection.\nPlease check for update manually at "APP_URL,GetLastError(),__LINE__);
		#endif
		return;
	}
	//Read file
	char data[20];
	DWORD numread;
	if (InternetReadFile(file,data,sizeof(data),&numread) == FALSE) {
		#ifdef DEBUG
		Error(L"InternetReadFile()",L"Could not read file.\nPlease check for update manually at "APP_URL,GetLastError(),__LINE__);
		#endif
		return;
	}
	data[numread]='\0';
	//Get error code
	wchar_t code[4];
	DWORD len=sizeof(code);
	HttpQueryInfo(file,HTTP_QUERY_STATUS_CODE,&code,&len,NULL);
	//Close connection
	InternetCloseHandle(file);
	InternetCloseHandle(http);
	
	//Make sure the server returned 200
	if (wcscmp(code,L"200")) {
		#ifdef DEBUG
		swprintf(txt,L"Server returned %s error when checking for update.\nPlease check for update manually at "APP_URL,code);
		MessageBox(NULL, txt, APP_NAME, MB_ICONWARNING|MB_OK);
		#endif
		return;
	}
	
	//New version available?
	if (strcmp(data,APP_VERSION)) {
		update=1;
		wcsncpy(traydata.szInfo,l10n->update_balloon,sizeof(traydata.szInfo)/sizeof(wchar_t));
		traydata.uFlags|=NIF_INFO;
		UpdateTray();
		traydata.uFlags^=NIF_INFO;
	}
}

void CheckForUpdate() {
	CreateThread(NULL,0,_CheckForUpdate,NULL,0,NULL);
}

//Entry point
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInstance, LPSTR szCmdLine, int iCmdShow) {
	//Check command line
	if (!strcmp(szCmdLine,"-hide")) {
		hide=1;
	}
	
	//Register messages
	WM_UPDATESETTINGS=RegisterWindowMessage(L"UpdateSettings");
	WM_ADDTRAY=RegisterWindowMessage(L"AddTray");
	WM_HIDETRAY=RegisterWindowMessage(L"HideTray");
	WM_SHUTDOWNBLOCKED=RegisterWindowMessage(L"ShutdownBlocked");
	
	//Look for previous instance
	HWND previnst;
	if ((previnst=FindWindow(APP_NAME,NULL)) != NULL) {
		PostMessage(previnst,WM_UPDATESETTINGS,0,0);
		if (hide) {
			PostMessage(previnst,WM_HIDETRAY,0,0);
		}
		else {
			PostMessage(previnst,WM_ADDTRAY,0,0);
		}
		return 0;
	}
	
	//Load settings
	wchar_t path[MAX_PATH];
	GetModuleFileName(NULL,path,sizeof(path)/sizeof(wchar_t));
	PathRenameExtension(path,L".ini");
	//Language
	GetPrivateProfileString(APP_NAME,L"Language",L"en-US",txt,sizeof(txt)/sizeof(wchar_t),path);
	int i;
	for (i=0; i < num_languages; i++) {
		if (!wcscmp(txt,languages[i].code)) {
			l10n=languages[i].strings;
		}
	}
	//Prevent
	settings.Prevent=l10n->prevent;
	GetPrivateProfileString(APP_NAME,L"Prevent",L"",txt,sizeof(txt)/sizeof(wchar_t),path);
	if (wcslen(txt) != 0) {
		settings.Prevent=malloc((wcslen(txt)+1)*sizeof(wchar_t));
		wcscpy(settings.Prevent,txt);
	}
	//Silent
	GetPrivateProfileString(APP_NAME,L"Silent",L"0",txt,sizeof(txt)/sizeof(wchar_t),path);
	swscanf(txt,L"%d",&settings.Silent);
	//HelpUrl
	GetPrivateProfileString(APP_NAME,L"HelpUrl",L"",txt,sizeof(txt)/sizeof(wchar_t),path);
	if (wcslen(txt) != 0 && (!wcsncmp(txt,L"http://",7) || !wcsncmp(txt,L"https://",8))) {
		settings.HelpUrl=malloc((wcslen(txt)+1)*sizeof(wchar_t));
		wcscpy(settings.HelpUrl,txt);
	}
	//Update
	GetPrivateProfileString(L"Update",L"CheckForUpdate",L"0",txt,sizeof(txt)/sizeof(wchar_t),path);
	swscanf(txt,L"%d",&settings.CheckForUpdate);
	
	//Create window class
	WNDCLASSEX wnd;
	wnd.cbSize=sizeof(WNDCLASSEX);
	wnd.style=0;
	wnd.lpfnWndProc=WindowProc;
	wnd.cbClsExtra=0;
	wnd.cbWndExtra=0;
	wnd.hInstance=hInst;
	wnd.hIcon=NULL;
	wnd.hIconSm=NULL;
	wnd.hCursor=LoadImage(NULL, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_DEFAULTCOLOR|LR_SHARED);
	wnd.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);
	wnd.lpszMenuName=NULL;
	wnd.lpszClassName=APP_NAME;
	
	//Register class
	RegisterClassEx(&wnd);
	
	//Create window
	HWND hwnd=CreateWindowEx(0, wnd.lpszClassName, APP_NAME, WS_OVERLAPPEDWINDOW^WS_SIZEBOX^WS_MAXIMIZEBOX^WS_MINIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT, 410, 165, NULL, NULL, hInst, NULL);
	/*
	CreateWindowEx(0, L"BUTTON", L"Log off", WS_TABSTOP|WS_VISIBLE|WS_CHILD|BS_PUSHBUTTON, 20, 90, 85, 27, hwnd, NULL, hInst, NULL);
	CreateWindowEx(0, L"BUTTON", L"Shutdown", WS_TABSTOP|WS_VISIBLE|WS_CHILD|BS_DEFPUSHBUTTON, 115, 90, 85, 27, hwnd, NULL, hInst, NULL);
	CreateWindowEx(0, L"BUTTON", L"Reboot", WS_TABSTOP|WS_VISIBLE|WS_CHILD|BS_PUSHBUTTON, 210, 90, 85, 27, hwnd, NULL, hInst, NULL);
	CreateWindowEx(0, L"BUTTON", L"Nothing", WS_TABSTOP|WS_VISIBLE|WS_CHILD|BS_PUSHBUTTON, 305, 90, 85, 27, hwnd, NULL, hInst, NULL);
	CreateWindowEx(0, L"STATIC", L"What do you want to do?", WS_TABSTOP|WS_VISIBLE|WS_CHILD, 95, 46, 200, 25, hwnd, NULL, hInst, NULL);
	ShowWindow(hwnd,iCmdShow);
	*/
	
	//Load icons
	if ((icon[0] = LoadImage(hInst, L"tray-disabled", IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR)) == NULL) {
		Error(L"LoadImage('tray-disabled')",L"",GetLastError(),__LINE__);
		PostQuitMessage(1);
	}
	if ((icon[1] = LoadImage(hInst, L"tray-enabled", IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR)) == NULL) {
		Error(L"LoadImage('tray-enabled')",L"",GetLastError(),__LINE__);
		PostQuitMessage(1);
	}
	
	//Create icondata
	traydata.cbSize=sizeof(NOTIFYICONDATA);
	traydata.uID=0;
	traydata.uFlags=NIF_MESSAGE|NIF_ICON|NIF_TIP;
	traydata.hWnd=hwnd;
	traydata.uCallbackMessage=WM_ICONTRAY;
	//Balloon tooltip
	traydata.uTimeout=10000;
	wcsncpy(traydata.szInfoTitle,APP_NAME,sizeof(traydata.szInfoTitle)/sizeof(wchar_t));
	traydata.dwInfoFlags=NIIF_USER;
	
	//Register TaskbarCreated so we can re-add the tray icon if explorer.exe crashes
	WM_TASKBARCREATED=RegisterWindowMessage(L"TaskbarCreated");
	
	//Update tray icon
	UpdateTray();
	
	//Associate icon to hwnd to make it appear in the Vista shutdown dialog
	SendMessage(hwnd,WM_SETICON,ICON_BIG,(LPARAM)icon[1]);
	
	//Check if we are in Vista and load vista-specific shutdown functions if we are
	OSVERSIONINFO vi;
	vi.dwOSVersionInfoSize=sizeof(OSVERSIONINFO);
	GetVersionEx(&vi);
	if (vi.dwMajorVersion >= 6) {
		//Load user32.dll
		if ((user32=LoadLibrary(L"user32.dll")) == NULL) {
			Error(L"LoadLibrary('user32.dll')",L"This really shouldn't have happened.\nGo check the "APP_NAME" website for an update. If the latest version doesn't fix this, please report it.",GetLastError(),__LINE__);
		}
		else {
			//Get address to ShutdownBlockReasonCreate
			if ((ShutdownBlockReasonCreate=GetProcAddress(user32,"ShutdownBlockReasonCreate")) == NULL) {
				Error(L"GetProcAddress('ShutdownBlockReasonCreate')",L"Failed to load Vista specific function.\nGo check the "APP_NAME" website for an update. If the latest version doesn't fix this, please report it.",GetLastError(),__LINE__);
			}
			//ShutdownBlockReasonDestroy
			if ((ShutdownBlockReasonDestroy=GetProcAddress(user32,"ShutdownBlockReasonDestroy")) == NULL) {
				Error(L"GetProcAddress('ShutdownBlockReasonDestroy')",L"Failed to load Vista specific function.\nGo check the "APP_NAME" website for an update. If the latest version doesn't fix this, please report it.",GetLastError(),__LINE__);
			}
			vista=1;
		}
	}
	
	//Make Windows query this program first
	if (SetProcessShutdownParameters(0x4FF,0) == 0) {
		Error(L"SetProcessShutdownParameters(0x4FF)",L"This means that programs started before "APP_NAME" will probably be closed before the shutdown can be stopped.",GetLastError(),__LINE__);
	}
	
	//Check for update
	if (settings.CheckForUpdate) {
		CheckForUpdate();
	}
	
	//Patch the IAT table of this process (proof-of-concept)
	LoadLibrary(L"patch.dll");
	
	//Message loop
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return msg.wParam;
}

void ShowContextMenu(HWND hwnd) {
	POINT pt;
	GetCursorPos(&pt);
	HMENU hMenu=CreatePopupMenu();
	
	//Toggle
	InsertMenu(hMenu, -1, MF_BYPOSITION, SWM_TOGGLE, (enabled?l10n->menu_disable:l10n->menu_enable));
	
	//Hide
	InsertMenu(hMenu, -1, MF_BYPOSITION, SWM_HIDE, l10n->menu_hide);
	
	//Check autostart
	int autostart_enabled=0, autostart_hide=0;
	//Open key
	HKEY key;
	RegOpenKeyEx(HKEY_CURRENT_USER,L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",0,KEY_QUERY_VALUE,&key);
	//Read value
	wchar_t autostart_value[MAX_PATH+10];
	DWORD len=sizeof(autostart_value);
	RegQueryValueEx(key,APP_NAME,NULL,NULL,(LPBYTE)autostart_value,&len);
	//Close key
	RegCloseKey(key);
	//Get path
	wchar_t path[MAX_PATH];
	GetModuleFileName(NULL,path,MAX_PATH);
	//Compare
	wchar_t pathcmp[MAX_PATH+10];
	swprintf(pathcmp,L"\"%s\"",path);
	if (!wcscmp(pathcmp,autostart_value)) {
		autostart_enabled=1;
	}
	else {
		swprintf(pathcmp,L"\"%s\" -hide",path);
		if (!wcscmp(pathcmp,autostart_value)) {
			autostart_enabled=1;
			autostart_hide=1;
		}
	}
	//Autostart
	HMENU hAutostartMenu=CreatePopupMenu();
	InsertMenu(hAutostartMenu, -1, MF_BYPOSITION|(autostart_enabled?MF_CHECKED:0), (autostart_enabled?SWM_AUTOSTART_OFF:SWM_AUTOSTART_ON), l10n->menu_autostart);
	InsertMenu(hAutostartMenu, -1, MF_BYPOSITION|(autostart_hide?MF_CHECKED:0), (autostart_hide?SWM_AUTOSTART_HIDE_OFF:SWM_AUTOSTART_HIDE_ON), l10n->menu_hide);
	InsertMenu(hMenu, -1, MF_BYPOSITION|MF_POPUP, (UINT)hAutostartMenu, l10n->menu_autostart);
	InsertMenu(hMenu, -1, MF_BYPOSITION|MF_SEPARATOR, 0, NULL);
	
	//Shutdown
	InsertMenu(hMenu, -1, MF_BYPOSITION, SWM_SHUTDOWN, l10n->menu_shutdown);
	InsertMenu(hMenu, -1, MF_BYPOSITION|MF_SEPARATOR, 0, NULL);
	
	//Update
	if (update) {
		InsertMenu(hMenu, -1, MF_BYPOSITION, SWM_UPDATE, l10n->menu_update);
		InsertMenu(hMenu, -1, MF_BYPOSITION|MF_SEPARATOR, 0, NULL);
	}
	
	//About
	InsertMenu(hMenu, -1, MF_BYPOSITION, SWM_ABOUT, l10n->menu_about);
	
	//Exit
	InsertMenu(hMenu, -1, MF_BYPOSITION, SWM_EXIT, l10n->menu_exit);

	//Track menu
	SetForegroundWindow(hwnd);
	TrackPopupMenu(hMenu, TPM_BOTTOMALIGN, pt.x, pt.y, 0, hwnd, NULL);
	DestroyMenu(hMenu);
}

int UpdateTray() {
	wcsncpy(traydata.szTip,(enabled?l10n->tray_enabled:l10n->tray_disabled),sizeof(traydata.szTip)/sizeof(wchar_t));
	traydata.hIcon=icon[enabled];
	
	//Only add or modify if not hidden or if balloon will be displayed
	if (!hide || traydata.uFlags&NIF_INFO) {
		int tries=0; //Try at least ten times, sleep 100 ms between each attempt
		while (Shell_NotifyIcon((tray_added?NIM_MODIFY:NIM_ADD),&traydata) == FALSE) {
			tries++;
			if (tries >= 10) {
				Error(L"Shell_NotifyIcon(NIM_ADD/NIM_MODIFY)",L"Failed to update tray icon.",GetLastError(),__LINE__);
				return 1;
			}
			Sleep(100);
		}
		
		//Success
		tray_added=1;
	}
	return 0;
}

int RemoveTray() {
	if (!tray_added) {
		//Tray not added
		return 1;
	}
	
	if (Shell_NotifyIcon(NIM_DELETE,&traydata) == FALSE) {
		Error(L"Shell_NotifyIcon(NIM_DELETE)",L"Failed to remove tray icon.",GetLastError(),__LINE__);
		return 1;
	}
	
	//Success
	tray_added=0;
	return 0;
}

void SetAutostart(int on, int hide) {
	//Open key
	HKEY key;
	int error;
	if ((error=RegOpenKeyEx(HKEY_CURRENT_USER,L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",0,KEY_SET_VALUE,&key)) != ERROR_SUCCESS) {
		Error(L"RegOpenKeyEx(HKEY_CURRENT_USER,'Software\\Microsoft\\Windows\\CurrentVersion\\Run')",L"Error opening the registry.",error,__LINE__);
		return;
	}
	if (on) {
		//Get path
		wchar_t path[MAX_PATH];
		if (GetModuleFileName(NULL,path,MAX_PATH) == 0) {
			Error(L"GetModuleFileName(NULL)",L"",GetLastError(),__LINE__);
			return;
		}
		//Add
		wchar_t value[MAX_PATH+10];
		swprintf(value,(hide?L"\"%s\" -hide":L"\"%s\""),path);
		if ((error=RegSetValueEx(key,APP_NAME,0,REG_SZ,(LPBYTE)value,(wcslen(value)+1)*sizeof(wchar_t))) != ERROR_SUCCESS) {
			Error(L"RegSetValueEx('"APP_NAME"')",L"",error,__LINE__);
			return;
		}
	}
	else {
		//Remove
		if ((error=RegDeleteValue(key,APP_NAME)) != ERROR_SUCCESS) {
			Error(L"RegDeleteValue('"APP_NAME"')",L"",error,__LINE__);
			return;
		}
	}
	//Close key
	RegCloseKey(key);
}

void ToggleState() {
	enabled=!enabled;
	UpdateTray();
	if (enabled) {
		SendMessage(traydata.hWnd,WM_UPDATESETTINGS,0,0);
	}
}

LRESULT CALLBACK ShutdownDialogProc(INT nCode, WPARAM wParam, LPARAM lParam) {
	if (nCode == HCBT_ACTIVATE) {
		//Edit the caption of the buttons
		SetDlgItemText((HWND)wParam,IDYES,l10n->shutdown_logoff);
		SetDlgItemText((HWND)wParam,IDNO,l10n->shutdown_shutdown);
		SetDlgItemText((HWND)wParam,IDCANCEL,l10n->shutdown_nothing);
	}
	return 0;
}

void AskShutdown() {
	//ShowWindow(traydata.hWnd, SW_SHOW);
	HHOOK hhk=SetWindowsHookEx(WH_CBT, &ShutdownDialogProc, 0, GetCurrentThreadId());
	int response=MessageBox(traydata.hWnd, l10n->shutdown_ask, APP_NAME, MB_ICONQUESTION|MB_YESNOCANCEL|(settings.HelpUrl!=NULL?MB_HELP:0)|MB_DEFBUTTON2|MB_SYSTEMMODAL);
	UnhookWindowsHookEx(hhk);
	if (response == IDYES || response == IDNO) {
		enabled=0;
		hide=0;
		UpdateTray();
		if (response == IDYES) {
			ExitWindowsEx(EWX_LOGOFF,0);
		}
		else {
			//Get process token
			HANDLE hToken;
			if (OpenProcessToken(GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY,&hToken) == 0) {
				Error(L"OpenProcessToken()",L"Could not get privilege to shutdown computer. Try shutting down manually.",GetLastError(),__LINE__);
				return;
			}
			
			//Get LUID for SeShutdownPrivilege
			TOKEN_PRIVILEGES tkp;
			LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);
			tkp.PrivilegeCount=1;
			tkp.Privileges[0].Attributes=SE_PRIVILEGE_ENABLED;
			
			//Enable SeShutdownPrivilege
			AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, NULL, 0); 
			if (GetLastError() != ERROR_SUCCESS) {
				Error(L"AdjustTokenPrivileges()",L"Could not get privilege to shutdown computer. Try shutting down manually.",GetLastError(),__LINE__);
				return;
			}
			
			//Do it!!
			ExitWindowsEx(EWX_SHUTDOWN,0);

			//Disable SeShutdownPrivilege
			tkp.Privileges[0].Attributes=0;
			AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, NULL, 0);
		}
		
	}
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (msg == WM_ICONTRAY) {
		if (lParam == WM_LBUTTONDOWN || lParam == WM_LBUTTONDBLCLK) {
			ToggleState();
		}
		else if (lParam == WM_RBUTTONDOWN) {
			ShowContextMenu(hwnd);
		}
		else if (lParam == WM_MBUTTONDOWN) {
			AskShutdown();
		}
		else if (lParam == NIN_BALLOONTIMEOUT) {
			if (hide) {
				RemoveTray();
			}
		}
		else if (lParam == NIN_BALLOONUSERCLICK) {
			if (!wcscmp(traydata.szInfo,l10n->update_balloon)) {
				hide=0;
				SendMessage(hwnd,WM_COMMAND,SWM_UPDATE,0);
			}
			else {
				AskShutdown();
			}
			if (hide) {
				RemoveTray();
			}
		}
	}
	else if (msg == WM_UPDATESETTINGS) {
		//Load settings
		wchar_t path[MAX_PATH];
		GetModuleFileName(NULL,path,sizeof(path)/sizeof(wchar_t));
		PathRenameExtension(path,L".ini");
		//Language
		GetPrivateProfileString(APP_NAME,L"Language",L"en-US",txt,sizeof(txt)/sizeof(wchar_t),path);
		int i;
		for (i=0; i < num_languages; i++) {
			if (!wcscmp(txt,languages[i].code)) {
				l10n=languages[i].strings;
			}
		}
		//Prevent
		if (settings.Prevent != l10n->prevent) {
			free(settings.Prevent);
			settings.Prevent=l10n->prevent;
		}
		GetPrivateProfileString(APP_NAME,L"Prevent",L"",txt,sizeof(txt)/sizeof(wchar_t),path);
		if (wcslen(txt) != 0) {
			settings.Prevent=malloc((wcslen(txt)+1)*sizeof(wchar_t));
			wcscpy(settings.Prevent,txt);
		}
		//HelpUrl
		if (settings.HelpUrl != NULL) {
			free(settings.HelpUrl);
			settings.HelpUrl=NULL;
		}
		GetPrivateProfileString(APP_NAME,L"HelpUrl",L"",txt,sizeof(txt)/sizeof(wchar_t),path);
		if (wcslen(txt) != 0 && (!wcsncmp(txt,L"http://",7) || !wcsncmp(txt,L"https://",8))) {
			settings.HelpUrl=malloc((wcslen(txt)+1)*sizeof(wchar_t));
			wcscpy(settings.HelpUrl,txt);
		}
		//Silent
		GetPrivateProfileString(APP_NAME,L"Silent",L"0",txt,sizeof(txt)/sizeof(wchar_t),path);
		swscanf(txt,L"%d",&settings.Silent);
	}
	else if (msg == WM_ADDTRAY && (!settings.Silent || GetAsyncKeyState(VK_SHIFT)&0x8000)) {
		hide=0;
		UpdateTray();
	}
	else if (msg == WM_HIDETRAY) {
		hide=1;
		RemoveTray();
	}
	else if (msg == WM_TASKBARCREATED) {
		tray_added=0;
		UpdateTray();
	}
	else if (msg == WM_SHUTDOWNBLOCKED) {
		//Display balloon tip
		wcsncpy(traydata.szInfo,settings.Prevent,(sizeof(traydata.szInfo))/sizeof(wchar_t));
		wcscat(traydata.szInfo,L"\n");
		wcscat(traydata.szInfo,l10n->balloon);
		traydata.uFlags|=NIF_INFO;
		UpdateTray();
		traydata.uFlags^=NIF_INFO;
	}
	else if (msg == WM_COMMAND) {
		int wmId=LOWORD(wParam), wmEvent=HIWORD(wParam);
		if (wmId == SWM_TOGGLE) {
			ToggleState();
		}
		else if (wmId == SWM_HIDE) {
			hide=1;
			RemoveTray();
		}
		else if (wmId == SWM_AUTOSTART_ON) {
			SetAutostart(1,0);
		}
		else if (wmId == SWM_AUTOSTART_OFF) {
			SetAutostart(0,0);
		}
		else if (wmId == SWM_AUTOSTART_HIDE_ON) {
			SetAutostart(1,1);
		}
		else if (wmId == SWM_AUTOSTART_HIDE_OFF) {
			SetAutostart(1,0);
		}
		else if (wmId == SWM_SHUTDOWN) {
			AskShutdown();
		}
		else if (wmId == SWM_UPDATE) {
			if (MessageBox(NULL, l10n->update_dialog, APP_NAME, MB_ICONINFORMATION|MB_YESNO|MB_SYSTEMMODAL) == IDYES) {
				ShellExecute(NULL, L"open", APP_URL, NULL, NULL, SW_SHOWNORMAL);
			}
		}
		else if (wmId == SWM_ABOUT) {
			MessageBox(NULL, l10n->about, l10n->about_title, MB_ICONINFORMATION|MB_OK);
		}
		else if (wmId == SWM_EXIT) {
			DestroyWindow(hwnd);
		}
	}
	else if (msg == WM_DESTROY) {
		showerror=0;
		RemoveTray();
		if (user32) {
			FreeLibrary(user32);
		}
		PostQuitMessage(0);
		return 0;
	}
	else if (msg == WM_QUERYENDSESSION) {
		if (enabled) {
			//Prevent shutdown
			if (vista) {
				ShutdownBlockReasonCreate(hwnd,settings.Prevent);
				hide=0;
				UpdateTray();
			}
			else if (!settings.Silent || !hide || GetAsyncKeyState(VK_SHIFT)&0x800) {
				//Show balloon, in vista it would just be automatically dismissed by the shutdown dialog
				wcsncpy(traydata.szInfo,settings.Prevent,(sizeof(traydata.szInfo))/sizeof(wchar_t));
				wcscat(traydata.szInfo,L"\n");
				wcscat(traydata.szInfo,l10n->balloon);
				traydata.uFlags|=NIF_INFO;
				UpdateTray();
				traydata.uFlags^=NIF_INFO;
			}
			return FALSE;
		}
		else {
			if (vista) {
				ShutdownBlockReasonDestroy(hwnd);
			}
			return TRUE;
		}
	}
	else if (msg == WM_HELP) {
		ShellExecute(NULL, L"open", settings.HelpUrl, NULL, NULL, SW_SHOWNORMAL);
	}
	else if (/*msg == WM_CLOSE ||*/ (msg == WM_KEYDOWN && wParam == VK_ESCAPE)) {
		ShowWindow(hwnd,SW_HIDE);
		return;
	}
	else if (msg == WM_SYSCOMMAND && wParam&0xFFF0 == SC_CLOSE) {
		return 0;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}
