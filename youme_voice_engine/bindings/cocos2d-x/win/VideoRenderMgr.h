#pragma once
#include"VideoRenderWnd.h"
#include <string>
#include<map>
#include <mutex>
#include"SDLDisplay.h"
#include"VideoRenderWnd.h"
#include "IYouMeVideoCallback.h"
#include <memory>

class videoRenderInfo{
public:
	videoRenderInfo();
	~videoRenderInfo();
	void inputFrame(void * data, int len, int width, int height, int fmt, uint64_t timestamp);
	bool isUpdate(){ return update; }
	void doDone(){ update = false; }
public:
	_HSDL  sdl;
	uint8_t* buffer;
	int size;
	int width;
	int height;
	int fmt;
	uint64_t timestamp;
	bool update;
};

class VideoRenderMgr : public IYouMeVideoFrameCallback
{
public:
	VideoRenderMgr();
	~VideoRenderMgr();
	static VideoRenderMgr* instance;
	static VideoRenderMgr * getInstance(){
		if (!instance)
			instance = new VideoRenderMgr();
		return instance;
	}
	static void destroy(){
		if (instance){
			instance->destroyRender();
			delete instance;
			instance = NULL;
		}
	}

	bool initRender();
	void destroyRender();
public:
	bool createLocalRender(HWND  hWnd);

	void deleteLocalRender();

	bool createRender(std::string& userId, HWND  hWnd);

	void deleteRender(std::string& userId);

	void deleteAllRender();
	
	void setRenderRotation(std::string& userId, int rotation);

    void onVideoFrameCallback(const char * userId, void * data, int len, int width, int height, int fmt, uint64_t timestamp) override;
	void onVideoFrameMixedCallback(void * data, int len, int width, int height, int fmt, uint64_t timestamp) override;

	void onVideoUpdate();
	bool onCreateLocalRender(HWND  hWnd);
	void onDeleteLocalRender();
	bool onCreateRender(const char* userId, HWND  hWnd);
	void onDeleteRender(const char* userId);
	void onDeleteAllRender();
	void onSetRenderRotation(const char* userId, int rotation);

private:
	bool m_init;
	typedef std::map<std::string, std::shared_ptr<videoRenderInfo>> map_render;
	map_render m_mapRenders;

	VideoRenderWnd m_videoRenderWnd;

	std::shared_ptr<videoRenderInfo> m_localPreview;

	std::mutex  m_mutex;
	std::mutex  m_mutex_data;
};

