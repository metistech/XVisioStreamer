#pragma once

#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <iostream>
#include <unistd.h>
#define PORT 8080

#include <boost/filesystem.hpp>

namespace XVisioStream
{
    enum REQUEST : uint8_t
    {
        GET_ALL_AVAILABLE_FRAMES = 0,
        RESET = 1,
        STOP = 2,
        START = 3
    };
    enum STATE : uint8_t
    {
        READY = 0,
        RUNNING = 1,
        RESETTING = 2,
        STOPPED = 3
    };
}

class XVisioStreamServer
{
private:
    XVisioStream::STATE state = XVisioStream::STATE::READY;
    std::mutex state_mutex;
    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};
    char *hello = "Hello from server";
    XVisioStream::REQUEST request_type;

    bool stopRequested = false;
    std::mutex stop_mutex;

    int cur_send_index = 0;
    
    boost::filesystem::path stream_output_path = boost::filesystem::path("/tmp/alcmm_stream");

public:
    void run();
    void requestOne();
    void requestStop();

};