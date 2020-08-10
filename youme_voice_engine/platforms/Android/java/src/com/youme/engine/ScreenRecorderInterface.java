package com.youme.engine;

import android.content.Context;
import android.content.Intent;

public interface ScreenRecorderInterface {

    public void init(Context env);
    public void setResolution(int width, int height);

    public void setFps(int fps);

    public boolean startScreenRecorder();

    public boolean stopScreenRecorder();

    public boolean onActivityResult(int requestCode, int resultCode, Intent data);

    public void orientationChange(int rotation,int width,int height);

}