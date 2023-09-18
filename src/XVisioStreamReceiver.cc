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


    // Storage variables for dataset creation
    boost::filesystem::path dataset_output_path;
    cv::FileStorage dataset;

    XVisioStreamClient* streamClient = new XVisioStreamClient();

    // Running can be set to false with C^c
    while (running)
    {
        streamClient->Request();
        break;
        // std::this_thread::sleep_for(std::chrono::milliseconds((rand() % 75) + 75));
    } // Data collection ended

    streamClient->Stop();

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
