package im.youme.video.externalSample;

import android.app.Activity;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.graphics.BitmapFactory;
import android.os.Build;
import android.os.IBinder;
import android.support.v4.app.NotificationCompat;
import android.util.Log;


public class RTCService extends Service {

    private static final String TAG = RTCService.class.getSimpleName();
    public static final int NOTICE_ID = 110;
    public static Context mContext;
    public static Activity mActivity;

    private final String CHANNEL_ID = "YMService";
    private final String CHANNEL_NAME = "YMService";

    private final String contentSub = "";
    public static String contentTitle = "";
    public static String contentText = "";
    Notification notification;
    Notification.Builder builder;

    @Override
    public void onCreate() {
        super.onCreate();

        Log.d(TAG, "onCreate()");
        try {
            Intent notificationIntent = new Intent(Intent.ACTION_MAIN);
            notificationIntent.addCategory(Intent.CATEGORY_LEANBACK_LAUNCHER);
            notificationIntent.setClass(this, mActivity.getClass());
            /*
            TaskStackBuilder stackBuilder = TaskStackBuilder.create(this);
            stackBuilder.addNextIntentWithParentStack(notificationIntent);

            PendingIntent pendingIntent =
                    stackBuilder.getPendingIntent(0, PendingIntent.FLAG_UPDATE_CURRENT);
            */
            PendingIntent pendingIntent = PendingIntent.getActivity(this.mContext, 0,
                    notificationIntent,  PendingIntent.FLAG_UPDATE_CURRENT);
          int ic_launcher = mContext.getResources().getIdentifier("ic_launcher", "mipmap", mContext.getPackageName());
          if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            NotificationChannel chan = new NotificationChannel(CHANNEL_ID, CHANNEL_NAME, NotificationManager.IMPORTANCE_MIN);
            chan.enableLights(false);
            chan.setLockscreenVisibility(Notification.VISIBILITY_PUBLIC);
            chan.setSound(null, null);
            NotificationManager manager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
            assert manager != null;
            manager.createNotificationChannel(chan);

            builder = new Notification.Builder(this, CHANNEL_ID);
            notification = builder
              .setSmallIcon(ic_launcher)
              .setContentText(contentText)
              //.setSubText(contentSub)
              .setLargeIcon(BitmapFactory.decodeResource(getResources(), ic_launcher))
              .setContentTitle(contentTitle)
                    .setContentIntent(pendingIntent)
              .build();

          } else if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR2) {
            builder = new Notification.Builder(this);
            builder.setSmallIcon(ic_launcher)
              .setVibrate(null)
              .setVibrate(new long[]{0l})
              .setSound(null)
              .setLights(0, 0, 0)
              .setDefaults(NotificationCompat.FLAG_ONLY_ALERT_ONCE);
            builder.setContentTitle(contentTitle);
            builder.setContentText(contentText);
            builder.setContentIntent(pendingIntent);
            notification = builder.build();
          }
        }catch (Throwable e){
          e.printStackTrace();
        }

    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        Log.d(TAG, "onStartCommand()");
        try{
          if(notification != null) startForeground(NOTICE_ID, notification);
        }catch (Throwable e){
          e.printStackTrace();
        }
        return START_NOT_STICKY;
    }

    @Override
    public IBinder onBind(Intent intent) {
        Log.d(TAG, "onBind()");
        return null;
    }
    @Override
    public void onDestroy() {
        Log.d(TAG, "RTCService---->onDestroy");
        try {
          stopForeground(true);
          // 如果Service被杀死，干掉通知
          if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR2) {
            NotificationManager mManager = (NotificationManager) getSystemService(NOTIFICATION_SERVICE);
            mManager.cancel(NOTICE_ID);
          }
        }catch (Throwable e){
          e.printStackTrace();
        }finally {
          super.onDestroy();
        }

    }
}
