package com.youme.mixers;

import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;

/**
 * Created by bhb on 2018/1/25.
 */

public class TaskThread{

    private Handler renderThreadHandler;
    private boolean async = true;
    public TaskThread(){
        final HandlerThread renderThread = new HandlerThread("TaskThread");
        renderThread.start();
        renderThreadHandler = new Handler(renderThread.getLooper());
    }

    public void release(){
        if (renderThreadHandler != null) {
            final Looper renderLooper = renderThreadHandler.getLooper();
            renderThreadHandler.post(new Runnable() {
                @Override
                public void run() {
                    renderLooper.quit();
                }
            });
            renderThreadHandler = null;
        }
    }

    public void postToEncoderThread(Runnable runnable) {
    	if(async && renderThreadHandler != null){
    		renderThreadHandler.post(runnable);
    	}else{
    		runnable.run();
    	}
    	
    }
    
    public void setAsyncEnabled(boolean enabled){
    	async = enabled;
    }
}
