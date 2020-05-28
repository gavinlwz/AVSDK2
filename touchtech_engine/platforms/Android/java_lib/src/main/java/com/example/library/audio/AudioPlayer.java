package main.java.com.example.library.audio;

import android.annotation.TargetApi;
import android.media.AudioAttributes;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.os.Build;
import android.util.Log;

import java.nio.ByteBuffer;
import java.util.Arrays;

import main.java.com.example.library.NativeEngine;

public class AudioPlayer
{
	private static final String TAG = "YoumeAudioPlayer";
	private static final int     DEFAULT_STREAM_TYPE = AudioManager.STREAM_MUSIC;
	private static final int     DEFAULT_SAMPLE_RATE = 44100;
	private static final int     DEFAULT_CHANNEL_NUM = 2;
	private static final int     DEFAULT_BYTES_PER_SAMPLE = 2;
	private static final int     DEFAULT_MODE = AudioTrack.MODE_STREAM;

	private static AudioTrack mAudioTrack;
	private static int           mMinBufferSize = 0;
	private static Thread mPlayerThread;
	private static boolean       mIsPlayerStarted = false;
	private static boolean       mIsLoopExit = false;
	private static int           mStreamType;
	private static int           mSamplerate;
	private static int           mChannelNum;
	private static int           mBytesPerSample;
	private static int           mMode;
	private static int           mInitStatus = 100;
	private static int           mPlayStatus = 0;
	public  static ByteBuffer mInBuffer = null;
	private static final int     DEFAULT_USAGE = getDefaultUsageAttribute();
	private static int           usageAttribute ;
	private static int           audioRecordSessionID = -1;
	private static boolean       mIsStreamVoice = true;

	public static boolean isPlayerStarted () {
		return mIsPlayerStarted;
	}

	public static int getPlayerStatus () {
		return mPlayStatus;
	}

	public static int getPlayerInitStatus () {
		return mInitStatus;
	}

	public static void initPlayer () {
		initPlayer(DEFAULT_SAMPLE_RATE, DEFAULT_CHANNEL_NUM, DEFAULT_BYTES_PER_SAMPLE, false);
	}

	@TargetApi(Build.VERSION_CODES.GINGERBREAD)
	public static void initPlayer (int sampleRateInHz, int channelNum, int bytesPerSample, boolean isStreamVoice) {
		int channelCfg;
		int pcmType;
		mStreamType      = DEFAULT_STREAM_TYPE;
		mSamplerate      = sampleRateInHz;
		mChannelNum      = channelNum;
		mBytesPerSample  = bytesPerSample;
		mMode            = DEFAULT_MODE;
		mIsStreamVoice   = isStreamVoice;
		if (mIsStreamVoice) {
			mStreamType = AudioManager.STREAM_VOICE_CALL;
			Log.w(TAG, "## player set voice stream:STREAM_VOICE_CALL");
		} else {
			mStreamType = AudioManager.STREAM_MUSIC;
			Log.w(TAG, "## player set voice stream:STREAM_MUSIC");
		}


		switch (channelNum) {
			case 1:
				channelCfg = AudioFormat.CHANNEL_OUT_MONO;
				break;
			case 2:
				channelCfg = AudioFormat.CHANNEL_OUT_STEREO;
				break;
			default:
				channelCfg = AudioFormat.CHANNEL_OUT_MONO;
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

		mMinBufferSize = AudioTrack.getMinBufferSize(mSamplerate, channelCfg, pcmType);
		if (mMinBufferSize == AudioTrack.ERROR_BAD_VALUE) { // AudioRecord.ERROR_BAD_VALUE = -2
			Log.e(TAG, "Invalid parameter !");
			mInitStatus = AudioTrack.ERROR_BAD_VALUE;
			return;
		}
		Log.d(TAG , "getMinBufferSize = "+mMinBufferSize+" bytes samplerate:"+mSamplerate);
		try {
			if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
				mAudioTrack = createAudioTrackOnLollipopOrHigher(
						mSamplerate, channelCfg, mMinBufferSize, pcmType);
			} else {
				mAudioTrack = new AudioTrack(mStreamType, mSamplerate, channelCfg, pcmType, mMinBufferSize, mMode,audioRecordSessionID==-1? AudioManager.AUDIO_SESSION_ID_GENERATE:audioRecordSessionID);
			}
		}catch (Throwable e) {
			e.printStackTrace();
			Log.e(TAG, "AudioPlayer initialize fail !");
			mInitStatus = AudioTrack.STATE_UNINITIALIZED;
			if(mAudioTrack != null)mAudioTrack.release();
			return;
		}

		if (mAudioTrack != null && mAudioTrack.getState() == AudioTrack.STATE_UNINITIALIZED) { // AudioRecord.STATE_UNINITIALIZED = 0, AudioRecord.STATE_INITIALIZED = 1
			Log.e(TAG, "AudioPlayer initialize fail !");
			mInitStatus = AudioTrack.STATE_UNINITIALIZED;
			mAudioTrack.release();
			return;
		}

		int readBufSize = mSamplerate * mChannelNum * mBytesPerSample / 100 * 2; // 20ms data
		if (readBufSize > mMinBufferSize) {
			Log.e(TAG , "Error play buffer overflow!");
		}
		mInBuffer = ByteBuffer.allocateDirect(readBufSize);
	}

