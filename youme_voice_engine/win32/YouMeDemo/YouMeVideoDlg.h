#pragma once
#include "afxwin.h"
#include <map>
#include <vector>
#include "..\..\bindings\cocos2d-x\win\SDLDisplay.h"

#include "IYouMeVoiceEngine.h"
// YouMeVideoDlg 对话框

class YouMeVideoDlg : public CDialogEx, public IYouMeVideoCallback
{
	DECLARE_DYNAMIC(YouMeVideoDlg)

public:
	YouMeVideoDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~YouMeVideoDlg();
	virtual BOOL OnInitDialog();
// 对话框数据
	enum { IDD = IDD_YOUME_VIDEO };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()

public:
	virtual void frameRender(int renderId, int nWidth, int nHeight, int nRotationDegree, int nBufSize, const void * buf) override;


	void addRender(int renderId);
	void removeRender(int renderId);
	int removeRender(std::string userid);

	void joinRoomSuc(bool state);

	void clearRender();
public:
	CStatic m_videoView1;
	CStatic m_videoView2;
	CStatic m_videoView3;
	CStatic m_videoView4;
	afx_msg void OnBnClickedButtonCamOpen();
	afx_msg void OnBnClickedButtonCamClose();

private:
	struct sdlinfo
	{
		_HSDL  hsdl;
		 long  winid;
		 std::string userid;
	};
	typedef std::map<int, sdlinfo> MapSdl;
	MapSdl m_mapRender;
	std::vector<long> m_vecWinId;

	bool m_openCamera;
	bool m_isJoinRoom;
};
