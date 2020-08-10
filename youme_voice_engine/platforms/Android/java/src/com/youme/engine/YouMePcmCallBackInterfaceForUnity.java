package com.youme.engine;

public interface YouMePcmCallBackInterfaceForUnity {
    public void onPcmDataRemote(int channelNum, int samplingRateHz, int bytesPerSample, YouMePcmDataForUnity data);

    public void onPcmDataRecord(int channelNum, int samplingRateHz, int bytesPerSample, YouMePcmDataForUnity data);

    public void onPcmDataMix(int channelNum, int samplingRateHz, int bytesPerSample, YouMePcmDataForUnity data);
}
