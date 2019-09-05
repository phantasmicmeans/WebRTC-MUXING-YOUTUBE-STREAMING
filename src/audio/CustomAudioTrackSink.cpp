/*
 * CustomAudioTrackSink.cpp
 *
 *  Created on: Aug 29, 2019
 *      Author: sangmin
 */

#include "../webrtc-processor/WebRTCProcessor.h"

#include "modules/audio_device/dummy/file_audio_device.h"
#include <openssl/hmac.h>
#include <sys/time.h>
#include <unistd.h>
#include <opencv2/core.hpp>
#include "opencv2/objdetect.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include <opencv2/core/utility.hpp>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <queue>
#include <limits>

std::queue<uint16_t> audio_queue;
int audio_buffer_cnt = 0;
int buffer_size = 0;
using namespace std;

int frame_count = 0;
int frame_count_left = 0;
static int sample_count = 480;
int64_t src_ch_layout = AV_CH_LAYOUT_MONO , dst_ch_layout = AV_CH_LAYOUT_MONO;
int src_rate = 48000, dst_rate = 48000; //16000;
enum AVSampleFormat src_sample_fmt = AV_SAMPLE_FMT_S16, dst_sample_fmt = AV_SAMPLE_FMT_S16;
int src_linesize, dst_linesize;
uint8_t **src_data = NULL, **dst_data = NULL;

int dst_bufsize = 0;
//static short* left_audio = new short[2304];
//static short* send_audio = new short[2304];
//static uint16_t* left_audio = new uint16_t[2500];
//static uint16_t* send_audio = new uint16_t[2500];
int send_audio_size = -1;
int left_audio_size = -1;

int temp_audio_size = 0;
int prev_left_audio_size = 0;
CustomAudioTrackSink::CustomAudioTrackSink(string& inChannelId) {
	mChannelId=inChannelId;
	int ret=-1;
	cout << "CustomAudioTrackSink::CustomAudioTrackSink > mChannelId "<< mChannelId << endl;

	/*
	swr_ctx_down = swr_alloc();
	if (!swr_ctx_down) {
		cout << "CustomAudioTrackSink::CustomAudioTrackSink > swr_ctx_down alloc error" << endl;
		exit(-1);
	}

	cout << "CustomAudioTrackSink::CustomAudioTrackSink >set int for rescale" << endl;
	av_opt_set_int(swr_ctx_down, "in_channel_layout", src_ch_layout, 0);
	av_opt_set_int(swr_ctx_down, "in_sample_rate", src_rate, 0);
	av_opt_set_sample_fmt(swr_ctx_down, "in_sample_fmt", src_sample_fmt, 0);

	av_opt_set_int(swr_ctx_down, "out_channel_layout", dst_ch_layout, 0);
	av_opt_set_int(swr_ctx_down, "out_sample_rate", dst_rate, 0);
	av_opt_set_sample_fmt(swr_ctx_down, "out_sample_fmt", dst_sample_fmt, 0);

	if ((ret = swr_init(swr_ctx_down)) < 0) {
		cout << "CustomAudioTrackSink::CustomAudioTrackSink > swr_init Error" << endl;
		exit(-1);
	}
	 */
	cout << "CustomAudioTrackSink::CustomAudioTrackSink > Create CustomAudioTrackSink finished" << endl;
}

