#pragma once

#include <xvsdk/xv-sdk.h>
#include <xvsdk/../../include2/xv-sdk-ex.h>
#include <xvsdk/xv-types.h>

#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <queue>
#include <mutex>
#include <chrono>
#include <thread>
#include "tick.h"
#include "AtomicTimestampOffset.h"
#include "AtomicQueue.h"
#include <boost/serialization/access.hpp>

#define FISHEYE_SLEEP_NS static_cast<uint64>((1.0 / 50.0) * 1e9)
#define IMU_SLEEP_NS static_cast<uint64>((1.0 / 1000.0) * 1e9)

class XVisio
{
public:
struct Settings {
    int max_camera_frequency = 50;
    int min_camera_frequency = 7;
    int max_imu_frequency = 1000;
    int min_imu_frequency = 200;
    int percent_change = 10;
};

struct IMU_Point
{
    float a_x;
    float a_y;
    float a_z;

    float w_x;
    float w_y;
    float w_z;

    double timestamp;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & a_x;
        ar & a_y;
        ar & a_z;
        ar & w_x;
        ar & w_y;
        ar & w_z;
        ar & timestamp;
    }

public:
    IMU_Point(float a_x, float a_y, float a_z, float w_x, float w_y, float w_z, double timestamp) :
        a_x(a_x), a_y(a_y), a_z(a_z), w_x(w_x), w_y(w_y), w_z(w_z), timestamp(timestamp) {}
    IMU_Point() : a_x(0.0), a_y(0.0), a_z(0.0), w_x(0.0), w_y(0.0), w_z(0.0), timestamp(0.0) {}

    cv::Mat asMat()
    {
        return cv::Mat_<double>(1, 7) << a_x, a_y, a_z, w_x, w_y, w_z, timestamp;
    }
};

class XVisioStreamable
{
public:
    double timestamp;
    bool operator<(const XVisioStreamable& rhs) const{
        return timestamp < rhs.timestamp;
    }
    bool operator>(const XVisioStreamable& rhs) const{
        return timestamp > rhs.timestamp;
    }
    cv::Mat image_left;
    IMU_Point imu;
    bool is_image;
};

private:
    std::shared_ptr<xv::Device> device;
    int fisheye_callback_id;
    int imu_callback_id;
    int keypoint_callback_id;

    AtomicQueue<XVisioStreamable> xvisio_queue;

    bool first_frame = true;

    bool reset_offset = false;
    uint64_t offset = 0;
    std::mutex offset_mutex;

    void reset_queues();
    void await_data();

    Tick imu_tick = Tick("IMU Rate");
    Tick frame_tick = Tick("Frame Rate");
    Tick get_tick;

    bool in_fisheye_callback = false;
    bool in_imu_callback = false;

    AtomicTimestampOffset t_off;

    int64_t prev_imu = 0;
    int64_t prev_fisheye = 0;

public:
    XVisio(XVisio::Settings);
    XVisioStreamable get_frame_mono();

    void reset() { first_frame = true; };
    void stop_all();
};
