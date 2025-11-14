#ifndef BUTTON_H
#define BUTTON_H

#include <Arduino.h>

// Button timing constants
#define DEBOUNCE_DELAY 50           // ms
#define SHORT_PRESS_MAX 1000        // ms
#define BRIGHTNESS_PRESS_MIN 1000   // ms (1-3s: brightness)
#define BRIGHTNESS_PRESS_MAX 3000   // ms
#define LONG_PRESS_MIN 3000         // ms (3-10s: config mode)
#define FACTORY_RESET_MIN 10000     // ms (10s+: factory reset)

// Capacitive touch settings
#define TOUCH_THRESHOLD_RATIO 0.7  // 70% of baseline is considered a touch

// Button events
enum ButtonEvent {
    NONE,
    SHORT_PRESS,        // < 1s: Cycle to next module
    BRIGHTNESS_CYCLE,   // 1-3s: Cycle brightness level
    LONG_PRESS,         // 3-10s: Enter config mode
    FACTORY_RESET       // 10s+: Clear all settings
};

class ButtonHandler {
private:
    uint8_t pin;
    bool lastState;
    unsigned long pressStartTime;
    unsigned long lastDebounceTime;
    bool isPressed;

    // Capacitive touch
    uint16_t touchBaseline;
    uint16_t touchThreshold;
    bool useCapacitiveTouch;

    bool isTouched();
    void calibrateTouch();

public:
    ButtonHandler(uint8_t buttonPin, bool capacitiveTouch = true);

    void init();
    ButtonEvent check();
    bool isCurrentlyPressed();
    unsigned long getCurrentPressDuration();
};

#endif // BUTTON_H
