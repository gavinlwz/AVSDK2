package main.java.com.example.library.audio;

import android.Manifest;
import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.app.Activity;
import android.bluetooth.BluetoothHeadset;
import android.bluetooth.BluetoothProfile;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.media.AudioManager;
import android.net.ConnectivityManager;
import android.os.Build;
import android.telephony.TelephonyManager;
import android.util.Log;

import java.lang.ref.WeakReference;
import java.util.Timer;
import java.util.TimerTask;

import main.java.com.example.library.AppPara;
import main.java.com.example.library.NetUtil;

@SuppressLint("InlinedApi")
public class AudioMgr
{
	private static AudioManager mAudioManager = null;
	private static Boolean mSpeakerOnBoolean = false;
	private static Boolean mIsOutputToSpeaker = false;
	private static int mMode = -1;//AudioManager.STREAM_VOICE_CALL;
	private static Boolean mHasChangedBoolean = false;
	private static String NET_CHANGE_ACTION = "android.net.conn.CONNECTIVITY_CHANGE";
	private static BroadcastReceiver mReceiver = null;
	private static WeakReference<Context> mContext = null;
	private static Boolean mIsBluetoothScoOn = false;
    private static boolean mHasHeadSet = false;
    private static boolean mIsBluetoothOn = false;
    private static int requestPermissionCount = 0;
	private static boolean isStopedByExternalNotify = false;
    
