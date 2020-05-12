//
//  main.cpp
//  SDKValidTest
//
//  Created by joexie on 16/2/15.
//  Copyright © 2016年 YouMe. All rights reserved.
//

#include <iostream>
#include "NgnLoginService.hpp"
int main(int argc, const char * argv[]) {
    // insert code here...
   /* CSDKValidate sdk;
    sdk.Init();
    sdk.SetAppKey("YOUMEF76F47174516F9C42C12847FCF18CF988BA4559D");
    sdk.SetAppSecret("5rcB7mR0wkuV2o0wt+7wa0JXRQ5sZ+LbieOE4g9vueTToHfVnzvgoRNL8hilZ6IkvI9vJT8IOLLNmx5L9IRh7uRbYKutxv2Y9eaxp3dffB67wkme6TC+8p4mCeaZZGTXiLa3Izxnu5iusssYWAGixGu5S7KufazyBoKZDdYqMyUBAAE=");
    
    bool bSuccess = sdk.ServerLoginIn();
    if (bSuccess) {
        printf("登陆成功");
    }
    else
    {
        printf("登陆失败");
    }*/
    
    NgnLoginService login;
    int ret = login.LoginServerAsync("120.0.0.1", 5506, "rommid");
    if (ret == 0) {
        printf("登陆成功");
    }
    else
    {
        printf("登陆失败");
    }
    return 0;
}
