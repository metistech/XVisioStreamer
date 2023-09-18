#include <iostream>
#include <algorithm>
#include <fstream>
#include <chrono>
#include <ctime>
#include <sstream>
#include <functional>
#include <cmath>

#include <opencv2/core/core.hpp>

#include "XVisio.h"
#include "cvmat_serialization.h"
#include "tick.h"
#include "XVisioStreamClient.h"
#include "XVisioStreamServer.h"
#include "SLAMFrame.h"
#include <csignal>
#include <boost/filesystem.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

volatile bool running = true;

void signalHandler(int signum)
{
    std::cout << "Interrupt signal (" << signum << ") received.\n";

    running = false;
}

// Stub for converting IMU measurements stored in vector to cv::Mat for dataset output
cv::Mat convertImuVectorToMat(std::vector<XVisio::IMU_Point>);

int main(int argc, char *argv[])
{
    Tick mainTick("Main Loop");
    mainTick.set_rate(100);
    std::signal(SIGINT, signalHandler);

    if (argc != 2)
    {
        std::cerr << std::endl
                  << "Usage: ./XVisioStreamer path_to_config_file" << std::endl;
        return 1;
    }

    // Open frontend config file
    cv::FileStorage config(argv[1], cv::FileStorage::READ);

    // Flag to enable dataset recording
    bool record_dataset;
    config["recordDataset"] >> record_dataset;

    // Storage variables for dataset creation
    boost::filesystem::path dataset_output_path;
    cv::FileStorage dataset;

    XVisioStreamServer* streamServer = new XVisioStreamServer();
    std::thread streamServerThread(&XVisioStreamServer::run, streamServer);

    if (record_dataset)
    {
        std::string _dataset_output_path;
        config["datasetOutputPath"] >> _dataset_output_path;
        dataset_output_path = boost::filesystem::path(_dataset_output_path);

        if (boost::filesystem::exists(dataset_output_path))
        {
            // TODO: Add a function prompting for user input to either overwrite or rename existing dataset
            std::cerr << "Dataset already exists. It will be overwritten." << std::endl;
            boost::filesystem::remove_all(dataset_output_path);
        }

        // Create folder structure for dataset output at dataset_output_path
        boost::filesystem::create_directories(dataset_output_path / "images");

        // Create dataset yaml file which will contain IMU readings, image timestamps, and image paths
        dataset = cv::FileStorage((dataset_output_path / "dataset.yml").c_str(), cv::FileStorage::WRITE);
    }

    boost::filesystem::path stream_output_path("/tmp/alcmm_stream");
    boost::filesystem::remove_all(stream_output_path);
    boost::filesystem::create_directories(stream_output_path);
    std::ofstream ssi((stream_output_path/"safe_send_index").c_str(), std::ios_base::binary);
    const int32_t initial_ssi = -1;
    ssi.write(reinterpret_cast<const char*>(&initial_ssi), sizeof(int32_t));
    ssi.close();
    // Current frame index for dataset output
    int frame_index = 0;

    // Object to control and pull data from XVisio DS80 camera
    XVisio::Settings settings;
    XVisio *cam = new XVisio(settings);

    // Storage objects for IMU readings between loops
    std::vector<XVisio::IMU_Point> imu_readings;

    // Running can be set to false with C^c
    while (running)
    {
        // Retrieves either an IMU measurement or an image
        XVisio::XVisioStreamable frame = cam->get_frame_mono();

        // Images are recorded and streamed immediately (assuming enough IMU readings)
        // IMU readings are pushed to a buffer and streamed upon receipt of next image
        if (!frame.is_image)
        {
            imu_readings.push_back(frame.imu);
            continue;
        }

        // If there are too few IMU readings for the frame, print an error and skip the image
        // These IMU readings will be included with the next
        if (imu_readings.size() < 10)
        {
            std::cout << "Too few IMU readings. Skipping Image" << std::endl;
            continue;
        }

        if (record_dataset)
        {
            // Write all IMU readings and image timestamp to dataset yaml file
            dataset << "imu_" + std::to_string(frame_index) << convertImuVectorToMat(imu_readings);
            dataset.write("image_timestamp_" + std::to_string(frame_index), frame.timestamp);

            // Output image as PNG to file, and write the image path to yaml
            std::string image_path((dataset_output_path / "images" / (std::to_string(frame_index) + ".png")).c_str());
            dataset.write("image_path_" + std::to_string(frame_index), image_path);
            cv::imwrite(image_path, frame.image_left);
        }

        // Populate our serializeable frame struct for transmission to the SLAM frontend     
        SLAMFrame f(frame.timestamp, frame.image_left, imu_readings);
        // f.image.resize(10);
        // memcpy(f.image.data(), frame.image_left.datastart, 10);
        {
            std::ofstream ofs((stream_output_path/(std::to_string(frame_index) + ".alcmm")).c_str());
            boost::archive::binary_oarchive oa(ofs);
            oa.register_type<SLAMFrame>();
            oa << f;
            // std::ofstream image_ofs((stream_output_path/"images"/(std::to_string(frame_index) + ".bin")).c_str(), std::ios_base::binary);
            // image_ofs.write(reinterpret_cast<const char*>(frame.image_left.data), 256000);

            // Update safe_send_index
            // Wait for lock to be free
            while(boost::filesystem::exists(stream_output_path/"safe_send_index_lock")) std::this_thread::sleep_for(std::chrono::milliseconds(10));

            // Lock the lock
            std::ofstream((stream_output_path/"safe_send_index_lock").c_str(), std::ios_base::binary).close();

            std::ofstream ssi((stream_output_path/"safe_send_index").c_str(), std::ios_base::binary);
            const int32_t initial_ssi = frame_index;
            ssi.write(reinterpret_cast<const char*>(&initial_ssi), sizeof(int32_t));
            ssi.close();

            boost::filesystem::remove(stream_output_path/"safe_send_index_lock");
        }
        // Clear imu_readings which have been passed to the SLAM system
        imu_readings.clear();

        // Increment the frame index in preparation for the next image
        frame_index++;

        mainTick.tick();
        
        // if(frame_index == 20){
        //     streamServer->requestOne();
        //     break;
        // }

    } // Data collection ended

    std::cout << "Stopping the system!" << std::endl;

    streamServer->requestStop();
    streamServerThread.join();
    // Write the total number of collected frames to the dataset yaml file and close the file
    if (record_dataset)
    {
        dataset.write("num_frames", frame_index);
        dataset.release();
    }

    // Deactivate all callbacks from the XVisio camera
    cam->stop_all();

    // Request all threads in the SLAM system complete their work and shut down
    return 0;
}

cv::Mat convertImuVectorToMat(std::vector<XVisio::IMU_Point> imus)
{
    cv::Mat out;
    for (auto imu : imus)
        out.push_back(imu.asMat());
    return out;
}
