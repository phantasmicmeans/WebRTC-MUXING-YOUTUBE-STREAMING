/*
 * Youtube.cpp
 *
 *  Created on: Aug 29, 2019
 *      Author: sangmin
 */

#include "Youtube.h"

Youtube::Youtube() {

	avformat_network_init();
	av_log_set_level(AV_LOG_TRACE);

	initialize_avformat_context(ofmt_ctx, "flv"); // Output Container (Muxer)
	initialize_io_context(ofmt_ctx, server);

	out_codec = avcodec_find_encoder(AV_CODEC_ID_H264); // Video Codec
	std::cout << "out_codec_video " << out_codec->name << std::endl;

	out_codec_audio = avcodec_find_encoder(AV_CODEC_ID_MP3); //Audio Codec
	std::cout << "out_codec_audio" << out_codec_audio->name << std::endl;

	out_stream = avformat_new_stream(ofmt_ctx, out_codec); // create video dummpy stream
	out_stream_audio = avformat_new_stream(ofmt_ctx, out_codec_audio); // create audio dummy stream

	out_stream->id = ofmt_ctx->nb_streams - 1;
	out_stream_audio->id = ofmt_ctx->nb_streams - 1;

	out_codec_ctx = avcodec_alloc_context3(out_codec); // video codec alloc
	out_codec_ctx_audio = avcodec_alloc_context3(out_codec_audio); // audio codec alloc

	set_codec_params(ofmt_ctx, out_codec_ctx, 1280, 720, 30, 2500000); // video codec params
	set_codec_params_audio(ofmt_ctx, out_codec_ctx_audio, 64000, 48000, 1); //bit-rate , sample-rate, channel

	initialize_codec_stream(out_stream, out_codec_ctx, out_codec, "high444");
	initialize_codec_stream_audio(out_stream_audio, out_codec_ctx_audio, out_codec_audio);

	out_stream->codecpar->extradata = out_codec_ctx->extradata;
	out_stream->codecpar->extradata_size = out_codec_ctx->extradata_size;

	/*
	out_stream_audio->codecpar->extradata = out_codec_ctx_audio->extradata;
	out_stream_audio->codecpar->extradata_size = out_codec_ctx_audio->extradata_size;
	*/
	av_dump_format(ofmt_ctx, 0, server, 1);

	//sws_ctx = initialize_sample_scaler(out_codec_ctx, 1280, 720); // for video

	int ret = avformat_write_header(ofmt_ctx, nullptr);
	if (ret < 0) {
		std::cout << "Could not write header!" << std::endl;
		exit(1);
	}

}

Youtube::~Youtube() {
	// TODO Auto-generated destructor stub
}

/**
 * YUV to AVFrame
 */
AVFrame* Youtube::dumpYuvToAVFrame(int width, int height, const uint8_t* y, const uint8_t* u, const uint8_t* v) {
	AVFrame *frame = av_frame_alloc(); // Initialize the frame;
	frame->width = width;
	frame->height = height;
	frame->format = AV_PIX_FMT_YUV420P;

	int numBytes = avpicture_get_size(AV_PIX_FMT_YUV420P, width, height);
	uint8_t *buffer = (uint8_t*) av_malloc(numBytes /** sizeof(uint8_t)*/);

	int ret = avpicture_fill((AVPicture*) frame, buffer, AV_PIX_FMT_YUV420P,frame->width, frame->height);
	if (ret < 0) {
		std::cout << "avpicture_fill failed" << std::endl;
		exit(-1);
	}

	/*
	memcpy(frame->data[0], y, frame->linesize[0] * frame->height);
	memcpy(frame->data[1], u, frame->linesize[1] * frame->height/2);
	memcpy(frame->data[2], v, frame->linesize[2] * frame->height/2);
	*/
	av_free(buffer);

	// Set frame -> data pointers manually
	frame->data[0] = (uint8_t*) y;
	frame->data[1] = (uint8_t*) u;
	frame->data[2] = (uint8_t*) v;
	return frame;
}

void Youtube::initialize_avformat_context(AVFormatContext *&fctx, const char *format_name) {
	int ret = avformat_alloc_output_context2(&fctx, nullptr, format_name, nullptr);
	if (ret < 0) {
		std::cout << "Could not allocate output format context!" << std::endl;
		exit(1);
	}
}

/**
 * output => "rtmp://~" or "file:~" or "rtp://~" etc..
 */
