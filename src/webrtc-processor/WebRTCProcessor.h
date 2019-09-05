/*
 * WebRTCProcessor.h
 *
 *  Created on: 2019. 5. 30.
 *      Author: alnova2
 */

#ifndef WEBRTCPROCESSOR_H_
#define WEBRTCPROCESSOR_H_

#include "api/mediastreaminterface.h"
#include "api/peerconnectioninterface.h"
#include "api/video/video_frame.h"
#include "api/video/video_sink_interface.h"

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
#include <glib.h>
#include "../common.h"

#include "../audio/CustomAudioTrackSink.h"
#include "../video/CustomVideoRenderer.h"
#include "../youtube/Youtube.h"

namespace webrtc {
class VideoCaptureModule;
}  // namespace webrtc

namespace cricket {
class VideoRenderer;
}

class SignalProcessor;
using namespace std;

class candidateObj{
public:
	string candidate;
	string sdpmid;
	int sdpmlineindex;
};
class WebRTCProcessor:
		public webrtc::PeerConnectionObserver,
		public webrtc::CreateSessionDescriptionObserver,
		public rtc::Runnable {
public:
	WebRTCProcessor(string &handleId, GAsyncQueue* sendQueue,GAsyncQueue* recvQueue);
	virtual ~WebRTCProcessor();
	void startWebRTC();
	void createAnswer(string& in_ms_id,string& in_sdp_type, string& in_sdp_string);
	void setAnswer(string& in_ms_id,string& in_sdp_type, string& in_sdp_string);
	void createOffer(string& in_ms_id);
	void setIce(string& candidate,string& sdpMid,int sdpMLineIndex);
	void plushIce();
	GAsyncQueue* mSendAudioQueue;
	GAsyncQueue* mRecvAudioProcessorQueue;
protected:
	//
	// PeerConnectionObserver implementation.
	//
	void OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state) override {};
	void OnAddTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>& streams) override;
	void OnRemoveTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) override;
	void OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> channel)	override;
	void OnRenegotiationNeeded() override {};
	void OnIceConnectionChange(	webrtc::PeerConnectionInterface::IceConnectionState new_state)	override {};
	void OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state) override {};
	void OnIceCandidate(const webrtc::IceCandidateInterface* candidate) override;
	void OnIceConnectionReceivingChange(bool receiving) override {}
	// CreateSessionDescriptionObserver implementation.
	void OnSuccess(webrtc::SessionDescriptionInterface* desc) override;
	void OnFailure(webrtc::RTCError error) override;
	//rtc::Runnable Implementation
	void Run(rtc::Thread* thread) override;
private:
	GAsyncQueue* mSendQueue;
	GAsyncQueue* mRecvQueue;

	bool mRecvLoopFlag=false;
	void sendQueueMsg(string& order,string& msgextra,string& sdp,string& sdptype,string& candidate,string& sdpMid,int sdpMLineIndex);
	bool isOffer;
	bool isSetRemoteDescription=false;
	stack<candidateObj> iceRepos;
	string ms_channel_id;
	rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> peer_connection_factory_;
	rtc::scoped_refptr<webrtc::PeerConnectionInterface> peer_connection_;
//	rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel_;
	bool InitializePeerConnection();
	void DeletePeerConnection();
	bool CreatePeerConnection(bool dtls);
	void AddTracks();
	Youtube* youtube;
	std::unique_ptr<cricket::VideoCapturer> OpenVideoCaptureDevice();
	std::unique_ptr<CustomVideoRenderer> remote_renderer_;
	CustomAudioTrackSink* audio_renderer;
};
#endif /* WEBRTCPROCESSOR_H_ */