	public static void init(Context env)
	{
		if(env==null){
			Log.e("AudioMgr",
					"context can not be null");
			return;
		}
		if(mContext != null){
			if(env instanceof Activity){
				mContext.clear();
				mContext = new WeakReference<Context>(env) ;
			}
			return;
		}
		mContext = new WeakReference<Context>(env);
		mAudioManager = (AudioManager) env.getSystemService  (Context.AUDIO_SERVICE);

		mIsOutputToSpeaker = mAudioManager.isSpeakerphoneOn();
		// Get the initial network type
        int networkType = NetUtil.getNetworkState(env);
        AppPara.onNetWorkChange(networkType);
//
//		mReceiver = new BroadcastReceiver()
//		{
//
//			@Override
//		    public void onReceive(Context context, Intent intent)
//		    {
//				try {
//					if(YouMeManager.mInited && api.isJoined() && mContext!=null && mContext.get() != null) {
//						AudioManager audioManager = (AudioManager) mContext.get()
//								.getSystemService(Context.AUDIO_SERVICE);
//						String action = intent.getAction();
//						// boolean hasHeadSet = false;
//						// boolean isBluetoothOn = false;
//						Log.i("AudioMgr", "onReceive action: " + action + "  state: " + intent.getIntExtra(
//								BluetoothProfile.EXTRA_STATE, -1));
//						if (action.equals(NET_CHANGE_ACTION)) {
//							int networkType = NetUtil.getNetworkState(mContext.get());
//							AppPara.onNetWorkChange(networkType);
//						}
//						if (action.equals(Intent.ACTION_HEADSET_PLUG)) {
//							mHasHeadSet = intent.getIntExtra("state", 0) == 0 ? false
//									: true;
//							OnHeadsetChange(audioManager, mHasHeadSet,
//									mIsBluetoothOn);
//						}
//						if (action
//								.equals(BluetoothHeadset.ACTION_CONNECTION_STATE_CHANGED)) {
//							int state = intent.getIntExtra(
//									BluetoothProfile.EXTRA_STATE, -1);
//							if (state == BluetoothProfile.STATE_CONNECTED) {
//								Log.i("AudioMgr",
//										"BluetoothProfile.STATE_CONNECTED");
//								mIsBluetoothOn = true;
//								OnHeadsetChange(audioManager, mHasHeadSet,
//										mIsBluetoothOn);
//							} else if (state == BluetoothProfile.STATE_CONNECTING) {
//								Log.i("AudioMgr",
//										"BluetoothProfile.STATE_CONNECTING");
//							} else if (state == BluetoothProfile.STATE_DISCONNECTED) {
//								Log.i("AudioMgr",
//										"BluetoothProfile.STATE_DISCONNECTED");
//								mIsBluetoothOn = false;
//								OnHeadsetChange(audioManager, mHasHeadSet,
//										mIsBluetoothOn);
//							} else if (state == BluetoothProfile.STATE_DISCONNECTING) {
//								Log.i("AudioMgr",
//										"BluetoothProfile.STATE_DISCONNECTING");
//							}
//						} else if (action
//								.equals(BluetoothHeadset.ACTION_AUDIO_STATE_CHANGED)) {
//							int state = intent.getIntExtra(
//									BluetoothProfile.EXTRA_STATE, -1);
//							if (state == BluetoothHeadset.STATE_AUDIO_CONNECTED) {
//								Log.i("AudioMgr",
//										"BluetoothHeadset.STATE_AUDIO_CONNECTED");
//								mIsBluetoothOn = true;
//								OnHeadsetChange(audioManager, mHasHeadSet,
//										mIsBluetoothOn);
//							} else if (state == BluetoothHeadset.STATE_AUDIO_DISCONNECTED) {
//								Log.i("AudioMgr",
//										"BluetoothHeadset.STATE_AUDIO_DISCONNECTED");
//								mIsBluetoothOn = false;
//								OnHeadsetChange(audioManager, mHasHeadSet,
//										mIsBluetoothOn);
//							} else if (state == BluetoothHeadset.STATE_AUDIO_CONNECTING) {
//								Log.i("AudioMgr",
//										"BluetoothHeadset.STATE_AUDIO_CONNECTING");
//							}
//						} else if (action.equals(TelephonyManager.ACTION_PHONE_STATE_CHANGED)) {
//							String stateStr = intent.getExtras().getString(TelephonyManager.EXTRA_STATE);
//							Log.i("AudioMgr",
//									"ACTION_PHONE_STATE_CHANGED stateStr:" + stateStr);
//							if (stateStr.equals(TelephonyManager.EXTRA_STATE_IDLE)) {
//								api.resumeChannel();
////							if( isCheckedPermission ) {
////								OnHeadsetChange(audioManager, mHasHeadSet,
////										mIsBluetoothOn);
////							}
//							} else if (stateStr.equals(TelephonyManager.EXTRA_STATE_OFFHOOK)) {
//								api.pauseChannel();
//							} else if (stateStr.equals(TelephonyManager.EXTRA_STATE_RINGING)) {
//								api.pauseChannel();
//							}
//						}
//					}
//				} catch (Throwable e) {
//					e.printStackTrace();
//				}
//		    }
//		};
//		IntentFilter mFilter = new IntentFilter();
//        mFilter.addAction(ConnectivityManager.CONNECTIVITY_ACTION);
//        mFilter.addAction(Intent.ACTION_HEADSET_PLUG);
//        mFilter.addAction(BluetoothHeadset.ACTION_CONNECTION_STATE_CHANGED);
//        mFilter.addAction(BluetoothHeadset.ACTION_AUDIO_STATE_CHANGED);
//        mFilter.addAction(TelephonyManager.ACTION_PHONE_STATE_CHANGED);
//		try {
//    		env.registerReceiver(mReceiver, mFilter);
//		} catch (Throwable e) {
//			Log.e("AudioMgr", "registerReceiver fail:");
//			e.printStackTrace();
//		}
	}
	public static void uinit()
	{
		Log.i("AudioMgr", "uinit");
		if(mContext!=null && mContext.get()!=null) {
			mContext.get().unregisterReceiver(mReceiver);
		}
		mContext = null;
		mReceiver = null;
	}
	
