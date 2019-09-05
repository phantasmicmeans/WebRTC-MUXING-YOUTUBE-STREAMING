/*
 * CustomAudioTrackSink.h
 *
 *  Created on: 2019. 7. 16.
 *      Author: phantasmicmeans
 */


#ifndef CUSTOMAUDIOTRACKSINK_H_
#define CUSTOMAUDIOTRACKSINK_H_

#include <glib.h>
#include <iostream>
#include <fstream>
#include <soxr.h>
#include <opencv2/core/utility.hpp>
#include <opencv2/core.hpp>
#include "opencv2/objdetect.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include <opencv2/core/utility.hpp>
#include "api/mediastreaminterface.h" //for webrtc::AudioTrackSinkInterface
#include "common.h"

#include <map>
#include <mutex>
#include <thread>
#include <json/json.h>
#include <alsa/asoundlib.h>
#include <alsa/control.h>
#include "../youtube/Youtube.h"

using namespace cv;
using namespace std;

class WebRTCProcessor;
class CustomAudioDeviceModule;
class CustomAudioTrackSink : public webrtc::AudioTrackSinkInterface {
public:
	CustomAudioTrackSink(string& inChannelId);
	~CustomAudioTrackSink() override;
	void OnData(const void* audio_data,
	                      int bits_per_sample,
	                      int sample_rate,
	                      size_t number_of_channels,
	                      size_t number_of_frames) override;
	void setCallback(WebRTCProcessor* processor, Youtube *youtube);

protected:
	WebRTCProcessor* mParentProcessor;
private:
	string mChannelId;

	//for audio resampling
	struct SwrContext *swr_ctx_down;
	Youtube *youtube;

	std::mutex mAccessTextLock;
};
#endif /* CUSTOMAUDIOTRACKSINK_H_ */
