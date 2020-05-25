#include "TLSMemory.h"
#include "tsk_time.h"
#include "XTimer.h"
#include "tsk_log.h"
CTLSMemory::CTLSMemory()
{
	m_clearTimer = CXTimer::GetInstance()->PostTimer(10 * 1000, &CTLSMemory::ClearProc, this);
}


CTLSMemory* CTLSMemory::GetInstance()
{
	static CTLSMemory* s_TLSMemory = new CTLSMemory;
	return s_TLSMemory;
}

CTLSMemory::~CTLSMemory()
{
	CXTimer::GetInstance()->CancleTimer(m_clearTimer);
}

void* CTLSMemory::Allocate(int iSize,int& iSlot)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	//获取线程ID
    TLS_ThreadID iThreadID = 0;//GetThreadID();
	std::map<TLS_ThreadID, std::map<int, std::shared_ptr<TLSMemoryInfo> > >::iterator it = m_tlsMemoryMap.find(iThreadID);
	if (m_tlsMemoryMap.end() == it)
	{
		m_tlsMemoryMap[iThreadID] = std::map<int, std::shared_ptr<TLSMemoryInfo> >();
		it = m_tlsMemoryMap.find(iThreadID);
	}
	//找到一个未使用的slot，返回
	std::map<int, std::shared_ptr<TLSMemoryInfo> >::iterator slotBegin = it->second.begin();
	std::map<int, std::shared_ptr<TLSMemoryInfo> >::iterator slotEnd = it->second.end();

	for (; slotBegin != slotEnd;slotBegin++)
	{
		if (slotBegin->second->bIsUse)
		{
			continue;
		}
		break;
	}
	//没有找到合适的
	if (slotBegin == slotEnd)
	{
		int iLastSlot = 1;
		std::map<int, std::shared_ptr<TLSMemoryInfo> >::iterator tmpSlotIter = it->second.end();
		if (it->second.size() !=0)
		{
			tmpSlotIter--; //前面一个
			iLastSlot = tmpSlotIter->first;
			iLastSlot++;
		}

		it->second.insert(std::map<int, std::shared_ptr<TLSMemoryInfo> >::value_type(iLastSlot, std::shared_ptr<TLSMemoryInfo>(new TLSMemoryInfo)));
		slotBegin = it->second.find(iLastSlot);
	}

	slotBegin->second->bIsUse = true;
	if (slotBegin->second->iSize < iSize)
	{
		//不够了，要重新申请下
		slotBegin->second->iSize = iSize;
		if (nullptr == slotBegin->second->pAddress)
		{
			slotBegin->second->pAddress = calloc(iSize, 1);
		}
		else
		{
			slotBegin->second->pAddress = realloc(slotBegin->second->pAddress, iSize);
		}
	}
	iSlot = slotBegin->first;
	return slotBegin->second->pAddress;
}


void CTLSMemory::Free(int iSlot)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	//获取线程ID
    TLS_ThreadID iThreadID = 0;//GetThreadID();
	std::map<TLS_ThreadID, std::map<int, std::shared_ptr<TLSMemoryInfo> > >::iterator it = m_tlsMemoryMap.find(iThreadID);
	if (m_tlsMemoryMap.end() == it)
	{
		m_tlsMemoryMap[iThreadID] = std::map<int, std::shared_ptr<TLSMemoryInfo> >();
		it = m_tlsMemoryMap.find(iThreadID);
	}
	//看下slot 是否存在
	std::map<int, std::shared_ptr<TLSMemoryInfo> >::iterator slotIter = it->second.find(iSlot);
	if (slotIter == it->second.end())
	{
		it->second.insert(std::map<int, std::shared_ptr<TLSMemoryInfo> >::value_type(iSlot, std::shared_ptr<TLSMemoryInfo>()));
		slotIter = it->second.find(iSlot);
	}
	slotIter->second->bIsUse = false;
	slotIter->second->iFreeTime = (int)(tsk_time_now() / 1000);
}

int CTLSMemory::ClearProc(const void* pParam, long iTimerID)
{
	CTLSMemory* pThis = (CTLSMemory*)pParam;
	std::lock_guard<std::mutex> lock(pThis->m_mutex);
	std::map<TLS_ThreadID, std::map<int, std::shared_ptr<TLSMemoryInfo> > >::iterator begin = pThis->m_tlsMemoryMap.begin();
	std::map<TLS_ThreadID, std::map<int, std::shared_ptr<TLSMemoryInfo> > >::iterator end = pThis->m_tlsMemoryMap.end();
	int iNowTime = (int)(tsk_time_now() / 1000);
	for (; begin != end;begin++)
	{
		std::map<int, std::shared_ptr<TLSMemoryInfo> >::iterator slotBegin = begin->second.begin();
		for (; slotBegin != begin->second.end();)
		{
			if (slotBegin->second->bIsUse)
			{
				slotBegin++;
				continue;
			}
			//没有使用的判断下时间,单位是秒
			if (iNowTime - slotBegin->second->iFreeTime >= 10)
			{
				slotBegin = begin->second.erase(slotBegin);
			}
			else
			{
				slotBegin++;
			}
		}
	}
	return 0;
}

TLS_ThreadID CTLSMemory::GetThreadID()
{
#if WIN32
	return GetCurrentThreadId();
#else
	return pthread_self();
#endif
}
