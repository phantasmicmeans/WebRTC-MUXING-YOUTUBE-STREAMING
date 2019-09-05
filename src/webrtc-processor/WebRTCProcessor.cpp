/*
* WebRTCProcessor.cpp
*
*  Created on: 2019. 5. 30.
*      Author: alnova2
*/

#include "WebRTCProcessor.h"

#include "absl/types/optional.h"
#include "api/audio/audio_mixer.h"
#include "api/audio_codecs/audio_decoder_factory.h"
#include "api/audio_codecs/audio_encoder_factory.h"
#include "api/audio_codecs/builtin_audio_decoder_factory.h"
#include "api/audio_codecs/builtin_audio_encoder_factory.h"
#include "api/audio_options.h"
#include "api/create_peerconnection_factory.h"
#include "api/rtpsenderinterface.h"
#include "api/video_codecs/builtin_video_decoder_factory.h"
#include "api/video_codecs/builtin_video_encoder_factory.h"
#include "api/video_codecs/video_decoder_factory.h"
#include "api/video_codecs/video_encoder_factory.h"
//#include "defaults.h"
#include "media/base/device.h"
#include "media/engine/webrtcvideocapturerfactory.h"
#include "modules/audio_device/include/audio_device.h"
#include "modules/audio_processing/include/audio_processing.h"
#include "modules/video_capture/video_capture.h"
#include "modules/video_capture/video_capture_factory.h"
#include "p2p/base/portallocator.h"
#include "rtc_base/checks.h"
#include "rtc_base/logging.h"
#include "rtc_base/refcountedobject.h"
#include "rtc_base/rtccertificategenerator.h"
#include "rtc_base/strings/json.h"

#include "api/test/fakeconstraints.h"
#include "api/mediaconstraintsinterface.h"
#include "modules/audio_device/dummy/file_audio_device.h"
#include "modules/audio_device/dummy/file_audio_device_factory.h"

#include "../signal/SignalProcessor.h"

#define QUEUE_MAX 10
// Names used for a IceCandidate JSON object.
const char kCandidateSdpMidName[] = "sdpMid";
const char kCandidateSdpMlineIndexName[] = "sdpMLineIndex";
const char kCandidateSdpName[] = "candidate";

// Names used for a SessionDescription JSON object.
const char kSessionDescriptionTypeName[] = "type";
const char kSessionDescriptionSdpName[] = "sdp";
const char kAudioLabel[] = "audio_label";
const char kVideoLabel[] = "video_label";
const char kStreamId[] = "stream_id";

class DummySetSessionDescriptionObserver
    : public webrtc::SetSessionDescriptionObserver {
 public:
  static DummySetSessionDescriptionObserver* Create() {
    return new rtc::RefCountedObject<DummySetSessionDescriptionObserver>();
  }
  virtual void OnSuccess() { cout<<"DummySetSessionDescriptor::onSuccess>>"<<endl; }
  virtual void OnFailure(webrtc::RTCError error) {
    RTC_LOG(INFO) << __FUNCTION__ << " " << ToString(error.type()) << ": " << error.message();
  }
};

class setRemoteDescriptionObserver : public webrtc::SetSessionDescriptionObserver {
 public:
  static setRemoteDescriptionObserver* Create(GAsyncQueue* callback) {
	  setRemoteDescriptionObserver* retObserver=new rtc::RefCountedObject<setRemoteDescriptionObserver>();
	  retObserver->setWebRTCProcessor(callback);
    return retObserver;
  }
  virtual void OnSuccess() {
		cout<<"setRemoveDescriptionObserver::onSuccess>>"<<endl;
		string order="answerSuccess";
		sigrtcmsg *sendMsg=(sigrtcmsg*)g_malloc(sizeof(sigrtcmsg));
		sendMsg->order=(char*)g_malloc(order.length()+1);
		sprintf(sendMsg->order,order.c_str());
		cout<<"setRemoveDescriptionObserver::onSuccess>> sendSetRDSuccess"<<endl;
		g_async_queue_push(mRecvQueue,sendMsg);
  }
  virtual void OnFailure(webrtc::RTCError error) {
    RTC_LOG(INFO) << __FUNCTION__ << " " << ToString(error.type()) << ": " << error.message();
  }
  void setWebRTCProcessor(GAsyncQueue* inQueue){
	  mRecvQueue=inQueue;
  }
 private:
  GAsyncQueue* mRecvQueue;
};

