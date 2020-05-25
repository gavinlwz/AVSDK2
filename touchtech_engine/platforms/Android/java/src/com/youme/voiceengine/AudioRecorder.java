package com.youme.voiceengine;

import android.util.Log;
import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaRecorder;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.Arrays;
import java.util.Timer;
import java.util.TimerTask;
import java.nio.ByteBuffer;

import android.os.Build;
import android.annotation.TargetApi;
//import android.os.Process;

public class AudioRecorder
{
    private static final String  TAG = "YoumeAudioRecorder";
    private static final int     DEFAULT_SOURCE = MediaRecorder.AudioSource.MIC;
    private static final int     DEFAULT_SAMPLE_RATE = 44100;
    private static final int     DEFAULT_CHANNEL_NUM = 2;
    private static final int     DEFAULT_BYTES_PER_SAMPLE = 2;
    private static final boolean DEBUG = false;
    
    private static String        AudioName;
    private static String        AudioRecordError = "";
    private static AudioRecord   mAudioRecord;
    private static int           mMinBufferSize = 0;
    private static Thread        mRecorderThread;
    private static boolean       mIsRecorderStarted = false;
    private static boolean       mIsLoopExit = false;
    private static int           mTypeSpeech = 1;
    private static int           mMicSource;
    private static int           mSamplerate;
    private static int           mChannelNum;
    private static int           mBytesPerSample;
    private static int           mInitStatus = 100;
    private static int           mRecordStatus = 0;
    public  static ByteBuffer    mOutBuffer = null;
    private static int           mCounter = 1;
    private static int           mLoopCounter = 1;
    private static int           mErrorCounter = 0;
    private static boolean       mInitSuceed = false;
    private static int           readBufSize = 0;
    private static boolean       pauseRecord = false;
    private static boolean       needChangeMode = false;
    private static boolean       isReleased = true;

    public static boolean isRecorderStarted () {
        return mIsRecorderStarted;
    }
    
    public static int getRecorderStatus () {
        return mRecordStatus;
    }
    
    public static int getRecorderInitStatus () {
        return mInitStatus;
    }
    
    public static void initRecorder () {
        initRecorder(DEFAULT_SAMPLE_RATE, DEFAULT_CHANNEL_NUM, DEFAULT_BYTES_PER_SAMPLE, mTypeSpeech);
    }
    
    // Creates an AudioRecord instance using AudioRecord.Builder which was added in API level 23.
    @TargetApi(23)
    private static AudioRecord createAudioRecordOnMarshmallowOrHigher(
            int audioSource, int sampleRateInHz, int channelConfig, int bufferSizeInBytes) {
        Log.d(TAG, "createAudioRecordOnMarshmallowOrHigher audioSource:" + audioSource);
        return new AudioRecord.Builder()
                .setAudioSource(audioSource)
                .setAudioFormat(new AudioFormat.Builder()
                        .setEncoding(AudioFormat.ENCODING_PCM_16BIT)
                        .setSampleRate(sampleRateInHz)
                        .setChannelMask(channelConfig)
                        .build())
                .setBufferSizeInBytes(bufferSizeInBytes)
                .build();
    }
    
