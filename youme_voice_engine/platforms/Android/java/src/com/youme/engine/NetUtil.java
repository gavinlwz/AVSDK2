package com.youme.engine;


import android.content.Context;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.NetworkInfo.State;

public class NetUtil 
{
	public static final int NETWORK_TYPE_NO = -1;
	
	/** Current network is WIFI */
	public static final int NETWORK_TYPE_WIFI = 1;
	public static final int NETWORK_TYPE_MOBILE = 2;


    public static int getNetworkState(Context context) 
    {
        ConnectivityManager connManager = (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
        NetworkInfo info = connManager.getNetworkInfo(ConnectivityManager.TYPE_WIFI);
        if (info != null) {
			State state =info.getState();
			if (state == State.CONNECTED) 
	        {
	            return NETWORK_TYPE_WIFI;
	        }
		}
        
        info = connManager.getNetworkInfo(ConnectivityManager.TYPE_MOBILE);
        if (info != null) {
        	State state =info.getState();
        	if (state == State.CONNECTED) 
            {
            	return NETWORK_TYPE_MOBILE;	
            }
		}
        
        return NETWORK_TYPE_NO;
    }
}

