#include "VideoRenderMgr.h"
#include "tsk.h"
#include "NgnTalkManager.hpp"

VideoRenderMgr* VideoRenderMgr::instance = NULL;


videoRenderInfo::videoRenderInfo() :sdl(NULL), size(0), buffer(NULL), update(false){

}
videoRenderInfo::~videoRenderInfo(){
	if (buffer)
		free(buffer);
	if (sdl)
		SDL_Display_Uninit(sdl);
}
void videoRenderInfo::inputFrame(void * data, int len, int width, int height, int fmt, uint64_t timestamp){
	if (this->size != len){
		this->buffer = (uint8_t*)_recalloc(this->buffer, 1, len);
		this->size = len;
	}
	memcpy(this->buffer, data, len);
	this->width = width;
	this->height = height;
	this->fmt = fmt;
	this->timestamp = timestamp;
	this->update = true;
}


VideoRenderMgr::VideoRenderMgr() : m_localPreview(NULL), m_init(false)
{

}


VideoRenderMgr::~VideoRenderMgr()
{
	TSK_DEBUG_INFO("@@ VideoRenderMgr exit");
}

bool VideoRenderMgr::initRender(){

	// std::lock_guard<std::mutex> l(m_mutex);
	if (!m_init){
		if (!m_videoRenderWnd.create()){
			TSK_DEBUG_INFO(" create window faild");
			return false;
		}
		m_init = true;
	}

	return true;
}

void VideoRenderMgr::destroyRender(){
	if (!m_init)
		return;
	m_init = false;
	onDeleteLocalRender();
	onDeleteAllRender();
	// std::lock_guard<std::mutex> l(m_mutex);
	m_videoRenderWnd.destroy();
	
}

bool VideoRenderMgr::createLocalRender(HWND  hWnd){
	TSK_DEBUG_INFO("@@ createLocalRender hWnd:%x", hWnd);
	// std::lock_guard<std::mutex> l(m_mutex);
	if (!m_init)
		return false;
   // return m_videoRenderWnd.createLocalRender(hWnd) == 0;
	return onCreateLocalRender(hWnd);
	
	/*
	if (!initRender() || m_localPreview)
		return false;
	_HSDL sdl = SDL_Display_Init(hWnd, 0, 0, 0);
	if (sdl){
		m_localPreview = new videoRenderInfo();
		m_localPreview->sdl = sdl;
		return true;
	}
	
	return true;
	*/

}

void VideoRenderMgr::deleteLocalRender(){
	TSK_DEBUG_INFO("@@ deleteLocalRender");
	// std::lock_guard<std::mutex> l(m_mutex);
	//m_videoRenderWnd.deleteLocalRender();
	onDeleteLocalRender();
}

bool VideoRenderMgr::createRender(std::string& userId, HWND  hWnd){
	TSK_DEBUG_INFO("@@ createRender:%s,  hWnd:%x", userId.c_str(), hWnd);
	// std::lock_guard<std::mutex> l(m_mutex);
	if (!m_init)
		return false;
	// return m_videoRenderWnd.createRender(userId.c_str(), hWnd) == 0;
	return onCreateRender(userId.c_str(), hWnd);
}

void VideoRenderMgr::deleteRender(std::string& userId){
	TSK_DEBUG_INFO("@@ deleteRender userId:%s", userId.c_str());
	// std::lock_guard<std::mutex> l(m_mutex);
	// m_videoRenderWnd.deleteRender(userId.c_str());
	onDeleteRender(userId.c_str());
}

void VideoRenderMgr::deleteAllRender(){
	TSK_DEBUG_INFO("@@ deleteAllRender");
	// std::lock_guard<std::mutex> l(m_mutex);
	// m_videoRenderWnd.deleteAllRender();
	onDeleteAllRender();
}

void VideoRenderMgr::onVideoFrameCallback(const char* userId, void * data, int len, int width, int height, int fmt, uint64_t timestamp){
	
	if (!m_init)
		return;
	// std::lock_guard<std::mutex> l(m_mutex);
	std::string strUserId = userId;
	map_render::iterator iter = m_mapRenders.find(strUserId);
	if (iter != m_mapRenders.end()){
		iter->second->inputFrame(data, len, width, height, fmt, timestamp);
	}
	//m_videoRenderWnd.update();
	onVideoUpdate();
}

void VideoRenderMgr::onVideoFrameMixedCallback(void * data, int len, int width, int height, int fmt, uint64_t timestamp){
	if (!m_init)
		return;
	// std::lock_guard<std::mutex> l(m_mutex);
	if (m_localPreview)
		m_localPreview->inputFrame(data, len, width, height, fmt, timestamp);
	map_render::iterator iter = m_mapRenders.find(CNgnTalkManager::getInstance()->m_strUserID);
	if (iter != m_mapRenders.end()){
		iter->second->inputFrame(data, len, width, height, fmt, timestamp);
	}
	// m_videoRenderWnd.update();
	onVideoUpdate();
}

void VideoRenderMgr::onVideoUpdate(){
	// std::lock_guard<std::mutex> l(m_mutex);
	if (!m_init)
		return;
	if (m_localPreview && m_localPreview->isUpdate()){
		SDL_Display(m_localPreview->sdl, m_localPreview->buffer, m_localPreview->width, m_localPreview->height, m_localPreview->width);
		m_localPreview->doDone();
	}
	map_render::iterator iter = m_mapRenders.begin();
	for (; iter != m_mapRenders.end(); ++iter){
		if (iter->second->isUpdate()){
			SDL_Display(iter->second->sdl, iter->second->buffer, iter->second->width, iter->second->height, iter->second->width);
			iter->second->doDone();
		}
	}

}

bool VideoRenderMgr::onCreateLocalRender(HWND  hWnd){
	//std::lock_guard<std::mutex> l(m_mutex);
	if ( m_localPreview)
		return false;

	_HSDL sdl = SDL_Display_Init(hWnd, 0, 0, 0);
	if (sdl){
		m_localPreview = std::make_shared<videoRenderInfo>();
		m_localPreview->sdl = sdl;
		return true;
	}
	return false;
}
void VideoRenderMgr::onDeleteLocalRender(){
	//std::lock_guard<std::mutex> l(m_mutex);
	if (m_localPreview){
		m_localPreview = NULL;
	}
}
bool VideoRenderMgr::onCreateRender(const char* userId, HWND  hWnd){
	//std::lock_guard<std::mutex> l(m_mutex);
	TSK_DEBUG_INFO("@@ onCreateRender:%s,  hWnd:%x", userId, hWnd);
	_HSDL sdl = SDL_Display_Init(hWnd, 0, 0, 0);
	if (sdl == NULL){
		TSK_DEBUG_INFO("@@ SDL_Display_Init failed");
		return false;
	}
	TSK_DEBUG_INFO("@@ SDL_Display_Init success");

	std::string strUserId = userId;
	auto info = std::make_shared<videoRenderInfo>();
	info->sdl = sdl;
	m_mapRenders.insert(std::make_pair(strUserId, info));
	return true;
}
void VideoRenderMgr::onDeleteRender(const char* userId){
	//std::lock_guard<std::mutex> l(m_mutex);
	std::string strUserId = userId;
	map_render::iterator iter = m_mapRenders.find(strUserId);
	if (iter != m_mapRenders.end()){
		m_mapRenders.erase(iter);
	}
}
void VideoRenderMgr::onDeleteAllRender(){
	//std::lock_guard<std::mutex> l(m_mutex);
	m_mapRenders.clear();
}