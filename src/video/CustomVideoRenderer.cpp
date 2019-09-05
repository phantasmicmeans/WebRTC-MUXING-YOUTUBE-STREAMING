/*
 * CustomVideoRenderer.h
 *
 *  Created on: Aug 29, 2019
 *      Author: sangmin (phantasmicmeans)
 */

#include "CustomVideoRenderer.h"

static mutex accessLock;
static int pts = 0;

CustomVideoRenderer::CustomVideoRenderer(webrtc::VideoTrackInterface* track_to_render) : rendered_track_(track_to_render) {
  rendered_track_->AddOrUpdateSink(this, rtc::VideoSinkWants());
  std::cout << "CustomVideoRenderer::CustomVideoRenderer Created" << endl;
}

void CustomVideoRenderer::OnFrame(const webrtc::VideoFrame& yuvframe) {
	rtc::scoped_refptr<webrtc::I420BufferInterface> yuvBuffer( yuvframe.video_frame_buffer()->ToI420());

	int yuvFrameWidth = yuvBuffer->width();
	int yuvFrameHeight = yuvBuffer->height();
	cout<<"\n\nCustomVideoRenderer::OnFrame" <<" width:"<<yuvFrameWidth<<" height:"<<yuvFrameHeight <<") frame_count:" << frame_count++ <<endl;

	if (yuvFrameWidth < 1280 || yuvFrameHeight < 720) return; // resize if you need

	accessLock.lock();
	AVFrame *frame = nullptr;
	try {
		frame = youtube->dumpYuvToAVFrame(yuvFrameWidth, yuvFrameHeight, yuvBuffer->DataY(), yuvBuffer->DataU(), yuvBuffer->DataV());
	} catch (...) {
		std::cout << "======================== [dump YUV TO AVFrame Failed] =============================" << std::endl;
		return;
	}

	pts += av_rescale_q(1, youtube -> out_codec_ctx -> time_base, youtube -> out_stream -> time_base);
	frame -> pts = pts;
	bool sendCompleted = youtube -> write_frame(youtube -> out_codec_ctx, youtube -> ofmt_ctx, frame, youtube -> out_stream);
	std::cout << "send video result :" << sendCompleted << std::endl;

	if (sendCompleted) youtube -> status = true;
	av_frame_free(&frame);
	accessLock.unlock();
}

void CustomVideoRenderer::setCallback(WebRTCProcessor* processor, Youtube* youtube){
	this -> mainProcessor = processor;
	this -> youtube = youtube;
}

CustomVideoRenderer::~CustomVideoRenderer() {
	rendered_track_->RemoveSink(this);
}