void Youtube::initialize_io_context(AVFormatContext *&fctx, const char *output) {
	if (!(fctx->oformat->flags & AVFMT_NOFILE)) {
		int ret = avio_open2(&fctx->pb, output, AVIO_FLAG_WRITE, nullptr, nullptr);
		if (ret < 0) {
			std::cout << "Could not open output IO context!" << std::endl;
			exit(1);
		}
	}
}

void Youtube::set_codec_params(AVFormatContext *&fctx, AVCodecContext *&codec_ctx, double width, double height, int fps, int bitrate) {
	const AVRational dst_fps = { fps, 1 };
	codec_ctx->codec_tag = 0;
	codec_ctx->codec_id = AV_CODEC_ID_H264;
	codec_ctx->codec_type = AVMEDIA_TYPE_VIDEO;
	codec_ctx->width = width;
	codec_ctx->height = height;
	codec_ctx->gop_size = 12;
	codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
	codec_ctx->framerate = dst_fps;
	codec_ctx->time_base = av_inv_q(dst_fps);
	if (fctx->oformat->flags & AVFMT_GLOBALHEADER) {
		codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	}

}

void Youtube::set_codec_params_audio(AVFormatContext *&fctx, AVCodecContext *&codec_ctx_audio, int bitrate, int sample_rate, int channel) {

	codec_ctx_audio->sample_fmt = AV_SAMPLE_FMT_S16;
	codec_ctx_audio->codec_id = AV_CODEC_ID_MP3;
	codec_ctx_audio->codec_type = AVMEDIA_TYPE_AUDIO;
	codec_ctx_audio->channel_layout = AV_CH_LAYOUT_MONO;
	codec_ctx_audio->bit_rate = 64000;
	codec_ctx_audio->sample_rate = 48000;
	codec_ctx_audio->channels = 1;
	codec_ctx_audio -> time_base = {48000, 1};
	if (fctx->oformat->flags & AVFMT_GLOBALHEADER) {
		codec_ctx_audio->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	}
}

void Youtube::initialize_codec_stream(AVStream *&stream, AVCodecContext *&codec_ctx, AVCodec *&codec, std::string codec_profile) {
	int ret = avcodec_parameters_from_context(stream->codecpar, codec_ctx);
	if (ret < 0) {
		std::cout << "Could not initialize stream codec parameters!" << std::endl;
		exit(1);
	}

	AVDictionary *codec_options = nullptr;
	av_dict_set(&codec_options, "profile", codec_profile.c_str(), 0);
	av_dict_set(&codec_options, "preset", "ultrafast", 0);
	av_dict_set(&codec_options, "tune", "zerolatency", 0);

	// open video encoder
	ret = avcodec_open2(codec_ctx, codec, &codec_options);
	if (ret < 0) {
		std::cout << "Could not open video encoder!" << std::endl;
		exit(1);
	}

}

void Youtube::initialize_codec_stream_audio(AVStream *&stream, AVCodecContext *&codec_ctx, AVCodec *&codec) {
	std::cout << "initialize_codec_stream_audio" << std::endl;
	int ret = avcodec_parameters_from_context(stream->codecpar, codec_ctx);
	if (ret < 0) {
		std::cout << "Could not initialize stream codec parameters!" << std::endl;
		exit(1);
	}

	ret = avcodec_open2(codec_ctx, codec, NULL);
	if (ret < 0) {
		std::cout << "Could not open video encoder!" << std::endl;
		exit(1);
	}
}

SwsContext* Youtube::initialize_sample_scaler(AVCodecContext *codec_ctx, double width, double height) {
	SwsContext *swsctx = sws_getContext(width, height, AV_PIX_FMT_RGB24, width, height, codec_ctx->pix_fmt, SWS_BICUBIC, nullptr, nullptr, nullptr);
	if (!swsctx) {
		std::cout << "Could not initialize sample scaler!" << std::endl;
		exit(1);
	}

	return swsctx;
}


AVFrame* Youtube::allocate_frame_buffer(AVCodecContext *codec_ctx, double width,double height) {
	AVFrame *frame = av_frame_alloc();
	int numBytes = avpicture_get_size(AV_PIX_FMT_YUV420P, codec_ctx->width, codec_ctx->height);
	uint8_t *buffer = (uint8_t*) av_malloc(numBytes * sizeof(uint8_t));

	int ret = avpicture_fill((AVPicture*) frame, buffer, AV_PIX_FMT_YUV420P, width, height);
	frame->width = width;
	frame->height = height;
	frame->format = static_cast<int>(codec_ctx->pix_fmt);
	return frame;
}

