package com.touchtech.video;

import android.animation.AnimatorSet;
import android.animation.ObjectAnimator;
import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.ActivityInfo;
import android.graphics.PixelFormat;
import android.hardware.Camera;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.OrientationEventListener;
import android.view.ScaleGestureDetector;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.ImageButton;
import android.widget.RelativeLayout;
import android.widget.TextView;

import androidx.annotation.IdRes;

import com.touchtech.utils.ToastMessage;
import com.youme.voiceengine.CameraMgr;
import com.youme.voiceengine.MemberChange;
import com.youme.voiceengine.NativeEngine;
import com.youme.voiceengine.ScreenRecorder;
import com.youme.voiceengine.VideoRenderer;
import com.youme.voiceengine.YouMeCallBackInterface;
import com.youme.voiceengine.YouMeCallBackInterfacePcm;
import com.youme.voiceengine.YouMeConst;
import com.youme.voiceengine.api;
import com.youme.voiceengine.video.EglBase;
import com.youme.voiceengine.video.RendererCommon;
import com.youme.voiceengine.video.SurfaceViewRenderer;
import com.youme.webrtc.PercentFrameLayout;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.lang.ref.WeakReference;
import java.util.HashMap;
import java.util.Map;
import java.util.Timer;
import java.util.TimerTask;


public class VideoCapturerActivity extends Activity implements YouMeCallBackInterface {

    public class RenderInfo {
        public String userId;
        public int rendViewIndex;
        public boolean RenderStatus;
        public int userIndex;
    }

    private static String TAG = "VideoCapturerActivity";

    ///声明video设置块相关静态变量
    public static int _videoWidth = 480;
    public static int _videoHeight = 640;
    public static boolean _bHWEnable = true;
    private int child_width = 240;
    private int child_height = 320;

    public int previewFps = 24;
    public static int _fps = 20;
    private int fpsChild = 12;
    private ViewGroup mViewGroup;
    private ImageButton micBtn;
    private ImageButton speakerBtn;

    public boolean inited = false;

    private SurfaceViewRenderer[] arrRenderViews = null;
    private static boolean lastTestmode = false;

    private MyHandler youmeVideoEventHandler;

    private TextView avTips = null;
    private Map<String, RenderInfo> renderInfoMap = null;

    private int[] m_UserViewIndexEn = {0, 0, 0, 0, 0, 0, 0};

    private String local_user_id;
    private String currentRoomID;
    private int areaId;

    int local_render_id = -1;
    int mUserCount = 0;
    boolean isP2P = false;
    boolean useUDPToSendCustomMessage = true;

    private boolean isJoinedRoom = false;
    private float beautifyLevel = 0.5f;

    private int mFullScreenIndex = -1;
    private boolean isOpenCamera = false;
    private boolean needResumeCamera = false;
    private boolean micLock = false;
    private boolean isOpenShare = false;
    private boolean isPreviewMirror = true;

    public static int mVideoShareWidth = 1080;
    public static int mVideoShareHeight = 1920;

    private Intent forgroundIntent;

    private boolean activity;

    //private int[] mAVStatistic = new int[20];
    private HashMap<String, int[]> avStaticMap = new HashMap<>();
    private Map<String, String> userLeaveMap = new HashMap<>();
    private Map<String, String> maskUsersMap = new HashMap<>();
    ; //被屏蔽的用户列表缓存

    private float mScaleFactor = 0.0f;
    private float mLastZoomFactor = 1.0f;
    private ScaleGestureDetector mScaleGestureDetector;
    private GestureDetector mGestureDetector;
    private View mFocusView;
    private Timer resetTimer;
    private int viewMaxCount = 4;


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.activity_video_capturer);
//        initCtrls();

        //取得启动该Activity的Intent对象
        Intent intent1 = getIntent();
        local_user_id = intent1.getStringExtra("userid");
        currentRoomID = intent1.getStringExtra("roomid");
        areaId = intent1.getIntExtra("Area", 0);

        ///初始化界面相关数据
        renderInfoMap = new HashMap<>();
        arrRenderViews = new SurfaceViewRenderer[viewMaxCount];
        getWindow().setFormat(PixelFormat.TRANSLUCENT);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);   //应用运行时，保持屏幕高亮，不锁屏

        youmeVideoEventHandler = new MyHandler(this);
        avTips = (TextView) findViewById(R.id.avtip);

        mFocusView = findViewById(R.id.camera_focus);

        //定时重置avstatic 统计信息
        resetTimer = new Timer();
        resetTimer.schedule(new TimerTask() {
            @Override
            public void run() {
                for (Map.Entry<String, int[]> entry : avStaticMap.entrySet()) {
                    String userid = entry.getKey();
                    int[] mAVStatistic = entry.getValue();
                    for (int i = 0; i < 15; i++) {
                        mAVStatistic[i] = 0;
                    }
                }
            }
        }, 10000, 10000);

        initSDK();
    }

    private OrientationReciver mOrientationReciver;

    private void addOrientationListener() {
        removeOrientationListener();

        mOrientationReciver = new OrientationReciver();
        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction("android.intent.action.CONFIGURATION_CHANGED");
        registerReceiver(mOrientationReciver, intentFilter);
    }

    private void removeOrientationListener() {
        if (mOrientationReciver != null)
            unregisterReceiver(mOrientationReciver);
    }

    private class OrientationReciver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            int rotation = VideoCapturerActivity.this.getWindowManager().getDefaultDisplay().getRotation() * 90;
            Log.i("Orientation", "onReceive Orientation: " + rotation);
            if (rotation == 90 || rotation == 270) {
                ScreenRecorder.orientationChange(rotation, mVideoShareHeight, mVideoShareWidth);
                api.setVideoNetResolutionForShare(mVideoShareHeight, mVideoShareWidth);
            } else {
                ScreenRecorder.orientationChange(rotation, mVideoShareWidth, mVideoShareHeight);
                api.setVideoNetResolutionForShare(mVideoShareWidth, mVideoShareHeight);
            }
        }
    }

    private int xDelta;
    private int yDelta;

