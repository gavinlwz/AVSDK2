#include "SyncTCPTalk.h"
#include <assert.h>
#include <time.h>
#include <fcntl.h>
#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WinSock2.h>
#include <Winerror.h> 
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <netinet/tcp.h>
#endif
//#include <YouMeCommon/Log.h>
#ifndef WIN32
#include <errno.h>

#endif

#ifndef WIN32
#include <errno.h>
#include <netdb.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <fcntl.h>
#else
#include <iphlpapi.h>
#include <ws2tcpip.h>
#pragma comment(lib, "iphlpapi.lib")
#endif

#include "tsk_debug.h"
#include "YouMeCommon/XNetworkUtil.h"

#define PACKAGE_HEAD_LEN    (4)  // 数据包头部长度
#define MAX_RECV_LEN        (5 * 1024 * 1024) // 最大接收
#define BLOCK_TIME_OUT_MS   (400) // Timeout(in ms) for blocking operations

namespace youmertc {
    
CSyncTCP::CSyncTCP()
{
    m_client = -1;
    m_bAborted = false;
}

CSyncTCP::~CSyncTCP()
{
	Close();
}

void CSyncTCP::Close()
{
	if (-1 != m_client)
	{
#ifdef WIN32
        shutdown(m_client, SD_BOTH);
		closesocket(m_client);
#else
        TSK_DEBUG_INFO ("shutdown");
        shutdown(m_client, SHUT_RDWR);
        TSK_DEBUG_INFO ("close");
		close(m_client);
        TSK_DEBUG_INFO ("closed");
#endif
		m_client = -1;
	}
	
}

void CSyncTCP::Reset()
{
    m_bAborted = false;
    }
    
void CSyncTCP::Abort()
    {
    m_bAborted = true;
}

bool CSyncTCP::Init(const std::string&strServerIP, int iPort,int iRecvTimeout)
{
	if (-1 != m_client)
	{
		return true;
	}
    
    m_strServerIP = strServerIP;
    m_iServerPort = iPort;
    if (iRecvTimeout > 0) {
        m_iTimeoutMs = iRecvTimeout * 1000;
    } else {
        m_iTimeoutMs = 0x7fffffff;
    }
    
    return true;
}

void CSyncTCP::ResetReciveTimeoutOption()
{
    struct timeval recvTimeOut = { 0, 0 };// 第二个参数是微妙位单位
    setsockopt(m_client, SOL_SOCKET, SO_RCVTIMEO, (const char*)&recvTimeOut, sizeof(recvTimeOut));
}
    
bool CSyncTCP::Connect(int timeout)
	{
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;//youmecommon::XNetworkUtil::DetectIsIPv6() ? AF_UNSPEC : AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    struct addrinfo *result = NULL;
    std::string strIP = m_strServerIP;
    int iRet = getaddrinfo(strIP.c_str(), NULL, &hints, &result);
    if (iRet != 0)
    {
        // YouMe_LOG_Info(__XT("getaddrinfo failed"));
		return false;
	}

    int errorCode = 0;
    struct addrinfo* pAddrInfo = NULL;

    for (pAddrInfo = result; pAddrInfo != NULL; pAddrInfo = pAddrInfo->ai_next)
    {
        if (AF_INET == pAddrInfo->ai_family)
        {
            // YouMe_LOG_Info(__XT("IPV4"));

            m_client = socket(pAddrInfo->ai_family, pAddrInfo->ai_socktype, pAddrInfo->ai_protocol);
            SetSocketOpt();
            SetBlock(false);
            struct sockaddr_in addr_in = *((struct sockaddr_in *) pAddrInfo->ai_addr);
            addr_in.sin_port = htons(m_iServerPort);
            errorCode = connect(m_client, (struct sockaddr *)&addr_in, pAddrInfo->ai_addrlen);
        }
        else if (AF_INET6 == pAddrInfo->ai_family)
        {
            TSK_DEBUG_INFO ("IPV6");
    
            m_client = socket(pAddrInfo->ai_family, pAddrInfo->ai_socktype, pAddrInfo->ai_protocol);
            SetSocketOpt();
            SetBlock(false);
            struct sockaddr_in6 addr_in6 = *((struct sockaddr_in6 *)pAddrInfo->ai_addr);
            addr_in6.sin6_port = htons(m_iServerPort);
            errorCode = connect(m_client, (struct sockaddr *)&addr_in6, pAddrInfo->ai_addrlen);
        }
	
#ifdef WIN32
        if (errorCode == WSAEWOULDBLOCK || errorCode == -1)
#else
        if (errorCode == -1)
#endif
        {
            fd_set set;
            FD_ZERO(&set);
            int blockTimeoutMs = BLOCK_TIME_OUT_MS;
            int totalTimeoutMs = timeout * 1000;
            if (totalTimeoutMs < blockTimeoutMs) {
                totalTimeoutMs = blockTimeoutMs;
            }
            int retryCnt = totalTimeoutMs / blockTimeoutMs;
            int status = -1;
            for (int i = 0; i < retryCnt; i++) {
                FD_ZERO(&set);
                FD_SET(m_client, &set);
                struct timeval  stimeout;
                stimeout.tv_sec = blockTimeoutMs / 1000;
                stimeout.tv_usec = (blockTimeoutMs % 1000) * 1000;
                status = select(m_client + 1, NULL, &set, NULL, &stimeout);
                // status != 0 means NOT timeout
               if ((0 != status) || (m_bAborted)) {
                    break;
                }
            }
  
            if (status == -1 || status == 0)
            {
                errorCode = -1;
            }
            else
            {
                if (FD_ISSET(this->m_client, &set)) {
                    socklen_t peerAdressLen;
                    struct  sockaddr peerAdress;
                    peerAdressLen = sizeof (peerAdress);
            
                    if (getpeername (this->m_client, &peerAdress, &peerAdressLen) == 0) { // real connect success
                        // int flags = fcntl(m_client, F_GETFL, 0);
                        // fcntl(m_client, F_SETFL, flags & ~O_NONBLOCK);
                        SetBlock(true);
                        errorCode = 0;
                    } else {
                        errorCode = -1;
                    }
            }
        }
    }
    
        break;
    }

    freeaddrinfo(result);
    return errorCode == 0;
}


int CSyncTCP::SendData(const char* buffer, int iBufferSize)
{
    if (-1 == this->m_client)
    {
        return -1;
    }
	//先发送数据的长度,转换成网络字节序
	int iNetOrder = htonl(iBufferSize);
    
	if (sizeof(int) != SendBufferData((const char*)&iNetOrder, sizeof(int)))
	{
		return -1;
	}
    
	return SendBufferData(buffer, iBufferSize);
}


void CSyncTCP::UnInit()
{
	Close();
}

int CSyncTCP::SendBufferData(const char*buffer, int iBufferSize)
{
	int iSend = 0;
    int retryCnt = m_iTimeoutMs / BLOCK_TIME_OUT_MS;

    for (int i = 0; i < retryCnt; i++) {
		int iTmpSend = send(m_client, buffer + iSend, iBufferSize - iSend, 0);
        
        if (m_bAborted) {
			break;
		}
        
		if (iTmpSend < 0) {
#ifdef WIN32
            //WSAETIMEDOUT
            int iErrorcode = WSAGetLastError();
            if (iErrorcode != WSAEWOULDBLOCK && iErrorcode != WSAETIMEDOUT) {
                break;
            }
#else
            int iErrorcode = errno;
            if (iErrorcode != EAGAIN) {
				break;
			}
#endif
		}
        
		iSend += iTmpSend;
        if (iSend >= iBufferSize) {
            break;
        }
	}
    
    return iSend;
}

int CSyncTCP::GetRecvDataLen()
{
	//4字节长度
    youmecommon::CXSharedArray<char> recvBuffer;
    
	if (PACKAGE_HEAD_LEN != RecvDataByLen(PACKAGE_HEAD_LEN, recvBuffer)) {
		return -1;
	}
    
	int iDataLen = *(int*)recvBuffer.Get();
	return ntohl(iDataLen);
}


int CSyncTCP::RecvDataByLen(int iLen, youmecommon::CXSharedArray<char>& recvBuffer)
{
	int iRecv = 0;
    int retryCnt = 50;//200; //m_iTimeoutMs / BLOCK_TIME_OUT_MS;
	recvBuffer.Allocate(iLen);
    
    for (int i = 0; i < retryCnt; i++) {
		int iCurRecv = (int)recv(m_client, recvBuffer.Get() + iRecv, iLen - iRecv, 0);
        
        // The server has closed the socket, or the user has aborted the current operation.
        if ((0 == iCurRecv) || (m_bAborted)) {
            TSK_DEBUG_ERROR("####tcp disconnect:%d", iCurRecv);
            break;
        }
        
        if (iCurRecv < 0) {
#ifdef WIN32
            int iErrorcode = WSAGetLastError();
            if (iErrorcode != WSAEWOULDBLOCK && iErrorcode != WSAETIMEDOUT) {
                break;
            }
#else
            int iErrorcode = errno;
            if (iErrorcode != EAGAIN) {
                break;
            }
#endif
            continue;
		}

		iRecv += iCurRecv;
        
		if (iRecv >= iLen) {
			break;
		}
	}
    
	return iRecv;
}


int CSyncTCP::RecvData(youmecommon::CXSharedArray<char>& recvBuffer)
{
    if (-1 == this->m_client) {
        return -1;
    }
    
	// 先接收4字节的长度
	int iDataLen = GetRecvDataLen();
    
	if (-1 == iDataLen || iDataLen > MAX_RECV_LEN) {
        TSK_DEBUG_ERROR("####tcp len err:%d", iDataLen);
		return -1;
	}
    
	return RecvDataByLen(iDataLen, recvBuffer);
}


/**
 * 检测是否有数据可读，参数为设置超时时间
 * 参数：
 *  sec      - 秒
 *  usec     - 毫秒
 * 返回值：
 *  -1 错误 0 无数据 1 可读
 */
int CSyncTCP::CheckRecv(uint32_t sec, uint32_t usec)
{
    if (-1 == this->m_client) {
        return -1;
    }
    
    fd_set read_set;
    struct timeval  tv;
    tv.tv_sec = sec;
    tv.tv_usec = usec;
    
    FD_ZERO(&read_set);
    FD_SET(this->m_client, &read_set);
    
    /*
     * http://linux.die.net/man/2/select
     * https://msdn.microsoft.com/en-us/library/windows/desktop/ms740141
     * Return Value
     * On success, select() and pselect() return the number of file descriptors
     * contained in the three returned descriptor sets
     * (that is, the total number of bits that are set in readfds, writefds, exceptfds)
     * which may be zero if the timeout expires before anything interesting happens.
     * On error, -1 is returned, and errno is set appropriately;
     * the sets and timeout become undefined, so do not rely on their contents after an error.
     *
     */
    int ret = select(this->m_client + 1, &read_set, NULL, NULL, &tv); // 类unix 系统中，第一个参数必须是client + 1
    
    if (-1 == ret) { // error
        return -1;
    }
    
    if (0 == ret) { // timeout
        return 0;
    }
    
    if (-1 == this->m_client) { // already exit outside
        return -1;
    }
    
    if (FD_ISSET(this->m_client, &read_set) ) {
        return 1;
    }
    
    return 0;
}

/**
 * 判断ipv4还是ipv6
 */
int CSyncTCP::GetIPType()
{
#ifdef WIN32
    FIXED_INFO* pFixedInfo = (FIXED_INFO*)malloc(sizeof(FIXED_INFO));
    if (pFixedInfo == NULL)
    {
        return AF_INET;
    }
    ULONG ulOutBufLen = sizeof(FIXED_INFO);
    if (GetNetworkParams(pFixedInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW)
    {
        free(pFixedInfo);
        pFixedInfo = (FIXED_INFO *)malloc(ulOutBufLen);
        if (pFixedInfo == NULL)
        {
            return AF_INET;
        }
    }
    std::string strDNSAddr;
    if (GetNetworkParams(pFixedInfo, &ulOutBufLen) == NO_ERROR)
    {
        strDNSAddr = pFixedInfo->DnsServerList.IpAddress.String;
    }
    if (pFixedInfo)
    {
        free(pFixedInfo);
    }
    
    struct hostent* pHostent = gethostbyname(strDNSAddr.c_str());
    if (pHostent != NULL)
    {
        return pHostent->h_addrtype;
}
    return AF_INET;
#else
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    struct addrinfo *result = NULL;
    int iRet = getaddrinfo("localhost", NULL, &hints, &result);
    if (iRet != 0)
    {
        if (nullptr != result) {
            freeaddrinfo(result);
        }
        
        return AF_INET;
    }
    int iFamily = result->ai_family;
    freeaddrinfo(result);
    return iFamily;
#endif // WIN32
}


/***
 * 设置套接字选项
 */
void CSyncTCP::SetSocketOpt()
{
    int r = 0;
#if (OS_IOS || OS_IOSSIMULATOR || OS_OSX)
    setsockopt(m_client, SOL_SOCKET, SO_NOSIGPIPE, &r, sizeof(int));
#elif OS_ANDROID
    setsockopt(m_client, SOL_SOCKET, MSG_NOSIGNAL, &r, sizeof(int));
#endif
    
    //if (m_iSendTimeout > 0)
    { //设置超时
#ifdef WIN32
        DWORD sendTimeOut = BLOCK_TIME_OUT_MS;
        DWORD recvTimeOut = BLOCK_TIME_OUT_MS;
        setsockopt(m_client, SOL_SOCKET, SO_SNDTIMEO, (const char*)&sendTimeOut, sizeof(sendTimeOut));
        setsockopt(m_client, SOL_SOCKET, SO_RCVTIMEO, (const char*)&recvTimeOut, sizeof(recvTimeOut));
#else
        struct timeval sendTimeOut;
        sendTimeOut.tv_sec = 0;
        sendTimeOut.tv_usec = BLOCK_TIME_OUT_MS * 1000;
        struct timeval recvTimeOut = sendTimeOut;
        int buffLen = 0x20000;
        setsockopt(m_client, SOL_SOCKET, SO_SNDTIMEO, (const char*)&sendTimeOut, sizeof(sendTimeOut));
        setsockopt(m_client, SOL_SOCKET, SO_RCVTIMEO, (const char*)&recvTimeOut, sizeof(recvTimeOut));
        setsockopt(m_client, SOL_SOCKET, SO_RCVBUF, (char *)&buffLen, sizeof (buffLen));
        setsockopt(m_client, SOL_SOCKET, SO_SNDBUF, (char *)&buffLen, sizeof (buffLen));
        int tcpNoDelay = 1;
        setsockopt(m_client, IPPROTO_TCP, TCP_NODELAY, (void *)&tcpNoDelay, sizeof(tcpNoDelay));
#endif // WIN32
    }
}


/**
 * 设置阻塞与非阻塞
 */
void CSyncTCP::SetBlock(bool block)
{
#ifdef WIN32
    unsigned long nMode = block ? 0 : 1;
    int nRet = ioctlsocket(m_client, FIONBIO, &nMode);
    if (nRet != NO_ERROR)
    {
        // YouMe_LOG_Error(__XT("ioctlsocket FIONBIO failed"));
    }
#else
    int flags = fcntl(m_client, F_GETFL, 0);
    if (block)
    {
        if (fcntl(m_client, F_SETFL, flags & ~O_NONBLOCK) == -1)
        {
            // YouMe_LOG_Error(__XT("fcntl FIONBIO failed"));
            int mode = 0;
            if(ioctl(m_client, FIONBIO, &mode) == -1)
            {
                // YouMe_LOG_Error(__XT("ioctl FIONBIO failed"));
                return;
            }
        }
    }
    else
    {
        if (fcntl(m_client, F_SETFL, flags | O_NONBLOCK) == -1)
        {
            // YouMe_LOG_Error(__XT("fcntl FIONBIO failed"));
            int mode = 1;
            if (ioctl(m_client, FIONBIO, &mode) == -1)
            {
                // YouMe_LOG_Error(__XT("ioctl FIONBIO failed"));
                return;
            }
        }
    }
#endif
}
    
} //namespace youmertc

