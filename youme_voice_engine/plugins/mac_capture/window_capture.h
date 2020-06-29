// for mac screen record

typedef struct obs_data_s {
	//todo
    uint32_t      	window_id;
    char       		owner_name[256];
    char        	window_name[256];
}obs_data_t;

typedef void (*VideoScreenCb)(char *data, uint32_t len, int width, int height, uint64_t timestamp);

void GetWindowDefaultSetting(obs_data_t *settings);

void SetCaptureWindow(int windowid); 

int SetWindowCaptureFramerate(void * data, int framerate);

void SetWindowCaptureVideoCallback(void *data, VideoScreenCb cb);

void* InitWindowMedia(obs_data_t *settings);

int StartWindowStream(void * data);

int StopWindowStream(void * data);

void DestroyWindowMedia(void* data);

void EnumerateWindowInfo(obs_data_t* info, int *count);

void setWindowShareContext( void* context);


/*
usage for mac cacture:
1. GetWindowList()
2. SetCaptureFramerate
3. SetCaptureVideoCallback
4. StartStream
5. DestroyMedia

*/
