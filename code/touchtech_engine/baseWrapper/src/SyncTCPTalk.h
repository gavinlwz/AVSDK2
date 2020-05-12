#pragma once
#include "YouMeCommon/XSharedArray.h"

#include <string>
#include <mutex>

//同步TCP 连接，使用的时候需要避免TCP 长链接,不要多线程调用，否则后果很严重

namespace youmertc {
class CSyncTCP
{
public:
	CSyncTCP();
	~CSyncTCP();
	//创建socket，只有真正发送数据的时候才会去connect ，单位毫秒
    bool Init(const std::string &strServerIP, int iPort, int iRecvTimeout=-1);
	void UnInit();
    void ResetReciveTimeoutOption();
    void Reset();
    void Abort();

	//直接就发送了，同步的，-1 发生错误,
    bool Connect(int iTimeOut);//单位毫秒
	int SendData(const char* buffer, int iBufferSize);
    int RecvData(youmecommon::CXSharedArray<char>& recvBuffer);
    //返回实际发送的个数，如果成功
    int SendBufferData(const char*buffer, int iBufferSize);
    int RecvDataByLen(int iLen, youmecommon::CXSharedArray<char>& recvBuffer);
	// int RawSocket();
    int CheckRecv(uint32_t sec, uint32_t usec); /* 检测是否有数据可读, sec 秒， usec 毫米 */
    
private:
	int GetRecvDataLen();
    int  GetIPType();
    void SetSocketOpt();
    void SetBlock(bool block);
    
	void Close();
	int m_client;
    bool m_bAborted = false;
    
	std::string m_strServerIP;
	int  m_iServerPort;
    int  m_iTimeoutMs;
};
}
