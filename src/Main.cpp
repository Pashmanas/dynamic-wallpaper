#include <iostream>
#include <iomanip>
#include <vector>
#include <deque>

#include "MediaPlayer.h"

// https://github.com/FFMS/ffms2
// ffmpeg easy lib


int main()
{
	av_log_set_level(AV_LOG_WARNING);

	if (!Monitor::Initialize()) {
		std::cout << "Can't get available monitors." << std::endl;
		return 1;
	}

	std::deque<MediaPlayer> media_players;
	for (auto& monitor : Monitor::monitors) {
		media_players.emplace_back(monitor);
	}


	while (true) {
		system("cls");
		std::cout << "Choose option:" << std::endl;
		std::cout << "   1. Play video on monitor." << std::endl;
		std::cout << "   2. Stop playing." << std::endl;
		std::cout << "   10. Exit." << std::endl;

		int option = 0;
		std::cin >> option;

		// play media on monitor
		if (option == 1) {
			std::cout << std::endl << "Select monitor by ID." << std::endl;

			for (size_t i = 0; i < Monitor::monitors.size(); i++) {
				std::cout << "   ID: " << Monitor::monitors[i].monitor_id << ". Is primary: " << (Monitor::monitors[i].is_primary ? "yes" : "no") << std::endl;
			}

			int input_id = -1;
			std::cout << std::endl << "ID: ";
			std::cin >> input_id;

			if (input_id < 0 || input_id >= Monitor::monitors.size()) {
				std::cout << "Wrong ID." << std::endl;
				continue;
			}

			int found_id = -1;
			for (int i = 0; i < media_players.size(); i++) {
				if (media_players[i].GetMonitorID() == input_id)
					found_id = i;
			}

			if (found_id == -1) {
				std::cout << "Can't find player." << std::endl;
				continue;
			}

			std::cout << std::endl << "Enter path to media file: ";

			std::string path_to_media;
			std::cin >> path_to_media;

			std::unique_ptr<MediaPack> media;
			try {
				media = std::make_unique<MediaPack>(path_to_media);
			}
			catch (std::exception & exception) {
				std::cout << "Failed to load. " << exception.what() << std::endl;
			}

			media_players[found_id].SetMedia(std::move(media));
			media_players[found_id].SetScaling();

			std::string loop_choice;
			std::cout << "Loop video (y/n) ?" << std::endl;
			std::cin >> loop_choice;

			media_players[found_id].StartPlayer(loop_choice[0] == 'y' ? true : false);
		}

		// stop media playing
		if (option == 2) {
			for (size_t i = 0; i < Monitor::monitors.size(); i++) {
				std::cout << "   ID: " << Monitor::monitors[i].monitor_id << ". Is primary: " << (Monitor::monitors[i].is_primary ? "yes" : "no") << std::endl;
			}

			int input_id = -1;
			std::cout << std::endl << "Monitor ID: ";
			std::cin >> input_id;

			if (input_id < 0 || input_id >= Monitor::monitors.size()) {
				std::cout << "Wrong ID." << std::endl;
				continue;
			}

			int found_id = -1;
			for (int i = 0; i < media_players.size(); i++) {
				if (media_players[i].GetMonitorID() == input_id)
					found_id = i;
			}

			if (found_id == -1) {
				std::cout << "Can't find player." << std::endl;
				continue;
			}

			media_players[found_id].StopPlayer();
		}

		// exit
		if (option == 10) {
			break;
		}
	}

	Monitor::Finilize();

	return 0;
}