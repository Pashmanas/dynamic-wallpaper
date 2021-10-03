#include "MediaPack.h"

#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "swscale.lib")
#pragma comment(lib, "avutil.lib")


MediaPack::MediaPack(std::string path) :
	path_to_media(path), frame_duration(0)
{
	if (!LoadMedia(path_to_media))
		throw std::exception("Can't load media.");
}

MediaPack::~MediaPack()
{
	FreeMedia();
}

MediaPack::MediaPack(const MediaPack& obj) :
	frame_duration(0)
{
	if (obj.is_loaded) {
		if (!LoadMedia(path_to_media))
			throw std::exception("Can't copy media object.");
	}
}


MediaPack::MediaPack(MediaPack&& obj) noexcept :
	path_to_media(std::move(obj.path_to_media)), is_loaded(obj.is_loaded), scaling_width(obj.scaling_width),
	scaling_height(obj.scaling_height), video_stream_idx(obj.video_stream_idx), audio_stream_idx(obj.audio_stream_idx),
	video_linesize(obj.video_linesize), sws_buffer_size(obj.sws_buffer_size), frame_width(obj.frame_width),
	frame_height(obj.frame_height), frame_rate(obj.frame_rate), base_time(obj.base_time), duration(obj.duration),
	frame_duration(obj.frame_duration)
{
	media_ctx = obj.media_ctx;
	obj.media_ctx = nullptr;

	video_codec_params = obj.video_codec_params;
	obj.video_codec_params = nullptr;

	video_codec = obj.video_codec;
	obj.video_codec = nullptr;

	video_codec_ctx = obj.video_codec_ctx;
	obj.video_codec_ctx = nullptr;

	video_frame_raw = obj.video_frame_raw;
	obj.video_frame_raw = nullptr;

	video_frame_rgb = obj.video_frame_rgb;
	obj.video_frame_rgb = nullptr;

	video_buffer = obj.video_buffer;
	obj.video_buffer = nullptr;

	sws_ctx = obj.sws_ctx;
	obj.sws_ctx = nullptr;

	sws_buffer = obj.sws_buffer;
	obj.sws_buffer = nullptr;

	obj.is_loaded = false;
}

bool MediaPack::SetScaling(long width, long height)
{
	if (!is_loaded)
		return false;

	// don't scale frames
	if (width == 0 || height == 0) {
		// exactly 2 must be zero
		if (width != 0 || height != 0)
			return false;

		width = video_codec_ctx->width;
		height = video_codec_ctx->height;
	}

	
	// scaling params
	scaling_width = width;
	scaling_height = height;

	video_frame_raw = av_frame_alloc();
	if (!video_frame_raw) {
		return false;
	}
	video_frame_rgb = av_frame_alloc();
	if (!video_frame_rgb) {
		return false;
	}

	// determine required buffer size and allocate buffer for converted frame
	video_buffer_size = av_image_get_buffer_size(AV_PIX_FMT_BGR24, width, height, 32);
	video_buffer = (uint8_t*)av_malloc(video_buffer_size);
	if (!video_buffer)
		return false;

	int ret_code = av_image_fill_arrays(video_frame_rgb->data, video_frame_rgb->linesize, video_buffer,
		AV_PIX_FMT_BGR24, width, height, 32);
	if (ret_code < 0) {
		return false;
	}

	video_linesize = video_frame_rgb->linesize[0];

	// initialize SWS context for software scaling
	sws_ctx = sws_getContext(
		video_codec_ctx->width,
		video_codec_ctx->height,
		video_codec_ctx->pix_fmt,
		scaling_width,
		scaling_height,
		AV_PIX_FMT_BGR24,
		SWS_BICUBIC,
		NULL,
		NULL,
		NULL
	);
	if (!sws_ctx) {
		return false;
	}

	return true;
}