//    @Override
//    public boolean onTouch(View view, MotionEvent event) {
//        final int x = (int) event.getRawX();
//        final int y = (int) event.getRawY();
//        Log.d(TAG, "onTouch: x= " + x + "y=" + y);
//        moveControl(event.getAction() & MotionEvent.ACTION_MASK, x, y, false);
//        return true;
//    }

//    public void moveControl(int event, int x, int y, boolean fromRemote) {
//        String message = "";
//        switch (event) {
//            case MotionEvent.ACTION_DOWN:
//                RelativeLayout.LayoutParams params = (RelativeLayout.LayoutParams) mButton
//                        .getLayoutParams();
//                if (!fromRemote) {
//                    xDelta = x - params.leftMargin;
//                    yDelta = y - params.topMargin;
//                    message = event + "|" + params.leftMargin + "|" + params.topMargin;
//                } else {
//                    //对齐远端点击时的位置
//                    RelativeLayout.LayoutParams layoutParams = (RelativeLayout.LayoutParams) mButton
//                            .getLayoutParams();
//                    layoutParams.leftMargin = x;
//                    layoutParams.topMargin = y;
//                    mButton.setLayoutParams(layoutParams);
//                }
//
//                Log.d(TAG, "ACTION_DOWN: xDelta= " + xDelta + "yDelta=" + yDelta);
//                break;
//            case MotionEvent.ACTION_MOVE:
//                RelativeLayout.LayoutParams layoutParams = (RelativeLayout.LayoutParams) mButton
//                        .getLayoutParams();
//                int xDistance = x - xDelta;
//                int yDistance = y - yDelta;
//                Log.d(TAG, "ACTION_MOVE: xDistance= " + xDistance + "yDistance=" + yDistance);
//                layoutParams.leftMargin = xDistance;
//                layoutParams.topMargin = yDistance;
//                mButton.setLayoutParams(layoutParams);
//                message = event + "|" + x + "|" + y;
//                break;
//        }
//        if (!fromRemote) {
//            if (useUDPToSendCustomMessage) {
//                api.inputCustomData(message.getBytes(), message.getBytes().length, System.currentTimeMillis());
//            } else {
//                api.sendMessage(currentRoomID, message);
//            }
//        }
//        mViewGroup.invalidate();
//    }