/*
class dataChannelObserver : public webrtc::DataChannelObserver {
public:
	static dataChannelObserver* Create() {
		return new rtc::RefCountedObject<dataChannelObserver>();
	}
	void OnStateChange() {};
	void OnMessage(const webrtc::DataBuffer& buffer) {
		cout << std::string(buffer.data.data<char>(), buffer.data.size()) << endl;
	};
	void OnBufferedAmountChange(uint64_t previous_amount) {};
};
*/

WebRTCProcessor::WebRTCProcessor(string &handleId, GAsyncQueue* sendQueue,GAsyncQueue* recvQueue){
	cout<<"WebRTCProcessor::WebRTCProcessor > handleId:"<<handleId<<endl;
	mSendQueue=sendQueue;
	mRecvQueue=recvQueue;
	youtube = new Youtube();

	auto youtubeThread = new thread([=]() {
		youtube -> startSend();
	});
}

WebRTCProcessor::~WebRTCProcessor(){
	cout<<"WebRTCProcessor::Descructor>>"<<endl;
}
void WebRTCProcessor::sendQueueMsg(string& order,string& msgextra,string& sdp,string& sdptype,string& candidate,string& sdpMid,int sdpMLineIndex){
	cout<<"WebRTCProcessor::sendQueueMsg > order:"<<order<<endl;
	sigrtcmsg *sendMsg=(sigrtcmsg*)g_malloc(sizeof(sigrtcmsg));
	sendMsg->order=(char*)g_malloc(order.length()+1);
	sprintf(sendMsg->order,order.c_str());
	if(order.compare("sendOffer")==0){
		sendMsg->sdp=(char*)g_malloc(sdp.length()+1);
		sprintf(sendMsg->sdp,sdp.c_str());
		sendMsg->sdptype=(char*)g_malloc(sdptype.length()+1);
		sprintf(sendMsg->sdptype,sdptype.c_str());
		sendMsg->msgextra=(char*)g_malloc(msgextra.length()+1);
		sprintf(sendMsg->msgextra,msgextra.c_str());
	} else if(order.compare("sendIce")==0){
		sendMsg->candidate=(char*)g_malloc(candidate.length()+1);
		sprintf(sendMsg->candidate,candidate.c_str());
		sendMsg->sdpMid=(char*)g_malloc(sdpMid.length()+1);
		sprintf(sendMsg->sdpMid,sdpMid.c_str());
		sendMsg->sdpMLineIndex=sdpMLineIndex;
		sendMsg->msgextra=(char*)g_malloc(msgextra.length()+1);
		sprintf(sendMsg->msgextra,msgextra.c_str());
	}
	g_async_queue_push(mSendQueue,sendMsg);
}

void WebRTCProcessor::startWebRTC(){
	sigrtcmsg* msg;
	msg=(sigrtcmsg*)g_async_queue_try_pop(mRecvQueue);
	if(msg){
		cout<<"WebRTCProcessor::start thread for check msg2"<<endl;
		string order=msg->order;
		cout<<"WebRTCProcessor::startWebRTC > Msg Received. Order:"<<msg->order<<endl;

		if(order.compare("createOffer")==0){
			string mschannelid=msg->msgextra;
			createOffer(mschannelid);
			g_free(msg->msgextra);
		} else if(order.compare("setAnswer")==0){
			string mschannelid=msg->msgextra;
			string sdp=msg->sdp;
			string type=msg->sdptype;
			cout<<"WebRTCProcessor::startWebRTC > Msg Received. sdp:"<<sdp<<" mschannelId:"<<mschannelid<<" type"<<type<<endl;
			setAnswer(mschannelid,type,sdp);
			g_free(msg->msgextra);
			g_free(msg->sdp);
			g_free(msg->sdptype);
		} else if(order.compare("setIce")==0){
			int sdpMLineIndex=msg->sdpMLineIndex;
			string candidate=msg->candidate;
			string sdpMid=msg->sdpMid;
			setIce(candidate,sdpMid,sdpMLineIndex);
			g_free(msg->candidate);
			g_free(msg->sdpMid);
		} else if(order.compare("answerSuccess")==0){
			//SetAnswerSuccess
			cout<<"WebRTCProcessor::startWebRTC >> Flush Ice Candidate. Size:"<<iceRepos.size()<<endl;
			while(iceRepos.size()>0){
				cout<<"WebRTCProcessor::startWebRTC >> Add Ice Candidate.. > "<<endl;
				while(iceRepos.empty()==false){
					candidateObj setCandidate=iceRepos.top();
					iceRepos.pop();
					webrtc::SdpParseError error;
					std::unique_ptr<webrtc::IceCandidateInterface> iceCandidate( webrtc::CreateIceCandidate(setCandidate.sdpmid, setCandidate.sdpmlineindex, setCandidate.candidate, &error));
					std::cout<<"WebRTCProcessor::setIce > Set Ice"<<endl;
					if(iceCandidate.get()){
						cout<<"Add Ice Candidate.. > "<<endl;
						if (!peer_connection_->AddIceCandidate(iceCandidate.get())) {
							std::cout<<"\n\n\n------------------"<< "Failed to apply the received candidate"<<"--------------\n\n\n";
						}
					}
				}
			}
		}
		g_free(msg->order);
		g_free(msg);
	}
}

