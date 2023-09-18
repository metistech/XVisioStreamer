#pragma once

#include "cvmat_serialization.h"
#include <opencv2/core/core.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include "XVisio.h"

struct SLAMFrame
{
    friend class boost::serialization::access;    
    double timestamp;
    cv::Mat image;
    std::vector<XVisio::IMU_Point> imu;

    template <class Archive>
    void serialize(Archive &ar, const unsigned int version)
    {
        ar & timestamp;
        ar & image;
        ar & imu;
    }
public:
    SLAMFrame() {};
    SLAMFrame(double timestamp, cv::Mat image, std::vector<XVisio::IMU_Point> imu) : timestamp(timestamp), image(image), imu(imu) {}
};

struct SLAMFrames
{
    friend class boost::serialization::access;    
    std::vector<SLAMFrame> slam_frames;

    template <class Archive>
    void serialize(Archive &ar, const unsigned int version)
    {
        ar & slam_frames;
    }
public:
    SLAMFrames() {};
    void push_back(SLAMFrame f) { slam_frames.push_back(f); }
};