//    @Override
//    public void onClick(View v) {
//
//    }


    private void initSDK() {
        ///初始化SDK相关设置
        api.setServerMode(0);
        api.setLogLevel(YouMeConst.YOUME_LOG_LEVEL.LOG_INFO, YouMeConst.YOUME_LOG_LEVEL.LOG_INFO);
        api.SetCallback(this);
//        api.setCameraAutoFocusCallBack(cameraFocusCallback);
//        int code = api.init(CommonDefines.appKey, CommonDefines.appSecret, areaId, "");

        VideoRendererSample.getInstance().setLocalUserId(local_user_id);
        api.setVideoFrameCallback(VideoRendererSample.getInstance());
        int code = api.connectServer(CommonDefines.appKey, CommonDefines.appSecret, 0, "");
        if (code == YouMeConst.YouMeErrorCode.YOUME_ERROR_WRONG_STATE) {
            //已经初始化过了，就不等初始化回调了，直接进频道就行
            joinClick();
            inited = true;
        }
    }


    private void initRender(int index, @IdRes int layoutId, @IdRes int viewId) {
        if (index < 0 || index > arrRenderViews.length || arrRenderViews[index] != null) {
            return;
        }
        SurfaceViewRenderer renderView = (SurfaceViewRenderer) this.findViewById(viewId);
        if (index != 6) {
            PercentFrameLayout layout = (PercentFrameLayout) this.findViewById(layoutId);
            layout.setPosition(0, 0, 100, 100);
        } else {
            renderView.setVisibility(View.INVISIBLE);
        }
        renderView.init(EglBase.createContext(api.sharedEGLContext()), null);
        renderView.setScalingType(RendererCommon.ScalingType.SCALE_ASPECT_FIT);
        renderView.setMirror(false);
        arrRenderViews[index] = renderView;
    }

    private void releaseRender() {
        for (int i = 0; i < arrRenderViews.length; i++) {
            if (arrRenderViews[i] != null) {
                arrRenderViews[i].release();
            }
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        resetTimer.cancel();
        if (forgroundIntent != null) {
            try {
                this.stopService(forgroundIntent);
            } catch (Throwable e) {
                e.printStackTrace();
            }
        }
        removeOrientationListener();
        releaseRender();
        ScreenRecorder.stopScreenRecorder();
        VideoRendererSample.getInstance().deleteAllRender();
    }

    private void startMic() {
        api.setMicrophoneMute(false);
    }

    private void stopMic() {
        api.setMicrophoneMute(true);
    }

    void setZoomFactor(float factor) {

        mLastZoomFactor = factor;
    }

    private void startCamera() {
        api.startCapturer();
        isOpenCamera = true;
    }

    private void stopCamera() {
        api.stopCapturer();
        isOpenCamera = false;
    }

    private void switchCamera() {
        api.switchCamera();
    }

    private void switchRotation() {
        String UserId = IndexToUserId(mFullScreenIndex);
        if (UserId != null) {
            VideoRendererSample.getInstance().switchRotation(UserId);
        }
    }

    private void resetAllRender() {
        for (RenderInfo info : renderInfoMap.values()) {
            VideoRenderer.getInstance().deleteRender(info.userId);

            SurfaceViewRenderer renderView = getRenderViewByIndex(info.rendViewIndex);

            if (renderView != null) {
                renderView.clearImage();
            }
        }

        renderInfoMap.clear();
        for (int i = 0; i < m_UserViewIndexEn.length; i++) {
            m_UserViewIndexEn[i] = 0;
        }

        //清理自己/合流的视频显示
        {
            if (local_render_id != -1) {
                VideoRenderer.getInstance().deleteRender(local_user_id);
            }

            SurfaceViewRenderer renderView = getRenderViewByIndex(6);
            if (renderView != null) {
                renderView.clearImage();
            }
        }

    }

    private void resetStatus() {
//    api.setCaptureFrontCameraEnable(true);
        stopCamera();
        startCamera();
    }

    private boolean RTCServiceStarted = false;

    protected void onPause() {
        // 放到后台时完全暂停: api.pauseChannel();
        if (!activity) {
            api.setVideoFrameCallback(null);
            needResumeCamera = isOpenCamera;
            //stopCamera();
            activity = true;
        }
//        if (api.isInChannel() && !RTCServiceStarted) {
//            try {
//                RTCService.mContext = getApplicationContext();
//                RTCService.mActivity = this;
//                if (RTCService.mContext != null && RTCService.mActivity != null) {
//                    forgroundIntent = new Intent(this, RTCService.class);
//                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
//                        this.startForegroundService(forgroundIntent);
//                    } else {
//                        this.startService(forgroundIntent);
//                    }
//                    RTCServiceStarted = true;
//                }
//            } catch (Throwable e) {
//                RTCServiceStarted = false;
//                e.printStackTrace();
//            }
//        }
//    Object[] obj = renderInfoMap.values().toArray(new Object[renderInfoMap.values().size()]);
//    for (int i= obj.length - 1 ;i > -1; i--) {
//        shutDOWN(((RenderInfo)obj[i]).userId);
//    }
        super.onPause();
    }

    protected void onResume() {
        //设置横屏
        if (_bLandscape && getRequestedOrientation() != ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE) {
            Log.d(TAG, "rotation:" + this.getWindowManager().getDefaultDisplay().getRotation());
//            setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
            Log.d(TAG, "rotation end:" + this.getWindowManager().getDefaultDisplay().getRotation());
            api.setScreenRotation(this.getWindowManager().getDefaultDisplay().getRotation());
        }

        super.onResume();

        //切回前台的时候恢复
        //   api.resumeChannel();
        if (activity) {
            if (needResumeCamera) startCamera();
            needResumeCamera = false;
            api.setVideoFrameCallback(VideoRendererSample.getInstance());
            activity = false;
        }

        if (forgroundIntent != null) {
            RTCServiceStarted = false;
            try {
                this.stopService(forgroundIntent);
            } catch (Throwable e) {
                e.printStackTrace();
            }
        }

//    Object[] obj = renderInfoMap.values().toArray(new Object[renderInfoMap.values().size()]);
//    for (int i= obj.length - 1 ;i > -1; i--) {
//        shutDOWN(((RenderInfo)obj[i]).userId);
//    }
    }

    private SurfaceViewRenderer getRenderViewByIndex(int index) {
        if (index < 0 || index > arrRenderViews.length) {
            return null;
        } else {
            return arrRenderViews[index];
        }
    }


    private void updateNewView(final String newUserId, final int index) {
        final SurfaceViewRenderer renderView = getRenderViewByIndex(index);
        if (renderView == null) {
            return;
        }

        renderView.setVisibility(View.VISIBLE);
        if (newUserId != local_user_id) {
            renderView.setZOrderOnTop(true);
        }
        //renderView.clearImage();

        VideoRendererSample.getInstance().addRender(newUserId, renderView);
        RenderInfo info = new RenderInfo();
        info.userId = newUserId;
        info.rendViewIndex = index;
        info.RenderStatus = true;
        info.userIndex = ++mUserCount;
        renderInfoMap.put(newUserId, info);
        m_UserViewIndexEn[index] = 1;

    }


    /// 回调数据
    @Override
    public void onEvent(int event, int error, String room, Object param) {
        ///里面会更新界面，所以要在主线程处理
        Log.i(TAG, "event:" + CommonDefines.CallEventToString(event) + ", error:" + error + ", room:" + room + ",param:" + param);
        Message msg = new Message();
        Bundle extraData = new Bundle();
        extraData.putString("channelId", room);
        msg.what = event;
        msg.arg1 = error;
        msg.obj = param;
        msg.setData(extraData);

        youmeVideoEventHandler.sendMessage(msg);
    }

    @Override
    public void onRequestRestAPI(int requestID, int iErrorCode, String strQuery, String strResult) {

    }

    @Override
    public void onMemberChange(String channelID, final MemberChange[] arrChanges, boolean isUpdate) {
        /**
         * 离开频道时移除该对象的聊天画面
         */
        Log.i(TAG, "onMemberChange:" + channelID + ",isUpdate:" + isUpdate);
        this.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                //此时已在主线程中，可以更新UI了
                for (int i = 0; i < arrChanges.length; i++) {
                    if (arrChanges[i].isJoin == false) {
                        userLeaveMap.put(String.valueOf(arrChanges[i].userID), "");
                        String leaveUserId = String.valueOf(arrChanges[i].userID);
                        shutDOWN(leaveUserId);
                        shutDOWN(leaveUserId + "_share");
                        if (avStaticMap.get(leaveUserId) != null) {
                            avStaticMap.remove(leaveUserId);
                        }
                    } else {
                        if (userLeaveMap.containsKey(String.valueOf(arrChanges[i].userID))) {
                            userLeaveMap.remove(String.valueOf(arrChanges[i].userID));
                        }
                        String[] userArray = new String[1];
                        int[] resolutionArray = new int[1];

                        userArray[0] = String.valueOf(arrChanges[i].userID);
                        ;
                        resolutionArray[0] = 1;
                        api.setUsersVideoInfo(userArray, resolutionArray);
                    }
                }
            }
        });
    }

    @Override
    public void onBroadcast(int bc, String room, String param1, String param2, String content) {

    }

    @Override
    public void onAVStatistic(final int avType, final String strUserID, final int value) {

        if (youmeVideoEventHandler != null) {
            youmeVideoEventHandler.post(new Runnable() {
                @Override
                public void run() {
                    if (avStaticMap.get(strUserID) == null) {
                        avStaticMap.put(strUserID, new int[15]);
                    }
                    int[] tmpAVStatistic = avStaticMap.get(strUserID);
                    tmpAVStatistic[avType] = value;
                    String tips = "";
                    for (Map.Entry<String, int[]> entry : avStaticMap.entrySet()) {
                        String userid = entry.getKey();
                        int[] mAVStatistic = entry.getValue();
                        String direction = userid.endsWith(local_user_id) ? "Up" : "Down";
                        tips += (userid.endsWith(local_user_id) ? userid + "(self)" : userid) + "\n" +
                                "FPS: " + (mAVStatistic[3]) + "\n" +
                                "audio" + direction + ": " + mAVStatistic[1] * 8 / 1000 + "kbps\n" +
                                "video" + direction + ": " + mAVStatistic[2] * 8 / 1000 + "kbps\n" +
                                "UpLoss: " + mAVStatistic[6] + "‰ \n" +
                                "DownLoss: " + mAVStatistic[7] + "‰ \n" +
                                (!userid.endsWith(local_user_id) ? "rtt: " + mAVStatistic[9] + "ms \n" : "") +
                                (userid.endsWith(local_user_id) ? "" : "block: " + mAVStatistic[10] + " \n") +
                                (userid.endsWith(local_user_id) ? "DownBW: " + mAVStatistic[13] * 8 / 1000 + "kbps\n" : "") +
                                (userid.endsWith(local_user_id) ? "connect:" + (isP2P ? "p2p" : "server") + "\n" : "");
                    }
                    avTips.setText(tips);

                }
            });
        }

    }

    @Override
    public void onTranslateTextComplete(int errorcode, int requestID, String text, int srcLangCode, int destLangCode) {

    }

