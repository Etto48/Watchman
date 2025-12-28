#include <cstdint>
#include <Arduino.h>

#include "core/events.hpp"
#include "core/logger.hpp"
#include "core/events.hpp"
#include "core/timekeeper.hpp"
#include "constants.hpp"

namespace events {
    QueueHandle_t event_queue = nullptr;
    uint64_t last_button_event_time[6*2] = {0};
    uint64_t button_press_start_time[6] = {0};
    hw_timer_t* timer_ptr = nullptr;
    uint64_t last_event_timestamp = 0;
    EventMask current_mask = EventMask::NONE;

    void IRAM_ATTR add_event(const Event& ev) {
        Event dummy;
        if (event_queue != nullptr) {
            auto highPriorityTaskWoken = pdFALSE;
            if(xQueueIsQueueFullFromISR(event_queue)) {
                xQueueReceiveFromISR(event_queue, &dummy, &highPriorityTaskWoken);
            }
            xQueueSendFromISR(event_queue, &ev, &highPriorityTaskWoken);
            
            if (highPriorityTaskWoken) {
                portYIELD_FROM_ISR();
            }
        }
    }

    // MUST be a single button
    size_t IRAM_ATTR get_button_index(Button button, bool pressed) {
        switch (button) {
            case Button::A:
                return pressed ? 0 : 1;
            case Button::B:
                return pressed ? 2 : 3;
            case Button::UP:
                return pressed ? 4 : 5;
            case Button::DOWN:
                return pressed ? 6 : 7;
            case Button::LEFT:
                return pressed ? 8 : 9;
            case Button::RIGHT:
                return pressed ? 10 : 11;
            default:
                return 0; // Should not happen
        }
    }

    uint32_t IRAM_ATTR button_to_pin_mask(Button button) {
        uint32_t mask = 0;
        mask |= ((button & Button::A) == Button::A) ? (1 << A_PIN) : 0;
        mask |= ((button & Button::B) == Button::B) ? (1 << B_PIN) : 0;
        mask |= ((button & Button::UP) == Button::UP) ? (1 << UP_PIN) : 0;
        mask |= ((button & Button::DOWN) == Button::DOWN) ? (1 << DOWN_PIN) : 0;
        mask |= ((button & Button::LEFT) == Button::LEFT) ? (1 << LEFT_PIN) : 0;
        mask |= ((button & Button::RIGHT) == Button::RIGHT) ? (1 << RIGHT_PIN) : 0;
        return mask;
    }

    void IRAM_ATTR handle_button_repeats() {
        uint64_t timestamp = timekeeper::now_us();
        auto gpio_state = GPIO.in.val;
        Event dummy;
        auto highPriorityTaskWoken = pdFALSE;

        for (int i = 0; i < 6; ++i) {
            Button button = static_cast<Button>(1 << i);
            bool pressed = (gpio_state & button_to_pin_mask(button)) == 0; // Active low
            if (pressed) {
                auto button_index = get_button_index(button, true);
                if (
                    last_button_event_time[button_index] + DEBOUNCE_DELAY_US <= timestamp 
                    && button_press_start_time[i] + REPEAT_DELAY_US <= timestamp
                ) {
                    // Generate repeat event
                    Event ev;
                    ev.type = EventType::BUTTON_PRESS;
                    ev.button_press_event.button = button;
                    // Determine hold state
                    Button hold = Button::NONE;
                    bool button_a = (gpio_state & (1 << A_PIN)) == 0;
                    bool button_b = (gpio_state & (1 << B_PIN)) == 0;
                    bool button_up = (gpio_state & (1 << UP_PIN)) == 0;
                    bool button_down = (gpio_state & (1 << DOWN_PIN)) == 0;
                    bool button_left = (gpio_state & (1 << LEFT_PIN)) == 0;
                    bool button_right = (gpio_state & (1 << RIGHT_PIN)) == 0;
                    hold |= button_a ? Button::A : Button::NONE;
                    hold |= button_b ? Button::B : Button::NONE;
                    hold |= button_up ? Button::UP : Button::NONE;
                    hold |= button_down ? Button::DOWN : Button::NONE;
                    hold |= button_left ? Button::LEFT : Button::NONE;
                    hold |= button_right ? Button::RIGHT : Button::NONE;
                    ev.button_press_event.hold = hold;
                    ev.button_press_event.timestamp = timestamp;
                    ev.button_press_event.repeated = true;

                    last_button_event_time[button_index] = timestamp;
                    if (event_queue != nullptr) {
                        if (xQueueIsQueueFullFromISR(event_queue)) {
                            xQueueReceiveFromISR(event_queue, &dummy, &highPriorityTaskWoken);
                        }
                        xQueueSendFromISR(event_queue, &ev, &highPriorityTaskWoken);
                    }
                }
            }
        }
        if (highPriorityTaskWoken) {
            portYIELD_FROM_ISR();
        }           
    }

