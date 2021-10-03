#pragma once

#include "Monitor.h"
#include "MediaPack.h"

#include <memory>
#include <atomic>
#include <thread>
#include <chrono>
#include <iostream>

using namespace std::chrono;


class MediaPlayer {
public:
	MediaPlayer() = delete;
	MediaPlayer(Monitor& monitor);
	
	~MediaPlayer();

	bool SetMedia(std::unique_ptr<MediaPack> new_media);
	bool SetScaling();

	bool StartPlayer(bool loop = false);
	void StopPlayer();
	void PlayerThreadFunction();

	int GetMonitorID();
	ms GetFrameDuration();

private:
	Monitor &monitor;
	std::unique_ptr<MediaPack> current_media;

	std::thread player_thread;
	bool is_playing = false;
	bool loop_media = false;

	std::atomic_flag keep_drawing = ATOMIC_FLAG_INIT;

	Frame frame;
};