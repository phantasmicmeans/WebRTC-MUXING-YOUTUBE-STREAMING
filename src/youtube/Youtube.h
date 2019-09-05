/*
 * Youtube.h
 *
 *  Created on: Aug 29, 2019
 *      Author: sangmin (phantasmicmeans)
 */

#ifndef YOUTUBE_YOUTUBE_H_
#define YOUTUBE_YOUTUBE_H_
#include <iostream>
#include <string.h>
#include <vector>
#include <unistd.h>
#include <math.h>
#include <thread>
#include <queue>

#define HAVE_CBRT true
#define HAVE_CBRTF true
#define HAVE_COPYSIGN true
#define HAVE_ERF true
#define HAVE_HYPOT true
#define HAVE_RINT true
#define HAVE_ROUND true
#define HAVE_ROUNDF true
#define HAVE_TRUNC true
#define HAVE_TRUNCF true

extern "C" {
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
	#include <libavdevice/avdevice.h>
	#include <libavutil/avutil.h>
	#include <libavutil/libm.h>
	#include <libavutil/avassert.h>
	#include <libavutil/channel_layout.h>
	#include <libswscale/swscale.h>
	#include <libavutil/timestamp.h>
	#include <libavformat/url.h>
	#include <libavutil/time.h>
	#include <libavutil/error.h>
	#include <libswresample/swresample.h>
}

/*
 * Muxing And Streaming
 * Encoder (video = h264 / audio = mp3 .. etc)
 * Muxer (flv .. etc)
 * Output (rtmp or file .. etc)
 */
class Youtube {
public:
	Youtube();
	virtual ~Youtube();

	/* ============================================ Video ============================================= */
	void initialize_avformat_context(AVFormatContext *&fctx, const char *format_name);

	void initialize_io_context(AVFormatContext *&fctx, const char *output);

	void set_codec_params(AVFormatContext *&fctx, AVCodecContext *&codec_ctx, double width, double height, int fps, int bitrate);

	void initialize_codec_stream(AVStream *&stream, AVCodecContext *&codec_ctx, AVCodec *&codec, std::string codec_profile);

	SwsContext *initialize_sample_scaler(AVCodecContext *codec_ctx, double width, double height);

	AVFrame *allocate_frame_buffer(AVCodecContext *codec_ctx,  double width, double height);

	bool write_frame(AVCodecContext *codec_ctx, AVFormatContext *fmt_ctx, AVFrame *frame, AVStream *out_stream);

	AVFrame* dumpYuvToAVFrame(int width, int height, const uint8_t* y, const uint8_t* u, const uint8_t* v);
	/* ============================================ Video ============================================= */

	/* ============================================ Audio ============================================= */
	void initialize_avformat_context_audio(AVFormatContext *&fctx_audio, const char *format_name_audio);

	void initialize_io_context_audio(AVFormatContext *&fctx_audio, const char *output_audio);

	void set_codec_params_audio(AVFormatContext *&fctx_audio, AVCodecContext *&codec_ctx_audio, int bitrate, int sample_rate, int channel);

	void initialize_codec_stream_audio(AVStream *&stream_audio, AVCodecContext *&codec_ctx, AVCodec *&codec);

	AVFrame* allocate_frame_buffer_audio(AVCodecContext *codec_ctx_audio);

	bool write_frame_audio(AVCodecContext *codec_ctx_audio, AVFormatContext *fmt_ctx, AVFrame *frame_audio, AVStream *out_stream_audio, int sample_count);
	/* ============================================ Audio ============================================= */

	/* ======================================== Video & Audio ========================================= */
	void writes(AVFormatContext *fmt_ctx, const AVRational *time_base, AVStream *st, AVPacket pkt);
	void startSend(); // send AVPacket .. from queue
	/* ======================================== Video & Audio ========================================= */

	/* const char* server = "file:/home/sangmin/test2.flv"; */
	const char* server = "rtmp://a.rtmp.youtube.com/live2/rpvp-ryt5-m42u-deyp";

	/* Video & Audio both output context */
	AVFormatContext *ofmt_ctx = nullptr;

	/* Video */
	AVOutputFormat *ofmt = nullptr;
	AVCodec *out_codec = nullptr;
	AVStream *out_stream = nullptr;
	AVCodecContext *out_codec_ctx = nullptr;
	SwsContext *sws_ctx;

	/* Audio */
	AVCodec *out_codec_audio = nullptr;
	AVStream *out_stream_audio = nullptr;
	AVCodecContext *out_codec_ctx_audio = nullptr;

	bool status = false;
	uint8_t *audio_outbuf;

	/* AVPacket Queue */
	std::queue<AVPacket> packetQueue;

	int video_pts = 0;
	int audio_pts = 0;
};

#endif /* YOUTUBE_YOUTUBE_H_ */
