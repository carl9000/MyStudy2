// speech.cpp: 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include "speech.h"

#ifdef _WIN64 
#pragma comment(lib,"../library/msc_x64.lib")
#else 
#pragma comment(lib, "../library/msc.lib")
#endif 

int __stdcall start(char* parm) {

	return 1111;
}

