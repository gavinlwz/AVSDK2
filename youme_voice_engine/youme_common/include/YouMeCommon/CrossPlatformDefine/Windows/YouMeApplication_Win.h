//
//  Application.h
//  YouMeVoiceEngine
//
//  Created by wanglei on 15/9/17.
//  Copyright (c) 2015年 tencent. All rights reserved.
//
#include <string>
#include <YouMeCommon/CrossPlatformDefine/IYouMeSystemProvider.h>

class YouMeApplication_Win : public IYouMeSystemProvider
{
public:
	virtual XString getBrand()override;
	virtual XString getSystemVersion()override;
	virtual XString getCpuArchive()override;
	virtual XString getPackageName()override;
	virtual XString getUUID()override;
	virtual XString getModel()override;
	virtual XString getCpuChip()override;
	virtual XString getDocumentPath()override;
	virtual XString getCachePath() override;

public:
    YouMeApplication_Win ();
    ~YouMeApplication_Win ();
};
