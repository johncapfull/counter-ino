#ifdef TEST
    #include "test_routines.h"
#else
    #include "avr_routines_inl.h"
#endif

namespace {
  const uint8_t PIN_COUNTER0 = 0;
  const uint8_t PIN_COUNTER1 = 1;
  const uint8_t PIN_BUTTON = 2;
  const uint8_t PIN_LED = 4;

  const uint8_t FILTER_DELTA_MS = 15;
  
  inline void initialize()
  {
#ifndef TEST
      pinMode(PIN_LED, OUTPUT);
      digitalWrite(PIN_LED, LOW); // main led off
      
      pinMode(PIN_COUNTER0, INPUT);
      pinMode(PIN_COUNTER1, INPUT);
      pinMode(PIN_BUTTON, INPUT);
    
      // enabling pull-up resistors
      digitalWrite(PIN_COUNTER0, HIGH);
      digitalWrite(PIN_COUNTER1, HIGH);
      digitalWrite(PIN_BUTTON, HIGH);
      
      GIMSK = 0b00100000;    // turns on pin change interrupts
      PCMSK = 0b00000111;    // turn on interrupts on pins PB0, PB1, &amp; PB4
      sei();                 // enables interrupt
#endif
  }

}

uint8_t g_workers = 0;

// The purpose of this class is to get rid of contact bounce.
// New value marked as stable if it active at least FILTER_DELTA_MS ms.

struct FilteredPin
{
    uint8_t index_ : 2;
    uint8_t last_ : 1; 
    uint8_t candidate_ : 1;
    uint8_t set_;// : 4; // up to 15

    // total members size: 8 bits    
    static_assert(FILTER_DELTA_MS < (1 << 4), "not enough bits to hold delta value");

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
            g_workers |= (1 << index_);
        }
    }

    // should be called once at ms
    bool check()
    {
        if (last_ != candidate_) {
            if (++set_ >= FILTER_DELTA_MS) {
                last_ = candidate_;

                // unsubscribe
                g_workers &= ~(1 << index_);
                return true;
            }
        }
        return false;
    }
};

struct Sensor
{
    FilteredPin pin;
    uint32_t counter;
};

void setup()
{
    initialize();
}

void loop()
{    
    Sensor sensors[] = {PIN_COUNTER0, PIN_COUNTER1};

    uint32_t start = millis();
    
    for (;;) {
        for (auto& sensor : sensors) {
            sensor.pin.update();
        }
            
        if (g_workers) {
            uint32_t time = millis();
            if (time - start > 0) {
                start = time;

                for (auto& sensor : sensors) {
                    if (sensor.pin.check() && sensor.pin.get()) {
                        sensor.counter++;
                    }
                }
            }
        } else {
            systemSleep();
        }
    }
}



