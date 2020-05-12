#ifndef ALI_TRANSCRIBER_H
#define ALI_TRANSCRIBER_H

#ifdef TRANSSCRIBER
#include "ITranscriber.h"
#include "speechTranscriberRequest.h"

using AlibabaNls::SpeechTranscriberRequest;

class AliTranscriberWin : public ITranscriber
{
public:
	AliTranscriberWin();

	virtual void setToken(std::string token)override;
	virtual void setCallback(ITranscriberCallback* pCallback)override;
	virtual bool start()override;
	virtual bool stop()override;
	virtual bool sendAudioData(uint8_t* data, int data_size)override;

public:
	ITranscriberCallback* m_pCallback;
	bool m_bFailed;
private:
	std::string m_token;
	
	SpeechTranscriberRequest* m_request;

	bool m_bTranscribing;
	

};
#endif //ALI_TRANSCRIBER_H

#endif //TRANSSCRIBER