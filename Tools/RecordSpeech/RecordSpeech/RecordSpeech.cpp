// RecordSpeech.cpp: 定义 DLL 应用程序的导出函数。
//
#pragma warning(disable:4996)

#include "stdafx.h"
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <conio.h>
#include <errno.h>
#include <process.h>

#include "msp_cmn.h"
#include "msp_errors.h"
//
//#include "Source/RecordSpeech.h"
//#include "source/SpeechRecognizer.h"

#define FRAME_LEN	640 
#define	BUFFER_SIZE	4096

enum {
	EVT_START = 0,
	EVT_STOP,
	EVT_QUIT,
	EVT_TOTAL
};
static HANDLE events[EVT_TOTAL] = { NULL,NULL,NULL };

static COORD begin_pos = { 0, 0 };
static COORD last_pos = { 0, 0 };


/*
* sub:				请求业务类型
* domain:			领域
* language:			语言
* accent:			方言
* sample_rate:		音频采样率
* result_type:		识别结果格式
* result_encoding:	结果编码格式
*
*/
//start(char* parm) = "sub = iat, domain = iat, language = zh_cn, accent = mandarin, sample_rate = 16000, result_type = plain, result_encoding = gb2312";


int __stdcall start(char* parm) {

	int			ret = MSP_SUCCESS;
	int			upload_on = 1; //是否上传用户词表
	const char* login_params = "appid = 5b0dfbb4, work_dir = ."; // 登录参数，appid与msc库绑定,请勿随意改动
	int aud_src = 0;

	/* 用户登录 */
	ret = MSPLogin(NULL, NULL, login_params); //第一个参数是用户名，第二个参数是密码，均传NULL即可，第三个参数是登录参数	
	if (MSP_SUCCESS != ret) {
		//printf("MSPLogin failed , Error code %d.\n", ret);
		goto exit; //登录失败，退出登录
	}

exit:
	//printf("按任意键退出 ...\n");
	_getch();
	MSPLogout(); //退出登录

	return 0;

}

int __stdcall stop(char* id) {
	return  2;
}