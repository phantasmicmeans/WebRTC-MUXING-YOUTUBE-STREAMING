/*
 * CustomVideoRenderer.h
 *
 *  Created on: Aug 29, 2019
 *      Author: sangmin (phantasmicmeans)
 */

#ifndef CUSTOMVIDEORENDERER_H_
#define CUSTOMVIDEORENDERER_H_

#include <iostream>
#include "api/mediastreaminterface.h"
#include "api/video/video_frame.h"
#include "api/video/video_sink_interface.h"

#include <thread>
#include <mutex>
#include "../youtube/Youtube.h"

using namespace std;
class WebRTCProcessor;
class CustomVideoRenderer : public rtc::VideoSinkInterface<webrtc::VideoFrame> {
public:
	CustomVideoRenderer( webrtc::VideoTrackInterface* track_to_render);
	virtual ~CustomVideoRenderer();

	void OnFrame(const webrtc::VideoFrame& frame) override;
	void setCallback(WebRTCProcessor* processor, Youtube* youtube);

protected:
	rtc::scoped_refptr<webrtc::VideoTrackInterface> rendered_track_ = nullptr;
	WebRTCProcessor* mainProcessor = nullptr;
	Youtube* youtube = nullptr;
private:
	int frame_count = 0;
};
#endif /* CUSTOMVIDEORENDERER_H_ */