int MediaPack::GetNextFrame(Frame& frame, bool loop_media)
{
	if (!is_loaded)
		return -1;

	// packet with undecoded frame
	AVPacket packet;

	int ret_code = 0;

	while (true) {
		ret_code = av_read_frame(media_ctx, &packet);
		if (ret_code < 0) {
			// seek to the first frame if looping is enabled
			if (ret_code == AVERROR_EOF && loop_media) {
				av_seek_frame(media_ctx, video_stream_idx, 0, AVSEEK_FLAG_ANY);
				return 0;
			}
			else
				return ret_code;

		}

		if (packet.stream_index == video_stream_idx) {
			// send packet to codec decoder
			ret_code = avcodec_send_packet(video_codec_ctx, &packet);
			if (ret_code < 0) {
				av_packet_unref(&packet);
				return ret_code;
			}

			while (ret_code >= 0) {
				ret_code = avcodec_receive_frame(video_codec_ctx, video_frame_raw);
				if (ret_code == AVERROR(EAGAIN) || ret_code == AVERROR_EOF) {
					av_packet_unref(&packet);
					break;
				}
				else if (ret_code < 0) {
					av_packet_unref(&packet);
					return ret_code;
				}

				if (ret_code >= 0) {
					int slice_height = sws_scale(sws_ctx, video_frame_raw->data,
						video_frame_raw->linesize, 0, video_codec_ctx->height,
						video_frame_rgb->data, video_frame_rgb->linesize);

					// because we convert frame to BGR format - we can send to output buffer only the first plane
					frame.frame_buf = video_frame_rgb->data[0];
					frame.original_width = scaling_width;
					frame.original_height = scaling_height;
					frame.linesize = video_linesize;

					av_frame_unref(video_frame_raw);
					av_packet_unref(&packet);

					return 0;
				}
			}
		}

		// Free the packet that was allocated by av_read_frame
		av_packet_unref(&packet);
	}

	return 0;
}

bool MediaPack::IsLoaded()
{
	return is_loaded;
}

bool MediaPack::GetVideoResolution(long& width, long& height)
{
	if (!is_loaded)
		return false;

	width = frame_width;
	height = frame_height;

	return true;
}

ms MediaPack::GetFrameDuration()
{
	return frame_duration;
}

std::string MediaPack::GetMediaPath()
{
	return path_to_media;
}

bool MediaPack::LoadMedia(std::string path)
{
	if (path.empty())
		return false;

	int ret_code = 0;

	// read media container header
	ret_code = avformat_open_input(&media_ctx, path.c_str(), NULL, NULL);
	if (ret_code) {
		return false;
	}

	if (avformat_find_stream_info(media_ctx, NULL) < 0) {
		return false;
	}

	// find the first video and audio stream
	video_stream_idx = -1;
	audio_stream_idx = -1;
	for (unsigned int i = 0; i < media_ctx->nb_streams; i++) {
		if (media_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && video_stream_idx == -1) {
			video_stream_idx = i;
		}
		if (media_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && audio_stream_idx == -1) {
			audio_stream_idx = i;
		}
	}
	if (video_stream_idx == -1) {
		return false;
	}

	// get a pointer to the codec params for the video stream
	video_codec_params = media_ctx->streams[video_stream_idx]->codecpar;

	// find the decoder for the video stream
	video_codec = avcodec_find_decoder(video_codec_params->codec_id);
	if (!video_codec) {
		return false;
	}
	
	// get codec context
	video_codec_ctx = avcodec_alloc_context3(video_codec);
	if (!video_codec_ctx) {
		return false;
	}
	if (avcodec_parameters_to_context(video_codec_ctx, video_codec_params) < 0) {
		return false;
	}

	// TODO: add multithreading suport (std::thread::hardware_concurrency)

	// multithreading decoding options
	//video_codec_ctx->thread_count = 3;
	//video_codec_ctx->thread_type = FF_THREAD_SLICE;

	// open codec for decoding video stream
	if (avcodec_open2(video_codec_ctx, video_codec, NULL) < 0) {
		return false;
	}

	// get needed info about video stream
	frame_width = video_codec_params->width;
	frame_height = video_codec_params->height;
	base_time = av_q2d(media_ctx->streams[video_stream_idx]->time_base);

	// check if we have duration value in stream info
	if (media_ctx->streams[video_stream_idx]->duration != AV_NOPTS_VALUE) {
		// MP4, AVI...
		duration = media_ctx->streams[video_stream_idx]->duration * base_time;
		frame_rate = av_q2d(media_ctx->streams[video_stream_idx]->r_frame_rate);
	}
	else {
		// WEBM...
		duration = media_ctx->duration / AV_TIME_BASE;
		frame_rate = av_q2d(media_ctx->streams[video_stream_idx]->avg_frame_rate);
	}

	typedef std::chrono::duration<double> sec;
	sec seconds_c{ (double)media_ctx->streams[video_stream_idx]->r_frame_rate.den / media_ctx->streams[video_stream_idx]->r_frame_rate.num };
	frame_duration = std::chrono::duration_cast<ms>(seconds_c);

	is_loaded = true;

	return true;
}

void MediaPack::FreeMedia()
{
	if (video_buffer)
		av_freep(&video_buffer);

	if (video_frame_raw)
		av_frame_free(&video_frame_raw);

	if (video_frame_rgb)
		av_frame_free(&video_frame_rgb);

	if (sws_ctx)
		sws_freeContext(sws_ctx);


	if (video_codec_ctx) {
		avcodec_close(video_codec_ctx);
		avcodec_free_context(&video_codec_ctx);
	}

	if (media_ctx)
		avformat_close_input(&media_ctx);
}
