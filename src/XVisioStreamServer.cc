#include "XVisioStreamServer.h"

#include <thread>
#include "cvmat_serialization.h"
#include <boost/serialization/vector.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include "SLAMFrame.h"

void XVisioStreamServer::run()
{
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        std::cerr << "XVisio Stream Server socket creation failed" << std::endl;
        return;
    }

    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET,
                   SO_REUSEADDR | SO_REUSEPORT, &opt,
                   sizeof(opt)))
    {
        std::cerr << "ERROR: XVisio Stream Server failed to create fd" << std::endl;
        return;
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (struct sockaddr *)&address,
             sizeof(address)) < 0)
    {
        std::cerr << "ERROR: XVisio Stream Server failed bind to port " << PORT << std::endl;
        return;
    }
    if (listen(server_fd, 1) < 0)
    {
        std::cerr << "ERROR: XVisio Stream Server connection request received, but failed to connect" << std::endl;
        return;
    }

    struct timeval tv1;
    tv1.tv_sec = 1;
    tv1.tv_usec = 0;
    while (true)
    {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(server_fd, &rfds);
        int recVal = select(server_fd + 1, &rfds, NULL, NULL, &tv1);
        if (recVal == 0)
        {
            std::cout << "Waiting for connection" << std::endl;
            continue;
        }
        if (recVal == -1)
            exit(EXIT_FAILURE);
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
                                 (socklen_t *)&addrlen)) < 0)
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        else
        {
            break;
        }
    }

    int32_t current_ssi = -1;

    while (true)
    {
        valread = read(new_socket, &request_type, sizeof(uint8_t));
        if (request_type == XVisioStream::REQUEST::START)
        {
            std::unique_lock<std::mutex>(state_mutex);
            state = XVisioStream::STATE::RUNNING;
        }
        else if (request_type == XVisioStream::REQUEST::GET_ALL_AVAILABLE_FRAMES)
        {
            std::cout << "Request received: GET_ALL_AVAILABLE_FRAMES" << std::endl;

            // Update safe_send_index
            while(current_ssi == -1)
            {
                while (boost::filesystem::exists(stream_output_path / "safe_send_index_lock"))
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                std::ofstream((stream_output_path / "safe_send_index_lock").c_str(), std::ios_base::binary).close();
                std::ifstream ssi((stream_output_path / "safe_send_index").c_str(), std::ios_base::binary);
                ssi.read(reinterpret_cast<char *>(&current_ssi), sizeof(int32_t));
                ssi.close();
                boost::filesystem::remove(stream_output_path / "safe_send_index_lock");
                if(current_ssi == -1) std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            std::cout << "Safe Send Index: " << current_ssi << std::endl;
            std::cout << "Number of frames to package: " << current_ssi - cur_send_index << std::endl;
            SLAMFrames fs;
            for (int i = cur_send_index; i <= current_ssi; i++)
            {
                SLAMFrame f;
                std::ifstream frame_data_ifstream((stream_output_path / (std::to_string(i) + ".alcmm")).c_str());
                boost::archive::binary_iarchive ia(frame_data_ifstream);
                ia >> f;
                fs.push_back(f);
            }

            std::stringstream ss;
            boost::archive::binary_oarchive oa(ss);
            oa << fs;
            ss.seekg(0, std::ios::end);
            uint32_t data_size = ss.tellg();
            ss.seekg(0, std::ios::beg);
            uint32_t sent = 0;
            write(new_socket, &data_size, sizeof(uint32_t));
            while(sent < data_size)
            {
                sent += write(new_socket, ss.str().data(), data_size - sent);
            }
            cur_send_index = current_ssi;
        }
        else if (request_type == XVisioStream::REQUEST::RESET)
        {
            std::cout << "Request received: RESET" << std::endl;
        }
        else if (request_type == XVisioStream::REQUEST::STOP)
        {
            std::cout << "Request received: STOP" << std::endl;
            {
                std::unique_lock<std::mutex>(stop_mutex);
                stopRequested = true;
            }
        }
        else
        {
            break;
        }

        {
            std::cout << "Checking Stop Condition" << std::endl;
            std::unique_lock<std::mutex>(stop_mutex);
            if (stopRequested)
                break;
        }
    }

    // closing the connected socket
    close(new_socket);
    // closing the listening socket
    shutdown(server_fd, SHUT_RDWR);
}