	public static void setAudioRecordSessionID(int id){
		audioRecordSessionID = id;
		if(mIsPlayerStarted){
			OnAudioPlayer(0);
			initPlayer (mSamplerate ,mChannelNum , mBytesPerSample, mIsStreamVoice);
			OnAudioPlayer(1);
		}
	}

	@TargetApi(21)
	private static AudioTrack createAudioTrackOnLollipopOrHigher(
			int sampleRateInHz, int channelConfig, int bufferSizeInBytes,int pcmType) {
		Log.d(TAG, "createAudioTrackOnLollipopOrHigher");
		// TODO(henrika): use setPerformanceMode(int) with PERFORMANCE_MODE_LOW_LATENCY to control
		// performance when Android O is supported. Add some logging in the mean time.
		final int nativeOutputSampleRate =
				AudioTrack.getNativeOutputSampleRate(AudioManager.STREAM_VOICE_CALL);
		Log.d(TAG, "nativeOutputSampleRate: " + nativeOutputSampleRate);
		if (sampleRateInHz != nativeOutputSampleRate) {
			Log.w(TAG, "Unable to use fast mode since requested sample rate is not native,native rate:"+nativeOutputSampleRate);
		}
		usageAttribute  = getDefaultUsageAttribute();
		if (usageAttribute != DEFAULT_USAGE) {
			Log.w(TAG, "A non default usage attribute is used: " + usageAttribute);
		}
		int usage = AudioAttributes.USAGE_MEDIA;
		int contentType = AudioAttributes.CONTENT_TYPE_UNKNOWN;
		if (mStreamType == AudioManager.STREAM_VOICE_CALL) {
			usage = AudioAttributes.USAGE_VOICE_COMMUNICATION;
			contentType = AudioAttributes.CONTENT_TYPE_SPEECH;
		}else {
			usage = AudioAttributes.USAGE_MEDIA;
			contentType = AudioAttributes.CONTENT_TYPE_MUSIC;
		}
		return new AudioTrack(
				new AudioAttributes.Builder()
						.setUsage(usage)
						.setContentType(contentType)
						.build(),
				new AudioFormat.Builder()
						.setEncoding(pcmType)
						.setSampleRate(sampleRateInHz)
						.setChannelMask(channelConfig)
						.build(),
				bufferSizeInBytes,
				mMode,
				audioRecordSessionID==-1? AudioManager.AUDIO_SESSION_ID_GENERATE:audioRecordSessionID);
	}

	public static boolean startPlayer () {
		if ((mAudioTrack == null) || (mInitStatus == AudioTrack.ERROR_BAD_VALUE) || (mInitStatus == AudioTrack.STATE_UNINITIALIZED)) {
			Log.e(TAG, "Player cannot be started because initial fail !");
			mInBuffer = null;
			if(mAudioTrack != null) {
			    mAudioTrack.release();
			}
			return false;
		}

		if (mIsPlayerStarted) {
			Log.e(TAG, "Player already started !");
			return false;
		}

		mAudioTrack.play();

		mIsLoopExit = false;
		mPlayerThread = new Thread(new AudioPlayerRunnable());
		mPlayerThread.start();

		mIsPlayerStarted = true;

		Log.d(TAG, "Start audio player success !");

		return true;
	}