void WebRTCProcessor::createOffer(string& in_ms_id){
	ms_channel_id=in_ms_id;
	isOffer=true;
	if (InitializePeerConnection()) {
		cout << "WebRTCProcessor::createOffer > CreateOffer"<<endl;
		peer_connection_->CreateOffer(this, webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());
	}
}

bool WebRTCProcessor::InitializePeerConnection() {
  RTC_DCHECK(!peer_connection_factory_);
  RTC_DCHECK(!peer_connection_);

	cout << "SetFileNames using mChannelId : " << ms_channel_id << endl;
	string outputFile = "/data/test/";
	outputFile = outputFile + ms_channel_id + "_" + ".pcm";
	mSendAudioQueue = g_async_queue_new();
	mRecvAudioProcessorQueue = g_async_queue_new();
	webrtc::FileAudioDeviceFactory::SetFilenamesToUse(
			"/data/test/testinput.pcm", outputFile.c_str(), mSendAudioQueue,
			mRecvAudioProcessorQueue, ms_channel_id.c_str());

  peer_connection_factory_ = webrtc::CreatePeerConnectionFactory(
      nullptr, nullptr ,
      nullptr , nullptr ,
      webrtc::CreateBuiltinAudioEncoderFactory(),
      webrtc::CreateBuiltinAudioDecoderFactory(),
      webrtc::CreateBuiltinVideoEncoderFactory(),
      webrtc::CreateBuiltinVideoDecoderFactory(), nullptr ,
      nullptr );

	if (!peer_connection_factory_) {
		cout<<"WebRTCProcessor::InitializePeerConnection > CreatePeerConnectionFactory Error"<<endl;
		DeletePeerConnection();
		return false;
	}
	if (!CreatePeerConnection(/*dtls=*/true)) {
		cout<<"WebRTCProcessor::CreatePeerConnection > False"<<endl;
		DeletePeerConnection();
	}
	AddTracks();
	return peer_connection_ != nullptr;
}

void WebRTCProcessor::DeletePeerConnection() {
	peer_connection_ = nullptr;
	peer_connection_factory_ = nullptr;
}

bool WebRTCProcessor::CreatePeerConnection(bool dtls) {
	std::cout<<"WebRTCProcessor::CreatePeerConnection-------------"<<std::endl;
	RTC_DCHECK(peer_connection_factory_);
	RTC_DCHECK(!peer_connection_);
	webrtc::PeerConnectionInterface::RTCConfiguration config;
	config.sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;
	config.enable_dtls_srtp = dtls;
	webrtc::PeerConnectionInterface::IceServer server;
	server.uri="stun:stun.l.google.com:19302";
	config.servers.push_back(server);
	std::cout<<"Conductor::CreatePeerConnection>CreatePeerConnection-------------"<<std::endl;
	peer_connection_ = peer_connection_factory_ -> CreatePeerConnection(config, nullptr, nullptr, this);

	//dataChannel
	/*webrtc::DataChannelInit dataConfig;
	data_channel_ = peer_connection_ -> CreateDataChannel("data_channel", &dataConfig);
	data_channel_ -> RegisterObserver(dataChannelObserver::Create());
	*/
	return peer_connection_ != nullptr;
}
//
// PeerConnectionObserver implementation.
//
void WebRTCProcessor::OnAddTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver, const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>& streams) {
  RTC_LOG(INFO) << __FUNCTION__ << " " << receiver->id();
  cout<<"WebRtcProcessor::OnAddTrack-----------------"<<std::endl;

  auto* track = static_cast<webrtc::VideoTrackInterface*> (receiver -> track().release());
  if (track -> kind() == webrtc::MediaStreamTrackInterface::kVideoKind) {
	  cout << "WebRtcProcessor::OnAddTrack--------------CustomVideoRenderer" << endl;
	  remote_renderer_.reset(new CustomVideoRenderer (track));
	  remote_renderer_ -> setCallback(this, youtube);
  }

  cout << ms_channel_id << "WebRtcProcessor::OnAddTrack-----------------AddCustomAudioSink" << std::endl;
  auto* atrack = static_cast<webrtc::AudioTrackInterface*>(receiver -> track().release()); //audio track
	if (atrack -> kind() == webrtc::MediaStreamTrackInterface::kAudioKind) {
		audio_renderer = new CustomAudioTrackSink(ms_channel_id);
		atrack -> AddSink(audio_renderer);
		audio_renderer -> setCallback(this, youtube);
	}
}

