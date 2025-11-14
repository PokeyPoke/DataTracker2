#ifndef BUTTON_H
#define BUTTON_H

#include <Arduino.h>

// Button timing constants
#define DEBOUNCE_DELAY 50           // ms
#define SHORT_PRESS_MAX 1000        // ms
#define BRIGHTNESS_START 4200       // ms (4.2s: start brightness adjustment)
#define BRIGHTNESS_STEP_INTERVAL 200  // ms between brightness steps

// Capacitive touch settings
#define TOUCH_THRESHOLD_RATIO 0.7  // 70% of baseline is considered a touch

// Button events
enum ButtonEvent {
    NONE,
    SHORT_PRESS,              // < 1s: Cycle to next module
    BRIGHTNESS_ADJUSTING,     // Held > 4.2s: Adjusting brightness
    BRIGHTNESS_RELEASED       // Released after brightness adjustment
};

class ButtonHandler {
private:
    uint8_t pin;
    bool lastState;
    unsigned long pressStartTime;
    unsigned long lastDebounceTime;
    bool isPressed;
    bool brightnessMode;
    unsigned long lastBrightnessStep;

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
