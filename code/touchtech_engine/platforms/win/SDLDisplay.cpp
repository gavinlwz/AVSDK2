

#include "SDLDisplay.h"
#include "SDL.h"
#include "SDL_shape.h"
#include "SDL_Thread.h"
#include "tsk_mutex.h"
#include "tsk.h"

typedef struct SDL_Handle {
	SDL_Window  * sdlwindow;
	SDL_Renderer* sdlRenderer;
	SDL_Texture* sdlTexture;
	SDL_Rect sdlRect;
	HWND hWnd;
	tsk_mutex_handle_t * mutex;
}SDL_Handle;


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
	
}

int MySdlEventFilter(void *userdata, SDL_Event * event)
{
	SDL_Handle* pHandle = (SDL_Handle*)userdata;

    if (event->type == SDL_WINDOWEVENT)
    {
        switch (event->window.event)
        {
        case SDL_WINDOWEVENT_RESIZED:
            tsk_mutex_lock(pHandle->mutex);
            SDL_SetWindowSize(pHandle->sdlwindow, event->window.data1, event->window.data2);
            tsk_mutex_unlock(pHandle->mutex);
            // return 0;
            break;
        case SDL_WINDOWEVENT_SIZE_CHANGED:
            tsk_mutex_lock(pHandle->mutex);
            // g_bResize = true;                    
            tsk_mutex_unlock(pHandle->mutex);
            // return 0;
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
	static bool sdl_init = false;
	if (!sdl_init)
	{
		if (SDL_Init(SDL_INIT_VIDEO )) {
			TSK_DEBUG_ERROR("sdl init faild");
			return NULL;
		}
		// if (SDL_VideoInit(NULL) == -1) {
		// 	TSK_DEBUG_ERROR("sdl video init faild");
		// 	return NULL;
		// }
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
		TSK_DEBUG_ERROR("sdl create window faild");
		return NULL;
	}
	
	SDL_Renderer* sdlRenderer = SDL_CreateRenderer(sdlwindow, -1, SDL_RENDERER_SOFTWARE | SDL_RENDERER_TARGETTEXTURE);
	if (sdlRenderer == NULL)
	{
		free(pHandle);
		TSK_DEBUG_ERROR("sdl create renderer faild");
		return NULL;
	}

	//SDL_Create_Texture(pHandle, rc.right - rc.left, rc.bottom - rc.top);

	SDL_ShowCursor(1);
	pHandle->sdlwindow = sdlwindow;
	pHandle->sdlRenderer = sdlRenderer;
	pHandle->hWnd = hWnd;
	
	pHandle->mutex = tsk_mutex_create();

	SDL_SetEventFilter(MySdlEventFilter, (void*)pHandle);

	TSK_DEBUG_INFO("sdl SDL_Display_Init end");
	return pHandle;
}

void SDL_Display_Clear(_HSDL pHandle){
    if (pHandle == NULL)
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
    SDL_Delay(100);
    tsk_mutex_unlock(h->mutex);
}


void SDL_Display_Uninit(_HSDL pHandle)
{
    SDL_Display_Clear(pHandle);
	SDL_Handle * h = (SDL_Handle*)pHandle;
	if (h) {
		bool isVisible = ::IsWindowVisible(h->hWnd);
		if (h->mutex)
			tsk_mutex_lock(h->mutex);
		if (h->sdlTexture)
			SDL_DestroyTexture(h->sdlTexture);
		if (h->sdlRenderer)
			SDL_DestroyRenderer(h->sdlRenderer);
		if (h->sdlwindow)
			SDL_DestroyWindow(h->sdlwindow);
		if (h->mutex)
		{
			tsk_mutex_unlock(h->mutex);
			tsk_mutex_destroy(&h->mutex);
		}
		if (isVisible)
		   ::ShowWindow(h->hWnd, SW_SHOWNORMAL);
		free(h);
	}
	
	//SDL_Quit();
}

// void SDL_Event_Process(SDL_Handle * pHandle)
// {
// 	SDL_Event event;
// 	while (pHandle && !pHandle->threadExitFlag) {
// 		if (SDL_PollEvent(&event)) {
// 			switch (sdlEvent.type)
// 	        {
// 		        case SDL_WINDOWEVENT:
// 		            SDL_GetWindowSize(pHandle->sdlwindow, &nScreenWidth, &nScreenHeight);
// 		            break;
// 		        case REFRESH_EVENT:            
// 		            //显示
// 		            SDL_LockMutex(g_pSdlMutex);
// 		            SDL_RenderPresent(pSDLRenderer);
// 		            SDL_UnlockMutex(g_pSdlMutex);
// 		            break;
		 
// 		        case SDL_QUIT:
// 		            g_nTheadExit = 1;
// 		            SDL_HideWindow(g_pScreen);
// 		            break;
// 		        case BREAK_EVENT:
// 		            break;
// 		        default:
// 		            break;

// 			}
// 		}
// 	}
// }





void SDL_Display(_HSDL pHandle, const unsigned char * data, int width, int height, int i_stride)
{
	if (pHandle == NULL) {
		return;
	}
	try
	{
		int ret = 0;
		SDL_Handle * h = (SDL_Handle*)pHandle;

		tsk_mutex_lock(h->mutex);
		RECT rcWin;
		int dst_width_ = 0, dst_height_ = 0;

		//::GetClientRect(h->hWnd, &rcWin);
		// dst_width_ = rcWin.right - rcWin.left;
		// dst_height_ = rcWin.bottom - rcWin.top;

		if (width != h->sdlRect.w || height != h->sdlRect.h)
			SDL_Create_Texture(h, width, height);

		if (!h->sdlRenderer || !h->sdlTexture){
			tsk_mutex_unlock(h->mutex);
			return;
		}
		/*
		int cropped_src_width, cropped_src_height;
		if (dst_width_ * height > width * dst_height_) {
			cropped_src_height = dst_height_;
			cropped_src_width = width*dst_height_ / (float)height;
			
		}
		else{

			cropped_src_width = dst_width_;
			cropped_src_height = height*dst_width_ / (float)width;
		}
		*/

		// SDL_Window * sdlwindow = NULL;
		// sdlwindow = SDL_CreateWindowFrom((const void *)h->hWnd);
		SDL_GetWindowSize(h->sdlwindow, &dst_width_, &dst_height_);

		SDL_Rect dstrect;
		// float scale = 1.0;
		if (dst_width_ * height > width * dst_height_)
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

	    dstrect.x = (dst_width_ - dstrect.w) / 2;
    	dstrect.y = (dst_height_ - dstrect.h) / 2;

		SDL_Rect rcSrc = { 0, 0, width, height };
		SDL_Rect rcDest = { 0, 0, dst_width_, dst_height_ };
		//SDL_Rect rcDest = { (dst_width_ - cropped_src_width) / 2, (dst_height_ - cropped_src_height) / 2, cropped_src_width, cropped_src_height };
		ret = SDL_UpdateTexture(h->sdlTexture, &rcSrc, data, i_stride);
		ret = SDL_RenderClear(h->sdlRenderer);
		// ret = SDL_RenderSetScale(h->sdlRenderer, scale, scale);
		// ret = SDL_RenderSetLogicalSize(h->sdlRenderer, width, height);
		// ret = SDL_RenderCopy(h->sdlRenderer, h->sdlTexture, NULL, NULL);
		SDL_RenderCopyEx(h->sdlRenderer, h->sdlTexture, NULL, &dstrect, 0.0, NULL, SDL_FLIP_NONE);
		SDL_RenderPresent(h->sdlRenderer);
		//SDL_Event_Process(h);
		
		tsk_mutex_unlock(h->mutex);
	}
	catch (...)
	{

	}

}