void CustomAudioTrackSink::OnData(const void* audio_data, int bits_per_sample, int sample_rate, size_t number_of_channels,size_t number_of_frames) {

//============================================ ORIGINAL ===================================================
	if (!youtube -> status) return;

	mAccessTextLock.lock();
	int dst_bufsize = av_samples_get_buffer_size(NULL,
			youtube->out_codec_ctx_audio->channels,
			youtube->out_codec_ctx_audio->frame_size,
			youtube->out_codec_ctx_audio->sample_fmt, 0);

	auto *frame = youtube->allocate_frame_buffer_audio(
			youtube->out_codec_ctx_audio);
	int ret = avcodec_fill_audio_frame(frame,
			youtube->out_codec_ctx_audio->channels,
			youtube->out_codec_ctx_audio->sample_fmt,
			(const uint8_t*) audio_data, dst_bufsize, 0);

	bool isSend = youtube->write_frame_audio(youtube->out_codec_ctx_audio,
			youtube->ofmt_ctx, frame, youtube->out_stream_audio, sample_count);

	if (isSend)
		//av_frame_free(&frame);

	if (ret >= 0) av_frame_free(&frame);
	sample_count += number_of_frames;
	mAccessTextLock.unlock();

// ============================================ ORIGINAL ===================================================


//nb_sample = number of audio samples (per channel) described by this frame

	/*
	 if (youtube->start) {
	 std::cout << " audio frame in " << std::endl;
	 mAccessTextLock.lock();
	 frame_count++;

	 int max = std::numeric_limits<int>::max();
	 //uint16_t* audio = (uint16_t*)audio_data;
	 short* audio = (short*)audio_data;
	 for (int i = 0; i < max; i++) {
	 //uint16_t data = *audio++;
	 short data = *audio++;
	 //std::cout << data;
	 if (!data) break;
	 if (send_audio_size < dst_bufsize) {
	 send_audio[++send_audio_size] = data;
	 } else {
	 left_audio[++left_audio_size] = data;
	 }
	 }

	 std::cout << "\n";
	 if (send_audio_size == dst_bufsize) {
	 std::cout << " AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA " << std::endl;
	 std::cout << "[send_audio_size] : " << send_audio_size << std::endl;
	 std::cout << "[left_audio_size] : " << left_audio_size << std::endl;
	 std::cout << "[dst_bufsize] : " << dst_bufsize << std::endl;
	 std::cout << " AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA " << std::endl;

	 auto *frame = youtube -> allocate_frame_buffer_audio(youtube -> out_codec_ctx_audio);

	 int left_frame_percent = ((float) left_audio_size /  dst_bufsize) * 100;
	 sample_count += number_of_frames * (frame_count - 1) + left_frame_percent;

	 int ret = avcodec_fill_audio_frame(frame, youtube -> out_codec_ctx_audio -> channels,
	 youtube -> out_codec_ctx_audio -> sample_fmt,
	 (uint8_t*)send_audio, dst_bufsize, 0);

	 for (int i = 0; i < send_audio_size; i++) {
	 if (i < left_audio_size) {
	 send_audio[i] = left_audio[i];
	 left_audio[i] = 0;
	 }
	 else {
	 send_audio[i] = 0;
	 }
	 }

	 send_audio_size = left_audio_size - 1; // convert size
	 left_audio_size = 0;

	 bool isSend = youtube -> write_frame_audio(youtube -> out_codec_ctx_audio,
	 youtube -> ofmt_ctx, frame, youtube -> out_stream_audio, sample_count);

	 if (ret >= 0)
	 av_frame_free(&frame);
	 }
	 mAccessTextLock.unlock()
	 //delete(real_data);
	 /*

	 uint16_t* buffer = (uint16_t*)audio_data;
	 for (int i = 0; i < max; i++) {
	 uint16_t buf = *buffer++;
	 if (!buf)
	 break;
	 audio_queue.push(buf);
	 buffer_size++;
	 if (buffer_size >= dst_bufsize) {

	 }
	 }
	 std::cout << "queuing" << std::endl;

	 if (buffer_size >= dst_bufsize) { // queue size full
	 int left_size = buffer_size - dst_bufsize;

	 uint16_t* real_buf = new uint16_t[dst_bufsize];
	 for (int i = 0; i < buffer_size; i++) {
	 if (i < dst_bufsize) {
	 uint16_t data = audio_queue.front();
	 if (!data) {
	 std::cout << "queue empty" << std::endl;
	 break;
	 }
	 real_buf[i] = data;//audio_queue.front();
	 audio_queue.pop();
	 buffer_size--;
	 }
	 //else
	 //	audio_queue.push(real_buf[i]);
	 } // create real dat

	 swr_convert(swr_ctx_down, dst_data, youtube->out_codec_ctx_audio-> frame_size, (const uint8_t **)&real_buf, youtube->out_codec_ctx_audio->frame_size); // convert

	 auto *frame = youtube -> allocate_frame_buffer_audio(youtube -> out_codec_ctx_audio);

	 std::cout << "left size :" << left_size << std::endl;
	 int left_frame_percent = ((float) left_size /  dst_bufsize) * 100;
	 sample_count += number_of_frames * (frame_count - 1) + left_frame_percent;

	 int ret = avcodec_fill_audio_frame(frame, youtube -> out_codec_ctx_audio -> channels,
	 youtube -> out_codec_ctx_audio -> sample_fmt,
	 (const uint8_t*)dst_data, dst_bufsize, 0);

	 bool isSend = youtube -> write_frame_audio(youtube -> out_codec_ctx_audio, youtube -> ofmt_ctx, frame, youtube -> out_stream_audio, sample_count);
	 frame_count = 0;
	 std::cout << "write_frame_audio, is Send : " << isSend << std::endl;

	 if (isSend) {
	 //			av_frame_free(&frame);
	 }
	 if (ret >= 0)
	 av_frame_free(&frame);
	 delete(real_buf);
	 }
	 }
	 */
	//}
}

void CustomAudioTrackSink::setCallback(WebRTCProcessor* processor, Youtube* ytbe){
	mParentProcessor = processor;
	youtube = ytbe;
	dst_bufsize = av_samples_get_buffer_size(&dst_linesize,
			youtube->out_codec_ctx_audio->channels,
			youtube->out_codec_ctx_audio->frame_size,
			youtube->out_codec_ctx_audio->sample_fmt, 0);
}

CustomAudioTrackSink::~CustomAudioTrackSink() {
    cout << "CustomAudioTrackSink::~CustomAudioTrackSink > Delete Resampler" << endl;
    //swr_free(&swr_ctx_down);
    cout << "CustomAudioTrackSink::~CustomAudioTrackSink > Delete STT" << endl;
}



















