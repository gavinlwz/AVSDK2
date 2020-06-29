package im.youme.video.sample2;

import android.animation.AnimatorSet;
import android.animation.ObjectAnimator;
import android.app.Activity;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.graphics.PixelFormat;
import android.hardware.Camera;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.support.annotation.IdRes;
import android.util.Log;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.OrientationEventListener;
import android.view.ScaleGestureDetector;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;

import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.RelativeLayout;
import android.widget.TextView;

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

import java.io.File;
import java.io.FileOutputStream;
import java.lang.ref.WeakReference;
import java.util.HashMap;
import java.util.Map;
import java.util.Timer;
import java.util.TimerTask;

import im.youme.video.externalSample.RTCService;
import im.youme.video.videoRender.PercentFrameLayout;


public class VideoCapturerActivity extends Activity implements YouMeCallBackInterface {

    public class RenderInfo {
        public String userId;
        public int renderViewIndex;
        public boolean RenderStatus;
        public int userIndex;
    }

    private static String TAG = "VideoCapturerActivity";

    private static Boolean DEBUG = false;

    ///声明video设置块相关静态变量
    public static int _videoWidth = 480;
    public static int _videoHeight = 640;
    public static int _maxBitRate = 0;
    public static int _minBitRate = 0;
    public static int _reportInterval = 3000;
    public static int _farendLevel = 10;
    public static boolean _bHighAudio = false;
    public static boolean _bHWEnable = true;
    public static boolean _bBeautify = false;
    public static boolean _bTcp = false;
    public static boolean _bLandscape = false;
    public static boolean _bVBR = true;
    public static boolean _minDelay = true; //低延迟模式
    public static boolean _bTestmode = false;
    public static boolean _bTestP2P = false;
    public static int _fps = 24;

    public boolean inited = false;
    private ImageButton cameraBtn;
    private ImageButton micBtn;
    private ImageButton speakerBtn;
    private ImageButton switchCameraBtn;
    private ImageButton swtichRotation;
    private ImageButton previewMirror;
    private ImageButton closeBtn;

    private SurfaceViewRenderer[] arrRenderViews = null;
    private MyHandler youmeVideoEventHandler;

    private TextView avTips = null;
    private Map<String, RenderInfo> renderInfoMap = null;

    private int[] m_UserViewIndexEn = {0, 0, 0, 0, 0, 0, 0};

    String local_user_id = null;
    int local_render_id = -1;
    int mUserCount = 0;
    boolean isP2P = false;

    private boolean isJoinedRoom = false;
    private float beautifyLevel = 0.5f;

    private String currentRoomID = "";
    private int area;

    private int mFullScreenIndex = -1;
    private boolean isOpenCamera = false;
    private boolean needResumeCamera = false;
    private boolean micLock = false;
    private boolean isOpenShare = false;
    private boolean isPreviewMirror = true;

    public static int mVideoShareWidth = 1080;
    public static int mVideoShareHeight = 1920;

    private Intent forgroundIntent;

    /**
     * 该状态是用来判断当前活跃的
     */
    private boolean activity;

    private HashMap<String, int[]> avStaticMap = new HashMap<>();
    private Map<String, String> userLeaveMap = new HashMap<>();
    ; //被屏蔽的用户列表缓存

    private float mScaleFactor = 0.0f;
    private float mLastZoomFactor = 1.0f;
    private View mFocusView;
    private Timer resetTimer;

    public static boolean bTokenV3 = false;

//    OrientationEventListener mOrientationListener;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

//        RTCService.contentTitle = "YoumeVideoDemo";
//        RTCService.contentText = "运行中...";
        setContentView(R.layout.activity_video_capturer);
        initCtrls();

        //取得启动该Activity的Intent对象
        Intent intent1 = getIntent();
        local_user_id = intent1.getStringExtra("userid");
        currentRoomID = intent1.getStringExtra("roomid");
        area = intent1.getIntExtra("Area", 0);

        renderInfoMap = new HashMap<>();
        ///初始化界面相关数据
        arrRenderViews = new SurfaceViewRenderer[4];
        getWindow().setFormat(PixelFormat.TRANSLUCENT);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);   //应用运行时，保持屏幕高亮，不锁屏

        youmeVideoEventHandler = new MyHandler(this);
        CameraMgr.setCameraAutoFocusCallBack(cameraFocusCallback);

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
//        addOrientationListener();
    }

