

#include "SDLDisplay.h"
#include "SDL.h"
#include "SDL_shape.h"
#include "SDL_Thread.h"
#include "tsk_mutex.h"
#include "tsk.h"

typedef struct tag_SDL_Frame{
	unsigned char* data;
	int width;
	int height;
	int i_stride;
	tag_SDL_Frame(){
		data = nullptr;
		width = 0;
		height = 0;
		i_stride = 0;
	}
}SDL_Frame;

typedef struct SDL_Handle {
	SDL_Window  * sdlwindow;
	SDL_Renderer* sdlRenderer;
	SDL_Texture* sdlTexture;
	SDL_Rect sdlRect;
	HWND hWnd;
	tsk_mutex_handle_t * mutex;
	int rotation;
	SDL_Frame lastFrame;
	bool uninited;
}SDL_Handle;

bool 	g_bResize = false;
static int g_nRenderCount = 0;
static bool sdl_init = false;

int SDL_Render(SDL_Handle * h, const unsigned char * data, int width, int height, int i_stride);
void SDL_Create_Texture(SDL_Handle* pHandle, int w, int h)
{
	if (pHandle->sdlTexture)
	{
		SDL_DestroyTexture(pHandle->sdlTexture);
		pHandle->sdlTexture = NULL;
	}
	SDL_Texture* sdlTexture = SDL_CreateTexture(
		pHandle->sdlRenderer,
		SDL_PIXELFORMAT_IYUV,
		SDL_TEXTUREACCESS_STREAMING,
		w,
		h);

	pHandle->sdlRect.x = 0;
	pHandle->sdlRect.y = 0;
	pHandle->sdlRect.w = w;
	pHandle->sdlRect.h = h;
	pHandle->sdlTexture = sdlTexture;
	pHandle->uninited = false;
	
}

int MySdlEventFilter(void *userdata, SDL_Event * event)
{
	SDL_Handle* pHandle = (SDL_Handle*)userdata;
	if (!pHandle || !pHandle->mutex || pHandle->uninited) {
		return 1;
	}

    if (event->type == SDL_WINDOWEVENT)
    {
        switch (event->window.event)
        {
        case SDL_WINDOWEVENT_RESIZED:
            tsk_mutex_lock(pHandle->mutex);
			g_bResize = true;
            SDL_SetWindowSize(pHandle->sdlwindow, event->window.data1, event->window.data2);
			SDL_Render(pHandle, pHandle->lastFrame.data, pHandle->lastFrame.width, pHandle->lastFrame.height, pHandle->lastFrame.i_stride);
            tsk_mutex_unlock(pHandle->mutex);
            break;
		case SDL_WINDOWEVENT_SIZE_CHANGED:
			tsk_mutex_lock(pHandle->mutex);
			//g_bResize = true;
			tsk_mutex_unlock(pHandle->mutex);
			break;
        default:
            break;
        }
    }
    return 1;
}

_HSDL SDL_Display_Init(HWND hWnd, unsigned int width, unsigned int height, unsigned int fps)
{
    if (!hWnd) 
    {
    	TSK_DEBUG_ERROR("sdl invalid param");
    	return NULL;
    }
    
	SDL_Handle * pHandle = NULL;
	if (!sdl_init)
	{
		if (SDL_Init(SDL_INIT_VIDEO )) {
			TSK_DEBUG_ERROR("sdl init faild:%s", SDL_GetError());
			return NULL;
		}

		sdl_init = true;
	}
	//atexit(SDL_Quit);

	// SDL_EventState(SDL_ACTIVEEVENT, SDL_IGNORE);
	//SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);
	SDL_EventState(SDL_SYSWMEVENT,  SDL_IGNORE);
	SDL_EventState(SDL_USEREVENT,   SDL_IGNORE);
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");  //linear,best使缩放渲染看起来更平滑   

	pHandle = (SDL_Handle*)malloc(sizeof(SDL_Handle));
	if (pHandle == NULL) {
		return NULL;
	}
	memset(pHandle, 0, sizeof(SDL_Handle));

	// RECT rc = {0};
	// ::GetClientRect(hWnd, &rc);

	SDL_Window * sdlwindow = NULL;
	sdlwindow = SDL_CreateWindowFrom((const void *)hWnd);

    if( !sdlwindow ) {  
		free(pHandle);
		TSK_DEBUG_ERROR("sdl create window faild:%s", SDL_GetError());
		return NULL;
	}
	
	SDL_Renderer* sdlRenderer = SDL_CreateRenderer(sdlwindow, -1, SDL_RENDERER_SOFTWARE | SDL_RENDERER_TARGETTEXTURE);
	if (sdlRenderer == NULL)
	{
		free(pHandle);
		TSK_DEBUG_ERROR("sdl create renderer faild:%s", SDL_GetError());
		return NULL;
	}

	//SDL_Create_Texture(pHandle, rc.right - rc.left, rc.bottom - rc.top);

	SDL_ShowCursor(1);
	pHandle->sdlwindow = sdlwindow;
	pHandle->sdlRenderer = sdlRenderer;
	pHandle->hWnd = hWnd;
	pHandle->rotation = 0;

	pHandle->mutex = tsk_mutex_create();

	SDL_SetEventFilter(MySdlEventFilter, (void*)pHandle);

	++g_nRenderCount;
	TSK_DEBUG_INFO("sdl SDL_Display_Init end");
	return pHandle;
}

