#include "tick.h"

// private:
//     int i = 1;
//     int rate = 1;
//     std::string name;
// uint64_t prev_ts;
//     uint64_t tot_freq;

// public:
//     Tick(std::string);
//     void set_rate(int);
//     void tick();
//     void printer();

Tick::Tick(std::string name) : rate(1), name(name) {}
void Tick::set_rate(int rate) { this->rate = rate; }
bool Tick::tick()
{
    uint64_t ts = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    if(prev_ts != 0)
    {
        tot_freq += (ts - prev_ts);
        x += 1;
    }
    prev_ts = ts;

    if(i % rate == 0)
    {
        std::cout << this->name << ": " << std::fixed << std::setprecision(2) << static_cast<double>(1 / ((tot_freq / rate) / 1e9)) << "Hz" << std::endl;
        tot_freq = 0;
        i = 1;
        return true;
    } else {
        i++;
    }
    return false;

}