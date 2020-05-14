package com.touchtech.video;

import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;

import android.content.Intent;
import android.os.Bundle;
import android.view.Menu;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;

import com.youme.voiceengine.mgr.YouMeManager;

public class MainActivity extends AppCompatActivity {
    private final String TAG = this.getApplication().getClass().getSimpleName();

    private Toolbar toolbar;
    private EditText roomIdEdit;
    private Button start_button;

    public String local_user_id;
    public String currentRoomID;
    public int choosedServerRegion;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        YouMeManager.Init(this); //这里需要传this参数，需要Activity实例，使用时用来请求摄像头和麦克风权限

        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        toolbar = findViewById(R.id.toolbar);
        toolbar.setTitle("  VideoDemo");
        setSupportActionBar(toolbar);
        roomIdEdit = findViewById(R.id.roomId);
        start_button = findViewById(R.id.start_button);
        start_button.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                switch (v.getId()) {
                    case R.id.start_button:
                        goVideoDemo();
                        break;
                }
            }
        });

        local_user_id = "an"+(int) (Math.random() * 1000000);// +"_" + (int) (Math.random() * 1000000)+"_1";
        currentRoomID = roomIdEdit.getText().toString().trim();
        choosedServerRegion = 0;
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        //加载菜单文件
        getMenuInflater().inflate(R.menu.setting, menu);
        return true;
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        YouMeManager.Uninit();
    }

    private void goVideoDemo() {
        //BackgroundMusic.getInstance(this).playBackgroundMusic("call_ringtone.ogg",false);
        Intent intent = new Intent(MainActivity.this, VideoCapturerActivity.class);
        intent.putExtra("userid", local_user_id);
        intent.putExtra("roomid", currentRoomID);
        intent.putExtra("Area", choosedServerRegion);
        startActivity(intent);
    }
}