void SDL_Display_Clear(_HSDL pHandle){
	if (pHandle == NULL )
    {
    	TSK_DEBUG_ERROR("sdl invalid param");
        return;
    }
    
    SDL_Handle * h = (SDL_Handle*)pHandle;
    if (h->sdlRenderer == NULL)
    {
    	TSK_DEBUG_ERROR("sdl invalid param 2");
        return;
    }
    tsk_mutex_lock(h->mutex);
    SDL_RenderClear(h->sdlRenderer);
    SDL_SetRenderDrawColor(h->sdlRenderer, 0, 0, 0, 255);
    // RECT rc = { 0 };
    // ::GetClientRect(h->hWnd, &rc);

	// SDL_Window * sdlwindow = NULL;
	// sdlwindow = SDL_CreateWindowFrom((const void *)h->hWnd);

	SDL_Rect dstrect;
	SDL_GetWindowBordersSize(h->sdlwindow, &dstrect.y, &dstrect.x, &dstrect.h, &dstrect.w);

    SDL_Rect rectangle;
    rectangle.x = dstrect.x;
    rectangle.y = dstrect.y;
    rectangle.w = dstrect.w;
    rectangle.h = dstrect.h;
    SDL_RenderFillRect(h->sdlRenderer, &rectangle);
    SDL_RenderPresent(h->sdlRenderer);
    //SDL_Delay(100);
    tsk_mutex_unlock(h->mutex);
}


void SDL_Display_Uninit(_HSDL* pHandle)
{
	if (pHandle == NULL || *pHandle == NULL){
		TSK_DEBUG_ERROR("sdl invalid param");
		return;
	}
    //SDL_Display_Clear(*pHandle);
	SDL_Handle * h = (SDL_Handle*)*pHandle;
	if (h) {
		h->uninited = true;
		bool isVisible = ::IsWindowVisible(h->hWnd);
		if (h->mutex)
			tsk_mutex_lock(h->mutex);
		
		if (h->sdlTexture)
			SDL_DestroyTexture(h->sdlTexture);
		if (h->sdlRenderer)
			SDL_DestroyRenderer(h->sdlRenderer);
		if (h->sdlwindow)
			SDL_DestroyWindow(h->sdlwindow);
		if (h->lastFrame.data) {
			free(h->lastFrame.data);
			h->lastFrame.data = nullptr;
		}
		if (h->mutex)
		{
			tsk_mutex_unlock(h->mutex);
			tsk_mutex_destroy(&h->mutex);
			h->mutex = NULL;
		}

		if (isVisible)
		   ::ShowWindow(h->hWnd, SW_SHOWNORMAL);
		free(h);
		*pHandle = NULL;
	}
	
	--g_nRenderCount;
	if (!g_nRenderCount) {
		TSK_DEBUG_INFO("sdl quit");
		SDL_Quit();
		sdl_init = false;
	}
	//
}

void SDL_Display_Set_Rotation(_HSDL pHandle, int rotation) {

	if (pHandle == NULL){
		TSK_DEBUG_ERROR("sdl invalid param");
		return;
	}
	SDL_Handle * h = (SDL_Handle*)pHandle;
	tsk_mutex_lock(h->mutex);
	h->rotation = rotation;
	tsk_mutex_unlock(h->mutex);
}

