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

public class YouMeManager
{

	public static String YOU_ME_LIB_NAME_STRING = "youme_voice_engine";
	

	private static final String TAG = "YOUME";

	/* 下载保存路径 */
	private static String mCachePath;
	private static Boolean m_bStopDownload = false;
	
	public  static WeakReference<Context> mContext = null;
	private static String mStrDownloadURL;
	private static String mStrFileMD5;
	
	public static Boolean mInited = false;

	public YouMeManager(Context context)
	{		
		mContext = new WeakReference<Context>(context);
	}
	public static boolean LoadSO(Context context)
	{
		//先获取应用的versioncode
		String pathNameSting = mCachePath + "/lib" + YOU_ME_LIB_NAME_STRING + ".so"; 
		try {
			int iVercode = context.getPackageManager().getPackageInfo(context.getPackageName(), 0).versionCode;
			SharedPreferences sp =context.getSharedPreferences("YouMeUpdate", Activity.MODE_PRIVATE);
			String strUpdateVercode =  sp.getString("UpdateVercode", "");
			if (!strUpdateVercode.equals(String.valueOf(iVercode))) {
				//当前没有这个版本的更新记录，那就删掉原来
				Log.i(TAG, "没有当前版本的更新，忽略已经下载的so 应用versioncode: " + iVercode + "可以更新的版本:" + strUpdateVercode);
				new File(pathNameSting).delete();
			}
			else {
				Log.i(TAG, "应用versioncode: " + iVercode + "可以更新的版本:" + strUpdateVercode);
			}
		} catch (Exception e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
			//如果出问题了还是不要升级，加载原来的，以免不兼容
			new File(pathNameSting).delete();
			return false;
		}		
		
		try {
			if (fileIsExists(pathNameSting)) {
				//加载更新文件的库
				Log.i(TAG, "load " + pathNameSting);
				//try 不住，所以还是别try了
				System.load(pathNameSting);			
			} else {
				System.loadLibrary(YOU_ME_LIB_NAME_STRING);
			}
		} catch (Throwable e) {
			e.printStackTrace();
			return false;
		}
		
		return true;
	}
	//加载文件
	public static boolean  Init(Context context) 
	{
		Log.i(TAG,"调用init 函数 开始，目录:");
		if(context == null){
			Log.e(TAG,
					"context can not be null");
			return false;
		}
		if (mInited) {
			Log.e(TAG,"Init: Already initialzed");
			if(context instanceof Activity){
				if(mContext != null){
					mContext.clear();
				}
			    mContext = new WeakReference<Context>(context);
				CameraMgr.init(context);
				AudioMgr.init(context);
			}
			return true;
		}
		
		m_bStopDownload= false;
		if(mContext != null){
			mContext.clear();
		}
		mContext = new WeakReference<Context>(context);
		mCachePath = context.getDir("libs", Context.MODE_PRIVATE).getAbsolutePath();
		Log.i(TAG,"调用init 函数 开始，目录:" + mCachePath);
		
		if (!LoadSO(context)) {
			Log.e(TAG,"Failed to load so");
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

		Log.i(TAG,"调用init 函数 结束");
		mInited = true;
		return true;
	}

	public static boolean  Init(Context context, String libPath)
	{
		Log.i(TAG,"调用init 函数 加载指定动态库:" + libPath);
		if(context == null){
			Log.e(TAG,
					"context can not be null");
			return false;
		}
		if (mInited) {
			Log.e(TAG,"Init: Already initialzed");
			if(context instanceof Activity){
				if(mContext != null){
					mContext.clear();
				}
				mContext = new WeakReference<Context>(context);
				CameraMgr.init(context);
				AudioMgr.init(context);
			}
			return true;
		}

		m_bStopDownload= false;
		if(mContext != null){
			mContext.clear();
		}
		mContext = new WeakReference<Context>(context);

		Log.i(TAG,"开始加载指定动态库:" + libPath);
		try {
			if (fileIsExists(libPath)) {
				//加载更新文件的库
				Log.i(TAG, "指定动态库存在，正在加载: " + libPath);
				//try 不住，所以还是别try了
				System.load(libPath);
			} else {
				Log.w(TAG, "指定动态库不存在，加载默认so库: " + YOU_ME_LIB_NAME_STRING);
				System.loadLibrary(YOU_ME_LIB_NAME_STRING);
			}
		} catch (Throwable e) {
			Log.i(TAG,"Failed to load so");
			e.printStackTrace();
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

		Log.i(TAG,"调用init 函数 结束");
		mInited = true;
		return true;
	}

	//设置so的名字，必须在YouMeManager.Init之前调用
	public static boolean  setSOName( String str )
	{
		YOU_ME_LIB_NAME_STRING = str;
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
	
	private static boolean fileIsExists(String path)
	{
		try
		{
			File f = new File(path);
			if (!f.exists())
			{
				return false;
			}
		}
		catch (Exception e)
		{

			return false;
		}
		return true;
	}
	 public static boolean DownloadFile(String url, String strPath) {
		 //先删除临时文件
			File localPath = new File(strPath);
			localPath.delete();
			try {
				//使用随机文件读写
				RandomAccessFile randomFile = new RandomAccessFile(localPath, "rwd");
				randomFile.seek(0);
				URL downloadUrl = new URL(url);
				 HttpURLConnection conn=(HttpURLConnection)downloadUrl.openConnection();  		
				// conn.setRequestProperty("Range",String.format("bytes=%d-", ulCurSize));
				 InputStream is = conn.getInputStream();
				if (is != null) {					
					byte[] b = new byte[64*1024];
					while (true) {
						if (m_bStopDownload) {
							break;
						}
						int len = is.read(b);
						if (len <= 0) {
							//读完了					
							is.close();
							conn.disconnect();
							randomFile.close();
							return true;
						}
						randomFile.write(b, 0, len);
					}
				}
				is.close();
				conn.disconnect();
				randomFile.close();
			} catch (Exception e) {
				e.printStackTrace();
			}
			localPath.delete();
			return false;
		}
	 
	 
	 //获取网络类型
	 public static void TriggerNetChange()
	 {
	 	if(mContext != null && mContext.get() != null) {
			int networkType = NetUtil.getNetworkState(mContext.get());
			AppPara.onNetWorkChange(networkType);
		}
	 }
	 //保存logcat 日志
	 public static void SaveLogcat(String strPath)
	 {
		 LogUtil.SaveLogcat(strPath);
	 }
	 //url 是更目录的地址
	public static void UpdateSelf(String strURL,String strMD5)
	{
		//已经调用过了，就不要调用了
		if (mStrDownloadURL != null) {
			return;
		}
		mStrDownloadURL = strURL + "/android/" + android.os.Build.CPU_ABI +"/libyoume_voice_engine.so";
		mStrFileMD5 = strMD5;
		Log.i(TAG, "UpdateSelf " + mStrDownloadURL + "\n MD5: " + strMD5);
		//先检查是否已经在下载了
		new Thread(new Runnable() {
			public void run() {
				String strOriSoPath = mCachePath + "/lib" + YOU_ME_LIB_NAME_STRING + ".so"; 
				String strTmpPath = strOriSoPath + ".tmp";
				if (DownloadFile(mStrDownloadURL, strTmpPath))
				{					
					//校验MD5 
					try {
						String strFileMD5= FileMD5.getFileMD5String(strTmpPath);
						Log.i(TAG, "下载成功，MD5: " + strFileMD5);
						if (strFileMD5.equalsIgnoreCase(mStrFileMD5)) {
							if (new File(strOriSoPath).delete())
							{
								Log.i(TAG, "删除当前so 成功:" + strOriSoPath);
							}
							else
							{
								Log.i(TAG, "删除当前so 失败:" + strOriSoPath);
							}
							if (new File(strTmpPath).renameTo(new File(strOriSoPath))
									&& mContext !=null && mContext.get() != null)
							{
								Log.i(TAG, "重命名成功: " + strOriSoPath);
								SharedPreferences sp =mContext.get().getSharedPreferences("YouMeUpdate", Activity.MODE_PRIVATE);
								int iVercode = mContext.get().getPackageManager().getPackageInfo(mContext.get().getPackageName(), 0).versionCode;
								Editor editor=sp.edit();
						        editor.putString("UpdateVercode",String.valueOf(iVercode));
						        editor.commit();
							}
							else {
								Log.i(TAG, "重命名失败: " + strOriSoPath);
							}
						}
						else {
							Log.i(TAG, "MD5 不正确，删掉已经下载的文件: " + strFileMD5 + "  " + mStrFileMD5);
							new File(strTmpPath).delete();
						}
					} catch (Throwable e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					}					
				}
				else {
					Log.i(TAG, "下载失败了 URL:" + mStrDownloadURL);
				}
				mStrDownloadURL = null;
			}
		}).start();
	}
	
}