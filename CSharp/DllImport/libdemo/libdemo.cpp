// libdemo.cpp: 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include "libdemo.h"

int __stdcall test(int a, int b) {
	
	//循环
	/*while (true)
	{
		Sleep(1);
	}*/


	//异常(好像捕获不到的，只能将返回状态码完善，待确认)
	//throw "dll异常";

	return a + b;
}