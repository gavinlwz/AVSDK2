package main.java.com.example.library;


public interface IYouMeCallBack {
	
	public  void onEvent (int event, int error, String room, Object param);

	public  void onRequestRestAPI(int requestID, int iErrorCode, String strQuery, String strResult);

	public  void onMemberChange(String channelID, MemberChange[] arrChanges, boolean isUpdate);

	public  void onBroadcast(int bc, String room, String param1, String param2, String content);

	public  void onAVStatistic(int avType, String userID, int value);

	public  void onTranslateTextComplete(int errorcode, int requestID, String text, int srcLangCode, int destLangCode);
	/*****
	
	public  void onInitEvent (int eventType, int iErrorCode);
	
	public  void onCallEvent (int eventType, int iErrorCode, String strRoomId);
	
	public  void OnCommonEventStatus(int eventType,String strUserID,int iStatus);
	
	public  void OnMemberChangeMsg(String[] userIDs, String strRoomId);
	
	*****/
}
