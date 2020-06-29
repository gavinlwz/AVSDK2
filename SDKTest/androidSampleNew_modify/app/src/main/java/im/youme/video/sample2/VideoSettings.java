package im.youme.video.sample2;

import android.content.Context;
import android.content.SharedPreferences;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.EditText;
import android.widget.Switch;

///参数设置界面
public class VideoSettings extends AppCompatActivity {
    private EditText mVideoWidth;
    private EditText mVideoHeight;
    private EditText mShareWidth;
    private EditText mShareHeight;
    private EditText mVideoFPS;
    private EditText mReportInterval;
    private EditText mMaxBitRate;
    private EditText mMinBitRate;
    private EditText mFarendLevel;

    private Switch mQualitySwitch;
    private Switch mbHWEnableSwitch;
//    private Switch mBeautifySwitch;
    private Switch mTCPSwitch;
    private Switch mLandscapeSwitch;
    private Switch mTestmodeSwitch;
    private Switch mVBRSwitch;
    private Switch mTestP2P;
    private Switch mMinDelay;//低延迟模式开关


    public static int getValue( String str , int defaultValue )
    {
        int value = defaultValue;
        try{
            value = Integer.parseInt( str );
        }
        catch ( Exception e  )
        {

        }

        return value;
    }

    private void initComponent(){
        SharedPreferences sharedPreferences = getSharedPreferences("demo_settings", Context.MODE_PRIVATE);
        mVideoWidth = (EditText)findViewById(R.id.editText_videoWidth);
        mVideoHeight = (EditText)findViewById(R.id.editText_videoHeight);
        mVideoFPS =  (EditText)findViewById(R.id.editText_videoFps);
        mReportInterval = (EditText)findViewById(R.id.editText_reportInterval);
        mMaxBitRate = (EditText)findViewById(R.id.editText_maxBitRate);
        mMinBitRate = (EditText)findViewById( R.id.editText_minBitRate);
        mFarendLevel = (EditText)findViewById(R.id.editText_farendLevel);
        mShareWidth=(EditText)findViewById(R.id.editText_shareWidth);
        mShareHeight=(EditText)findViewById(R.id.editText_shareHeight);
        mQualitySwitch = (Switch)findViewById(R.id.switch_audioQuality);
        mbHWEnableSwitch = (Switch)findViewById(R.id.switch_bHWEnable);
//        mBeautifySwitch =  (Switch)findViewById( R.id.switch_beautify );
        mTCPSwitch =  (Switch)findViewById( R.id.switch_tcp );
        mLandscapeSwitch =  (Switch)findViewById( R.id.switch_landscape );
        mTestmodeSwitch = (Switch)findViewById( R.id.switch_testmode );
        mVBRSwitch = (Switch)findViewById( R.id.switch_vbr );
        mMinDelay = (Switch)findViewById(R.id.switch_min_delay);
//        mTestP2P=(Switch)findViewById(R.id.switch_testP2P);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.activity_video_set);
        initComponent();
    }


    //点击确定按钮的响应
    public void onConfirmClick(View view){
        SharedPreferences sharedPreferences = getSharedPreferences("demo_settings", Context
                .MODE_PRIVATE);
        //VideoCapturerActivity._serverIp = mServerIp.getText().toString();

        VideoCapturerActivity._videoWidth = getValue(mVideoWidth.getText().toString(), 240 );
        VideoCapturerActivity._videoHeight = getValue(mVideoHeight.getText().toString(), 320 );
        VideoCapturerActivity._fps = getValue(mVideoFPS.getText().toString(), 15 );
        VideoCapturerActivity._reportInterval = getValue(mReportInterval.getText().toString(), 5000 );
        VideoCapturerActivity._maxBitRate = getValue(mMaxBitRate.getText().toString(), 0 );
        VideoCapturerActivity._minBitRate = getValue(mMinBitRate.getText().toString(), 0 );
        VideoCapturerActivity._farendLevel = getValue(mFarendLevel.getText().toString(), 10 );
        VideoCapturerActivity._bHighAudio = mQualitySwitch.isChecked();
        VideoCapturerActivity._bHWEnable  = mbHWEnableSwitch.isChecked();
//        VideoCapturerActivity._bBeautify = mBeautifySwitch.isChecked();
        VideoCapturerActivity._bTcp  = mTCPSwitch.isChecked();
        VideoCapturerActivity._bLandscape = mLandscapeSwitch.isChecked();
        VideoCapturerActivity._bTestmode = mTestmodeSwitch.isChecked();
        VideoCapturerActivity._bVBR = mVBRSwitch.isChecked();
        VideoCapturerActivity._minDelay = mMinDelay.isChecked();
        VideoCapturerActivity._bTestP2P=false;//mTestP2P.isChecked();

        VideoCapturerActivity.mVideoShareWidth = getValue(mShareWidth.getText().toString(),720);
        VideoCapturerActivity.mVideoShareHeight = getValue(mShareHeight.getText().toString(),1280);

        VideoSettings.this.finish();
    }
}
