#pragma once

#include <string>
#include <chrono>
#include <thread>
#include <iomanip>
#include <iostream>

class Tick
{
private:
    int i = 1;
    int rate = 1;
    std::string name;
    bool print = false;

    uint64_t prev_ts = 0;
    uint64_t tot_freq;
    int x = 0;
public:
    Tick() {};
    Tick(std::string);
    void set_rate(int);
    bool tick();
    void start_printing() { print = true; }
    void stop_printing() { print = false; }
    void printer();
};