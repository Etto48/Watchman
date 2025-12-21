#pragma once

#include "core/sound.hpp"

constexpr sound::Note boot_jingle_melody[] = {
    { sound::NoteFrequency::NOTE_C5, 200 },
    { sound::NoteFrequency::NOTE_F5, 200 },
    { sound::NoteFrequency::NOTE_A5, 200 },
    { sound::NoteFrequency::NOTE_C6, 200 },
};

constexpr sound::Note deepsleep_jingle_melody[] = {
    { sound::NoteFrequency::NOTE_C6, 200 },
    { sound::NoteFrequency::NOTE_A5, 200 },
    { sound::NoteFrequency::NOTE_F5, 200 },
    { sound::NoteFrequency::NOTE_C5, 200 },
};