// XVisioStream::STATE XVisioStreamServer::checkState()
// {
//     int32_t current_ssi;
//     std::ifstream ssi;

//     uint32_t num_frames_to_send;
//     std::ifstream frame_data_ifstream;
//     std::ifstream frame_image_ifstream;
//     uint32_t frame_data_size;
//     char frame_data_buf[2048];
//     char frame_image_buf[256000];
//     struct SLAMFrames frames_to_send;

//     std::cout << "Request received: GET_ALL_AVAILABLE_FRAMES" << std::endl;

//     // Update safe_send_index
//     // Wait for lock to be free
//     while (boost::filesystem::exists(stream_output_path / "safe_send_index_lock"))
//         std::this_thread::sleep_for(std::chrono::milliseconds(10));

//     // Lock the lock
//     std::ofstream((stream_output_path / "safe_send_index_lock").c_str(), std::ios_base::binary).close();

//     ssi = std::ifstream((stream_output_path / "safe_send_index").c_str(), std::ios_base::binary);
//     ssi.read(reinterpret_cast<char *>(&current_ssi), sizeof(int32_t));
//     ssi.close();
//     boost::filesystem::remove(stream_output_path / "safe_send_index_lock");
//     // num_frames_to_send = current_ssi - cur_send_index;
//     // send(new_socket, &num_frames_to_send, sizeof(uint32_t), 0);

//     std::cout << "Number of frames to package: " << current_ssi - cur_send_index << std::endl;

//     for (int i = cur_send_index; i <= current_ssi; i++)
//     {
//         SLAMFrame f = SLAMFrame();
//         std::ifstream ifs((stream_output_path / (std::to_string(i) + ".alcmm")).c_str());
//         boost::archive::binary_iarchive ia(ifs);
//         ia >> f;
//         frames_to_send.push_back(f);

//         // frame_data_size = 0;
//         // frame_data_ifstream.open((stream_output_path/(std::to_string(i)+".alcmm")).c_str(), std::ios_base::binary | std::ios_base::ate);
//         // frame_image_ifstream.open((stream_output_path/"images"/(std::to_string(i)+".bin")).c_str(), std::ios_base::binary);
//         // frame_data_size = static_cast<uint32_t>(frame_data_ifstream.tellg());
//         // frame_data_ifstream.seekg(0, std::ios::beg);
//         // frame_data_ifstream.read(frame_data_buf, frame_data_size);
//         // frame_data_ifstream.close();
//         // frame_image_ifstream.read(frame_image_buf, 256000);
//         // frame_image_ifstream.close();
//         // std::cout << "Sending Next Frame: " << frame_data_size << std::endl;
//         // std::cout << "SEND " << send(new_socket, &frame_data_size, sizeof(uint32_t), 0) << std::endl;
//         // std::cout << "SEND " << send(new_socket, frame_data_buf, frame_data_size, 0) << std::endl;
//         // std::cout << "SEND " << send(new_socket, frame_image_buf, 256000, 0) << std::endl;
//         // std::cout << "Send Frame Complete" << std::endl;
//         // std::this_thread::sleep_for(std::chrono::milliseconds(100));
//         // break;
//         std::cout << i << std::endl;
//     }

//     SLAMFrames fs;
//     std::stringstream ss;
//     boost::archive::binary_oarchive oa(ss);
//     oa << frames_to_send;
//     frames_to_send.slam_frames.clear();
//     // std::cout << "DATA SIZE: " << ss.str().size() << std::endl;

//     // send(new_socket, ss.str().data(), ss.str().size(), 0);

//     SLAMFrames rebuilt;
//     boost::archive::binary_iarchive ia(ss);
//     ia >> rebuilt;
//     cur_send_index = current_ssi;
//     std::cout << "Current Safe Send Index: " << current_ssi << std::endl;
// }

void XVisioStreamServer::requestStop()
{
    std::unique_lock<std::mutex>(stop_mutex);
    stopRequested = true;
}