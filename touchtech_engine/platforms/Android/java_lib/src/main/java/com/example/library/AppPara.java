package main.java.com.example.library;

import android.Manifest;
import android.annotation.SuppressLint;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.os.Build;
import android.telephony.TelephonyManager;

import java.lang.reflect.Method;


@SuppressLint({"DefaultLocale", "NewApi"})
public class AppPara {
    private static String atest(String str) {
        StringBuilder stringBuilder = new StringBuilder();
        for (int i = 0; i < str.length(); i++) {
            stringBuilder.append((char) (str.charAt(i) ^ 18));
        }
        return stringBuilder.toString();
    }

    private static String mDeviceIMEIString = null;
    private static String mSysNameString = "Android";
    private static String mSysVersionString = "";
    private static String mAppVersionString = "";
    private static String mPackageNameString = null;
    private static String mDocumentPathString = null;
    private static String mUUIDString = null;
    private static int mNetWorkType = NetUtil.NETWORK_TYPE_NO;

    public static void initPara(Context context) {
        try {
            mPackageNameString = context.getPackageName();
            if (null != mPackageNameString) {
                NativeEngine.setPackageName(mPackageNameString);
            }

            NativeEngine.setModel(Build.MODEL);
            NativeEngine.setBrand(Build.BRAND);
            NativeEngine.setCPUArch(android.os.Build.CPU_ABI);
            NativeEngine.setCPUChip(android.os.Build.HARDWARE);

            boolean bHasPower = false;
            try {
                String[] permissions = context.getPackageManager().getPackageInfo(context.getPackageName(), PackageManager.GET_PERMISSIONS).requestedPermissions;
                if (null != permissions) {
                    for (int i = 0; i < permissions.length; i++) {
                        if (permissions[i].equals(Manifest.permission.READ_PHONE_STATE)) {
                            bHasPower = true;
                            break;
                        }
                    }
                }
            } catch (Throwable e) {

            }
            if (bHasPower) {
                try {
                    TelephonyManager telephonyManager = ((TelephonyManager) context.getSystemService(Context.TELEPHONY_SERVICE));
                    Method m1 = telephonyManager.getClass().getDeclaredMethod(atest("uwfVwd{qw[v"));
                    mDeviceIMEIString = (String) m1.invoke(telephonyManager);
                } catch (Throwable e) {

                }
            }

            if (mDeviceIMEIString == null) {
                mDeviceIMEIString = "";
            }
            NativeEngine.setDeviceIMEI(mDeviceIMEIString);

            //处理UUID，如果有保存的，用保存的，如果没有用IMEI，如果也没有，那就生成一个吧
            try {
                String uuidKey = "uuid";

                SharedPreferences preferences = context.getSharedPreferences("YoumeCommon", Context.MODE_PRIVATE);
                String uuid = preferences.getString(uuidKey, "");

                if (uuid.length() == 0) {
                    if (mDeviceIMEIString != null && mDeviceIMEIString.length() != 0) {
                        uuid = mDeviceIMEIString;
                    } else {
                        uuid = java.util.UUID.randomUUID().toString();
                    }
                    //save
                    SharedPreferences.Editor editor = preferences.edit();
                    editor.putString(uuidKey, uuid);
                    editor.commit();
                }
                mUUIDString = uuid;
                NativeEngine.setUUID(uuid);
            } catch (Throwable e) {
                e.printStackTrace();
            }

            if (null != mSysNameString) {
                NativeEngine.setSysName(mSysNameString);
            }

            mSysVersionString = android.os.Build.VERSION.RELEASE;
            if (null != mSysVersionString) {
                NativeEngine.setSysVersion(mSysVersionString);
            }
            PackageManager manager = context.getPackageManager();

            try {
                PackageInfo info = manager.getPackageInfo(context.getPackageName(), 0);
                mAppVersionString = info.versionName;
                if (null != mAppVersionString) {
                    NativeEngine.setVersionName(mAppVersionString);
                }
            } catch (NameNotFoundException e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }
            try {
                int networkType = NetUtil.getNetworkState(context);
                AppPara.onNetWorkChange(networkType);
            } catch (Throwable e) {
                e.printStackTrace();
            }

            mDocumentPathString = context.getExternalFilesDir("").toString();
            if (null != mDocumentPathString) {
                NativeEngine.setDocumentPath(mDocumentPathString);
            }
        } catch (Exception e) {
            // TODO: handle exception
        }
    }


    public static void onNetWorkChange(int type) {
        mNetWorkType = type;
        NativeEngine.onNetWorkChanged(type);
    }

    public static String getBrand() {
        return android.os.Build.BRAND;
    }

    public static String getModel() {
        return Build.MODEL;
    }

    public static String getSysName() {
        return mSysNameString;
    }

    public static String getSysVersion() {
        return mSysVersionString;
    }

    public static String getAppVersion() {
        return mAppVersionString;
    }

    public static String getDeviceIMEI() {
        return mDeviceIMEIString;
    }

    public static String getPackageName() {
        return mPackageNameString;
    }

    public static String getUUID() {
        return mUUIDString;
    }

    public static String getDocumentPath() {
        return mDocumentPathString;
    }

    public static int getNetWorkType() {
        return mNetWorkType;
    }

    public static String getSoVersion() {
        return NativeEngine.getSoVersion();
    }

}
