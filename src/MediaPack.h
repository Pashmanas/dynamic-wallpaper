#pragma once

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

#include <iostream>
#include <string>
#include <chrono>
#include <thread>

#include "Frame.h"

typedef std::chrono::duration<float, std::milli> ms;


class MediaPack {
public:
	MediaPack(std::string path);

	~MediaPack();

	MediaPack(const MediaPack& obj);
	MediaPack& operator=(const MediaPack& obj) = delete;
	MediaPack(MediaPack&& obj) noexcept;
	MediaPack& operator=(MediaPack&& obj) = delete;

	bool SetScaling(long width = 0, long height = 0);

	int GetNextFrame(Frame &frame, bool loop_media = false);

	bool IsLoaded();

	bool GetVideoResolution(long& width, long& height);
	ms GetFrameDuration();
	std::string GetMediaPath();

private:
	bool LoadMedia(std::string path);
	void FreeMedia();

	std::string path_to_media;

	bool is_loaded = false;
	long scaling_width = 0, scaling_height = 0;

	int video_stream_idx = -1, audio_stream_idx = -1;

	// media contexts
	AVFormatContext* media_ctx = NULL;
	AVCodecParameters* video_codec_params = NULL;
	AVCodec* video_codec = NULL;
	AVCodecContext* video_codec_ctx = NULL;

	// video decoding params
	AVFrame* video_frame_raw = NULL, * video_frame_rgb = NULL;
	uint8_t* video_buffer = NULL;
	size_t video_buffer_size = 0;
	int video_linesize = 0;

	// video scaling/conversion context
	SwsContext* sws_ctx = NULL;
	uint8_t* sws_buffer = NULL;
	size_t sws_buffer_size = 0;

	// video stream
	long frame_width = 0, frame_height = 0;
	double frame_rate = 0.0;
	double base_time = 0.0;
	int64_t duration = 0;
	ms frame_duration;
};