    public static void initRecorder (int sampleRateInHz, int channelNum, int bytesPerSample, int typeSpeech) {
    	try {
            if(pauseRecord){
                pauseRecord = false;
                if(needChangeMode){
                    AudioMgr.setVoiceModeYouMeCoutum();
                }
            }
	        int channelCfg;
	        int pcmType;
            mTypeSpeech = typeSpeech;
	        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB) {
	            mMicSource = typeSpeech == 1 ? MediaRecorder.AudioSource.VOICE_COMMUNICATION : MediaRecorder.AudioSource.MIC;
	        }else{
	            mMicSource = typeSpeech == 1 ? MediaRecorder.AudioSource.VOICE_CALL : MediaRecorder.AudioSource.MIC;
            }
            Log.d(TAG, "initRecorder mMicSource:" + mMicSource);
	        mSamplerate      = sampleRateInHz;
	        mChannelNum      = channelNum;
	        mBytesPerSample  = bytesPerSample;
	        mLoopCounter     = 1;
	        mInitSuceed      = true;
            mRecordStatus    = 0;
            isReleased       = false;
	        
	        switch (channelNum) {
	            case 1:
	                channelCfg = AudioFormat.CHANNEL_IN_MONO;
	                break;
	            case 2:
	                channelCfg = AudioFormat.CHANNEL_IN_STEREO;
	                break;
	            default:
	                channelCfg = AudioFormat.CHANNEL_IN_MONO;
	                break;
	        }
	        switch (bytesPerSample) {
	            case 1:
	                pcmType = AudioFormat.ENCODING_PCM_8BIT;
	                break;
	            case 2:
	                pcmType = AudioFormat.ENCODING_PCM_16BIT;
	                break;
	            default:
	                pcmType = AudioFormat.ENCODING_PCM_16BIT;
	                break;
	        }
	        
	        readBufSize = mSamplerate * mChannelNum * mBytesPerSample / 100 * 2; // 20ms data
	        mOutBuffer = ByteBuffer.allocateDirect(readBufSize);
	        
	        mMinBufferSize = AudioRecord.getMinBufferSize(mSamplerate, channelCfg, pcmType);
	        if (mMinBufferSize == AudioRecord.ERROR_BAD_VALUE) { // AudioRecord.ERROR_BAD_VALUE = -2
	            Log.e(TAG, "Invalid parameter !");
	            mInitStatus = AudioRecord.ERROR_BAD_VALUE;
	            mInitSuceed = false;
                isReleased  = true;
	        }
	        mMinBufferSize = mMinBufferSize * 2;
	        Log.d(TAG, "getMinBufferSize = "+mMinBufferSize+" bytes");


	        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
	            // Use AudioRecord.Builder to create the AudioRecord instance if we are on API level 23 or
	            // higher.
	            mAudioRecord = createAudioRecordOnMarshmallowOrHigher(
                    mMicSource, mSamplerate, channelCfg, mMinBufferSize);
	        } else {
	            // Use default constructor for API levels below 23.
	            mAudioRecord = new AudioRecord(mMicSource, mSamplerate, channelCfg, pcmType, mMinBufferSize);
	        }
	        