//    @Override
//    public void onVideoPreDecode(String userId, byte[] data, int dataSizeInByte, long timestamp) {
//        Log.i(TAG, "onVideoPreDecode:" + userId + ", ts:" + timestamp);
//        if (DEBUG) {
//            if (mPreDecodeFos != null) {
//                try {
//                    mPreDecodeFos.write(data);
//                } catch (IOException e) {
//                    e.printStackTrace();
//                }
//            }
//        }
//    }

    public void joinOK(String roomid) {
        currentRoomID = roomid;
        if (!isJoinedRoom) {
            Log.d(TAG, "进频道成功");
            //api.maskVideoByUserId(userid,block);
            //进频道成功后可以设置视频回调
            api.SetVideoCallback();
            //设置远端语音音量回调
            //开启扬声器
            api.setSpeakerMute(false);

            isJoinedRoom = true;
            //远端视频渲染的view
            initRender(0, R.id.PercentFrameLayout0, R.id.SurfaceViewRenderer0);
            initRender(1, R.id.PercentFrameLayout1, R.id.SurfaceViewRenderer1);
            initRender(2, R.id.PercentFrameLayout2, R.id.SurfaceViewRenderer2);
            //本地视频
            initRender(3, R.id.remote_video_view_twelve1, R.id.remote_video_view_twelve1);

            VideoRendererSample.getInstance().setLocalUserId(local_user_id);
            updateNewView(local_user_id, 3);
            autoOpenStartCamera();
            mFullScreenIndex = 3;
        }
    }

    public void nextViewShow() {

        int validIndex = -1;
        int minIndex = 999999;
        String userId = "";

        for (int i = 0; i < 6; i++) {
            if (m_UserViewIndexEn[i] == 0) {
                validIndex = i;
                break;
            }
        }
        if (validIndex == -1)
            return;

        for (RenderInfo vaule : renderInfoMap.values()) {
            if (!vaule.RenderStatus && vaule.userIndex <= minIndex) {
                minIndex = vaule.userIndex;
                userId = vaule.userId;
            }
        }

        if (minIndex != 999999) {
            renderInfoMap.remove(userId);
            updateNewView(userId, validIndex);
        }

    }

    public void shutDOWN(String userId) {

        RenderInfo info = renderInfoMap.get(userId);
        if (info == null)
            return;

        if (info.RenderStatus) {
            if (mFullScreenIndex == info.rendViewIndex) {
                switchFullScreen(6);
            }
            final SurfaceViewRenderer renderView = getRenderViewByIndex(info.rendViewIndex);
            renderView.clearImage();
            renderView.setVisibility(View.INVISIBLE);
            VideoRendererSample.getInstance().deleteRender(userId);
            api.deleteRenderByUserID(userId);//不移除可能收不到VIDEO_ON事件
            m_UserViewIndexEn[info.rendViewIndex] = 0;
            nextViewShow();
        }
        renderInfoMap.remove(userId);

        //有人退出，接触屏蔽，触发重新分配
        for (Map.Entry<String, String> entry : maskUsersMap.entrySet()) {
            api.deleteRenderByUserID(entry.getKey());//不移除可能收不到VIDEO_ON事件
            api.maskVideoByUserId(entry.getKey(), false);
        }
        maskUsersMap.clear();

    }


    public void videoON(String userId) {
        Log.d(TAG, "新加的user ID=" + userId);
        // 默认请求小流
        onVideoSwitch(userId, 1);

        int validIndex = -1;
        if (renderInfoMap.containsKey(userId))
            return;
        if (userId.equals(local_user_id)) {
            updateNewView(local_user_id, 6);
            return;
        }
        for (int i = 0; i < 6; i++) {
            if (m_UserViewIndexEn[i] == 0) {
                validIndex = i;
                break;
            }
        }
        if (validIndex != -1) {
            api.maskVideoByUserId(userId, false);
            updateNewView(userId, validIndex);
        } else {
            if (userId.indexOf("_share") < 0) {
                //显示不下，就屏蔽了
                api.maskVideoByUserId(userId, true);
                maskUsersMap.put(userId, userId);
            }
        }

    }

