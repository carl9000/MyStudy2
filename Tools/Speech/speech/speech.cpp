// speech.cpp: 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include "stdlib.h"
#include <conio.h>
#include <process.h>
#include "msp_cmn.h"
#include "msp_errors.h"
#include "recognizer.h"
#include "speech.h"

#ifdef _WIN64 
#pragma comment(lib,"../library/msc_x64.lib")
#else 
#pragma comment(lib, "../library/msc.lib")
#endif 

#define FRAME_LEN	640 
#define	BUFFER_SIZE	4096

/*
const char* login_params = "appid = 5b0dfbb4, work_dir = ."; // 登录参数，appid与msc库绑定,请勿随意改动
*/

/*
* sub:				请求业务类型
* domain:			领域
* language:			语言
* accent:			方言
* sample_rate:		音频采样率
* result_type:		识别结果格式
* result_encoding:	结果编码格式
*
//const char* session_begin_params = "sub = iat, domain = iat, language = zh_cn, accent = mandarin, sample_rate = 16000, result_type = plain, result_encoding = gb2312";
*/

enum {
	EVT_START = 0,
	EVT_STOP,
	EVT_QUIT,
	EVT_TOTAL
};

static HANDLE events[EVT_TOTAL] = { NULL,NULL,NULL };

static COORD begin_pos = { 0, 0 };
static COORD last_pos = { 0, 0 };

static int DEFAULTERRORCODE = -1;

static startFun on_start;
static stopFun on_stop;
static resultFun on_res;
static errorFun on_error;

static char *g_result = NULL;
static unsigned int g_buffersize = BUFFER_SIZE;


/* helper thread: to listen to the keystroke */
static unsigned int  __stdcall helper_thread_proc(void * para)
{
	int key;
	int quit = 0;

	do {
		key = _getch();
		switch (key) {
		case 'r':
		case 'R':
			SetEvent(events[EVT_START]);
			break;
		case 's':
		case 'S':
			SetEvent(events[EVT_STOP]);
			break;
		case 'q':
		case 'Q':
			quit = 1;
			SetEvent(events[EVT_QUIT]);
			PostQuitMessage(0);
			break;
		default:
			break;
		}

		if (quit)
			break;
	} while (1);

	return 0;
}

static HANDLE start_helper_thread()
{
	HANDLE hdl;

	hdl = (HANDLE)_beginthreadex(NULL, 0, helper_thread_proc, NULL, 0, NULL);

	return hdl;
}


void on_speech_begin()
{
	if (g_result)
	{
		free(g_result);
	}
	g_result = (char*)malloc(BUFFER_SIZE);
	g_buffersize = BUFFER_SIZE;
	memset(g_result, 0, g_buffersize);
	on_start();
}

void on_speech_end(int reason)
{
	on_stop(reason);//reason == END_REASON_VAD_DETECT

	//if (reason == END_REASON_VAD_DETECT) {
	//	//printf("\nSpeaking done \n");
	//}
	//else {
	//	//printf("\nRecognizer error %d\n", reason);
	//}
}

void on_result(const char *result, char is_last)
{
	if (result) {
		size_t left = g_buffersize - 1 - strlen(g_result);
		size_t size = strlen(result);
		if (left < size) {
			g_result = (char*)realloc(g_result, g_buffersize + BUFFER_SIZE);
			if (g_result)
				g_buffersize += BUFFER_SIZE;
			else {
				on_error(MSP_ERROR_EXCEPTION, "mem alloc failed");
				return;
			}
		}
		strncat(g_result, result, size);
		on_res(g_result, is_last);
	}
}


static void mic(const char* session_begin_params)
{
	int i = 0;
	HANDLE helper_thread = NULL;
	DWORD waitres;
	char isquit = 0;

	struct speech_rec iat;
	struct speech_rec_notifier recnotifier = {
		on_result,
		on_speech_begin,
		on_speech_end
	};

	int res = sr_init(&iat, session_begin_params, SR_MIC, DEFAULT_INPUT_DEVID, &recnotifier);
	if (res) {
		on_error(res, "speech recognizer init failed");
		return;
	}

	for (i = 0; i < EVT_TOTAL; ++i) {
		events[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
	}

	helper_thread = start_helper_thread();
	if (helper_thread == NULL) {
		on_error(res, "create thread failed");
		goto exit;
	}

	on_error(res, "while");

	while (1) {
		waitres = WaitForMultipleObjects(EVT_TOTAL, events, FALSE, INFINITE);
		switch (waitres) {
		case WAIT_FAILED:
		case WAIT_TIMEOUT:
			on_error(res, "WAIT_FAILED / WAIT_TIMEOUT Why it happened !");
			break;
		case WAIT_OBJECT_0 + EVT_START:
			if (res = sr_start_listening(&iat)) {
				on_error(res, "start listen failed");
				isquit = 1;
			}
			break;
		case WAIT_OBJECT_0 + EVT_STOP:
			if (res = sr_stop_listening(&iat)) {
				on_error(res, "stop listening failed");
				isquit = 1;
			}
			else
			{
				if (res = sr_start_listening(&iat)) {
					on_error(res, "start listen failed");
					isquit = 1;
				}
			}
			break;
		case WAIT_OBJECT_0 + EVT_QUIT:
			sr_stop_listening(&iat);
			isquit = 1;
			break;
		default:
			break;
		}
		if (isquit)
			break;
	}

exit:

	if (helper_thread != NULL) {
		WaitForSingleObject(helper_thread, INFINITE);
		CloseHandle(helper_thread);
	}

	for (i = 0; i < EVT_TOTAL; ++i) {
		if (events[i])
			CloseHandle(events[i]);
	}

	sr_uninit(&iat);
}


int __stdcall startupTask(char* login_params, char* session_begin_params,
	int(*startCallback)(),
	int(*stopCallback)(int),
	int(*resultCallback)(char*, char),
	int(*errorCallback)(int, char*)) {

	int ret = MSP_SUCCESS;

	//设置回调
	on_start = startCallback;
	on_stop = stopCallback;
	on_res = resultCallback;
	on_error = errorCallback;

	/* 用户登录 */
	ret = MSPLogin(NULL, NULL, login_params);
	if (MSP_SUCCESS != ret) {
		on_error(ret, "MSPLogin failed");
		goto exit;
	}

	/*启动识别*/
	mic(session_begin_params);

exit:
	MSPLogout();

	return ret;
}