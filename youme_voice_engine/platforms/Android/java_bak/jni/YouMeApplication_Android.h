//
//  Application.h
//  YouMeVoiceEngine
//
//  Created by wanglei on 15/9/17.
//  Copyright (c) 2015年 tencent. All rights reserved.
//
#include <string>
#include <YouMeCommon/CrossPlatformDefine/IYouMeSystemProvider.h>

class YouMeApplication_Android : public IYouMeSystemProvider
{
public:
	virtual XString getBrand()override
    {
        return m_strBrand;
    }
	virtual XString getSystemVersion()override
    {
        return m_strSystemVersion;
    }
	virtual XString getCpuArchive()override
    {
        return m_strCPUArchive;
    }
	virtual XString getPackageName()override
    {
        return m_strPackageName;
    }
	virtual XString getUUID()override
    {
        return m_strUUID;
    }
	virtual XString getModel()override
    {
        return m_strModel;
    }
	virtual XString getCpuChip()override
    {
        return m_strCpuChip;
    }
	virtual XString getDocumentPath()override
    {
        return m_strDocumentPath;
    }
	virtual XString getCachePath() override
	{
		return m_strDocumentPath;
	}

public:
    YouMeApplication_Android ();
    ~YouMeApplication_Android ();

    XString m_strBrand;
    XString m_strSystemVersion;
    XString m_strCPUArchive;
    XString m_strPackageName;
    XString m_strUUID;
    XString m_strModel;
    XString m_strCpuChip;
    XString m_strDocumentPath;
};