//    @Override
//    public void onRecvCustomData(byte[] bytes, long l) {
//        //模拟
//        Message msg = new Message();
//        Bundle extraData = new Bundle();
//        extraData.putString("channelId", "");
//        msg.what = 999999; //模拟成event事件
//        msg.arg1 = 0;
//        msg.obj = new String(bytes);
//        msg.setData(extraData);
//
//        youmeVideoEventHandler.sendMessage(msg);
//    }


    private static class MyHandler extends Handler {

        private final WeakReference<VideoCapturerActivity> mActivity;

        public MyHandler(VideoCapturerActivity activity) {
            mActivity = new WeakReference<VideoCapturerActivity>(activity);
        }

        @Override
        public void handleMessage(Message msg) {
            VideoCapturerActivity activity = mActivity.get();
            if (activity != null) {
                super.handleMessage(msg);
                switch (msg.what) {
                    case YouMeConst.YouMeEvent.YOUME_EVENT_INIT_OK:
                        Log.d(TAG, "初始化成功");
                        ToastMessage.showToast(mActivity.get(), "初始化成功", 1000);
                        activity.joinClick();
                        activity.inited = true;
                        break;
                    case YouMeConst.YouMeEvent.YOUME_EVENT_INIT_FAILED:
                        Log.d(TAG, "初始化失败");
                        ToastMessage.showToast(mActivity.get(), "初始化失败，重试", 3000);
                        activity.inited = false;
                        mActivity.get().initSDK();
                        break;
                    case YouMeConst.YouMeEvent.YOUME_EVENT_JOIN_OK:
                        ToastMessage.showToast(mActivity.get(), "已经进入通话频道", 1000);
                        String roomId = msg.getData().getString("channelId");
                        activity.joinOK(roomId);
                        TimerTask task = new TimerTask() {
                            public void run() {
                                // 停止播放铃声
                                //BackgroundMusic.getInstance(null).stopBackgroundMusic();
                            }
                        };
                        Timer timer = new Timer();
                        timer.schedule(task, 1000);
                        break;
                    case YouMeConst.YouMeEvent.YOUME_EVENT_JOIN_FAILED:
                        ToastMessage.showToast(mActivity.get(), "已经进入通话频道失败:" + msg.arg1, 1500);
                        break;
                    case YouMeConst.YouMeEvent.YOUME_EVENT_LEAVED_ALL:
                        api.removeMixAllOverlayVideo();
                        activity.leavedUI();
                        break;
                    case YouMeConst.YouMeEvent.YOUME_EVENT_OTHERS_VIDEO_INPUT_START:
                        Log.d(TAG, "YOUME_EVENT_OTHERS_VIDEO_INPUT_START");
                        String userID = String.valueOf(msg.obj);
                        String[] userArray = {userID};
                        int[] resolutionArray = {1};
                        api.setUsersVideoInfo(userArray, resolutionArray);
                        break;
                    case YouMeConst.YouMeEvent.YOUME_EVENT_OTHERS_VIDEO_ON: {
                        Log.d(TAG, "YOUME_EVENT_OTHERS_VIDEO_ON:" + msg.obj);
                        String newUserId1 = String.valueOf(msg.obj);
                        if (newUserId1.indexOf("_share") > -1) {
                            newUserId1 = newUserId1.substring(0, newUserId1.length() - 6);
                        }
                        if (activity.userLeaveMap.containsKey(newUserId1)) {
                            api.deleteRenderByUserID(String.valueOf(msg.obj));//不移除可能收不到VIDEO_ON事件
                            Log.d(TAG, "YOUME_EVENT_OTHERS_VIDEO_ON not found:" + newUserId1);
                        } else {
                            activity.videoON(String.valueOf(msg.obj));
                        }
                    }
                    break;
                    case YouMeConst.YouMeEvent.YOUME_EVENT_OTHERS_VIDEO_INPUT_STOP:
                        Log.d(TAG, "YOUME_EVENT_OTHERS_VIDEO_INPUT_STOP");
                        break;
                    case YouMeConst.YouMeEvent.YOUME_EVENT_OTHERS_VIDEO_SHUT_DOWN:
                        Log.d(TAG, "YOUME_EVENT_OTHERS_VIDEO_SHUT_DOWN");
                        //超过时间收不到新的画面，就先移除掉
                        String leaveUserId = String.valueOf(msg.obj);
                        activity.shutDOWN(leaveUserId);
                        break;
                    case YouMeConst.YouMeEvent.YOUME_EVENT_LOCAL_MIC_OFF:
                        if (msg.arg1 == 0) activity.micBtn.setSelected(true); // 自己关闭麦克风
                        break;
                    case YouMeConst.YouMeEvent.YOUME_EVENT_LOCAL_MIC_ON:
                        if (msg.arg1 == 0) activity.micBtn.setSelected(false); // 自己打开麦克风
                        break;
                    case YouMeConst.YouMeEvent.YOUME_EVENT_MIC_CTR_OFF:
                        activity.micLock = true;
                        activity.micBtn.setSelected(true); // 主持人关闭我的麦克风
                        break;
                    case YouMeConst.YouMeEvent.YOUME_EVENT_MIC_CTR_ON:
                        activity.micLock = false;
                        activity.micBtn.setSelected(false); // 主持人打开麦我的克风
                        break;
                    case YouMeConst.YouMeEvent.YOUME_EVENT_MASK_VIDEO_BY_OTHER_USER:
                        break;
                    case YouMeConst.YouMeEvent.YOUME_EVENT_RESUME_VIDEO_BY_OTHER_USER:
                        break;
                    case YouMeConst.YouMeEvent.YOUME_EVENT_MASK_VIDEO_FOR_USER:
                        break;
                    case YouMeConst.YouMeEvent.YOUME_EVENT_RESUME_VIDEO_FOR_USER:
                        break;
                    case YouMeConst.YouMeEvent.YOUME_EVENT_QUERY_USERS_VIDEO_INFO:
                        break;
                    case YouMeConst.YouMeEvent.YOUME_EVENT_SET_USERS_VIDEO_INFO:

                        break;
                    case YouMeConst.YouMeEvent.YOUME_EVENT_VIDEO_ENCODE_PARAM_REPORT:
                        Log.d(TAG, "YOUME_EVENT_VIDEO_ENCODE_PARAM_REPORT");
                        String param = String.valueOf(msg.obj);
                        Log.d(TAG, "param:" + param);
                        break;
                    case YouMeConst.YouMeEvent.YOUME_EVENT_RTP_ROUTE_P2P:
                        // p2p通路检测ok
                        activity.isP2P = true;
                        break;
                    case YouMeConst.YouMeEvent.YOUME_EVENT_RTP_ROUTE_SEREVER:
                        // p2p通路检测失败，根据业务需求是否退出房间 或者切换server转发
                        activity.isP2P = false;
                        break;
                    case YouMeConst.YouMeEvent.YOUME_EVENT_RTP_ROUTE_CHANGE_TO_SERVER:
                        // p2p传输过程中连接异常，切换server转发
                        activity.isP2P = false;
                        break;
                    case 10000: //avStatistic回调
                        break;
                    case YouMeConst.YouMeEvent.YOUME_EVENT_FAREND_VOICE_LEVEL:

                        break;
                }
            }
        }
    }

    public void leaveChannel() {
        api.removeMixAllOverlayVideo();
        api.leaveChannelAll();

        finish();
        Intent intent = new Intent();
        intent.setClass(VideoCapturerActivity.this, MainActivity.class);
    }

    public void leavedUI() {
        finish();
        Intent intent = new Intent();
        intent.setClass(VideoCapturerActivity.this, MainActivity.class);
    }

    @Override
    public void onBackPressed() {
        //BackgroundMusic.getInstance(this).stopBackgroundMusic();
        leaveChannel();
    }


    /**
     * 自动进入频道方法   在初始化后面使用
     */
    private void joinClick() {
        // 录屏模块初始化
//        ScreenRecorder.init(this);
//        ScreenRecorder.setResolution(mVideoShareWidth, mVideoShareHeight);
//        ScreenRecorder.setFps(_fps);

        //加入频道前进行video设置
        api.setVideoPreviewFps(previewFps);
        api.setVideoFps(_fps);
        api.setVideoLocalResolution(_videoWidth, _videoHeight);
        api.setVideoNetResolution(_videoWidth, _videoHeight);

        //NativeEngine.setVideoEncodeParamCallbackEnable(true);

        //设置视频小流分辨率
        api.setVideoFpsForSecond(fpsChild);
        api.setVideoNetResolutionForSecond(child_width, child_height);

        api.setVideoFpsForShare(_fps);
        api.setVideoNetResolutionForShare(mVideoShareWidth, mVideoShareHeight);

        api.setFarendVoiceLevelCallback(10);

        //调用这个方法来设置时间间隔
//        api.setAVStatisticInterval(_reportInterval);

        //设置视频编码比特率
//        api.setVideoCodeBitrate(_maxBitRate, _minBitRate);
//        api.setVideoCodeBitrateForShare(_maxBitRate, _minBitRate);

        //设置远端语音水平回调
//        api.setFarendVoiceLevelCallback(_farendLevel);
        //设置视屏是软编还是硬编
        api.setVideoHardwareCodeEnable(_bHWEnable);

        //同步状态给其他人
//        api.setAutoSendStatus(true);
        // 设置视频无帧渲染的等待超时时间，超过这个时间会给上层回调YOUME_EVENT_OTHERS_VIDEO_SHUT_DOWN, 单位ms
        api.setVideoNoFrameTimeout(5000);
//        api.setVBR(_bVBR);
        int sampleRate = 44100;
        int channels = 1;
//        api.setPcmCallbackEnable(mOnYouMePcm, YouMeConst.YouMePcmCallBackFlag.PcmCallbackFlag_Remote |
//                YouMeConst.YouMePcmCallBackFlag.PcmCallbackFlag_Record |
//                YouMeConst.YouMePcmCallBackFlag.PcmCallbackFlag_Mix, true, 48000, 1);

//        if (_bTestP2P) {
//            api.setLocalConnectionInfo(_LocalIP, _LocalPort, _RemoteIP, _RemotePort);
//            api.setRouteChangeFlag(true);
//        } else {
//            api.clearLocalConnectionInfo();
//        }

        api.joinChannelSingleMode(local_user_id, currentRoomID, YouMeConst.YouMeUserRole.YOUME_USER_HOST, false);
    }

    /**
     * 自动打开摄像头
     */
    private void autoOpenStartCamera() {
        //开启摄像头
        startCamera();
    }

    private void initCtrls() {

    }

