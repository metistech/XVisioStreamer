#include <iostream>
#include <algorithm>
#include <fstream>
#include <chrono>
#include <ctime>
#include <sstream>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui.hpp>

#include <System.h>
#include "ImuTypes.h"
#include "XVisio.h"
#include "tick.h"
#include <csignal>
#include <chrono>
// COVINS
#include <covins_base/config_comm.hpp> //for covins_params

#ifdef COMPILEDWITHC17
#include <filesystem>
#else
#include <boost/filesystem.hpp>
#endif

volatile bool running = true;

void signalHandler(int signum)
{
    cout << "Interrupt signal (" << signum << ") received.\n";

    running = false;
}

int main(int argc, char *argv[])
{
    const bool frame_by_frame = false;
    const bool force_resets = true;

    uint64_t frame_index = 0;

    std::signal(SIGINT, signalHandler);

    Tick mainLoopTicker("Main Loop");
    mainLoopTicker.set_rate(20);

    if (argc != 5)
    {
        cerr << endl
             << "Usage: ./mono_inertial_euroc path_to_vocabulary path_to_settings server_ip path/to/dataset" << endl;
        return 1;
    }

    // Create SLAM system. It initializes all system threads and gets ready to process frames.
#ifdef COVINS_MOD
    ORB_SLAM3::System SLAM(argv[1], argv[2], ORB_SLAM3::System::IMU_MONOCULAR, argv[3], covins_params::orb::activate_visualization);
#else
    ORB_SLAM3::System SLAM(argv[1], argv[2], ORB_SLAM3::System::IMU_MONOCULAR, argv[3], true);
#endif

    cout.precision(5);
    std::string dataset_path = argv[4];
    cv::FileStorage dataset = cv::FileStorage(dataset_path + "/dataset.yml", cv::FileStorage::READ);

    int num_frames;
    dataset["num_frames"] >> num_frames;

    bool first_frame = true;

    cv::namedWindow("Current Frame");
    cv::startWindowThread();

    double start_time;
    double start_image_ts;

    while (running && frame_index < num_frames)
    {
        std::vector<ORB_SLAM3::IMU::Point> imu_readings;
        cv::Mat imu_dataset_readings;
        if (force_resets)
        {
            bool reset = false;
            bool reset_active_map = false;
            dataset["reset_" + std::to_string(frame_index)] >> reset;
            dataset["reset_active_map_" + std::to_string(frame_index)] >> reset_active_map;
            if (reset)
            {
                SLAM.Reset();
            }
            else if (reset_active_map)
            {
                SLAM.ResetActiveMap();
            }
        }
        dataset["imu_" + std::to_string(frame_index)] >> imu_dataset_readings;
        for (int i = 0; i < imu_dataset_readings.rows; i++)
        {
            ORB_SLAM3::IMU::Point p = ORB_SLAM3::IMU::Point(
                imu_dataset_readings.at<double>(i, 1), imu_dataset_readings.at<double>(i, 2), imu_dataset_readings.at<double>(i, 3),
                imu_dataset_readings.at<double>(i, 4), imu_dataset_readings.at<double>(i, 5), imu_dataset_readings.at<double>(i, 6),
                imu_dataset_readings.at<double>(i, 0));
            imu_readings.push_back(p);
            if (frame_index == 0 && i == 0)
                std::cout << p << std::endl;
        }

        std::string image_path;
        dataset["image_path_" + std::to_string(frame_index)] >> image_path;
        cv::Mat image = cv::imread(dataset_path + "/" + image_path, cv::IMREAD_GRAYSCALE);

        double image_timestamp;
        dataset["image_timestamp_" + std::to_string(frame_index)] >> image_timestamp;
        frame_index++;

        if (frame_by_frame)
        {
            cv::waitKey(0);
        }
        else
        {
            if (first_frame)
            {
                start_time = static_cast<double>(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count()) / 1e9;
                start_image_ts = image_timestamp;
                first_frame = false;
            }
            else
            {
                double now = static_cast<double>(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count()) / 1e9;
                if (now < start_time + (image_timestamp - start_image_ts))
                    std::this_thread::sleep_for(std::chrono::nanoseconds(static_cast<uint64_t>(((start_time + (image_timestamp - start_image_ts)) - now) * 1e9)));
            }
        }

        cv::equalizeHist(image, image);
        cv::imshow("Current Frame", image);
        if(rand() % 5 != 0) SLAM.TrackMonocular(image, image_timestamp, imu_readings);
    }
    dataset.release();
    SLAM.Shutdown();

    return 0;
}
