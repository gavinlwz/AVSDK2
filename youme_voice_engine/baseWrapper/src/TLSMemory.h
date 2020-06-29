#pragma once
#include <map>
#include <mutex>
#include <memory>

#if WIN32
#include <Windows.h>
typedef unsigned long TLS_ThreadID;
#else
#include <pthread.h>
#include <sched.h>
#include <cstdlib>
typedef pthread_t TLS_ThreadID;
#endif


struct TLSMemoryInfo
{
	void* pAddress;
	int iSize;
	bool bIsUse;//因为和线程相关，所以同一时刻只会有一个人使用到
	int iFreeTime; //释放的时间戳
	TLSMemoryInfo()
	{
		pAddress = nullptr;
		iSize = 0;
		bIsUse = false;
		iFreeTime = -1;
	}
	~TLSMemoryInfo()
	{
		free(pAddress);
	}
};


//为了解决频繁的申请内存。线程相关 内存。只做临时中转，绝对不要保存指针
class CTLSMemory
{
public:
	static CTLSMemory* GetInstance();
	~CTLSMemory();
private:
	CTLSMemory();
public:
	//由于是和线程相关，所以申请的内存不需要考虑线程同步，但是内存仅仅是只能用做临时中转
	//绝对不要保存 地址。使用的时候直接申请，不要预申请。
	void* Allocate(int iSize, int& iSlot);
	void Free(int iSlot);
private:
	static int ClearProc(const void* pParam, long iTimerID);
	TLS_ThreadID GetThreadID();
	std::mutex m_mutex;
	std::map<TLS_ThreadID, std::map<int, std::shared_ptr<TLSMemoryInfo> > >m_tlsMemoryMap;
	
	long m_clearTimer;
};

