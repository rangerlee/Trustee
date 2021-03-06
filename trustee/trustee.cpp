// trustee.cpp : Basic service mainhandler。
//
#define _CRT_SECURE_NO_WARNINGS
#include <plog/Log.h>
#include <plog/Appenders/ColorConsoleAppender.h>
#include <scarlet/string/format.hpp>
#include <scarlet/core/singleton.hpp>
#include <iostream>
#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")
#include "proc_man.hpp"

namespace trustee {
	static const size_t kNameSize = 128;
	static char ServiceName[kNameSize] = "trustee";
	static SERVICE_STATUS_HANDLE ServiceHandle;
}

DWORD WINAPI ServiceCtrlHandler(DWORD  dwControl, DWORD  dwEventType, LPVOID lpEventData, LPVOID lpContext){
	LOGD << "Catch Service Ctrl " << dwControl;
	switch (dwControl){
	case SERVICE_CONTROL_STOP:
	case SERVICE_CONTROL_SHUTDOWN:
		SERVICE_STATUS status;
		ZeroMemory(&status, sizeof(SERVICE_STATUS));
		status.dwWin32ExitCode = 0;
		status.dwCurrentState = SERVICE_STOP_PENDING;
		SetServiceStatus(trustee::ServiceHandle, &status);
		scarlet::singleton<ProcMan>::get()->Quit();
		scarlet::singleton<ProcMan>::get()->join();
		return NO_ERROR;
	default:
		break;
	}

	return ERROR_CALL_NOT_IMPLEMENTED;
}

VOID WINAPI ServiceMain(DWORD  dwArgc, LPTSTR *lpszArgv) {
	char path[MAX_PATH] = { 0 };
	GetModuleFileName(NULL, path, MAX_PATH);
	PathRemoveFileSpec(path);
	SetCurrentDirectory(path);

	static plog::RollingFileAppender<plog::TxtFormatter> file("trustee.log", 10485760, 10);
	plog::init(plog::info, &file);

	scarlet::singleton<ProcMan>::reset(new ProcMan());

	trustee::ServiceHandle = RegisterServiceCtrlHandlerEx(trustee::ServiceName, &ServiceCtrlHandler, NULL);
	if (trustee::ServiceHandle == (SERVICE_STATUS_HANDLE)0){
		LOGE << "Service RegisterServiceCtrlHandlerEx failed " << GetLastError();
		return ;
	}

	SERVICE_STATUS status;
	ZeroMemory(&status, sizeof(SERVICE_STATUS));
	status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	status.dwCurrentState = SERVICE_RUNNING;
	status.dwControlsAccepted = SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_STOP;
	SetServiceStatus(trustee::ServiceHandle, &status);

	scarlet::singleton<ProcMan>::get()->start();
	scarlet::singleton<ProcMan>::get()->join();

	status.dwCurrentState = SERVICE_STOPPED;
	SetServiceStatus(trustee::ServiceHandle, &status);
	scarlet::singleton<ProcMan>::reset();
	LOGI << "Service Exit.";
}

int main(int argc, char** argv){
#ifdef _DEBUG
	static plog::ColorConsoleAppender<plog::TxtFormatter> appender;
	plog::init(plog::verbose, &appender);

	char path[MAX_PATH] = { 0 };
	GetModuleFileName(NULL, path, MAX_PATH);
	PathRemoveFileSpec(path);
	SetCurrentDirectory(path);
	
	LOGD << "CurrentDirectory " << path;

	scarlet::singleton<ProcMan>::reset(new ProcMan());
	scarlet::singleton<ProcMan>::get()->start();
	LOGD << "Press any key ...";
	getchar();
	scarlet::singleton<ProcMan>::get()->Quit();
	scarlet::singleton<ProcMan>::get()->join();
	LOGD << "Done!";
	scarlet::singleton<ProcMan>::reset();
#else
	SERVICE_TABLE_ENTRY ServiceTable[2];
	ServiceTable[0].lpServiceName = trustee::ServiceName;
	ServiceTable[0].lpServiceProc = (LPSERVICE_MAIN_FUNCTION)&ServiceMain;
	ServiceTable[1].lpServiceName = NULL;
	ServiceTable[1].lpServiceProc = NULL;
	StartServiceCtrlDispatcher(ServiceTable);
#endif

	return 0;
}
