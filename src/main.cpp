//============================================================================
// Name        : WebRTC_M.cpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include <glib.h>

#include "rtc_base/flags.h"
#include "rtc_base/messagequeue.h"
#include "rtc_base/physicalsocketserver.h"
#include "rtc_base/refcountedobject.h"
#include "rtc_base/scoped_ref_ptr.h"
#include "rtc_base/ssladapter.h"
#include "rtc_base/thread.h"
#include "system_wrappers/include/field_trial.h"
#include "test/field_trial.h"

#include "webrtc-processor/WebRTCProcessor.h"
#include "signal/SignalProcessor.h"

using namespace std;

class MainThreadServer : public rtc::PhysicalSocketServer {
 public:
  explicit MainThreadServer(WebRTCProcessor* client): webrtcprocessor_(client) {}
  virtual ~MainThreadServer() {cout<<"MainThreadServer::Destructor>>";}

  void SetMessageQueue(rtc::MessageQueue* queue) override {
    message_queue_ = queue;
  }
  void set_webrtcprocessor(WebRTCProcessor* client) { webrtcprocessor_ = client; }
  // Override so that we can also pump the GTK message loop.
  bool Wait(int cms, bool process_io) override {
	  webrtcprocessor_->startWebRTC();
     return rtc::PhysicalSocketServer::Wait(0 /*cms == -1 ? 1 : cms*/,  process_io);
  }
 protected:
  rtc::MessageQueue* message_queue_;
  WebRTCProcessor* webrtcprocessor_;
};

int main(int argc,char* argv[]) {
	if(argc<2) {
		cout<<"Wrong Argument"<<endl;
		return -1;
	}
	string paramHandleId=argv[1];

	//Create Message Queue
	GAsyncQueue *sig2rtc=g_async_queue_new ();//Signal To WebRTC
	GAsyncQueue *rtc2sig=g_async_queue_new ();//WebRTC To Signal


	//Create SignalProcessor
	string url="wss://clouddev.gigagenie.ai:40081/wsapi/v1";
	auto signalProcessor=new SignalProcessor(url,sig2rtc,rtc2sig);
	signalProcessor->setHandleId(paramHandleId);

	signalProcessor->setP2pMode(false);
	signalProcessor->setPublichser(false);
	auto signalThread  = new thread([=]() {
		signalProcessor->startSignaling();
	});

	//Create WebRTCProcessor
	rtc::scoped_refptr<WebRTCProcessor> webRtcProcessor(new rtc::RefCountedObject<WebRTCProcessor>(paramHandleId,rtc2sig,sig2rtc));
	MainThreadServer mainThreadServer(webRtcProcessor);
	rtc::AutoSocketServerThread thread(&mainThreadServer);
	rtc::InitializeSSL();
	thread.Run();


	signalThread->join();
	g_async_queue_unref(sig2rtc);
	g_async_queue_unref(rtc2sig);

	rtc::CleanupSSL();
	return 0;
}