AVFrame* Youtube::allocate_frame_buffer_audio(AVCodecContext *codec_ctx_audio) {
	/* frame containing input raw audio */
	AVFrame* audio_frame = av_frame_alloc();
	audio_frame->format = codec_ctx_audio->sample_fmt;
	audio_frame->channel_layout = codec_ctx_audio->channel_layout;
	audio_frame->nb_samples = codec_ctx_audio->frame_size;

	int ret = av_frame_get_buffer(audio_frame, 0);
	if (ret < 0) {
		std::cout << "error allocating an audio buffer" << std::endl;
		exit(-1);
	}

	return audio_frame;
}

bool Youtube::write_frame(AVCodecContext *codec_ctx, AVFormatContext *fmt_ctx, AVFrame *frame, AVStream *out_stream) {
	AVPacket pkt = { 0 };
	av_init_packet(&pkt);

	int ret = avcodec_send_frame(codec_ctx, frame);
	if (ret < 0) {
		std::cout << "Error sending frame to codec context!" << std::endl;
		av_packet_unref(&pkt);
		return false;
	}

	ret = avcodec_receive_packet(codec_ctx, &pkt);
	if (ret < 0) {
		if (ret == AVERROR(EAGAIN)) {
			std::cout << "AVERROR(EAGAIN)" << std::endl;
		} else if (ret == AVERROR_EOF) {
			std::cout << "AVERROR_EOF" << std::endl;
		}
		std::cout << "\n\n [[[ Error receiving packet from video codec context! ]]] \n\n" << std::endl;
		av_packet_unref(&pkt);
		exit(1);
		return false;
	}

	writes(fmt_ctx, &out_codec_ctx_audio->time_base, out_stream, pkt);
	return true;
}

bool Youtube::write_frame_audio(AVCodecContext *codec_ctx, AVFormatContext *fmt_ctx, AVFrame *frame_audio, AVStream *out_stream_audio, int sample_count) {
	frame_audio->pts = av_rescale_q(sample_count, (AVRational ) { 1, 48000 }, out_stream_audio->time_base);
	AVPacket pkt = { 0 };
	av_init_packet(&pkt);

	int ret = avcodec_send_frame(codec_ctx, frame_audio);
	if (ret < 0) {
		std::cout << "Error sending frame audio to codec context! " << std::endl;
		av_packet_unref(&pkt);
		return false;
	}

	ret = avcodec_receive_packet(codec_ctx, &pkt);
	if (ret < 0) {
		if (ret == AVERROR(EAGAIN)) {
			std::cout << "AVERROR(EAGAIN)" << std::endl;
		} else if (ret == AVERROR_EOF) {
			std::cout << "AVERROR_EOF" << std::endl;
		}
		std::cout << "\n\n [[[ Error receiving packet from audio codec context! ]]] \n\n" << std::endl;
		av_packet_unref(&pkt);
		return false;
	}

	writes(fmt_ctx, &codec_ctx->time_base, out_stream_audio, pkt);
	return true;
}


void Youtube::writes(AVFormatContext *fmt_ctx, const AVRational *time_base, AVStream *st, AVPacket pkt) {
	//av_packet_rescale_ts(&pkt, (AVRational) {1, 90000}, st -> time_base);
	switch (st->codec->codec_type) {
	case AVMEDIA_TYPE_VIDEO:
		video_pts = pkt.pts;
		std::cout << "video pts => " << video_pts << std::endl;
		break;
	case AVMEDIA_TYPE_AUDIO:
		audio_pts = pkt.pts;
		audio_pts = pkt.dts;
		std::cout << "audio pts => " << video_pts << std::endl;
		break;
	}

	/* ==================== Original =========================== */
	pkt.stream_index = st->index;
	packetQueue.push(pkt); // queueing ..
	std::cout << "push packet " << std::endl;
	/*==================== Original ==============================*/
}

/*
 * IF LEAVE OR RESOURCE DESTROY Msg from Signal -> exit the client after send left AVPacket
 */
void Youtube::startSend() {
	while (true) {
		if (packetQueue.size() > 30) {
			AVPacket packet = packetQueue.front();
			packetQueue.pop();
			int ret = av_interleaved_write_frame(ofmt_ctx, &packet);
			if (ret < 0) {
				std::cout << "[[[[[[ write frame failed ]]]]]]] : " << ret << std::endl;
			}
			std::cout << "send packet !! " << std::endl;
		}
	}
}

