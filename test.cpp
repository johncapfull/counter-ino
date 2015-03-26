#ifdef TEST

#include <vector>
#include <thread>
#include <chrono>
#include <iostream>
#include <cassert>
#include <mutex>
#include <map>

#include "test_routines.h"

namespace {

struct ValueTimePair {
    uint8_t value;
    size_t ms;
};

// 4 pulses should make 4 increments
const std::vector<ValueTimePair> timeline = {
    {0, 2000},

    // 1st pulse
    {1, 1},
    {0, 1},
    {1, 1},
    {0, 1},
    {1, 1},
    {0, 1},
    {1, 2000},

    // pause
    {0, 10000},

    // 2nd pulse
    {1, 2},
    {0, 4},
    {1, 2},
    {0, 2},
    {1, 1000},

    // pause
    {0, 2},
    {1, 4},
    {0, 5000},
    {1, 4},
    {0, 2},
    {1, 2},
    {0, 3000},

    // 3rd pulse
    {1, 1},
    {0, 1},
    {1, 1},
    {0, 1},
    {1, 1},
    {0, 1},
    {1, 1000},
    {0, 10},
    {1, 2},
    {0, 1},
    {1, 1000},

    // pause
    {0, 2},
    {1, 4},
    {0, 5000},
    {1, 2},
    {0, 3000},

    // 4th pulse
    {1, 2000}
};

struct Pins {
    std::atomic<uint8_t> state_;

    Pins() : state_(0) {}

    void set(uint8_t pin, uint8_t value)
    {
        if (value) {
            state_.fetch_or((1 << pin));
        } else {
            state_.fetch_and(~(1 << pin));
        }

        //std::cout << "after set: " << (int)state_.load() << " should be " << (int)(uint8_t)(1 << pin) << " pin " << (int)pin << "\n";
    }

    void emit(uint8_t pin, std::vector<ValueTimePair> values)
    {
        for (auto& pair : values) {
//            std::cout << " emit pin " << (int)pin <<  " value " << (int)pair.value << " \n";
            set(pin, pair.value);
            std::this_thread::sleep_for(std::chrono::milliseconds(pair.ms));
        }
    }

};

Pins g_pins;
std::map<uint8_t, uint32_t> g_counters;

void thread()
{
    std::thread first([]
    {
        for (size_t idx = 0; idx < 2; ++idx) {
            g_pins.emit(0, timeline);
        }
    });

    std::thread second([]
    {
        // start after short pause
        std::this_thread::sleep_for(std::chrono::milliseconds(15945));
        g_pins.emit(1, timeline);
    });

    first.join();
    second.join();

    // take some time to handle final values
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    assert(g_counters.size() == 2);
    assert(g_counters[0] == 8);
    assert(g_counters[1] == 4);
}

} // namespace

uint8_t pins()
{
    return g_pins.state_.load();
}

uint8_t pin(uint8_t idx)
{
    return !!(pins() & (1 << idx));
}

void setCounter(uint8_t number, uint32_t value)
{
    std::cout << "Counter " << int(number) << " set to " << value << "\n";
    g_counters[number] = value;
}

void initialize() {}

uint32_t millis()
{
    auto duration = std::chrono::system_clock::now().time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    return (uint32_t)(unsigned long long)(millis);
}

// firmware entry points
void setup();
void loop();

int main()
{
    setup();

    std::thread([]
    {
        for (;;) { loop(); }
    }).detach();

    std::cout << "### starting test\n";
    std::thread(thread).join();
    std::cout << "### test done\n";

    return 0;
}

#endif // TEST