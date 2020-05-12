package com.youme.voiceengine;

import android.content.Context;
import android.content.Intent;
import android.os.Build;

public class ScreenRecorder {

    public static int SCREEN_RECODER_REQUEST_CODE = 93847;
    private static ScreenRecorderInterface recoderInstance = null;

    public static boolean init(Context env){
        if(Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            if (api.isInited() && env != null) {
                if (recoderInstance == null) {
                    // recoderInstance =  api.getVideoHardwareCodeEnable() && api.getUseGL()  ? new ScreenRecorderGLES() : new ScreenRecorderARGB();
                    recoderInstance =   new ScreenRecorderGLES();
                }
                recoderInstance.init(env);
                return true;
            }
        }
        return false;
    }
    public static boolean setResolution(int width, int height){
        if(Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            if (recoderInstance != null) {
                recoderInstance.setResolution(width, height);
                return true;
            }
        }
        return false;
    }

    public static boolean setFps(int fps){
        if(Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            if (recoderInstance != null) {
                recoderInstance.setFps(fps);
                return true;
            }
        }
        return false;
    }

    public static boolean startScreenRecorder(){
        if(Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            if (recoderInstance != null) {
                return recoderInstance.startScreenRecorder();
            }
        }
        return false;
    }

    public static boolean stopScreenRecorder(){
        if(Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            if (recoderInstance != null) {
                return recoderInstance.stopScreenRecorder();
            }
        }
        return false;
    }

    public static boolean onActivityResult(int requestCode, int resultCode, Intent data){
        if(Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            if (recoderInstance != null) {
                return recoderInstance.onActivityResult(requestCode, resultCode, data);
            }
        }
        return false;
    }

    public static boolean orientationChange(int rotation,int width,int height){
        if(Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            if (recoderInstance != null) {
                recoderInstance.orientationChange(rotation, width, height);
                return true;
            }
        }
        return false;
    }

}