	public static void stopPlayer() {
		if (!mIsPlayerStarted) {
			return;
		}

		mIsLoopExit = true;
		try {
			mPlayerThread.interrupt();
			mPlayerThread.join(5000);
		}
		catch (InterruptedException e) {
			e.printStackTrace();
		}

		if(mAudioTrack != null) {
		    if (mAudioTrack.getPlayState() == AudioTrack.PLAYSTATE_PLAYING) {
			    mAudioTrack.stop();
		    }

		    mAudioTrack.release();
		}
		mIsPlayerStarted = false;
		mInBuffer = null;

		Log.d(TAG, "Stop audio player success !");
	}

	private static class AudioPlayerRunnable implements Runnable {
		@Override
		public void run() {
			try {
				// Process.setThreadPriority(Process.THREAD_PRIORITY_URGENT_AUDIO);
				int readBufSize = mSamplerate * mChannelNum * mBytesPerSample / 100 * 2; // 20ms data
				while ((!mIsLoopExit) && (!Thread.interrupted())) {

					if (readBufSize > mMinBufferSize) {
						Log.e(TAG , "Error play buffer overflow!");
					}

					if (mInBuffer == null) {
						mInBuffer = ByteBuffer.allocateDirect(readBufSize);
					}
					byte[] array = mInBuffer.array();
					Arrays.fill(array, (byte)0);
					mInBuffer.clear();
					OnAudioPlayerRefresh(mInBuffer, mSamplerate, mChannelNum, mBytesPerSample); // Call for 20ms data
					if(mAudioTrack != null) {
						int ret = AudioTrack.SUCCESS;
						try{
							if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
								ret = mAudioTrack.write(mInBuffer, mInBuffer.capacity(), AudioTrack.WRITE_BLOCKING);
							}else{
								array = mInBuffer.array();
								mAudioTrack.write(array, 0, readBufSize);
							}
						}
					    catch (NoSuchMethodError error) {
							array = mInBuffer.array();
							mAudioTrack.write(array, 0, readBufSize);
						}
					    if (ret == AudioTrack.ERROR_INVALID_OPERATION) {
						    Log.e(TAG , "Error ERROR_INVALID_OPERATION");
						    mPlayStatus = AudioTrack.ERROR_INVALID_OPERATION;
					    }
					    else if (ret == AudioTrack.ERROR_BAD_VALUE) {
						    Log.e(TAG , "Error ERROR_BAD_VALUE");
						    mPlayStatus = AudioTrack.ERROR_BAD_VALUE;
					    }
					    else {
						    mPlayStatus = mAudioTrack.getPlayState();
						}
					}
				}
			}catch (Throwable e) {
				e.printStackTrace();
				Log.e(TAG, "AudioPlayer thread exit!");
			}
		}
	}

	private static int getDefaultUsageAttribute() {
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
			return getDefaultUsageAttributeOnLollipopOrHigher();
		} else {
			// Not used on SDKs lower than L.
			return 0;
		}
	}

	@TargetApi(21)
	private static int getDefaultUsageAttributeOnLollipopOrHigher() {
		return mStreamType == DEFAULT_STREAM_TYPE ? AudioAttributes.USAGE_MEDIA : AudioAttributes.USAGE_VOICE_COMMUNICATION;
	}

	public static void OnAudioPlayer(int isStart) {
		Log.d("AudioRecorder", "AudioRecorder : " + isStart);
		if (isStart != 0) {
			startPlayer();
		} else {
			stopPlayer();
		}
	}

	public static void OnAudioPlayerRefresh(ByteBuffer audBuf, int samplerate, int channelnum, int bps) {
		// Notify native layer to refresh IO buffer
		NativeEngine.AudioPlayerBufRefresh(audBuf, samplerate, channelnum, bps);
	}
}
