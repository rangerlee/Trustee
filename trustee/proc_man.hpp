#pragma once
#include <map>
#include <scarlet/thread/native_thread.hpp>
#include <plog/Log.h>
#include "proc_trd.hpp"

class ProcMan : public scarlet::native_thread {
	enum { PM_QUIT = WM_QUIT, PM_LOAD_CFG = WM_USER};
public:
	void Quit() {
		PostThreadMessage(id(), PM_QUIT, 0, 0);
	}

private:
	virtual void run() {
		PostMessage(NULL, PM_LOAD_CFG, 0, 0);

		MSG message;
		while (GetMessage(&message, NULL, 0, 0)) {
			switch (message.message) {
			case PM_LOAD_CFG:
				ReloadProc(ReadLocalConfig());
				break;
			}
		}

		CloseAllThreads();
		LOGI << "ProcMan Exit.";
	}

private:
	std::map<std::string, ProcConfig> ReadLocalConfig() {
		std::map<std::string, ProcConfig> cfg;
		static const size_t kBufSize = 4096;
		static const char*  kAppCfg = "app.ini";
		
		char current[MAX_PATH] = { 0 };
		GetCurrentDirectory(MAX_PATH, current);
		std::string ini = scarlet::format("%s\\%s", current, kAppCfg);

		char temp[kBufSize] = { 0 };
		char buff[kBufSize] = { 0 };
		DWORD size = GetPrivateProfileSectionNames(temp, kBufSize, ini.c_str());
		char* pos = temp;
		for (size_t i = 0; i < size; ++i){
			if (temp[i] == 0){
				std::string name = pos;
				ProcConfig proc;
				GetPrivateProfileString(name.c_str(), "Exec", "", buff, kBufSize, ini.c_str());
				proc.exec = buff;
				GetPrivateProfileString(name.c_str(), "Path", "", buff, kBufSize, ini.c_str());
				proc.path = buff;
				proc.restart = GetPrivateProfileInt(name.c_str(), "Restart", 0, ini.c_str());
				cfg[name] = proc;

				LOGD << "[" << name << "] (" << proc.exec << ")@[" << proc.path << "] " << proc.restart;

				pos = temp + i + 1;
			}
		}	

		return cfg;
	}

	void ReloadProc(const std::map<std::string, ProcConfig>& cfg) {
		for (auto it = proc_.begin(); it != proc_.end();) {
			if (cfg.find(it->first) == cfg.end()) {
				LOGI << "ProcMan found [" << it->first << "] was removed";
				it->second->Fini();
				it = proc_.erase(it);
			} else {
				it++;
			}
		}

		for (auto it = cfg.begin(); it != cfg.end(); ++it) {
			std::shared_ptr<ProcThread> proc(new ProcThread);
			if (!proc->Init(it->second)) {
				LOGW << "ProcMan found [" << it->first << "] Init failed";
				continue;
			}

			LOGI << "ProcMan start [" << it->first << "]";
			proc_[it->first] = proc;
		}
	}

	void CloseAllThreads() {
		std::for_each(proc_.begin(), proc_.end(), 
			[](const std::pair<std::string, std::shared_ptr<ProcThread>>& pair) {
			pair.second->Fini();
		});

		proc_.clear();
	}

private:
	std::map<std::string, std::shared_ptr<ProcThread>> proc_;
};
