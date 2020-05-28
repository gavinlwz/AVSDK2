package com.youme.voiceengine.mgr;

import java.io.File;
import java.io.InputStream;
import java.io.RandomAccessFile;
import java.lang.ref.WeakReference;
import java.net.HttpURLConnection;
import java.net.URL;

import android.app.Activity;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.provider.MediaStore.Images.Thumbnails;
import android.util.Log;

import com.youme.voiceengine.AppPara;
import com.youme.voiceengine.AudioMgr;
import com.youme.voiceengine.CameraMgr;
import com.youme.voiceengine.NetUtil;


/**
 * @author brucewang
 * @date 2014-1-27
 */

public class YouMeManager {

    public static String YOU_ME_LIB_NAME_STRING = "youme_voice_engine";


    private static final String TAG = "YOUME";

    /* 下载保存路径 */
    private static String mCachePath;
    private static Boolean m_bStopDownload = false;

    public static WeakReference<Context> mContext = null;
    private static String mStrDownloadURL;
    private static String mStrFileMD5;

    public static Boolean mInited = false;

    public YouMeManager(Context context) {
        mContext = new WeakReference<Context>(context);
    }

    public static boolean LoadSO() {
        try {
            System.loadLibrary(YOU_ME_LIB_NAME_STRING);
        } catch (Throwable e) {
            e.printStackTrace();
            return false;
        }

        return true;
    }

    //加载文件
    public static boolean Init(Context context) {
        Log.i(TAG, "调用init函数开始");
        if (context == null) {
            Log.e(TAG, "context can not be null");
            return false;
        }

        if (mInited) {
            return true;
        }

        m_bStopDownload = false;
        if (mContext != null) {
            mContext.clear();
        }
        mContext = new WeakReference<Context>(context);

        if (!LoadSO()) {
            Log.e(TAG, "Failed to load so");
            return false;
        }

        try {
            AppPara.initPara(context);
            AudioMgr.init(context);
            CameraMgr.init(context);
        } catch (Throwable e) {
            e.printStackTrace();
            return false;
        }

        Log.i(TAG, "调用init 函数 结束");
        mInited = true;
        return true;
    }

    //反初始化
    public static void Uninit() {
        try {
            m_bStopDownload = true;
            AudioMgr.uinit();
            mInited = false;
        } catch (Throwable e) {
            e.printStackTrace();
        }
    }

    private static boolean fileIsExists(String path) {
        try {
            File f = new File(path);
            if (!f.exists()) {
                return false;
            }
        } catch (Exception e) {
            return false;
        }
        return true;
    }

    //保存logcat 日志
    public static void SaveLogcat(String strPath) {
        LogUtil.SaveLogcat(strPath);
    }

}