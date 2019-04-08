#pragma once
#include "proc_cfg.h"
#include <cstring>
#include <scarlet/thread/native_thread.hpp>
#include <plog/Log.h>

class ProcThread : public scarlet::native_thread {
public:
	ProcThread() {
		ZeroMemory(&pi_, sizeof(pi_));
		event_ = CreateEventA(NULL, FALSE, FALSE, NULL);
		delay_ = CreateEventA(NULL, FALSE, FALSE, NULL);
	}

	virtual ~ProcThread() {
		CloseProcessHandles();
		CloseHandle(event_);
		CloseHandle(delay_);
	}

public:
	bool Init(ProcConfig cfg){
		ZeroMemory(&pi_, sizeof(pi_));
		config_ = cfg;
		return start();
	}

	void Fini() {
		SetEvent(delay_);
		SetEvent(event_);		
		join();
	}

private:
	virtual void run(){
		static const int kRunOnce = 0;
		for(;;) {
			ExecProcess();

			if (config_.restart == kRunOnce){
				break;
			} else if (WAIT_OBJECT_0 == WaitForSingleObject(delay_, config_.restart)) {
				break;
			}
		}

		LOGI << "ProcThread Exit.";
	}

private:
	void ExecProcess(){
		LOGD << "Execute [" << config_.exec << "] @ [" << config_.path << "]";
		static const size_t kMaxCmdSize = 2048;
		static const uint32_t kWaitSize = 2;
		char command[kMaxCmdSize] = { 0 };
		strncpy(command, config_.exec.data(), kMaxCmdSize);

		STARTUPINFOA si;
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);		

		BOOL ret = ::CreateProcessA(NULL, command,	NULL, NULL,	FALSE,
			0, NULL, config_.path.data(), &si, &pi_);
		if(ret){
			LOGI << "CreateProcess [" << config_.exec << "] PID " << pi_.dwProcessId;
			HANDLE wait[kWaitSize] = { pi_.hProcess, event_ };
			int ret = WaitForMultipleObjects(kWaitSize, wait, FALSE, INFINITE);
			if (ret == WAIT_OBJECT_0) {
				LOGW << "Process [" << config_.exec << "] Exit, restart " << config_.restart;
			} else if (ret == WAIT_OBJECT_0 + 1) {
				LOGI << "Catch exit signal, terminate process " << pi_.dwProcessId;
				TerminateProcess(pi_.hProcess, 0);
			} else {
				LOGW << "WaitForMultipleObjects return " << ret << " [" << GetLastError() << "]";
			}
		} else {
			LOGW << "Create Process [" << config_.exec << "] Failed " << GetLastError();
		}

		CloseProcessHandles();
	}

	void CloseProcessHandles() {
		if (pi_.hThread) {
			CloseHandle(pi_.hThread);
		}

		if (pi_.hProcess) {
			CloseHandle(pi_.hProcess);
		}

		ZeroMemory(&pi_, sizeof(pi_));
	}

private:
	ProcConfig			config_;
	PROCESS_INFORMATION pi_;
	HANDLE				event_;
	HANDLE				delay_;
};
