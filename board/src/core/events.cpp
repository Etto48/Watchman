#include <cstdint>
#include <Arduino.h>

#include "core/events.hpp"
#include "constants.hpp"
#include "logger.hpp"

namespace events {

    volatile Event event_queue[EVENT_QUEUE_SIZE];
    volatile uint8_t event_queue_head = 0;
    volatile uint8_t event_queue_tail = 0;
    uint64_t last_button_event_time[6*2] = {0};
    hw_timer_t* timer_ptr = nullptr;
    uint64_t last_event_timestamp = 0;
    EventMask current_mask = EventMask::NONE;

    void IRAM_ATTR handle_button_interrupt(Button button, bool pressed) {
        uint64_t timestamp = millis();
        Event ev;
        if (pressed) {
            ev.type = EventType::BUTTON_PRESS;
        } else {
            ev.type = EventType::BUTTON_RELEASE;
        }
        
        if ( (pressed && (static_cast<uint16_t>(current_mask) & (1 << (static_cast<uint8_t>(button)) ) ) != 0) ||
            (!pressed && (static_cast<uint16_t>(current_mask) & (1 << (static_cast<uint8_t>(button) + 6)) ) != 0) ) {
            return; // Event is masked
        }

        auto button_index = static_cast<uint8_t>(button) * 2 + (pressed ? 0 : 1);
        if (timestamp - last_button_event_time[button_index] < DEBOUNCE_DELAY_MS) {
            return; // Debounce: ignore this event
        }
        last_button_event_time[button_index] = timestamp;

        ev.buttonEvent.button = button;
        ev.buttonEvent.timestamp = timestamp;

        uint8_t next_head = (event_queue_head + 1) % EVENT_QUEUE_SIZE;
        if (next_head != event_queue_tail) {
            *(Event*)&event_queue[event_queue_head] = ev; // This should be fine because we are in a no-interrupt zone
            event_queue_head = next_head;
        }
    }

    namespace ISRs {
        void IRAM_ATTR button_A() {
            handle_button_interrupt(Button::A, digitalRead(A_PIN) == LOW);
        }
        void IRAM_ATTR button_B() {
            handle_button_interrupt(Button::B, digitalRead(B_PIN) == LOW);
        }
        void IRAM_ATTR button_UP() {
            handle_button_interrupt(Button::UP, digitalRead(UP_PIN) == LOW);
        }
        void IRAM_ATTR button_DOWN() {
            handle_button_interrupt(Button::DOWN, digitalRead(DOWN_PIN) == LOW);
        }
        void IRAM_ATTR button_LEFT() {
            handle_button_interrupt(Button::LEFT, digitalRead(LEFT_PIN) == LOW);
        }
        void IRAM_ATTR button_RIGHT() {
            handle_button_interrupt(Button::RIGHT, digitalRead(RIGHT_PIN) == LOW);
        }

        void IRAM_ATTR timer() {
            //log::info("Timer interrupt fired.");
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
        noInterrupts();
        event_queue_head = 0;
        event_queue_tail = 0;
        interrupts();
    }

    Event get_next_event_noint() {
        if (event_queue_head == event_queue_tail) {
            return Event{EventType::NONE, {}};
        } else {
            Event ev = *(const Event *)&event_queue[event_queue_tail]; // This can be done because we are in a no-interrupt zone
            event_queue_tail = (event_queue_tail + 1) % EVENT_QUEUE_SIZE;
            return ev;
        }
    }
    
    Event get_next_event()
    {
        noInterrupts();
        auto ret = get_next_event_noint();
        interrupts();
        switch (ret.type) {
            case EventType::BUTTON_PRESS:
            case EventType::BUTTON_RELEASE:
                last_event_timestamp = ret.buttonEvent.timestamp;
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

    EventMask operator&(EventMask a, EventMask b)
    {
        return static_cast<EventMask>(static_cast<uint16_t>(a) & static_cast<uint16_t>(b));
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
        timerAlarmWrite(timer_ptr, 1000, true); // 1 second interval
        timerAlarmEnable(timer_ptr);
        logger::info("Timer started correctly.");
    }

    void enable_events() {
        attachInterrupt(digitalPinToInterrupt(A_PIN), ISRs::button_A, CHANGE);
        attachInterrupt(digitalPinToInterrupt(B_PIN), ISRs::button_B, CHANGE);
        attachInterrupt(digitalPinToInterrupt(UP_PIN), ISRs::button_UP, CHANGE);
        attachInterrupt(digitalPinToInterrupt(DOWN_PIN), ISRs::button_DOWN, CHANGE);
        attachInterrupt(digitalPinToInterrupt(LEFT_PIN), ISRs::button_LEFT, CHANGE);
        attachInterrupt(digitalPinToInterrupt(RIGHT_PIN), ISRs::button_RIGHT, CHANGE);
        start_timer_interrupt();
        last_event_timestamp = millis();
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
}

