#ifdef TEST
    #include "test_routines.h"
    #include <iostream>
#else
    #include "avr_routines.cpp.inl"
#endif

namespace {

const uint8_t FILTER_DELTA_MS = 15;

}

uint8_t g_subscribers = 0;

struct FilteredPin
{
    uint8_t index_ : 2;
    uint8_t last_ : 1; 
    uint8_t candidate_ : 1;
    uint8_t set_ : 4; // up to 15

    FilteredPin(uint8_t index) : index_(index), last_(pin(index)), candidate_(last_), set_(0)
    {
    }

    uint8_t get() const { return last_; }

    // should be called every iteration
    void update()
    {
        uint8_t value = pin(index_);

        if (value != candidate_) {
            set_ = 0;
            candidate_ = value;

            // subscribe to updates
            g_subscribers |= (1 << index_);
        }
    }

    // should be called once at ms
    bool check()
    {
        if (last_ != candidate_) {
            if (++set_ >= FILTER_DELTA_MS) {
                last_ = candidate_;

                // unsubscribe
                g_subscribers &= ~(1 << index_);
                return true;
            }
        }
        return false;
    }
};

void setup()
{
    initialize();
}

void loop()
{
    FilteredPin g_sensor0(0);
    FilteredPin g_sensor1(1);

    uint32_t counter0 = 0;
    uint32_t counter1 = 0;

    uint32_t start = millis();

    for (;;) {
        g_sensor0.update();
        g_sensor1.update();

        if (g_subscribers) {
            if (millis() - start > 0) {
                start = millis();

                if (g_sensor0.check() && g_sensor0.get()) {
                    setCounter(0, ++counter0);
                }
                if (g_sensor1.check() && g_sensor1.get()) {
                    setCounter(1, ++counter1);
                }
            }            
        } else {
            // TODO: sleep here...
        }
    }
}