//    private OrientationReciver mOrientationReciver;
//
//    private void addOrientationListener() {
//        removeOrientationListener();
//
//        mOrientationReciver = new OrientationReciver();
//        IntentFilter intentFilter = new IntentFilter();
//        intentFilter.addAction("android.intent.action.CONFIGURATION_CHANGED");
//        registerReceiver(mOrientationReciver, intentFilter);
//    }
//
//    private void removeOrientationListener() {
//        if (mOrientationReciver != null)
//            unregisterReceiver(mOrientationReciver);
//    }
//
//    private class OrientationReciver extends BroadcastReceiver {
//        @Override
//        public void onReceive(Context context, Intent intent) {
//            int rotation = VideoCapturerActivity.this.getWindowManager().getDefaultDisplay().getRotation() * 90;
//            Log.i("Orientation", "onReceive Orientation: " + rotation);
//            if (rotation == 90 || rotation == 270) {
//                ScreenRecorder.orientationChange(rotation, mVideoShareHeight, mVideoShareWidth);
//                api.setVideoNetResolutionForShare(mVideoShareHeight, mVideoShareWidth);
//            } else {
//                ScreenRecorder.orientationChange(rotation, mVideoShareWidth, mVideoShareHeight);
//                api.setVideoNetResolutionForShare(mVideoShareWidth, mVideoShareHeight);
//            }
//        }
//    }

    private void initCtrls() {
        ButtonClickListener clickListener = new ButtonClickListener();
        cameraBtn = (ImageButton) findViewById(R.id.vt_btn_camera);
        cameraBtn.setOnClickListener(clickListener);
        micBtn = (ImageButton) findViewById(R.id.vt_btn_mic);
        micBtn.setOnClickListener(clickListener);
        speakerBtn = (ImageButton) findViewById(R.id.vt_btn_speaker);
        speakerBtn.setOnClickListener(clickListener);
        switchCameraBtn = (ImageButton) findViewById(R.id.vt_btn_switch_camera);
        switchCameraBtn.setOnClickListener(clickListener);
        swtichRotation = (ImageButton) findViewById(R.id.vt_btn_Render_Rotation);
        swtichRotation.setOnClickListener(clickListener);
        previewMirror = (ImageButton) findViewById(R.id.vt_btn_preview_mirror);
        previewMirror.setOnClickListener(clickListener);
        closeBtn = (ImageButton) findViewById(R.id.vt_btn_close);
        closeBtn.setOnClickListener(clickListener);
        avTips = (TextView) findViewById(R.id.avtip);
        mFocusView = findViewById(R.id.camera_focus);

//        Button shareBtn = (Button) findViewById(R.id.vt_btn_share);
//        shareBtn.setOnClickListener(clickListener);
//        shareBtn.setText("共享桌面");
    }

    private void initSDK() {
        String modeTips = "[正常模式]";

        ///初始化SDK相关设置
        NativeEngine.setServerMode(0);
        api.setLogLevel(YouMeConst.YOUME_LOG_LEVEL.LOG_INFO, YouMeConst.YOUME_LOG_LEVEL.LOG_INFO);
        api.SetCallback(this);
        VideoRendererSample.getInstance().setLocalUserId(local_user_id);
        api.setVideoFrameCallback(VideoRendererSample.getInstance());

        //调用初始化
        ToastMessage.showToast(this, "安全校验中: " + modeTips, 1000);
        int code = api.init(CommonDefines.appKey, CommonDefines.appSecret, area, "");
        if (code == YouMeConst.YouMeErrorCode.YOUME_ERROR_WRONG_STATE) {
            //已经初始化过了，就不等初始化回调了，直接进频道就行
            autoJoinClick();
            inited = true;
        }
    }

    private void autoJoinClick() {
        // 录屏模块初始化
//        ScreenRecorder.init(this);
//        ScreenRecorder.setResolution(mVideoShareWidth, mVideoShareHeight);
//        ScreenRecorder.setFps(_fps);

        int child_width = 160;//_videoWidth/2;
        int child_height = 224;//_videoHeight/2;
        api.setVideoNetResolutionForSecond(child_width, child_height);

        api.setPcmCallbackEnable(mOnYouMePcm, YouMeConst.YouMePcmCallBackFlag.PcmCallbackFlag_Remote |
                YouMeConst.YouMePcmCallBackFlag.PcmCallbackFlag_Record |
                YouMeConst.YouMePcmCallBackFlag.PcmCallbackFlag_Mix, true, 48000, 1);

        ToastMessage.showToast(this, "进入通话频道中...", 1000);
        api.joinChannelSingleMode(local_user_id, currentRoomID, YouMeConst.YouMeUserRole.YOUME_USER_HOST, false);
    }

    public void joinOK() {
        if (!isJoinedRoom) {
            Log.d(TAG, "进频道成功");
            isJoinedRoom = true;
            api.SetVideoCallback();
            //远端视频渲染的view
            initRender(0, R.id.PercentFrameLayout0, R.id.SurfaceViewRenderer0);
            initRender(1, R.id.PercentFrameLayout1, R.id.SurfaceViewRenderer1);
            initRender(2, R.id.PercentFrameLayout2, R.id.SurfaceViewRenderer2);
            //本地视频
            initRender(3, R.id.remote_video_view_twelve1, R.id.remote_video_view_twelve1);

            updateNewView(local_user_id, 3);
            startCamera();
            mFullScreenIndex = 3;
        }
    }

    public void nextViewShow() {

        int validIndex = -1;
        int minIndex = 999999;
        String userId = "";

        for (int i = 0; i < 3; i++) {
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
            if (mFullScreenIndex == info.renderViewIndex) {
                switchFullScreen(3);
            }
            final SurfaceViewRenderer renderView = getRenderViewByIndex(info.renderViewIndex);
            renderView.clearImage();
            renderView.setVisibility(View.INVISIBLE);
            VideoRendererSample.getInstance().deleteRender(userId);
            api.deleteRenderByUserID(userId);//不移除可能收不到VIDEO_ON事件
            m_UserViewIndexEn[info.renderViewIndex] = 0;
            nextViewShow();
        }
        renderInfoMap.remove(userId);
    }

    public void videoON(String userId) {
        Log.d(TAG, "新加的user ID=" + userId);
        if (renderInfoMap.containsKey(userId))
            return;
        if (userId.equals(local_user_id)) {
            return;
        }

        int validIndex = -1;
        for (int i = 0; i < 4; i++) {
            if (m_UserViewIndexEn[i] == 0) {
                validIndex = i;
                break;
            }
        }
        if (validIndex != -1) {
            onVideoSwitch(userId, 1);
            updateNewView(userId, validIndex);
        }
    }

    private void initRender(int index, @IdRes int layoutId, @IdRes int viewId) {
        if (index < 0 || index > arrRenderViews.length || arrRenderViews[index] != null) {
            return;
        }
        SurfaceViewRenderer renderView = (SurfaceViewRenderer) this.findViewById(viewId);
        if (index != 3) {
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

    private SurfaceViewRenderer getRenderViewByIndex(int index) {
        if (index < 0 || index >= arrRenderViews.length) {
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

        VideoRendererSample.getInstance().addRender(newUserId, renderView);
        RenderInfo info = new RenderInfo();
        info.userId = newUserId;
        info.renderViewIndex = index;
        info.RenderStatus = true;
        info.userIndex = ++mUserCount;
        renderInfoMap.put(newUserId, info);
        m_UserViewIndexEn[index] = 1;
    }

    private String IndexToUserId(int index) {
        for (RenderInfo info : renderInfoMap.values()) {
            if (info.renderViewIndex == index) {
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
                VideoRendererSample.getInstance().addRender(local_user_id, getRenderViewByIndex(3));

                // 请求小流
                onVideoSwitch(fullUserId, 1);
            }
            mFullScreenIndex = -1;
        }

        if (index != 3 && index != tempIndex) {
            String userId = IndexToUserId(index);
            if (userId != null) {
                VideoRendererSample.getInstance().deleteRender(local_user_id);
                VideoRendererSample.getInstance().deleteRender(userId);
                VideoRendererSample.getInstance().addRender(userId, getRenderViewByIndex(3));
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
                switchFullScreen(3);
                break;
        }
    }

    private void releaseRender() {
        for (int i = 0; i < arrRenderViews.length; i++) {
            if (arrRenderViews[i] != null) {
                arrRenderViews[i].release();
            }
        }
    }

    private void resetAllRender() {
        for (RenderInfo info : renderInfoMap.values()) {
            VideoRenderer.getInstance().deleteRender(info.userId);
            SurfaceViewRenderer renderView = getRenderViewByIndex(info.renderViewIndex);
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

            SurfaceViewRenderer renderView = getRenderViewByIndex(3);
            if (renderView != null) {
                renderView.clearImage();
            }
        }
    }

    private void startCamera() {
        api.startCapturer();
        isOpenCamera = true;
    }

    private void stopCamera() {
        api.stopCapturer();
        isOpenCamera = false;
    }

    private void switchRotation() {
        String UserId = IndexToUserId(mFullScreenIndex);
        if (UserId != null) {
            VideoRendererSample.getInstance().switchRotation(UserId);
        }
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
//        this.runOnUiThread(new Runnable() {
//            @Override
//            public void run() {
//                //此时已在主线程中，可以更新UI了
//                for (int i = 0; i < arrChanges.length; i++) {
//                    if (arrChanges[i].isJoin == false) {
//                        userLeaveMap.put(String.valueOf(arrChanges[i].userID), "");
//                        String leaveUserId = String.valueOf(arrChanges[i].userID);
//                        shutDOWN(leaveUserId);
//                        shutDOWN(leaveUserId + "_share");
//                        if (avStaticMap.get(leaveUserId) != null) {
//                            avStaticMap.remove(leaveUserId);
//                        }
//                    } else {
//                        if (userLeaveMap.containsKey(String.valueOf(arrChanges[i].userID))) {
//                            userLeaveMap.remove(String.valueOf(arrChanges[i].userID));
//                        }
//                        String[] userArray = new String[1];
//                        int[] resolutionArray = new int[1];
//                        userArray[0] = String.valueOf(arrChanges[i].userID);
//                        resolutionArray[0] = 1;
//                        api.setUsersVideoInfo(userArray, resolutionArray);
//                    }
//                }
//            }
//        });
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

    private YouMeCallBackInterfacePcm mOnYouMePcm = new YouMeCallBackInterfacePcm() {
        private int mPcmFrameCount = 0;

        private boolean mNeedDump = true;
        private FileOutputStream fileRemote;
        private FileOutputStream fileRecord;
        private FileOutputStream fileMix;

        public FileOutputStream createFileWriter(String fileName) {
//            String filename = getApplicationContext().getExternalCacheDir().getAbsolutePath()  + File.separator + fileName;

            String filename = getExternalFilesDir("").toString() + File.separator + fileName;
            try {
                File file = new File(filename);
                if (!file.exists()) {
                    boolean res = file.createNewFile();
                }
                FileOutputStream outFile = new FileOutputStream(filename);
                return outFile;
            } catch (Exception e) {
                Log.d(CommonDefines.LOG_TAG, "create File stream fail:" + e.getMessage());
            }
            return null;
        }

        public void dumpPcm(FileOutputStream outFile, byte[] data) {
            if (outFile != null) {
                try {
                    outFile.write(data);
                } catch (Exception e) {
                    if ((mPcmFrameCount % 500) == 0) {
                        Log.i(CommonDefines.LOG_TAG, "Pcm write Exception:" + e.getMessage());
                    }
                }
            }
        }

        @Override
        public void onPcmDataRemote(int channelNum, int samplingRateHz, int bytesPerSample, byte[] data) {
//            if( mNeedDump )
//            {
//                if( fileRemote == null )
//                {
//                    fileRemote = createFileWriter( "dump_remote.pcm");
//                }
//                dumpPcm( fileRemote, data  );
//            }
//
//
//            if ((mPcmFrameCount % 500) == 0) {
//                Log.i(CommonDefines.LOG_TAG, "Remote: ++PCM callback, channelNum:" + channelNum + ", sf:" + samplingRateHz + ", bytesPerSample:" + bytesPerSample
//                        + ", data_size:" + data.length);
//            }
//            mPcmFrameCount++;
        }

        @Override
        public void onPcmDataRecord(int channelNum, int samplingRateHz, int bytesPerSample, byte[] data) {
//            if( mNeedDump )
//            {
//                if( fileRecord == null )
//                {
//                    fileRecord = createFileWriter( "dump_record.pcm");
//                }
//                dumpPcm( fileRecord, data  );
//            }
//
//            if ((mPcmFrameCount % 500) == 0) {
//                Log.i(CommonDefines.LOG_TAG, "Record:++PCM callback, channelNum:" + channelNum + ", sf:" + samplingRateHz + ", bytesPerSample:" + bytesPerSample
//                        + ", data_size:" + data.length);
//            }
//            mPcmFrameCount++;
        }

        @Override
        public void onPcmDataMix(int channelNum, int samplingRateHz, int bytesPerSample, byte[] data) {
//            if( mNeedDump )
//            {
//                if( fileMix == null )
//                {
//                    fileMix = createFileWriter( "dump_mix.pcm");
//                }
//                dumpPcm( fileMix, data  );
//            }
//
//            if ((mPcmFrameCount % 500) == 0) {
//                Log.i(CommonDefines.LOG_TAG, "Mix:++PCM callback, channelNum:" + channelNum + ", sf:" + samplingRateHz + ", bytesPerSample:" + bytesPerSample
//                        + ", data_size:" + data.length);
//            }
//            mPcmFrameCount++;
        }
    };

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
                        Log.d(TAG, "bruce >>> 初始化成功");
                        ToastMessage.showToast(mActivity.get(), "初始化成功", 1000);
                        activity.autoJoinClick();
                        activity.inited = true;
                        break;
                    case YouMeConst.YouMeEvent.YOUME_EVENT_INIT_FAILED:
                        Log.d(TAG, "bruce >>> 初始化失败");
                        ToastMessage.showToast(mActivity.get(), "初始化失败，重试", 3000);
                        activity.inited = false;
                        mActivity.get().initSDK();
                        break;
                    case YouMeConst.YouMeEvent.YOUME_EVENT_JOIN_OK:
                        ToastMessage.showToast(mActivity.get(), "已经进入通话频道", 1000);
                        Log.d(TAG, "handleMessage: bruce >>> 加入房间成功");
                        activity.joinOK();
                        break;
                    case YouMeConst.YouMeEvent.YOUME_EVENT_JOIN_FAILED:
                        ToastMessage.showToast(mActivity.get(), "已经进入通话频道失败:" + msg.arg1, 1500);
                        Log.d(TAG, "handleMessage: bruce >>> 加入房间失败");
                        break;
                    case YouMeConst.YouMeEvent.YOUME_EVENT_LEAVED_ALL:
                        api.removeMixAllOverlayVideo();
                        activity.leavedUI();
                        break;
                    case YouMeConst.YouMeEvent.YOUME_EVENT_OTHERS_VIDEO_INPUT_START:
                        Log.d(TAG, "bruce >>> YOUME_EVENT_OTHERS_VIDEO_INPUT_START");
//                        String userID = String.valueOf(msg.obj);
//                        String[] userArray = {userID};
//                        int[] resolutionArray = {1};
//                        api.setUsersVideoInfo(userArray, resolutionArray);
                        break;
                    case YouMeConst.YouMeEvent.YOUME_EVENT_OTHERS_VIDEO_ON: {
                        Log.d(TAG, "bruce >>> YOUME_EVENT_OTHERS_VIDEO_ON:" + msg.obj);
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
                        Log.d(TAG, "bruce >>> YOUME_EVENT_OTHERS_VIDEO_INPUT_STOP");
                        break;
                    case YouMeConst.YouMeEvent.YOUME_EVENT_OTHERS_VIDEO_SHUT_DOWN:
                        Log.d(TAG, "bruce >>> YOUME_EVENT_OTHERS_VIDEO_SHUT_DOWN");
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
                    case YouMeConst.YouMeEvent.YOUME_EVENT_VIDEO_ENCODE_PARAM_REPORT:
                        Log.d(TAG, "YOUME_EVENT_VIDEO_ENCODE_PARAM_REPORT");
                        String param = String.valueOf(msg.obj);
                        Log.d(TAG, "param:" + param);
                        break;
                    default:
                        Log.d(TAG, "handleMessage: event:" + msg.what);
                        break;
                }
            }
        }
    }

    public void leaveChannel() {
        api.removeMixAllOverlayVideo();
        api.leaveChannelAll();
        leavedUI();
    }

    public void leavedUI() {
        finish();
        Intent intent = new Intent();
        intent.setClass(VideoCapturerActivity.this, Video.class);
    }

    //设置按钮监听
    private class ButtonClickListener implements View.OnClickListener {
        @Override
        public void onClick(View v) {
            Log.i("按钮", "" + v.getId());
            switch (v.getId()) {
                case R.id.vt_btn_camera: {
                    Log.d(TAG, "onClick: bruce >>> v.isSelected()=" + v.isSelected());
                    v.setSelected(!v.isSelected());
                    boolean result = v.isSelected();
                    if (!result) {
                        startCamera();
                    } else {
                        stopCamera();
                    }
                }
                break;
                case R.id.vt_btn_mic: {
                    Log.d(TAG, "onClick: bruce >>> v.isSelected()=" + v.isSelected());
                    if (!micLock) {
                        v.setSelected(!v.isSelected());
                        boolean result = v.isSelected();
                        if (result) {
                            api.setMicrophoneMute(true);
                        } else {
                            api.setMicrophoneMute(false);
                        }
                    }
                }
                break;
                case R.id.vt_btn_speaker: {
                    Log.d(TAG, "onClick: bruce >>> v.isSelected()=" + v.isSelected());
                    v.setSelected(!v.isSelected());
                    boolean disableSpeaker = v.isSelected();
                    if (disableSpeaker) {
                        //关闭声音
                        api.setSpeakerMute(true);
                    } else {
                        //打开声音
                        api.setSpeakerMute(false);
                    }
                }
                break;
                case R.id.vt_btn_close: {
                    leaveChannel();
                }
                break;
                case R.id.vt_btn_switch_camera: {
                    api.switchCamera();
                }
                break;
                case R.id.vt_btn_Render_Rotation: {
                    switchRotation();
                }
                break;
                case R.id.vt_btn_preview_mirror: {
                    if (isPreviewMirror) {
                        api.setlocalVideoPreviewMirror(false);
                    } else {
                        api.setlocalVideoPreviewMirror(true);
                    }
                    isPreviewMirror = !isPreviewMirror;
                }
                break;
                case R.id.vt_btn_share: {
                    if (!isOpenShare) {
                        ScreenRecorder.startScreenRecorder();
                        ((Button) v).setText("停止共享桌面");
                    } else {
                        ScreenRecorder.stopScreenRecorder();
                        ((Button) v).setText("开始共享桌面");
                    }
                    isOpenShare = !isOpenShare;
                }
                break;
            }
        }
    }

    //重写onTouchEvent方法 获取手势
    @Override
    public boolean onTouchEvent(MotionEvent event) {
        return super.onTouchEvent(event);
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

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == ScreenRecorder.SCREEN_RECODER_REQUEST_CODE) {
            if (resultCode == Activity.RESULT_OK) {
//                ScreenRecorder.onActivityResult(requestCode, resultCode, data);
            } else {
                isOpenShare = false;
            }
        }
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
        if (api.isInChannel() && !RTCServiceStarted) {
            try {
                RTCService.mContext = getApplicationContext();
                RTCService.mActivity = this;
                if (RTCService.mContext != null && RTCService.mActivity != null) {
                    forgroundIntent = new Intent(this, RTCService.class);
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                        this.startForegroundService(forgroundIntent);
                    } else {
                        this.startService(forgroundIntent);
                    }
                    RTCServiceStarted = true;
                }
            } catch (Throwable e) {
                RTCServiceStarted = false;
                e.printStackTrace();
            }
        }
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
            setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
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

    @Override
    public void onBackPressed() {
        leaveChannel();
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
//        removeOrientationListener();
        releaseRender();
//        ScreenRecorder.stopScreenRecorder();
        VideoRendererSample.getInstance().deleteAllRender();
    }
}