    void IRAM_ATTR handle_button_interrupt(Button button) {
        uint64_t timestamp = timekeeper::now_us();
        Event ev;

        auto gpio_state = GPIO.in.val;
        bool pressed = (gpio_state & button_to_pin_mask(button)) == 0; // Active low

        if (pressed) {
            ev.type = EventType::BUTTON_PRESS;
        } else {
            ev.type = EventType::BUTTON_RELEASE;
        }
        
        if ( (pressed && (static_cast<uint16_t>(current_mask) & static_cast<uint8_t>(button) ) != 0) ||
            (!pressed && (static_cast<uint16_t>(current_mask) & (static_cast<uint8_t>(button) << 6))) != 0) {
            return; // Event is masked
        }

        auto button_index = get_button_index(button, pressed);
        if (last_button_event_time[button_index] + DEBOUNCE_DELAY_US > timestamp) {
            return; // Debounce: ignore this event
        }
        last_button_event_time[button_index] = timestamp;
        if (pressed) {
            button_press_start_time[button_index / 2] = timestamp;
        }

        Button hold = Button::NONE;

        bool button_a = (gpio_state & (1 << A_PIN)) == 0;
        bool button_b = (gpio_state & (1 << B_PIN)) == 0;
        bool button_up = (gpio_state & (1 << UP_PIN)) == 0;
        bool button_down = (gpio_state & (1 << DOWN_PIN)) == 0;
        bool button_left = (gpio_state & (1 << LEFT_PIN)) == 0;
        bool button_right = (gpio_state & (1 << RIGHT_PIN)) == 0;
        hold |= button_a ? Button::A : Button::NONE;
        hold |= button_b ? Button::B : Button::NONE;
        hold |= button_up ? Button::UP : Button::NONE;
        hold |= button_down ? Button::DOWN : Button::NONE;
        hold |= button_left ? Button::LEFT : Button::NONE;
        hold |= button_right ? Button::RIGHT : Button::NONE;

        if (pressed) {
            ev.button_press_event.button = button;
            ev.button_press_event.hold = hold;
            ev.button_press_event.timestamp = timestamp;
            ev.button_press_event.repeated = false; // Initial press, not a repeat
        } else {
            ev.button_release_event.button = button;
            ev.button_release_event.hold = hold;
            ev.button_release_event.timestamp = timestamp;
            // Calculate duration
            auto press_time = button_press_start_time[button_index / 2];
            ev.button_release_event.duration = timestamp - press_time;
        }

        add_event(ev);
    }

    namespace ISRs {
        void IRAM_ATTR button_A() {
            handle_button_interrupt(Button::A);
        }
        void IRAM_ATTR button_B() {
            handle_button_interrupt(Button::B);
        }
        void IRAM_ATTR button_UP() {
            handle_button_interrupt(Button::UP);
        }
        void IRAM_ATTR button_DOWN() {
            handle_button_interrupt(Button::DOWN);
        }
        void IRAM_ATTR button_LEFT() {
            handle_button_interrupt(Button::LEFT);
        }
        void IRAM_ATTR button_RIGHT() {
            handle_button_interrupt(Button::RIGHT);
        }

        void IRAM_ATTR timer() {
            handle_button_repeats();
        }
    }

    void mask_event(EventMask mask) {
        noInterrupts();
        current_mask = static_cast<EventMask>(static_cast<uint16_t>(current_mask) | static_cast<uint16_t>(mask));
        interrupts();
    }