//    //设置按钮监听
//    private class ButtonClickListener implements View.OnClickListener {
//        /**
//         * Called when a view has been clicked.
//         *
//         * @param v The view that was clicked.
//         */
//        @Override
//        public void onClick(View v) {
//            Log.i("按钮", "" + v.getId());
//            switch (v.getId()) {
//                case R.id.vt_btn_camera: {
//                    v.setSelected(!v.isSelected());
//                    boolean disableCamera = v.isSelected();
//                    if (!disableCamera) {
//                        startCamera();
//                    } else {
//                        stopCamera();
//                    }
//
//                }
//                break;
//
//                case R.id.vt_btn_mic: {
//                    if (!micLock) {
////                    v.setSelected(!v.isSelected());
//                        boolean disableMic = !v.isSelected();
//                        if (disableMic) {
//                            //关闭麦克风
//                            stopMic();
//                        } else {
//                            //打开麦克风
//                            startMic();
//                        }
//                    }
//                }
//                break;
//
//                case R.id.vt_btn_speaker: {
//                    v.setSelected(!v.isSelected());
//                    boolean disableSpeaker = v.isSelected();
//                    if (disableSpeaker) {
//                        //关闭声音
//                        api.setSpeakerMute(true);
//                    } else {
//                        //打开声音
//                        api.setSpeakerMute(false);
//                    }
//
//
//                }
//                break;
//
//                case R.id.vt_btn_close: {
//                    leaveChannel();
//
//                }
//                break;
//
//                case R.id.vt_btn_switch_camera: {
//                    switchCamera();
//
//                }
//                break;
//
//                case R.id.vt_btn_Render_Rotation: {
//                    switchRotation();
//                }
//                break;
//
//                case R.id.vt_btn_preview_mirror: {
//                    if (isPreviewMirror) {
//                        api.setlocalVideoPreviewMirror(false);
//                    } else {
//                        api.setlocalVideoPreviewMirror(true);
//                    }
//                    isPreviewMirror = !isPreviewMirror;
//                }
//                break;
//
//                case R.id.vt_btn_share: {
//                    if (!isOpenShare) {
//                        ScreenRecorder.startScreenRecorder();
//                        ((Button) v).setText("停止录屏");
//                    } else {
//                        ScreenRecorder.stopScreenRecorder();
//                        ((Button) v).setText("开始录屏");
//                    }
//                    isOpenShare = !isOpenShare;
//                }
//                break;
//            }
//        }
//    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == ScreenRecorder.SCREEN_RECODER_REQUEST_CODE) {
            if (resultCode == Activity.RESULT_OK) {
                ScreenRecorder.onActivityResult(requestCode, resultCode, data);
            } else {
                isOpenShare = false;
            }
        }
    }

    private String IndexToUserId(int index) {
        for (RenderInfo info : renderInfoMap.values()) {
            if (info.rendViewIndex == index) {
                return info.userId;
            }
        }
        return null;
    }

    //大小流切换
    public void onVideoSwitch(String userId, int videoId) {

        String[] userArray = new String[1];
        int[] resolutionArray = new int[1];

        userArray[0] = userId;
        resolutionArray[0] = videoId;

        api.setUsersVideoInfo(userArray, resolutionArray);
    }

    private void switchFullScreen(int index) {
        int tempIndex = mFullScreenIndex;
        if (mFullScreenIndex != -1) {
            String fullUserId = IndexToUserId(mFullScreenIndex);
            if (fullUserId != null) {
                VideoRendererSample.getInstance().deleteRender(fullUserId);
                VideoRendererSample.getInstance().deleteRender(local_user_id);
                VideoRendererSample.getInstance().addRender(fullUserId, getRenderViewByIndex(mFullScreenIndex));
                VideoRendererSample.getInstance().addRender(local_user_id, getRenderViewByIndex(6));

                // 请求小流
                onVideoSwitch(fullUserId, 1);
            }
            mFullScreenIndex = -1;
        }

        if (index != 6 && index != tempIndex) {
            String userId = IndexToUserId(index);
            if (userId != null) {
                VideoRendererSample.getInstance().deleteRender(local_user_id);
                VideoRendererSample.getInstance().deleteRender(userId);
                VideoRendererSample.getInstance().addRender(userId, getRenderViewByIndex(6));
                VideoRendererSample.getInstance().addRender(local_user_id, getRenderViewByIndex(index));
                mFullScreenIndex = index;

                // 请求大流
                onVideoSwitch(userId, 0);
            }
        }

    }

    public void onVideoViewClick(View v) {
        switch (v.getId()) {
            case R.id.SurfaceViewRenderer0:
                switchFullScreen(0);
                break;
            case R.id.SurfaceViewRenderer1:
                switchFullScreen(1);
                break;
            case R.id.SurfaceViewRenderer2:
                switchFullScreen(2);
                break;
            case R.id.remote_video_view_twelve1:
                switchFullScreen(6);
                break;
        }
    }

    Camera.AutoFocusCallback cameraFocusCallback = new Camera.AutoFocusCallback() {
        @Override
        public void onAutoFocus(final boolean success, Camera camera) {
            Log.d(TAG, "auto focus result: " + success);
            if (mFocusView.getTag() == this && mFocusView.getVisibility() == View.VISIBLE) {
                mFocusView.setVisibility(View.INVISIBLE);
            }
        }
    };
}