void WebRTCProcessor::OnRemoveTrack(
    rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) {
  RTC_LOG(INFO) << __FUNCTION__ << " " << receiver->id();
}

void WebRTCProcessor::OnIceCandidate(const webrtc::IceCandidateInterface* candidate) {
  // For loopback test. To save some connecting delay.
	string order="sendIce";
	string sdp;
	string type;
    string candidateStr;
    string sdpmid=candidate->sdp_mid();
    int sdpmlineindex=candidate->sdp_mline_index();
	if (!candidate->ToString(&candidateStr)) {
		RTC_LOG(LS_ERROR) << "Failed to serialize candidate";
		return;
	}
	cout<<"WebRTCProcessor::OnIceCandidate > order"<<order<<" candidate:"<<candidateStr<<" sdpmid:"<<sdpmid<<" sdpmlineindex:"<<sdpmlineindex<<endl;
	sendQueueMsg(order,ms_channel_id,sdp,type,candidateStr,sdpmid,sdpmlineindex);
}

void WebRTCProcessor::OnSuccess(webrtc::SessionDescriptionInterface* desc) {
	string sdp;
	desc->ToString(&sdp);
	std::cout<<"\n\n\n\n\n\n-WebRTCProcessor::OnSuccess > SetLocalDescription-------------sdp:"<<sdp<<"\n\n\n\n\n\n"<<std::endl;
	peer_connection_->SetLocalDescription(DummySetSessionDescriptionObserver::Create(), desc);
	string sdptype=webrtc::SdpTypeToString(desc->GetType());
    string order;
    string msgextra;
    string candidate;
    string sdpmid;
    int sdpmlineindex=0;
	if(isOffer) order="sendOffer";
	else order="sendAnswer";
	sendQueueMsg(order,ms_channel_id,sdp,sdptype,candidate,sdpmid,sdpmlineindex);
}
void WebRTCProcessor::OnFailure(webrtc::RTCError error) {
  RTC_LOG(LERROR) << ToString(error.type()) << ": " << error.message();
}

void WebRTCProcessor::OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> channel){
	std::cout<<"WebRTCProcessor::OnDataChannel--------------Label:"<<channel->label()<<"\n";
}

void WebRTCProcessor::AddTracks() {

	std::cout<<"\n\n\n------------------Conductor::AddTracks--------------\n\n\n";
	  if (!peer_connection_->GetSenders().empty()) {
	    return;  // Already added tracks.
	  }
	  rtc::scoped_refptr<webrtc::AudioTrackInterface> audio_track(
	      peer_connection_factory_->CreateAudioTrack(
	          kAudioLabel, peer_connection_factory_->CreateAudioSource(
	                           cricket::AudioOptions())));
	  auto result_or_error = peer_connection_->AddTrack(audio_track, {kStreamId});
	  if (!result_or_error.ok()) {
	    RTC_LOG(LS_ERROR) << "Failed to add audio track to PeerConnection: "
	                      << result_or_error.error().message();
	  }

	  std::unique_ptr<cricket::VideoCapturer> video_device =
	      OpenVideoCaptureDevice();
	  if (video_device) {
	    rtc::scoped_refptr<webrtc::VideoTrackInterface> video_track_(
	        peer_connection_factory_->CreateVideoTrack(
	            kVideoLabel, peer_connection_factory_->CreateVideoSource(
	                             std::move(video_device), nullptr)));

	    result_or_error = peer_connection_->AddTrack(video_track_, {kStreamId});
	    if (!result_or_error.ok()) {
	      RTC_LOG(LS_ERROR) << "Failed to add video track to PeerConnection: "
	                        << result_or_error.error().message();
	    }
	  } else {
	    RTC_LOG(LS_ERROR) << "OpenVideoCaptureDevice failed";
	  }
  cout<<"Add Track Complete"<<endl;

}
void WebRTCProcessor::createAnswer(string& in_ms_id,string& in_sdp_type, string& in_sdp_string){
	ms_channel_id=in_ms_id;
	cout<<"WebRTCProcessor::createAnswer"<<endl;
	absl::optional<webrtc::SdpType> type_maybe =webrtc::SdpTypeFromString(in_sdp_type);
	webrtc::SdpType type=*type_maybe;
	webrtc::SdpParseError error;
	std::unique_ptr<webrtc::SessionDescriptionInterface> session_description = webrtc::CreateSessionDescription(type, in_sdp_string, &error);
	isOffer=false;
	if (InitializePeerConnection()) {
		cout<<"WebRtcProcessor::createAnswer--------------SetRemoteDescription\n";
	    if (!session_description) {
	      RTC_LOG(WARNING) << "Can't parse received session description message. "
	                       << "SdpParseError was: " << error.description;
	      return;
	    }
		peer_connection_->SetRemoteDescription(DummySetSessionDescriptionObserver::Create(),  session_description.release());
		cout<<"WebRtcProcessor::createAnswer--------------CreateAnswer\n";
	   peer_connection_->CreateAnswer(this, webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());
	} else {
		//main_wnd_->MessageBox("Error", "Failed to initialize PeerConnection", true);
	}
}

