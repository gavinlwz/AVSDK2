package im.youme.video.sample2;


import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.text.InputType;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.inputmethod.EditorInfo;
import android.widget.AdapterView;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Spinner;


import com.youme.voiceengine.YouMeConst;
import com.youme.voiceengine.mgr.YouMeManager;


/**
 * Created by lingguodong on 2018/3/19.
 * 这个是ViewDemo的初始界面
 */

public class Video extends AppCompatActivity {
    private Toolbar toolbar;
    private Button start_button;
    private EditText sessionid_edittext1;

    public String local_user_id;
    public String currentRoomID;
    public int area;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        Context context = getApplicationContext();

        //========= load youme so =========
        YouMeManager.Init(this); //这里需要传this参数，需要Activity实例，使用时用来请求摄像头和麦克风权限

        super.onCreate(savedInstanceState);
        if ((getIntent().getFlags() & Intent.FLAG_ACTIVITY_BROUGHT_TO_FRONT) != 0) {
            finish();
            return;
        }

        setContentView(R.layout.video);
        toolbar = (Toolbar) findViewById(R.id.toolbar);
        toolbar.setTitle("  VideoDemo");
        setSupportActionBar(toolbar);

        local_user_id = "an"+(int) (Math.random() * 1000000);// +"_" + (int) (Math.random() * 1000000)+"_1";
        currentRoomID = "";
        area = 0;

        //找到房间ID输入组件
        sessionid_edittext1 = (EditText) findViewById(R.id.sessionid_edittext1);
        sessionid_edittext1.setInputType(InputType.TYPE_TEXT_VARIATION_EMAIL_ADDRESS);
        sessionid_edittext1.setImeOptions(EditorInfo.IME_ACTION_DONE);
        start_button = (Button) findViewById(R.id.start_button);
        start_button.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                currentRoomID = sessionid_edittext1.getText().toString().trim();
                //roomId不为空
                if (currentRoomID.length() >= 1) {
                    //跳转渲染视频源
                    goVideoDemo();
                }
            }
        });
    }

    private void goVideoDemo() {
        Intent intent = new Intent(Video.this, VideoCapturerActivity.class);
        intent.putExtra("userid", local_user_id);
        intent.putExtra("roomid", currentRoomID);
        intent.putExtra("Area", area);
        startActivity(intent);
    }

    /**
     * 该方法是用来加载菜单布局的
     *
     * @param menu
     * @return
     */
    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        //加载菜单文件
        getMenuInflater().inflate(R.menu.main, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case R.id.action_settings:
                Intent intent = new Intent();
                intent.setClass(Video.this, VideoSettings.class);
                startActivity(intent);
                return true;
            default:
                return super.onOptionsItemSelected(item);
        }
    }

    @Override
    protected void onDestroy() {
        YouMeManager.Uninit();
        super.onDestroy();
    }
}
