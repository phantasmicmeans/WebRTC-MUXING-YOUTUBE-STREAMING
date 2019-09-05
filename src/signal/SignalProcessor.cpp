/*
 * SignalProcessor.cpp
 *
 *  Created on: 2019. 5. 30.
 *      Author: alnova2
 */
#include "../webrtc-processor/WebRTCProcessor.h"
#include "SignalProcessor.h"


#define QUEUE_MAX 10

SignalProcessor::SignalProcessor(string &url,GAsyncQueue* sendQueue,GAsyncQueue* recvQueue){
	cout<<"SignalProcessor::SignalProcessor > url:"<<url<<endl;
	mAddress=url;
	mSendQueue=sendQueue;
	mRecvQueue=recvQueue;
}

void SignalProcessor::setHandleId(string& inHandleId){
	mHandleId=inHandleId;
}

void SignalProcessor::setRecvLoop(bool inFlag){
	mRecvLoopLock.lock();
	mRecvLoopFlag=inFlag;
	mRecvLoopLock.unlock();
}

bool SignalProcessor::getRecvLoop(){
	bool returnFlag=false;
	mRecvLoopLock.lock();
	returnFlag=mRecvLoopFlag;
	mRecvLoopLock.unlock();
	return returnFlag;
}
void SignalProcessor::sendQueueMsg(
		string& order,
		string& msgextra,
		string& sdp,
		string& sdptype,
		string& candidate,
		string& sdpMid,
		int sdpMLineIndex){

	cout<<"SignalProcessor::sendQueueMsg > order:"<<order<<endl;
	sigrtcmsg *sendMsg=(sigrtcmsg*)g_malloc(sizeof(sigrtcmsg));
	sendMsg->order=(char*)g_malloc(order.length()+1);
	sprintf(sendMsg->order,order.c_str());
	if(order.compare("createOffer")==0){
		sendMsg->msgextra=(char*)g_malloc(msgextra.length()+1);
		sprintf(sendMsg->msgextra,msgextra.c_str());
		cout<<"SignalProcessor::sendQueueMsg > set msgextra:"<<sendMsg->msgextra<<endl;
	} else if(order.compare("setAnswer")==0){
		sendMsg->msgextra=(char*)g_malloc(msgextra.length()+1);
		sprintf(sendMsg->msgextra,msgextra.c_str());
		sendMsg->sdp=(char*)g_malloc(sdp.length()+1);
		sprintf(sendMsg->sdp,sdp.c_str());
		sendMsg->sdptype=(char*)g_malloc(sdptype.length()+1);
		sprintf(sendMsg->sdptype,sdptype.c_str());
	} else if(order.compare("setIce")==0){
		sendMsg->candidate=(char*)g_malloc(candidate.length()+1);
		sprintf(sendMsg->candidate,candidate.c_str());
		sendMsg->sdpMid=(char*)g_malloc(sdpMid.length()+1);
		sprintf(sendMsg->sdpMid,sdpMid.c_str());
		sendMsg->sdpMLineIndex=sdpMLineIndex;
	}
	g_async_queue_push(mSendQueue,sendMsg);
}
void SignalProcessor::startSignaling(){
	cout<<"SignalProcessor::startSignaling > Connect To Signaling Server. by handleId:"<<mHandleId<<endl;
	string subprotocol = "device-protocol";
	websocket_client_config config;
	config.add_subprotocol(subprotocol);

	mWSClient = new websocket_callback_client(config);
	mWSClient->set_message_handler([=](websocket_incoming_message msg) {
		string msg_txt=msg.extract_string().get();
		cout<<"SignalProcessor::startSignaling > Received Msg:"<<msg_txt<<endl;
		processRecvMsg(msg_txt);
	});
	mWSClient->set_close_handler(
			[=](websocket_close_status status, const utility::string_t& reason, const std::error_code& code) {
				cout<<"SignalProcessor::SignalProcessor > Websocket Closed"<<endl;
				//mConnected=false;
			});
	uri w_uri(mAddress.c_str());
	mWSClient->connect(w_uri).then([=]() {
		cout<<"Signalprocessor::connect > Websocket connected"<<endl;
	});
	cout<<"SignalProcessor::startSignaling > Enter msgRecvLoop"<<endl;
	setRecvLoop(true);
	while(getRecvLoop()){
		sigrtcmsg* msg;
		msg=(sigrtcmsg*)g_async_queue_pop(mRecvQueue);
		processQueueMsg(msg);
	}
}

