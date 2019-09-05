/*
 * SignalProcessor.h
 *
 *  Created on: 2019. 5. 30.
 *      Author: alnova2
 */

#ifndef SIGNALPROCESSOR_H_
#define SIGNALPROCESSOR_H_
#include <glib.h>

#include <string>
#include <iostream>
#include <cpprest/ws_client.h>
#include <json/json.h>
#include <sys/types.h>
#include <unistd.h>
#include <thread>

#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include "common.h"

using namespace std;
using namespace web::websockets::client;//for websocket_callback_client
using namespace web;

class WebRTCProcessor;

class SignalProcessor{
public:
	SignalProcessor(string &url,GAsyncQueue* sendQueue,GAsyncQueue* recvQueue);
	virtual ~SignalProcessor();
	void startSignaling();
	void setWebRTCProcessor(WebRTCProcessor* inClass);
	void sendMsg(string& tag, string& writeMsg);
	string getTrxId();
	void setHandleId(string& inHandleId);
	void setP2pMode(bool flag);
	void setPublichser(bool flag);
protected:
private:
	GAsyncQueue* mSendQueue;
	GAsyncQueue* mRecvQueue;
	bool mRecvLoopFlag=false;
	mutex mRecvLoopLock;
	void setRecvLoop(bool inFlag);
	bool getRecvLoop();
	void sendQueueMsg(string& order,string& msgextra,string& sdp,string& sdptype,string& candidate,string& sdpMid,int sdpMLineIndex);
	void processQueueMsg(sigrtcmsg* inMsg);

	bool mP2PModeFlag=false;
	bool isPublisher = true;
	string mSendChannelId;
	string mAddress;

	string mHandleId;
	bool hbTimerFlag;
	int mRandVal;
	thread *hbTimer;

	websocket_callback_client* mWSClient;
	Json::Reader jsonReader;
	Json::StyledWriter jsonWriter;
	void processRecvMsg(string& msg_txt);
	void processHBTimer();
	std::map<string, rtc::scoped_refptr<WebRTCProcessor>> mWebRTCMap;
	std::map<string, GAsyncQueue*> mQueueMap;
};

#endif /* SIGNALPROCESSOR_H_ */
