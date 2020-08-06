package com.youme.voiceengine;

public interface YouMeCallBackInterfacePcmForUnity {
    public void onPcmDataRemote(int channelNum, int samplingRateHz, int bytesPerSample, YouMePcmDataForUnity data);

    public void onPcmDataRecord(int channelNum, int samplingRateHz, int bytesPerSample, YouMePcmDataForUnity data);

    public void onPcmDataMix(int channelNum, int samplingRateHz, int bytesPerSample, YouMePcmDataForUnity data);
}