    void unmask_event(EventMask mask) {
        noInterrupts();
        current_mask = static_cast<EventMask>(static_cast<uint16_t>(current_mask) & ~static_cast<uint16_t>(mask));
        interrupts();
    }
    
    void clear_event_queue() {
        if (event_queue != nullptr) {
            xQueueReset(event_queue);
        }
    }
    
    Button operator|(Button a, Button b) {
        return static_cast<Button>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
    }
    Button operator|=(Button& a, Button b) {
        a = a | b;
        return a;
    }
    Button operator&(Button a, Button b) {
        return static_cast<Button>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
    }
    Button operator&=(Button& a, Button b) {
        a = a & b;
        return a;
    }
    Button operator~(Button a) {
        return static_cast<Button>(~static_cast<uint8_t>(a));
    }

    Event get_next_event(uint64_t timeout_ms)
    {
        Event ret = { .type = EventType::NONE, {}};
        auto res = xQueueReceive(event_queue, &ret, pdMS_TO_TICKS(timeout_ms));
        switch (ret.type) {
            case EventType::BUTTON_PRESS:
                last_event_timestamp = ret.button_press_event.timestamp;
                break;
            case EventType::BUTTON_RELEASE:
                last_event_timestamp = ret.button_release_event.timestamp;
                break;
            default:
                break;
        }
        return ret;
    }

    EventMask operator|(EventMask a, EventMask b)
    {
        return static_cast<EventMask>(static_cast<uint16_t>(a) | static_cast<uint16_t>(b));
    }
    EventMask operator|=(EventMask& a, EventMask b)
    {
        a = a | b;
        return a;
    }
    EventMask operator&(EventMask a, EventMask b)
    {
        return static_cast<EventMask>(static_cast<uint16_t>(a) & static_cast<uint16_t>(b));
    }
    EventMask operator&=(EventMask& a, EventMask b)
    {
        a = a & b;
        return a;
    }
    EventMask operator~(EventMask a)
    {
        return static_cast<EventMask>(~static_cast<uint16_t>(a));
    }
    
    void start_timer_interrupt() {
        timer_ptr = timerBegin(0, 1000, true); 
        if (timer_ptr == nullptr) {
            logger::error("Error starting timer.");
            return;
        }
        timerAttachInterrupt(timer_ptr, ISRs::timer, true);
        timerAlarmWrite(timer_ptr, 100, true); // 100 ms
        timerAlarmEnable(timer_ptr);
        logger::info("Timer started correctly.");
    }

    void enable_events() {
        if (event_queue == nullptr) {
            event_queue = xQueueCreate(EVENT_QUEUE_SIZE, sizeof(Event));
        }
        attachInterrupt(digitalPinToInterrupt(A_PIN), ISRs::button_A, CHANGE);
        attachInterrupt(digitalPinToInterrupt(B_PIN), ISRs::button_B, CHANGE);
        attachInterrupt(digitalPinToInterrupt(UP_PIN), ISRs::button_UP, CHANGE);
        attachInterrupt(digitalPinToInterrupt(DOWN_PIN), ISRs::button_DOWN, CHANGE);
        attachInterrupt(digitalPinToInterrupt(LEFT_PIN), ISRs::button_LEFT, CHANGE);
        attachInterrupt(digitalPinToInterrupt(RIGHT_PIN), ISRs::button_RIGHT, CHANGE);
        start_timer_interrupt();
        update_last_event_timestamp();
    }

    void disable_events() {
        detachInterrupt(digitalPinToInterrupt(A_PIN));
        detachInterrupt(digitalPinToInterrupt(B_PIN));
        detachInterrupt(digitalPinToInterrupt(UP_PIN));
        detachInterrupt(digitalPinToInterrupt(DOWN_PIN));
        detachInterrupt(digitalPinToInterrupt(LEFT_PIN));
        detachInterrupt(digitalPinToInterrupt(RIGHT_PIN));
    }

    uint64_t get_last_event_timestamp() {
        return last_event_timestamp;
    }

    void update_last_event_timestamp() {
        last_event_timestamp = timekeeper::now_us();
    }
}