#pragma once

#include <cstdint>

struct Frame {
	// will be changed by MediaPack
	uint8_t* frame_buf = nullptr;
	long original_width = 0, original_height = 0;
	int linesize = 0;

	// will be changed by MediaPlayer
	long x_offset = 0, y_offset = 0;
	long crop_width = 0, crop_height = 0;
};