void WebRTCProcessor::setAnswer(string& in_ms_id,string& in_sdp_type, string& in_sdp_string){
	ms_channel_id=in_ms_id;
	absl::optional<webrtc::SdpType> type_maybe =webrtc::SdpTypeFromString(in_sdp_type);
	webrtc::SdpType type=*type_maybe;
	cout<<"WebRTCProcessor::setAnswer. Type:"<<endl;
	webrtc::SdpParseError error;
	std::unique_ptr<webrtc::SessionDescriptionInterface> session_description = webrtc::CreateSessionDescription(type, in_sdp_string, &error);
	isOffer=false;
    if (!session_description) {
      RTC_LOG(WARNING) << "Can't parse received session description message. " << "SdpParseError was: " << error.description;
      return;
    }
    auto remoteDescriptionObserver=setRemoteDescriptionObserver::Create(mRecvQueue);
	peer_connection_->SetRemoteDescription(remoteDescriptionObserver,  session_description.release());
}

void WebRTCProcessor::setIce(string& candidate,string& sdpMid,int sdpMLineIndex){
	std::cout<<"WebRTCProcessor::setIce > candidate:"<<candidate<<" sdpMid:"<<sdpMid<<" sdpMLineIndex:"<<sdpMLineIndex<<endl;
	webrtc::SdpParseError error;
	std::unique_ptr<webrtc::IceCandidateInterface> iceCandidate( webrtc::CreateIceCandidate(sdpMid, sdpMLineIndex, candidate, &error));
	std::cout<<"set_ice > sdpMid:"<<sdpMid<<" sdpMLineIndex:"<<sdpMLineIndex<<" candidate:"<<candidate<<endl;
	if(isSetRemoteDescription){
		std::cout<<"WebRTCProcessor::setIce > Set Ice"<<endl;
		if(iceCandidate.get()){
			cout<<"Add Ice Candidate.. > "<<endl;
			if (!peer_connection_->AddIceCandidate(iceCandidate.get())) {
				std::cout<<"\n\n\n------------------"<< "Failed to apply the received candidate"<<"--------------\n\n\n";
			}
		};
	} else {
		std::cout<<"WebRTCProcessor::setIce > add Ice to repository"<<endl;
		candidateObj insCandidate;
		insCandidate.candidate=candidate;
		insCandidate.sdpmid=sdpMid;
		insCandidate.sdpmlineindex=sdpMLineIndex;
		iceRepos.push(insCandidate);
	}
}


void WebRTCProcessor::Run(rtc::Thread* thread){
	cout<<"WebRTCPRocessor::RUN"<<endl;
	startWebRTC();
}
std::unique_ptr<cricket::VideoCapturer> WebRTCProcessor::OpenVideoCaptureDevice() {
	std::cout<<"\n\n\n------------------Conductor::OpenVideoCaptureDevice--------------\n\n\n";
	  std::vector<std::string> device_names;
	  {
	    std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> info(
	        webrtc::VideoCaptureFactory::CreateDeviceInfo());
	    if (!info) {
	      return nullptr;
	    }
	    int num_devices = info->NumberOfDevices();
	    for (int i = 0; i < num_devices; ++i) {
	      const uint32_t kSize = 256;
	      char name[kSize] = {0};
	      char id[kSize] = {0};
	      if (info->GetDeviceName(i, name, kSize, id, kSize) != -1) {
	        device_names.push_back(name);
	      }
	    }
	  }

	  cricket::WebRtcVideoDeviceCapturerFactory factory;
	  std::unique_ptr<cricket::VideoCapturer> capturer;
	  for (const auto& name : device_names) {
	    capturer = factory.Create(cricket::Device(name, 0));
	    if (capturer) {
	      break;
	    }
	  }
	  return capturer;
}
