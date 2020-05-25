//
//  YMVideoRecorderManager.hpp
//  VideoRecord
//
//  Created by pinky on 2019/5/16.
//  Copyright © 2019 youme. All rights reserved.
//

#ifndef YMVideoRecorderManager_hpp
#define YMVideoRecorderManager_hpp

#include "YMVideoRecorder.hpp"

#include <stdio.h>
#include <string>
#include <map>

class YMVideoRecorderManager
{
public:
    static YMVideoRecorderManager* getInstance();
private:
    YMVideoRecorderManager();
    ~YMVideoRecorderManager();
   
public:
    //或者这里用sessionid？
    YMVideoRecorder* startRecord( std::string  userid,  std::string saveFile );
    void stopRecord( std::string  userid );
    //离开房间的时候，全都停了
    void stopRecordAll();
    
    bool needRecord( std::string userid  );
    
    YMVideoRecorder* getRecorder( std::string userid );
    YMVideoRecorder* getRecorderBySession( int sessionId );
    
//    void setDirPath( std::string path );
    
//    std::string getFilePath( std::string userid );
    
    void NotifyUserName( int32_t sessionId, const std::string& userID, bool isLocalUser = false  );
    
    //为share放进来所有的音频吧
    void inputAudioData( uint8_t* data, int data_size, int timestamp );
    
    void annexbToAvcc(uint8_t* input, int in_size, uint8_t** output, int *out_size);
    void setSpsAndPps( int sessionId, uint8_t* sps, int sps_size, uint8_t* pps, int pps_size );
    void inputVideoData( int sessionId, uint8_t* data, int data_size, int timestamp, int nalu_type );
    
    //音频全从一个地方出吧，使用同样的采样率就好了
    void setAudioInfo( int sampleRate );
	void setVideoInfo(int sessionId, int width, int height, int fps, int birrate, int timebaseden);

private:
    void deleteRecord( std::string userid );
public:
    std::recursive_mutex m_mutex;
    std::map< std::string, YMVideoRecorder*> m_mapRecorder;
//    std::string m_dirPath;
    
    std::map< int, std::string > m_mapSessionToName;
    int m_localSessionId;
};
#endif /* YMVideoRecorderManager_hpp */