	@TargetApi(Build.VERSION_CODES.JELLY_BEAN )
	public static void setVoiceModeYouMeCoutum() 
	{
		try {
			if (null == mAudioManager) {
				Log.e("AudioMgr", "mAudioManager is null");
				return;
			}
			mSpeakerOnBoolean = mAudioManager.isSpeakerphoneOn();
			mIsBluetoothScoOn = mAudioManager.isBluetoothScoOn();
			int communicationModeValue = 0;
			if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB) {
				communicationModeValue = AudioManager.MODE_IN_COMMUNICATION;
			} else {
				communicationModeValue = AudioManager.MODE_IN_CALL;
			}
			Log.i("AudioMgr", "==mMode:" + mMode + " mSpeakerOnBoolean:" + mSpeakerOnBoolean + " mIsBluetoothScoOn:" + mIsBluetoothScoOn + " isBluetoothA2dpOn:" + mAudioManager.isBluetoothA2dpOn());
			if (mMode == -1 ){
				mMode = mAudioManager.getMode();
				if(mMode != communicationModeValue) {
					Log.i("AudioMgr", "start setmode:"+communicationModeValue);
					mAudioManager.setMode(communicationModeValue);
				}else{
					Log.w("AudioMgr","Already in MODE_IN_COMMUNICATION");
				}
			}

			if (mAudioManager.isBluetoothA2dpOn() || mIsBluetoothScoOn) {
				/* 某些设备restore方法中mAudioManager.stopBluetoothSco()之后isBluetoothScoOn()返回的仍然是true，
				 * 原来的 if (mAudioManager.isBluetoothA2dpOn() && !mIsBluetoothScoOn)
				 * 的代码此case走不到，会导致离开再进入房间走到扬声器输出
				 */
				mAudioManager.startBluetoothSco();
				if(mAudioManager.isSpeakerphoneOn()) {
					mAudioManager.setSpeakerphoneOn(false);
					Log.i("AudioMgr", "setSpeakerphoneOn false");
				}
				mAudioManager.setBluetoothScoOn(true);
				Log.i("AudioMgr", "to bluetooth");
			}else{
				boolean isToSpeaker = !mAudioManager.isWiredHeadsetOn();
				Log.i("AudioMgr", "isToSpeaker:"+isToSpeaker);
				if(mAudioManager.isSpeakerphoneOn() != isToSpeaker) {
					Log.i("AudioMgr", "setSpeakerphoneOn "+isToSpeaker);
					mAudioManager.setSpeakerphoneOn(isToSpeaker);
				}
			}

			Log.i("AudioMgr", "设置communication 模式");
			mHasChangedBoolean = true;
		} catch (Throwable e) {
			e.printStackTrace();
		}
	}
	
	public static void restoreOldMode()
	{
		try {
			if (null == mAudioManager)
			{
				return;
			}
			mIsOutputToSpeaker = mSpeakerOnBoolean;
			if (false == mHasChangedBoolean)
			{
				return;
			}
			else
			{
				mHasChangedBoolean = false;
			}
			int mode = mAudioManager.getMode();
			Log.i("AudioMgr", "restoreOldMode:" + mMode);
			if (mode !=mMode && mMode > -1) {
				Log.i("AudioMgr", "stop setmode:"+mMode+" setSpeakerphoneOn:"+mSpeakerOnBoolean);
			    mAudioManager.setMode(mMode);
			    mAudioManager.setSpeakerphoneOn(mSpeakerOnBoolean);
				mMode = -1;
			}

			if (!mIsBluetoothScoOn) {
				Log.i("AudioMgr", "restoreOldMode stop BluetoothSco");
			    mAudioManager.stopBluetoothSco();
			    mAudioManager.setBluetoothScoOn(false);
			}
		} catch (Throwable e) {
			e.printStackTrace();
		}
	}

	public static boolean hasChangedCoutum(){
		return mHasChangedBoolean;
	}
	
	public static void initAudioSettings(boolean outputToSpeaker) 
	{
		boolean isWiredHeadsetOn = mAudioManager.isWiredHeadsetOn();
		boolean isBluetoothScoOn = mAudioManager.isBluetoothScoOn();
		boolean isBluetoothA2dpOn = mAudioManager.isBluetoothA2dpOn();
		mIsOutputToSpeaker = outputToSpeaker;
		try {
			if (isWiredHeadsetOn||isBluetoothScoOn||isBluetoothA2dpOn) {
				if(mAudioManager.isSpeakerphoneOn()) {
					mAudioManager.setSpeakerphoneOn(false);
					Log.i("AudioMgr", "setSpeakerphoneOn false");
				}
				Log.i("AudioMgr", "initAudioSettings setSpeakerphoneOn:false (isWiredHeadsetOn:" + isWiredHeadsetOn+ " isBluetoothScoOn:" + isBluetoothScoOn + " isBluetoothA2dpOn:" + isBluetoothA2dpOn + ")");
			}else{
				if(mAudioManager.isSpeakerphoneOn() != outputToSpeaker) {
					mAudioManager.setSpeakerphoneOn(outputToSpeaker);
					Log.i("AudioMgr", "initAudioSettings setSpeakerphoneOn "+outputToSpeaker);
				}
			}
		} catch (Throwable e) {
			e.printStackTrace();
		}
	}

	public static int isWiredHeadsetOn() {
		if (mAudioManager != null) {
			return mAudioManager.isWiredHeadsetOn() ? 1 : 0;
		} else {
			return 0;
		}
	}

