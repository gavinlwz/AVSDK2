package com.youme.voiceengine;

import android.Manifest;
import android.app.Activity;
import android.app.Application;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.graphics.ImageFormat;
import android.graphics.PixelFormat;
import android.hardware.display.DisplayManager;
import android.hardware.display.VirtualDisplay;
import android.media.Image;
import android.media.ImageReader;
import android.media.projection.MediaProjection;
import android.media.projection.MediaProjectionManager;
import android.os.Build;
import android.os.Environment;
import android.support.v4.content.ContextCompat;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Surface;

import com.youme.voiceengine.YouMeConst;
import com.youme.voiceengine.api;

import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.lang.ref.WeakReference;
import java.nio.ByteBuffer;
import java.text.SimpleDateFormat;
import java.util.Date;

public interface ScreenRecorderInterface {

    public void init(Context env);
    public void setResolution(int width, int height);

    public void setFps(int fps);

    public boolean startScreenRecorder();

    public boolean stopScreenRecorder();

    public boolean onActivityResult(int requestCode, int resultCode, Intent data);

    public void orientationChange(int rotation,int width,int height);

}