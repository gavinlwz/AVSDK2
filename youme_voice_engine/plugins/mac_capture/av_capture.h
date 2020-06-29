
#include "window_capture.h"

#include "screen_capture.hpp"

/* init capture module
 * mode: 1: capture device, 2: window, 3:screen
 * setting: for window capture, setting is window id 
 *          for screen capture, setting id screen id
 */
void* InitMedia(int mode, void * setting);

/* start to capture frame
 * 		data is capture handle which create by InitMedia
 */
int StartStream(void * data);


void SetShareContext(void* context );


/* stop to capture frame
 * 		data is capture handle which create by InitMedia
 */
int StopStream(void * data);

/* destroy capture module
 * 		data is capture handle which create by InitMedia
 */
void DestroyMedia(void *data);

/*  setting framerate of capture frame
 *		data is capture handle which create by InitMedia
 *		fps is framerate
*/
int SetCaptureFramerate(void *data, int fps);

/*
 *  共享屏幕排除窗口
 */
int SetExclusiveWnd( void * data, int wnd );

/* setting callback func which send frame to caller
 *		data is capture handle which create by InitMedia
 *		cb is the callback func
*/
void SetCaptureVideoCallback(void *data, ScreenCb cb);


/* GetWindowList
 * mode: 1: capture device(not used), 2: window, 3:screen
 * setting: for window capture, list is window list (inlcude window-id, window-name, owner-name)
 *          for screen capture, list is screen count (from 0~n)
*/
void GetWindowList(int mode, obs_data_t *info, int *count);

