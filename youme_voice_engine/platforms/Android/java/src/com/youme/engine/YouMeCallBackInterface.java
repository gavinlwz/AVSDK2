package com.youme.engine;

public interface YouMeCallBackInterface {
	
	public  void onEvent (int event, int error, String room, Object param);
	
	public  void onRequestRestAPI(int requestID , int iErrorCode , String strQuery, String strResult);

	public  void onMemberChange(String channelID, MemberChange[] arrChanges , boolean isUpdate);

	public  void onBroadcast(int bc , String room, String param1, String param2, String content);

	public  void onAVStatistic( int avType,  String userID, int value );

	public  void onTranslateTextComplete( int errorcode, int requestID, String text, int srcLangCode, int destLangCode );

	public void onRecvCustomData(byte[] data, long timestamp);
}