SignalProcessor::~SignalProcessor(){
	setRecvLoop(false);
}
void SignalProcessor::processQueueMsg(sigrtcmsg* inMsg){
	string order=inMsg->order;
	cout<<"SignalProcessor::processQueueMsg > order:"<<order<<endl;
	Json::StyledWriter writer;
	Json::Value sendMsgRoot;

	if(order.compare("sendOffer")==0){
		string sdp=inMsg->sdp;
		string sdptype=inMsg->sdptype;
		string ms_channel_id=inMsg->msgextra;
		Json::Value sdpJson;
		sdpJson["type"]=sdptype;
		sdpJson["sdp"]=sdp;

		cout<<"SignalProcessor::processQueueMsg SendSdp> sdp:"<<sdp<<endl;
		sendMsgRoot["operation"]="send_sdp";
		sendMsgRoot["ms_channel_id"]=ms_channel_id;
		sendMsgRoot["sdp"]=sdpJson;
		sendMsgRoot["trx_id"]=getTrxId();
		string writeMsg=writer.write(sendMsgRoot);
		string tag="Res_SendOffer";
		sendMsg(tag,writeMsg);
	    g_free(inMsg->sdp);
	    g_free(inMsg->sdptype);
	    g_free(inMsg->msgextra);
	} else if(order.compare("sendIce")==0){
		string candidate=inMsg->candidate;
		string sdpmid=inMsg->sdpMid;
		string ms_channel_id=inMsg->msgextra;
		int sdpmlineindex=inMsg->sdpMLineIndex;
		cout<<"Signalprocessor::processQueueMsg send ice candidate:"<<candidate<<" sdpMid:"<<sdpmid<<" sdpmlineindex:"<<sdpmlineindex<<endl;

		sendMsgRoot["operation"]="send_ice";
		sendMsgRoot["ms_channel_id"]=ms_channel_id;
		Json::Value candidateJson;
		candidateJson["candidate"]=candidate;
		candidateJson["sdpMid"]=sdpmid;
		candidateJson["sdpMLineIndex"]=sdpmlineindex;
		sendMsgRoot["candidate"]=candidateJson;
		sendMsgRoot["trx_id"]=getTrxId();
		string writeMsg=writer.write(sendMsgRoot);
		string tag="Send_ICECandidate";
		sendMsg(tag,writeMsg);
	    g_free(inMsg->candidate);
	    g_free(inMsg->sdpMid);
	    g_free(inMsg->msgextra);
	}
	g_free(inMsg->order);
	g_free(inMsg);
	cout<<"SignalProcessor::processQueueMsg > exit"<<endl;
}

