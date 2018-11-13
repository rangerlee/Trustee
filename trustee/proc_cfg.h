#pragma once
#include <string>

typedef struct {
	std::string exec;
	std::string path;
	int32_t     restart;
} ProcConfig;