int SDL_Render(SDL_Handle * h, const unsigned char * data, int width, int height, int i_stride){
	int ret = -1;
	RECT rcWin;
	int dst_width_ = 0, dst_height_ = 0;

	//::GetClientRect(h->hWnd, &rcWin);
	// dst_width_ = rcWin.right - rcWin.left;
	// dst_height_ = rcWin.bottom - rcWin.top;

	if (!h || true == h->uninited || !data) {
		return ret;
	}

	if (width != h->sdlRect.w || height != h->sdlRect.h)
		SDL_Create_Texture(h, width, height);
	if (!h->sdlTexture){
		return ret;
	}

	// SDL_Window * sdlwindow = NULL;
	// sdlwindow = SDL_CreateWindowFrom((const void *)h->hWnd);
	SDL_GetWindowSize(h->sdlwindow, &dst_width_, &dst_height_);

	SDL_Rect dstrect = { 0, 0, 0, 0 };
	// float scale = 1.0;
	if (dst_width_ * height >= width * dst_height_)
	{
		// scale = dst_height_ / height;
		dstrect.w = width * dst_height_ / (float)height;
		dstrect.h = dst_height_;
	}
	else
	{
		// scale = dst_width_ / width;
		dstrect.w = dst_width_;
		dstrect.h = height * dst_width_ / (float)width;
	}

	if (90 == h->rotation || 270 == h->rotation) {
		dstrect.h = dst_width_;
		dstrect.w = width * dst_width_ / (float)height;
	}

	dstrect.x = (dst_width_ - dstrect.w) / 2;
	dstrect.y = (dst_height_ - dstrect.h) / 2;

	SDL_Rect rcSrc = { 0, 0, width, height };
	//SDL_Rect rcDest = { 0, 0, dst_width_, dst_height_ };
	//SDL_Rect rcDest = { (dst_width_ - cropped_src_width) / 2, (dst_height_ - cropped_src_height) / 2, cropped_src_width, cropped_src_height };
	ret = SDL_UpdateTexture(h->sdlTexture, &rcSrc, data, i_stride);
	ret = SDL_RenderClear(h->sdlRenderer);
	// ret = SDL_RenderSetScale(h->sdlRenderer, scale, scale);
	// ret = SDL_RenderSetLogicalSize(h->sdlRenderer, width, height);
	// ret = SDL_RenderCopy(h->sdlRenderer, h->sdlTexture, NULL, NULL);
	SDL_RenderCopyEx(h->sdlRenderer, h->sdlTexture, NULL, &dstrect, h->rotation, NULL, SDL_FLIP_NONE);
	SDL_RenderPresent(h->sdlRenderer);
	return ret;
}

void SDL_Display(_HSDL pHandle, const unsigned char * data, int width, int height, int i_stride)
{
	if (pHandle == NULL || !data || !(width*height)) {
		return;
	}

	try
	{
		int ret = 0;
		SDL_Handle * h = (SDL_Handle*)pHandle;
		if (!h || !h->mutex || h->uninited) {
			return;
		}

		tsk_mutex_lock(h->mutex);
		if (g_bResize)
		{
			g_bResize = false;
			tsk_mutex_unlock(h->mutex);
			return;
		}
		if (h->uninited) {
			tsk_mutex_unlock(h->mutex);
			return;
		}
		SDL_Render(h, data, width, height, i_stride);

		if (!h->lastFrame.data || h->lastFrame.width != width || h->lastFrame.height != height){
			if (h->lastFrame.data)
				free(h->lastFrame.data);
			h->lastFrame.data = (unsigned char *)malloc(width*height * 3 / 2);
			//h->lastFrame.data = (unsigned char *)realloc(h->lastFrame.data, width*height * 3 / 2);
			h->lastFrame.width = width;
			h->lastFrame.height = height;
			h->lastFrame.i_stride = i_stride;
		}
		if (h->lastFrame.data)
		   memcpy(h->lastFrame.data, data, width*height * 3 / 2);

		tsk_mutex_unlock(h->mutex);
	}
	catch (...)
	{

	}

}