void SignalProcessor::processRecvMsg(string& msg_txt) {
	Json::Value root;
	if (jsonReader.parse(msg_txt, root, true)) {
		string operation = root.get("operation", "Unknown").asString();
		cout << "SignalProcessor::processRecvMsg > Operation:" << operation
				<< endl;
		Json::Value sendMsgRoot;
		string order;
		string msgextra;
		string sdp;
		string sdptype;
		string candidate;
		string sdpmid;
		int sdpmlineindex=0;
		if (operation.compare("WHO") == 0) {
			cout << "SignalProcessor::processRecvMsg > Operation WHO" << endl;
			sendMsgRoot["operation"] = "IAM";
			sendMsgRoot["handle_id"] = mHandleId;
			sendMsgRoot["timestamp"] = "test";
			sendMsgRoot["signature"] = "test";
			string writeMsg = jsonWriter.write(sendMsgRoot);
			string tag = "Res_WHO";
			sendMsg(tag, writeMsg);
		} else if (operation.compare("WELCOME") == 0) {
			cout << "SignalProcessor::processRecvMsg > Start Heartbeat Thread" 	<< endl;
			hbTimerFlag = true;
			hbTimer = new thread([=]() {
				processHBTimer();
			});
			hbTimer->detach();

			if(mP2PModeFlag==false && isPublisher){
				//create publisher channel
				std::cout << "[CREATE PUBLISHER CHANNEL]" << std::endl;
				sendMsgRoot["operation"]="create_ms_channel";
				Json::Value channel_option;
				channel_option["type"]="publisher";
				channel_option["mode_flag"]="0";
				channel_option["trx_id"]=getTrxId();
				sendMsgRoot["channel_option"]=channel_option;
				string writeMsg = jsonWriter.write(sendMsgRoot);
				string tag = "Send_CreatePublisherChannel";
				sendMsg(tag,writeMsg);
			}

			if (mP2PModeFlag==false && !isPublisher) {
				std::cout << "[CREATE SUBSCRIBER CHANNEL]" << std::endl;
				sendMsgRoot["operation"] = "query_ms_pub_channels";
				sendMsgRoot["trx_id"] = getTrxId();
				std::string writeMsg = jsonWriter.write(sendMsgRoot);
				std::string tag = "Send_QueryMsPubChannels";
				sendMsg(tag, writeMsg);
			}

		} else if (operation.compare("send_offer")==0){
			mSendChannelId=root.get("ms_channel_id","Unknown").asString();
			order="createOffer";
			msgextra=mSendChannelId;
			sendQueueMsg(order,msgextra,sdp,sdptype,candidate,sdpmid,sdpmlineindex);

		} else if (operation.compare("res_send_sdp")==0){
			mSendChannelId=root.get("ms_channel_id","Unknown").asString();
			sdp=root["sdp"].get("sdp","UTF-8").asString();
			sdptype=root["sdp"].get("type","UTF-8").asString();
			cout<<"\n\n\nsdp:"<<sdp<<" type:"<<sdptype<<endl;
			msgextra=mSendChannelId;

			if(sdptype.compare("answer")==0){
				order="setAnswer";
			} else {
				cout<<"\n\n\nError"<<endl;
			}
			sendQueueMsg(order,msgextra,sdp,sdptype,candidate,sdpmid,sdpmlineindex);

		} else if(operation.compare("set_answer")==0){
			mSendChannelId=root.get("ms_channel_id","Unknown").asString();
			string sdp=root["sdp"].get("sdp","UTF-8").asString();
			string sdptype=root["sdp"].get("type","UTF-8").asString();
			cout<<"\n\n\nsdp:"<<sdp<<" type:"<<sdptype<<endl;
			order="setAnswer";
			msgextra=mSendChannelId;
			sendQueueMsg(order,msgextra,sdp,sdptype,candidate,sdpmid,sdpmlineindex);

		} else if(operation.compare("set_ice")==0){
			mSendChannelId=root.get("ms_channel_id","Unknown").asString();
			candidate=root["ice"].get("candidate","UTF-8").asString();
			sdpmid=root["ice"].get("sdpMid","UTF-8").asString();
			sdpmlineindex=root["ice"].get("sdpMLineIndex","UTF-8").asInt();
			order="setIce";
			sendQueueMsg(order,msgextra,sdp,sdptype,candidate,sdpmid,sdpmlineindex);

		} else if(operation.compare("send_ice")==0){
			mSendChannelId=root.get("ms_channel_id","Unknown").asString();
			candidate=root["candidate"].get("candidate","UTF-8").asString();
			sdpmid=root["candidate"].get("sdpMid","UTF-8").asString();
			sdpmlineindex=root["candidate"].get("sdpMLineIndex","UTF-8").asInt();
			order="setIce";
			sendQueueMsg(order,msgextra,sdp,sdptype,candidate,sdpmid,sdpmlineindex);

		} else if (operation.compare("res_query_ms_pub_channels")==0) {
			int result = root.get("result", 500).asInt();

			std::string mSubFeedChannelId = root["feed_ids"][0].asString();
			cout << "SignalProcessor::processRecvMsg > Publisher feed_ids:" << mSubFeedChannelId<<endl;
			sendMsgRoot["operation"]="create_ms_channel";

			Json::Value channel_option;
			channel_option["type"]="subscriber";
			channel_option["feed_channel_id"] = mSubFeedChannelId;

			sendMsgRoot["channel_option"] = channel_option;
			string writeMsg = jsonWriter.write(sendMsgRoot);
			string tag = "createSubChannel";
			sendMsg(tag, writeMsg);

		} else if (operation.compare("res_create_ms_channel")==0) {
			int result = root.get("result", 500).asInt();
			string type = root.get("type", "unknown").asString();
			//publisher ms channel created.
			if (result == 200 && type.compare("publisher") == 0) {
				mSendChannelId=root.get("ms_channel_id","Unknown").asString();
				std::cout << "publisher !!! " << std::endl;
				//Json::Value feed_ids=root.get("feed_ids",NULL);
				order="createOffer";
				msgextra=mSendChannelId;
				sendQueueMsg(order,msgextra,sdp,sdptype,candidate,sdpmid,sdpmlineindex);
			}
			//subscriber ms channel created.
			if (result == 200 && type.compare("subscriber") == 0) {
				//Open Video Capture Device
				std::cout << "subscriber !!! " << std::endl;
				string subscriberChannelId = root["ms_channel_id"].asString(); // @suppress("Invalid arguments")
				order="createOffer";
				msgextra=subscriberChannelId;
				sendQueueMsg(order,msgextra,sdp,sdptype,candidate,sdpmid,sdpmlineindex);

			}
		} else if (operation.compare("event")  ==   0) {
			string type = root.get("type","UnKnown").asString();
			cout << "Event Received :" << ", type :" << type << endl;
			if (type.compare("leave")  ==   0) {
				std::cout << "MESSAGE] SIGNALING >> Leave Envent Received " << std::endl;
				exit(0);

			}
			else if (type.compare("resource_destroyed") == 0) {
				std::cout << "MESSAGE] SIGNALING >> Resource Destroyed Received " << std::endl;
				exit(0);
			}
		}
	}
}
void SignalProcessor::sendMsg(string& tag, string& writeMsg) {
	cout << "SignalProcessor::sendMsg > " << tag << " msg:" << writeMsg << endl;
	websocket_outgoing_message ws_msg;
	ws_msg.set_utf8_message(writeMsg);
	mWSClient->send(ws_msg).then([]() {
		cout << "SignalProcessor::sendMsg > Send Msg Success"<<endl;
	});
}
string SignalProcessor::getTrxId() {
	string target;
	const char* charset = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
	target = "";
	int c = clock() % 52;
	(mRandVal == c) ? mRandVal = (c + 1) % 52 : mRandVal = c % 52;
	for (int i = 0; i < 12; i++) {
		int randompos = (mRandVal++) % 52;
		target += charset[randompos];
	}
	return target;
}
void SignalProcessor::processHBTimer() {
	Json::Value sendMsgRoot;
	while (hbTimerFlag) {
		cout << "SignalProcessor::processHBTimer > send Heartbeat" << endl;
		sendMsgRoot["operation"] = "heartbeat";
		sendMsgRoot["trx_id"] = getTrxId();
		string writeMsg = jsonWriter.write(sendMsgRoot);
		string tag = "SendHeartbeat";
		sendMsg(tag, writeMsg);
		sleep(10); //seconds
		cout << "SignalProcessor::processHBTimer > After Sleep" << endl;
	}
	cout << "SignalProcessor::processHBTimer > Exit Heartbeat Thread" << endl;
}
void SignalProcessor::setWebRTCProcessor(WebRTCProcessor* inClass){
//	mWebRTCProcessor=inClass;
}
void SignalProcessor::setP2pMode(bool flag){
	mP2PModeFlag=flag;
}

void SignalProcessor::setPublichser(bool flag) {
	isPublisher = flag;
}

