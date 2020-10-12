// YouMeDllTest.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <iostream>
using namespace std;

#include "fun_define.h"

std::string g_strKey = "YOUME5BE427937AF216E88E0F84C0EF148BD29B691556";
std::string g_strSecret = "y1sepDnrmgatu/G8rx1nIKglCclvuA5tAvC0vXwlfZKOvPZfaUYOTkfAdUUtbziW8Z4HrsgpJtmV/RqhacllbXD3abvuXIBlrknqP+Bith9OHazsC1X96b3Inii6J7Und0/KaGf3xEzWx/t1E1SbdrbmBJ01D1mwn50O/9V0820BAAE=";


int _tmain(int argc, _TCHAR* argv[])
{
    WSADATA wsaData;
    int nRet;
    if ((nRet = WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0){
        cout << "over" << endl;
    }
    else
    {
        if (InitFunction())
        {
            youme_init(g_strKey.c_str(), g_strSecret.c_str());
            for (int i = 0; i < 10000; ++i)
            {
                Sleep(1000);
            }
        }
		else
		{
			cout << "dll初始化失败!!!" << endl;
		}
    }
    
	return 0;
}

