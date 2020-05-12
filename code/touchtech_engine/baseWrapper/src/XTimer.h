#pragma once
#include "YouMeCommon/XSemaphore.h"
#include "YouMeCommon/XCondWait.h"
#include "tsk_thread.h"
#include <memory>
#include <list>
#include <map>
typedef int(TSK_STDCALL *TimerCallback)(const void* pParam, long iTimerID);

struct TimerParam
{
	uint64_t ulTimeout;
    youmecommon::CXCondWait conWait;
	tsk_thread_handle_t* waitThrad;
	TimerCallback pCallback;
	void* pParam;
    //从汇编来看加上volatile，编译器不会再使用寄存器来保存这个变量，每次需要存取的时候都会读一次内存，极大的减少重入的概率
	volatile bool bCancle;
    bool bOnce;
	TimerParam()
	{
		pCallback = NULL;
		pParam = NULL;
		bCancle = false;
		waitThrad = NULL;
		ulTimeout = 0;
        bOnce = false;
	}
};

class CXTimer
{
public:
	static CXTimer* GetInstance();
    CXTimer();
	~CXTimer();

	//启动一个主线成，用来回调
	bool Init();
	void UnInit();

	//返回一个定时器ID
	long PostTimer(uint64_t ulTimeout, TimerCallback callback, void* pParam,bool bOnceTimer=false);
	bool CancleTimer(long iTimerID);
private:
	
	static void* TSK_STDCALL WaitThread(void * pParam);
	std::mutex m_mapMutex;
	bool m_bExit;
	long m_iTimerID;
	std::map<long, std::shared_ptr<TimerParam> > m_timerMap;
};