	        if (mAudioRecord.getState() == AudioRecord.STATE_UNINITIALIZED) { // AudioRecord.STATE_UNINITIALIZED = 0, AudioRecord.STATE_INITIALIZED = 1
	            Log.e(TAG, "AudioRecord initialize fail !");
	            mInitStatus = AudioRecord.STATE_UNINITIALIZED;
	            mAudioRecord.release(); // Initial fail will release handler
	            mInitSuceed = false;
                isReleased  = true;
	        }
	        
	        
	        if (mInitSuceed && (readBufSize > mMinBufferSize)) {
	            Log.e(TAG, "Error record buffer overflow!");
	        }
	        
    	} catch (Throwable e) {
            Log.e(TAG, "Error create record");
    		mInitSuceed = false;
            isReleased  = true;
			e.printStackTrace();
		}
    }

    private static void initWithLatestParm(){
        initRecorder(mSamplerate,mChannelNum,mBytesPerSample, mTypeSpeech);
    }

    public static void setMicMuteStatus(int muteMic){
        return ;
//        isMute = muteMic != 0;
//        if(!isMute && mIsRecorderStarted && mInitSuceed && !isRealRecording){
//            mAudioRecord.startRecording();
//            isRealRecording = true;
//        }
    }

    public static boolean startRecorder () {
        if (mIsRecorderStarted) {
            Log.e(TAG, "Recorder already started !");
            return false;
        }

        if (mInitSuceed) {
            if(isReleased){
                initWithLatestParm();
            }
            mAudioRecord.startRecording();

        }else{
            Log.e(TAG, "Recorder not init successed.");
            //return false;
            Log.e(TAG, "Output mute data");
        }

        mIsLoopExit = false;
        mRecorderThread = new Thread(new AudioRecorderRunnable());
        mRecorderThread.start();
        
        mIsRecorderStarted = true;
        
        Log.d(TAG, "Start audio recorder success !");
        
        return true;
    }
    
    public static void stopRecorder() {

        if (!mIsRecorderStarted) {
            return;
        }

        mIsLoopExit = true;
        try {
            mRecorderThread.interrupt();
            mRecorderThread.join(5000);
        }
        catch (Throwable e) {
            e.printStackTrace();
        }
        
        if (mInitSuceed && (mAudioRecord.getRecordingState() == AudioRecord.RECORDSTATE_RECORDING)) {
            try {
                mAudioRecord.stop();
                mAudioRecord.release();
            }catch (Throwable e){
                e.printStackTrace();
            }
        }
        isReleased  = true;
        mIsRecorderStarted = false;
        mOutBuffer = null;

        Log.d(TAG, "Stop audio recorder success !");
    }
    
    private static class AudioRecorderRunnable implements Runnable {
        
        @Override
        public void run() {
//            Process.setThreadPriority(Process.THREAD_PRIORITY_URGENT_AUDIO);
            FileOutputStream fos = null;
            if (DEBUG) {
                AudioName = String.format("/sdcard/test_%d.pcm", mCounter++);
                File file = new File(AudioName);
                try {
                    if (file.exists()) {
                        file.delete();
                    }
                    fos = new FileOutputStream(file); //建立一个可以存取字节的文件
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
            
            try {
                while ((!mIsLoopExit) && (!Thread.interrupted())) {
                    
                    //int readBufSize = mSamplerate * mChannelNum * mBytesPerSample / 100 * 2; // 20ms data
                    if (mInitSuceed && (readBufSize > mMinBufferSize)) {
                        Log.e(TAG, "Error record buffer overflow!");
                    }
                    
                    if (mInitSuceed && mIsRecorderStarted) {
                        if (mOutBuffer == null) {
                            int readBufSize = mSamplerate * mChannelNum * mBytesPerSample / 100 * 2; // 20ms data
                            mOutBuffer = ByteBuffer.allocateDirect(readBufSize);
                        }
                        int ret = mAudioRecord.read(mOutBuffer, mOutBuffer.capacity());
                        if (ret > 0) {
                            if (DEBUG) {
                                try {
                                    byte[] data = Arrays.copyOf(mOutBuffer.array(), mOutBuffer.capacity());
                                    Log.d(TAG, "OK, Recorder " + ret + " bytes: [" + data[0] + "][" + data[1] + "][" + data[2] + "][" + data[3] + "]");
                                    fos.write(data);
                                } catch (IOException e) {
                                    e.printStackTrace();
                                }
                            }
                            if ((mLoopCounter < 5) && (mLoopCounter >= 0)) {
                                Log.e(TAG, "Record success: ret="+ret);
                            }
                            OnAudioRecorderRefresh(mOutBuffer, mSamplerate, mChannelNum, mBytesPerSample);
                            mRecordStatus = AudioRecord.RECORDSTATE_RECORDING;
                        } else {
                            switch (ret) {
                                case AudioRecord.ERROR_INVALID_OPERATION:     // =-3, if the object isn't properly initialized
                                    mInitStatus = ret;
                                    AudioRecordError = "Error ERROR_INVALID_OPERATION";
                                    break;
                                case AudioRecord.ERROR_BAD_VALUE:             // =-2, if the parameters don't resilve to valid data and indexes
                                    mInitStatus = ret;
                                    AudioRecordError = "Error ERROR_BAD_VALUE";
                                    break;
                                    //case AudioRecord.ERROR_DEAD_OBJECT:           // =-6, if and error indicating that the object reporting it is no longer valid and needs to be recreated
                                    //    AudioRecordError = "Error ERROR_DEAD_OBJECT";
                                    //    break;
                                case AudioRecord.ERROR:                       // =-1, in case of other error
                                    mInitStatus = ret;
                                    AudioRecordError = "Error Other ERRORs";
                                    break;
                                case 0:
                                    mInitStatus = -5; //对应c++的 YOUME_ERROR_REC_NO_DATA 错误
                                    AudioRecordError = "Error Record Size=0, maybe record right NOT be enabled in some special android phone!!";
                                    break;
                            }
                            if(ret == 0){
                                AudioRecordError = "Error no data";
                                mRecordStatus = -4;//对应c++的 YOUME_ERROR_REC_NO_PERMISSION 错误
                            }
                            if(mErrorCounter % 100 == 0) {
                                NativeEngine.logcat(NativeEngine.LOG_WARN, TAG, "mRecordStatus: " + mRecordStatus + "AudioRecordError：" + AudioRecordError);
                            }
                            mErrorCounter ++;
                            mRecordStatus = ret;
                            byte[] array = mOutBuffer.array();
                            Arrays.fill(array, (byte)0);
                            OnAudioRecorderRefresh(mOutBuffer, mSamplerate, mChannelNum, mBytesPerSample);
                            Thread.sleep(20);
                            if ((mLoopCounter < 5) && (mLoopCounter >= 0)) {
                                Log.e(TAG, AudioRecordError);
                            }
                        }
                        mLoopCounter++;
                    } else { // Dummy record data if initial fail
                        if (mOutBuffer == null) {
                            int readBufSize = mSamplerate * mChannelNum * mBytesPerSample / 100 * 2; // 20ms data
                            mOutBuffer = ByteBuffer.allocateDirect(readBufSize);
                        }
                    	if(mOutBuffer != null){
                    		byte[] array = mOutBuffer.array();
                            Arrays.fill(array, (byte)0);
                    		OnAudioRecorderRefresh(mOutBuffer, mSamplerate, mChannelNum, mBytesPerSample);
                    	}
                        Thread.sleep(20);
                    }
                }
                Log.d(TAG, "Recorder thread exit!");
            } catch (Throwable e) {
            	e.printStackTrace();
                Log.e(TAG, "Recorder thread exit!");
            }
            
            if (DEBUG) {
                try {
                    fos.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }
    }
    
    public static void OnAudioRecorder(int isStart) {
    	try {
			// Log.d("AudioRecorder", "AudioRecorder : " + isStart);
			if (isStart != 0) {
				startRecorder();
			} else {
                stopRecorder();
                mInitSuceed = false;
			}
		} catch (Throwable e) {
			e.printStackTrace();
		}
    }
    public static void OnAudioRecorderTmp(int isStart) {
        try {
            Log.d(TAG, "enter OnAudioRecorderTmp : " + isStart);
            if (!mInitSuceed) {
                Log.e(TAG, "OnAudioRecorderTmp return because already stop avsessionMgr");
                return;
            }
            if (isStart != 0) {
                if(pauseRecord){
                    if(AudioMgr.hasChangedCoutum()){
                        needChangeMode = true;
                        AudioMgr.restoreOldMode();
                    }else{
                        needChangeMode = false;
                    }
                    Log.e(TAG, "delay  150ms");
                    TimerTask task = new TimerTask(){
                        public void run(){
                            try {
                                if(pauseRecord) {
                                    startRecorder();
                                }
                            }catch (Throwable e) {
                                e.printStackTrace();
                            }
                        }
                    };
                    Timer timer = new Timer();
                    timer.schedule(task, 150);
                }
            } else {
                if (mIsRecorderStarted) {
                    stopRecorder();
                    pauseRecord = true;
                }
            }
            Log.d(TAG, "leave OnAudioRecorderTmp : " + isStart);
        } catch (Throwable e) {
            e.printStackTrace();
        }
    }


    public static void OnAudioRecorderRefresh(ByteBuffer audBuf, int samplerate, int channelnum, int bps) {
    	try {
			// Notify native layer to refresh IO buffer
			NativeEngine.AudioRecorderBufRefresh(audBuf, samplerate,
					channelnum, bps);
		} catch (Throwable e) {
			e.printStackTrace();
		}
    }
}
