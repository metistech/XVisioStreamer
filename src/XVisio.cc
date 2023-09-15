#include "XVisio.h"
#include <typeinfo>
#include <pthread.h>
#include <functional>

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

XVisio::XVisio(XVisio::Settings settings)
{
    std::cout << "Discovering XVisio Devices" << std::endl;
    auto devices = xv::getDevices(10.);
    if (!devices.empty())
    {
        device = devices.begin()->second;
    }
    else
    {
        std::cerr << "No compatible camera found. Ensure camera is plugged in." << std::endl;
        exit(1);
    }

    imu_callback_id = device->imuSensor()->registerCallback([this](xv::Imu const &imu)
    {
        if((imu.edgeTimestampUs - prev_imu) >= 500){
            double timestamp = static_cast<double>(t_off.get(imu.edgeTimestampUs)) / 1e6;
            IMU_Point p(
                imu.accel[0], imu.accel[1], imu.accel[2],
                imu.gyro[0], imu.gyro[1], imu.gyro[2],
                timestamp
            );
            XVisioStreamable imu_p;
            imu_p.imu = p;
            imu_p.timestamp = timestamp;
            imu_p.is_image = false;
            xvisio_queue.push(imu_p);
            prev_imu = imu.edgeTimestampUs;
        }
    });

    fisheye_callback_id = device->fisheyeCameras()->registerCallback([this](xv::FisheyeImages const &stereo)
    {
        if((stereo.edgeTimestampUs - prev_fisheye) >= 40000)
        {
            XVisioStreamable image;
            image.timestamp = static_cast<double>(t_off.get(stereo.edgeTimestampUs)) / 1e6;
            image.is_image = true;
            auto const &leftInput = stereo.images[0];
            if (leftInput.data != nullptr)
            {
                image.image_left = cv::Mat::zeros(leftInput.height, leftInput.width, CV_8UC1);
                std::memcpy(image.image_left.data, leftInput.data.get(), static_cast<size_t>(image.image_left.rows * image.image_left.cols));
                xvisio_queue.push(image);
            }
            prev_fisheye = stereo.edgeTimestampUs;
        }
    });

    // DO NOT REMOVE -- For some reason, the XVISIO SDK will not output edge timestamps unless there is a keypoint callback registered.
    // keypoint_callback_id = device->orientationStream()->registerCallback([this](xv::Orientation const &orientation) {std::cout << "HERE" << std::endl;});
    keypoint_callback_id = std::dynamic_pointer_cast<xv::FisheyeCamerasEx>(device->fisheyeCameras())->registerKeyPointsCallback([](const xv::FisheyeKeyPoints<2, 32> &keypoints) {});

    std::dynamic_pointer_cast<xv::FisheyeCamerasEx>(device->fisheyeCameras())->setExposure(1, 8, 1);
}

XVisio::XVisioStreamable XVisio::get_frame_mono()
{
    if (first_frame)
    {
        reset_queues();
        first_frame = false;
    }
    while(xvisio_queue.size() < 1050) std::this_thread::sleep_for(std::chrono::nanoseconds(IMU_SLEEP_NS));
    return xvisio_queue.getAndPop();
}

void XVisio::reset_queues()
{
    xvisio_queue.clear();
    t_off.unset();
}

void XVisio::stop_all()
{
    device->imuSensor()->unregisterCallback(imu_callback_id);
    device->fisheyeCameras()->unregisterCallback(fisheye_callback_id);
    std::dynamic_pointer_cast<xv::FisheyeCamerasEx>(device->fisheyeCameras())->unregisterCallback(keypoint_callback_id);
    device.reset();
}