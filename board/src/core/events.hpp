#pragma once

namespace events {
    enum class EventType : uint8_t {
        NONE = 0,// no fields of Event are valid apart from type
        BUTTON_PRESS = 1, // buttonEvent will contain details
        BUTTON_RELEASE = 2, // buttonEvent will contain details
    };

    enum class Button : uint8_t {
        A = 0,
        B = 1,
        UP = 2,
        DOWN = 3,
        LEFT = 4,
        RIGHT = 5
    };

    struct ButtonEvent {
        Button button;
        uint64_t timestamp;
    };

    struct Event {
        EventType type;
        union {
            ButtonEvent button_event; // valid if type is BUTTON_PRESS or BUTTON_RELEASE
        };
    };

    enum class EventMask : uint16_t {
        NONE = 0,
        BUTTON_PUSH_A = 1 << 0,
        BUTTON_PUSH_B = 1 << 1,
        BUTTON_PUSH_UP = 1 << 2,
        BUTTON_PUSH_DOWN = 1 << 3,
        BUTTON_PUSH_LEFT = 1 << 4,
        BUTTON_PUSH_RIGHT = 1 << 5,
        BUTTON_RELEASE_A = 1 << 6,
        BUTTON_RELEASE_B = 1 << 7,
        BUTTON_RELEASE_UP = 1 << 8,
        BUTTON_RELEASE_DOWN = 1 << 9,
        BUTTON_RELEASE_LEFT = 1 << 10,
        BUTTON_RELEASE_RIGHT = 1 << 11,
        ALL = 0xFFFF
    };

    EventMask operator|(EventMask a, EventMask b);
    EventMask operator&(EventMask a, EventMask b);
    EventMask operator~(EventMask a);

    // Retrieves the next event from the queue blocking up to timeout_ms milliseconds (0 = non blocking).
    Event get_next_event(uint64_t timeout_ms = 0);

    // Masks (ignores) events of the specified types.
    void mask_event(EventMask mask);

    // Unmasks (accepts) events of the specified types.
    void unmask_event(EventMask mask);

    // Clears all pending events from the queue.
    void clear_event_queue();

    // Enables event handling by attaching all necessary interrupts.
    void enable_events(); 

    // Disables event handling by detaching all interrupts.
    void disable_events();

    // Returns the timestamp of the last event processed (in microseconds).
    uint64_t get_last_event_timestamp();

    // Updates the timestamp with the current time.
    void update_last_event_timestamp();
}