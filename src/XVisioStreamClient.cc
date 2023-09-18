#include "XVisioStreamClient.h"

#include <thread>
#include <chrono>
#include <iostream>

#include "SLAMFrame.h"
#include <boost/filesystem.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui.hpp>
#include <vector>
#include <istream>
#include <streambuf>
#include <XVisio.h>


XVisioStreamClient::XVisioStreamClient()
{
    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return;
    }
  
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
  
    // Convert IPv4 and IPv6 addresses from text to binary
    // form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)
        <= 0) {
        printf(
            "\nInvalid address/ Address not supported \n");
        return;
    }
  
    if ((status
         = connect(client_fd, (struct sockaddr*)&serv_addr,
                   sizeof(serv_addr)))
        < 0) {
        printf("\nConnection Failed \n");
        return;
    }
    
}

void XVisioStreamClient::Request()
{
    uint8_t REQUEST_ALL_FRAMES = 0;
    send(client_fd, &REQUEST_ALL_FRAMES, sizeof(uint8_t), 0);
    std::cout << "SENT REQUEST" << std::endl;
    uint32_t data_size;
    read(client_fd, &data_size, sizeof(uint32_t));
    std::cout << "EXPECTING " << data_size << " BYTES" << std::endl;
    int received = 0;
    std::cout << "Creating Buffer" << std::endl;
    std::vector<char> buf;
    buf.resize(data_size);
    std::cout << "Buffer Created" << std::endl;
    while(received < data_size)
    { 
        received += read(client_fd, &buf[0] + received, data_size - received);
    }
    std::cout << "Request Completed. Received " << data_size << " bytes" << std::endl;
    std::istringstream iss(std::string(&buf[0], data_size), std::ios_base::in | std::ios_base::binary);
    std::cout << "Transformed to istringstream" <<std::endl;
    boost::archive::binary_iarchive ia(iss);
    std::cout << "created archive" << std::endl;
    SLAMFrames f;
    ia >> f;
    std::cout << "Unarchived" << std::endl;
    for(auto frame : f.slam_frames)
    {
        cv::imshow("Received Image", frame.image);
        cv::waitKey(100);
    }
}

void XVisioStreamClient::Stop()
{
    uint8_t STOP = 2;
    send(client_fd, &STOP, sizeof(uint8_t), 0);
    std::cout << "CLIENT DONE" << std::endl;
    // closing the connected socket
    close(client_fd);
}