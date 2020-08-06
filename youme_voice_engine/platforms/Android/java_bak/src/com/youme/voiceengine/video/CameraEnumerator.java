package com.youme.voiceengine.video;

/**
 * Created by fire on 2017/2/14.
 */

import com.youme.voiceengine.video.CameraEnumerationAndroid.CaptureFormat;

import java.util.List;

public interface CameraEnumerator {
    public String[] getDeviceNames();
    public boolean isFrontFacing(String deviceName);
    public boolean isBackFacing(String deviceName);
    public List<CaptureFormat> getSupportedFormats(String deviceName);

    public CameraVideoCapturer createCapturer(
            String deviceName, CameraVideoCapturer.CameraEventsHandler eventsHandler);
}
