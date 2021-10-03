#include "MediaPlayer.h"

MediaPlayer::MediaPlayer(Monitor& monitor) :
	monitor(monitor)
{

}

MediaPlayer::~MediaPlayer()
{
	StopPlayer();
}

bool MediaPlayer::SetMedia(std::unique_ptr<MediaPack> new_media)
{
	if (!new_media.get()->IsLoaded())
		return false;

	StopPlayer();

	current_media = std::unique_ptr(std::move(new_media));

	return true;
}

bool MediaPlayer::SetScaling()
{
	if (!current_media)
		return false;

	long monitor_width, monitor_height;
	if (!monitor.GetResolution(monitor_width, monitor_height)) {
		return false;
	}

	long media_width, media_height;
	if (!current_media->GetVideoResolution(media_width, media_height)) {
		return false;
	}


	// set frame params to default
	frame.x_offset = 0;
	frame.y_offset = 0;
	frame.crop_width = monitor_width;
	frame.crop_height = monitor_height;

	// check resolutions
	if (monitor_width == media_width && monitor_height == media_height) {
		// same resolution

		current_media->SetScaling(monitor_width, monitor_height);
	} 
	else if (monitor_width < media_width || monitor_height < media_height) {
		// media is bigger

		std::cout << "Crop (1) or scale (2) image?" << std::endl;

		int option = -1;
		std::cin >> option;

		if (option == 1) {
			// crop

			current_media->SetScaling();

			frame.x_offset = (media_width - monitor_width) / 2;
			frame.y_offset = (media_height - monitor_height) / 2;

			frame.crop_width = (media_width > monitor_width) ? (monitor_width) : media_width;
			frame.crop_height = (media_height > monitor_height) ? (monitor_height) : media_height;
		}
		else if (option == 2) {
			// scale

			current_media->SetScaling(monitor_width, monitor_height);
		}
		else {
			std::cout << "Wrong option." << std::endl;
			return false;
		}
	}
	else {
		// media is smaller
		// just scale it to monitor resolution

		current_media->SetScaling(monitor_width, monitor_height);
	}
	
	return true;
}

bool MediaPlayer::StartPlayer(bool loop)
{
	StopPlayer();

	loop_media = loop;
	keep_drawing.test_and_set();
	player_thread = std::thread(&MediaPlayer::PlayerThreadFunction, this);

	return true;
}

void MediaPlayer::StopPlayer()
{
	if (player_thread.joinable()) {
		keep_drawing.clear();
		player_thread.join();
	}

	return;
}

void MediaPlayer::PlayerThreadFunction()
{
	is_playing = true;

	// timings
	ms frame_duration = GetFrameDuration();
	ms wait_duration, elapsed;
	steady_clock::time_point start, end;

	while (keep_drawing.test_and_set()) {
		start = steady_clock::now();

		int code = current_media->GetNextFrame(frame, loop_media);
		if (code)
			break;

		bool success = monitor.DrawFrame(frame);
		if (!success)
			break;

		end = steady_clock::now();

		elapsed = end - start;
		if (elapsed > frame_duration) {
			wait_duration = ms{ 0 };
			elapsed -= frame_duration;
			//std::cout << "Warning! Frame was delayed by " << elapsed.count() << " ms." << std::endl;
		}
		else {
			wait_duration = frame_duration - elapsed;
		}

		std::this_thread::sleep_for(duration_cast<milliseconds>(wait_duration));
	}

	is_playing = false;

	return;
}

int MediaPlayer::GetMonitorID()
{
	return monitor.monitor_id;
}

ms MediaPlayer::GetFrameDuration()
{
	if (!current_media)
		return std::chrono::duration_cast<ms>(std::chrono::milliseconds(0));

	return current_media->GetFrameDuration();
}
