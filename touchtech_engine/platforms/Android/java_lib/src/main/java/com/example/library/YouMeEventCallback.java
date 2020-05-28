package main.java.com.example.library;

import org.json.JSONArray;
import org.json.JSONObject;

public class YouMeEventCallback {

	public static IYouMeCallBack callBack=null;
//	public static YouMeCallBackInterfacePcm mCallbackPcm = null;
//	public static YouMeCallBackInterfacePcmForUnity mCallbackPcmForUntiy = null;
//	public static YouMeCustomDataCallbackInterface mCustomDataCallback = null;

	public static void onEvent (int eventType, int iErrorCode, String channel, String param)
	{

		if(callBack!=null){
			callBack.onEvent(eventType, iErrorCode, channel, param);
		}
	}

	//sendMessage的 param里可能有emoji ,也许不被支持
	public static void onEventByte(int eventType, int iErrorCode, String channel , byte[] param )
	{
		if( callBack != null )
		{
			String strParam = new String( param );
			callBack.onEvent( eventType, iErrorCode, channel, strParam );
		}
	}

//	public static void onPcmDataRemote(int channelNum, int samplingRateHz, int bytesPerSample, byte[] data)
//	{
//		if (mCallbackPcm != null) {
//			mCallbackPcm.onPcmDataRemote(channelNum, samplingRateHz, bytesPerSample, data);
//		}else if(mCallbackPcmForUntiy != null){
//			mCallbackPcmForUntiy.onPcmDataRemote(channelNum, samplingRateHz, bytesPerSample, new YouMePcmDataForUnity(data));
//		}
//	}
//
//	public static void onPcmDataRecord(int channelNum, int samplingRateHz, int bytesPerSample, byte[] data)
//	{
//		if (mCallbackPcm != null) {
//			mCallbackPcm.onPcmDataRecord(channelNum, samplingRateHz, bytesPerSample, data);
//		}else if(mCallbackPcmForUntiy != null){
//			mCallbackPcmForUntiy.onPcmDataRecord(channelNum, samplingRateHz, bytesPerSample, new YouMePcmDataForUnity(data));
//		}
//	}
//
//	public static void onPcmDataMix(int channelNum, int samplingRateHz, int bytesPerSample, byte[] data)
//	{
//		if (mCallbackPcm != null) {
//			mCallbackPcm.onPcmDataMix(channelNum, samplingRateHz, bytesPerSample, data);
//		}else if(mCallbackPcmForUntiy != null){
//			mCallbackPcmForUntiy.onPcmDataMix(channelNum, samplingRateHz, bytesPerSample, new YouMePcmDataForUnity(data));
//		}
//	}
//
	public static void onRequestRestAPI(int requestID , int iErrorCode , String strQuery, String strResult)
	{
		if (callBack != null) {
			callBack.onRequestRestAPI(requestID, iErrorCode, strQuery, strResult);
		}
	}
	
	public static void onMemberChange(String channelID  , String memListJson, boolean isUpdate)
	{
		if (callBack != null) {
			try{
				JSONObject obj = new JSONObject(memListJson);
				//{"channelid":"2418video","memchange":[{"isJoin":true,"userid":"1001590"}]}
				JSONArray array = obj.optJSONArray("memchange");
				if( array !=null ){
					MemberChange[] arrChanges= new MemberChange[array.length()];
					for(int i=0; i<arrChanges.length; i++){
						arrChanges[i] = new MemberChange();
						arrChanges[i].userID = array.optJSONObject(i).optString("userid");
						arrChanges[i].isJoin = array.optJSONObject(i).optBoolean("isJoin");
					}
					callBack.onMemberChange(channelID, arrChanges ,isUpdate );
				}

			}catch(Exception e){
				e.printStackTrace();
			}
		}
	}
	
	public static void onBroadcast(int bc , String room, String param1, String param2, String content)
	{
		if (callBack != null){
			callBack.onBroadcast(bc, room, param1, param2, content);
		}
	}

	public static void onAVStatistic(int avType, String userID, int value )
	{
		if (callBack != null )
		{
			callBack.onAVStatistic( avType, userID, value );
		}
	}
//	public static void onRecvCustomData(byte[] data,long timeSpan)
//	{
//		if (null != mCustomDataCallback)
//		{
//			mCustomDataCallback.onRecvCustomData(data,timeSpan);
//		}
//	}

	public static void onTranslateTextComplete(int errorcode, int requestID, String text, int srcLangCode, int destLangCode )
	{
		if (callBack != null )
		{
			callBack.onTranslateTextComplete( errorcode, requestID, text, srcLangCode,  destLangCode);
		}
	}
}