//
//	public static void OnHeadsetChange(final AudioManager audioManager, final Boolean hasHeadSet, final Boolean isBluetoothOn) {
//		Log.i("AudioMgr","isBluetoothScoOn:" + audioManager.isBluetoothScoOn()+"  isBluetoothA2dpOn:" + audioManager.isBluetoothA2dpOn());
//        if (hasHeadSet) {
//            // Headset plugin, whatever BT headset plugin or not, will render audio to headset
//            // And treat it as headset plug in, voice can render to headset when monitor mode is on
//            audioManager.setSpeakerphoneOn(false);
//            audioManager.setBluetoothScoOn(false);
//            AppPara.onHeadSetPlugin(1);
//            Log.i("AudioMgr","hasHeadSet:" + hasHeadSet + " isBluetoothOn:" +isBluetoothOn);
//        } else if (isBluetoothOn) {
//            // BT headset plugin, will render audio to BT headset
//            // And treat it as NO headset plug in, voice can NOT render to BT headset when monitor mode is on
//			TimerTask task = new TimerTask(){
//				public void run(){
//					try {
//						if (!audioManager.isBluetoothScoOn() && mMode > -1) {
//							Log.i("AudioMgr","not isBluetoothScoOn, need to startBluetoothSco");
//							audioManager.startBluetoothSco();
//							audioManager.setSpeakerphoneOn(false);
//							audioManager.setBluetoothScoOn(true);
//						}
//						AppPara.onHeadSetPlugin(0);
//						Log.i("AudioMgr","hasHeadSet:" + hasHeadSet + " isBluetoothOn:" +isBluetoothOn);
//					}catch (Throwable e) {
//						e.printStackTrace();
//					}
//				}
//			};
//			Timer timer = new Timer();
//			timer.schedule(task, 500);
//        } else {
//            // No BT headset and NO headset
//            audioManager.setBluetoothScoOn(false);
//            audioManager.setSpeakerphoneOn(mIsOutputToSpeaker);
//            AppPara.onHeadSetPlugin(0);
//            Log.i("AudioMgr","hasHeadSet:" + hasHeadSet + " isBluetoothOn:" +isBluetoothOn+" output2Speaker:"+mIsOutputToSpeaker);
//		}
//	}

	public static void setAudioFrameCallback(AudioFrameCallbackInterface callback) {
		YouMeAudioCallback.callback = callback;
	}
}
