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
  
#ifndef TEST

  void initialize()
  {
      const uint8_t input =  _BV(PIN_COUNTER0) | _BV(PIN_COUNTER1) | _BV(PIN_BUTTON);
      const uint8_t output = _BV(PIN_LED);

      // pin modes: 1 for output, 0 for input 
      DDRB = output;
      
      // enabling pull-up resistors (LED stay off because it's bit remains zero)
      PORTB = input;

      GIMSK = 0b00100000;    // turns on pin change interrupts
      PCMSK = input;         // turn on interrupts on input pins
      sei();                 // enables interrupt
  }
  
  ISR(PCINT0_vect)
  {
      // noop, just get up mc
  }

  uint8_t pin(uint8_t idx)
  {
    return !!(PINB & _BV(idx));
  }
  
  void ledOn()
  {
    PORTB |= _BV(PIN_LED);
  }

  void ledOff()
  {
    PORTB &= ~_BV(PIN_LED);
  }
  
  // only need for test
  void setCounter(uint8_t, uint32_t) {}
#endif // !TEST
  

}

uint8_t g_workers = 0;

// The purpose of this class is to get rid of contact bounce.
// New value marked as stable if it active at least FILTER_DELTA_MS ms.

struct FilteredPin
{
    uint8_t index_ : 2;
    uint8_t last_ : 1; 
    uint8_t candidate_ : 1;
    uint8_t set_ : 4; // up to 15

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

            // indicate that we should not go to sleep
            g_workers |= (1 << index_);
        }
    }

    // should be called once at ms
    bool check()
    {
        if (last_ != candidate_) {
            if (++set_ >= FILTER_DELTA_MS) {
                last_ = candidate_;

                // one more worker free -> way to shutdown mc
                g_workers &= ~(1 << index_);
                return true;
            }
        }
        return false;
    }
};

// Sensor change handling. Changes is filtered by pin filter.

struct Sensor
{
    FilteredPin pin;
    uint32_t counter;

    Sensor(uint8_t pinIndex) : pin(pinIndex), counter(0) {}

    void timer()
    {
      if (pin.check() && pin.get()) {
        // increment counter on accending signal
        counter++;
        setCounter(pin.index_, counter);
      }
    }
};

// Data state rendering handling

struct Renderer
{
  uint32_t start_;
  uint8_t progress;
  
  union {
    uint32_t l[2];
    uint8_t b[8];
  } data_;
  
  void run(uint32_t one, uint32_t two)
  {
    if (!progress) {
      start_ = millis();
      progress = 1;

      data_.l[1] = one;
      data_.l[2] = two;
    }
  }
  
  void finish()
  {
    progress = 0;
  }
  
  void render()
  {    
    auto elasped = millis() - start_;
    if (elasped <= 8 * 8) {
      uint8_t state = (data_.b[elasped / 8] >> (elasped % 8)) & 1;
      if (state) ledOn(); else ledOff();
    } else {
      finish();
    }
  }
};


void setup()
{
    initialize();
}


void loop()
{
    Sensor sensors[] = {PIN_COUNTER0, PIN_COUNTER1};
    Renderer renderer;

    uint32_t start = millis();
    
    for (;;) {
        for (auto& sensor : sensors) {
            sensor.pin.update();
        }
        
        if (pin(PIN_BUTTON)) {
          renderer.run(sensors[1].counter, sensors[2].counter);
        }
        
        if (g_workers | renderer.progress) {
            uint32_t time = millis();
            if (time - start > 0) {
                start = time;
                
                for (auto& sensor : sensors) {
                  sensor.timer();
                }
            }
            
            if (renderer.progress) {
               renderer.render();
            }
        } else {
            systemSleep();
        }
    }
}



