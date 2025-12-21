#include <cstdint>
#include <Arduino.h>

#include "core/sound.hpp"
#include "constants.hpp"
#include "core/events.hpp"
#include "sound.hpp"

namespace sound {
    static TaskHandle_t async_melody_task_handle = nullptr;
    static QueueHandle_t async_melody_task_queue = nullptr;

    struct AsyncMelodyCommand {
        bool single_tone;
        bool interruptible;
        union {
            struct {
                NoteFrequency frequency;
                uint16_t duration;
            } tone;
            struct {
                const Note* melody;
                size_t length;
            } melody;
        };
    };

    void play_melody(const Note* melody, size_t length) {
        for (size_t i = 0; i < length; ++i) {
            if (melody[i].frequency == sound::NoteFrequency::NOTE_REST) {
                noTone(BUZZER_PIN);
            } else {
                tone(BUZZER_PIN, static_cast<unsigned int>(melody[i].frequency));
            }
            vTaskDelay(pdMS_TO_TICKS(melody[i].duration));
        }
        noTone(BUZZER_PIN);
    }

    bool play_interruptible_melody(const Note* melody, size_t length) {
        bool was_interrupted = false;
        events::mask_event(events::EventMask::ALL & ~events::EventMask::BUTTON_PUSH_A);
        events::clear_event_queue();

        for (size_t i = 0; i < length; ++i) {
            events::Event ev = events::get_next_event();
            if (ev.type == events::EventType::BUTTON_PRESS && ev.buttonEvent.button == events::Button::A) {
                was_interrupted = true;
                break; // Interrupt melody on A button press
            }
            if (melody[i].frequency == sound::NoteFrequency::NOTE_REST) {
                noTone(BUZZER_PIN);
            } else {
                tone(BUZZER_PIN, static_cast<unsigned int>(melody[i].frequency));
            }
            vTaskDelay(pdMS_TO_TICKS(melody[i].duration));
        }
        noTone(BUZZER_PIN);

        events::unmask_event(events::EventMask::ALL);
        return was_interrupted;
    }

    void async_play_interruptible_melody(const Note *melody, size_t length)
    {
        if (async_melody_task_queue == nullptr) {
            return; // Task not initialized
        }
        AsyncMelodyCommand command = {
            .single_tone = false,
            .interruptible = true,
            .melody = {
                .melody = melody,
                .length = length
            }
        };
        xQueueSend(async_melody_task_queue, &command, 0); // Drop if busy   
    }

    void stop_async_interruptible_melody()
    {
        if (async_melody_task_handle != nullptr) {
            xTaskNotifyGive(async_melody_task_handle);
        }
    }

    bool is_melody_playing()
    {
        if (async_melody_task_queue == nullptr) {
            return false; // Task not initialized
        }
        return uxQueueMessagesWaiting(async_melody_task_queue) > 0;
    }

    void play_tone(NoteFrequency frequency, uint16_t duration)
    {
        if (frequency == NoteFrequency::NOTE_REST) {
            noTone(BUZZER_PIN);
        } else {
            tone(BUZZER_PIN, static_cast<unsigned int>(frequency));
        }
        vTaskDelay(pdMS_TO_TICKS(duration));
        noTone(BUZZER_PIN);
    }

    void async_interruptible_melody_helper(const Note* melody, size_t length)
    {
        ulTaskNotifyTake(pdTRUE, 0); // Clear any previous notifications
        for (size_t i = 0; i < length; ++i) {
            if (ulTaskNotifyTake(pdTRUE, 0) > 0) {
                break; // Interrupted
            }
            if (melody[i].frequency == sound::NoteFrequency::NOTE_REST) {
                noTone(BUZZER_PIN);
            } else {
                tone(BUZZER_PIN, static_cast<unsigned int>(melody[i].frequency));
            }
            vTaskDelay(pdMS_TO_TICKS(melody[i].duration));
        }
        noTone(BUZZER_PIN);
    }

    void async_melody_task(void* param) {
        AsyncMelodyCommand command;
        while (true) {
            if (xQueuePeek(async_melody_task_queue, &command, portMAX_DELAY) == pdTRUE) {
                if (command.single_tone) {
                    play_tone(command.tone.frequency, command.tone.duration);
                } else if(!command.interruptible) {
                    play_melody(command.melody.melody, command.melody.length);
                } else {
                    async_interruptible_melody_helper(command.melody.melody, command.melody.length);
                }
                xQueueReceive(async_melody_task_queue, &command, 0); // Remove the processed command
            }
        }
    }

    void init() {
        async_melody_task_queue = xQueueCreate(1, sizeof(AsyncMelodyCommand));
        xTaskCreate(
            async_melody_task,
            "AsyncMelodyTask",
            2048,
            nullptr,
            1,
            &async_melody_task_handle
        );
    }

    void async_play_tone(NoteFrequency frequency, uint16_t duration) {
        if (async_melody_task_queue == nullptr) {
            return; // Task not initialized
        }
        AsyncMelodyCommand command = {
            .single_tone = true,
            .interruptible = false,
            .tone = {
                .frequency = frequency,
                .duration = duration
            }
        };
        xQueueSend(async_melody_task_queue, &command, 0); // Drop if busy
    }

    void async_play_melody(const Note* melody, size_t length) {
        if (async_melody_task_queue == nullptr) {
            return; // Task not initialized
        }
        AsyncMelodyCommand command = {
            .single_tone = false,
            .interruptible = false,
            .melody = {
                .melody = melody,
                .length = length
            }
        };
        xQueueSend(async_melody_task_queue, &command, 0); // Drop if busy
    }

    void play_confirm_tone() {
        async_play_tone(confirm_tone, 150);
    }

    void play_cancel_tone() {
        async_play_tone(cancel_tone, 150);
    }

    void play_navigation_tone() {
        async_play_tone(navigation_tone, 100);
    }
}