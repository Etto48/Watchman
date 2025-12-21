#pragma once

namespace sound {

    enum class NoteFrequency: unsigned int {
        NOTE_C0 = 16,
        NOTE_D0b = 17,
        NOTE_D0 = 18,
        NOTE_E0b = 19,
        NOTE_E0 = 21,
        NOTE_F0 = 22,
        NOTE_G0b = 23,
        NOTE_G0 = 25,
        NOTE_A0b = 26,
        NOTE_A0 = 28,
        NOTE_B0b = 29,
        NOTE_B0 = 31,
        NOTE_C1 = 33,
        NOTE_D1b = 35,
        NOTE_D1 = 37,
        NOTE_E1b = 39,
        NOTE_E1 = 41,
        NOTE_F1 = 44,
        NOTE_G1b = 46,
        NOTE_G1 = 49,
        NOTE_A1b = 52,
        NOTE_A1 = 55,
        NOTE_B1b = 58,
        NOTE_B1 = 62,
        NOTE_C2 = 65,
        NOTE_D2b = 69,
        NOTE_D2 = 73,
        NOTE_E2b = 78,
        NOTE_E2 = 82,
        NOTE_F2 = 87,
        NOTE_G2b = 93,
        NOTE_G2 = 98,
        NOTE_A2b = 104,
        NOTE_A2 = 110,
        NOTE_B2b = 117,
        NOTE_B2 = 123,
        NOTE_C3 = 131,
        NOTE_D3b = 139,
        NOTE_D3 = 147,
        NOTE_E3b = 156,
        NOTE_E3 = 165,
        NOTE_F3 = 175,
        NOTE_G3b = 185,
        NOTE_G3 = 196,
        NOTE_A3b = 208,
        NOTE_A3 = 220,
        NOTE_B3b = 233,
        NOTE_B3 = 247,
        NOTE_C4 = 262,
        NOTE_D4b = 277,
        NOTE_D4 = 294,
        NOTE_E4b = 311,
        NOTE_E4 = 330,
        NOTE_F4 = 349,
        NOTE_G4b = 370,
        NOTE_G4 = 392,
        NOTE_A4b = 415,
        NOTE_A4 = 440,
        NOTE_B4b = 466,
        NOTE_B4 = 494,
        NOTE_C5 = 523,
        NOTE_D5b = 554,
        NOTE_D5 = 587,
        NOTE_E5b = 622,
        NOTE_E5 = 659,
        NOTE_F5 = 698,
        NOTE_G5b = 740,
        NOTE_G5 = 784,
        NOTE_A5b = 831,
        NOTE_A5 = 880,
        NOTE_B5b = 932,
        NOTE_B5 = 988,
        NOTE_C6 = 1047,
        NOTE_D6b = 1109,
        NOTE_D6 = 1175,
        NOTE_E6b = 1245,
        NOTE_E6 = 1319,
        NOTE_F6 = 1397,
        NOTE_G6b = 1479,
        NOTE_G6 = 1568,
        NOTE_A6b = 1661,
        NOTE_A6 = 1760,
        NOTE_B6b = 1865,
        NOTE_B6 = 1976,
        NOTE_C7 = 2093,
        NOTE_D7b = 2217,
        NOTE_D7 = 2349,
        NOTE_E7b = 2489,
        NOTE_E7 = 2637,
        NOTE_F7 = 2794,
        NOTE_G7b = 2959,
        NOTE_G7 = 3136,
        NOTE_A7b = 3322,
        NOTE_A7 = 3520,
        NOTE_B7b = 3729,
        NOTE_B7 = 3951,
        NOTE_C8 = 4186,

        NOTE_REST = 0
    };

    constexpr NoteFrequency confirm_tone = NoteFrequency::NOTE_C6;
    constexpr NoteFrequency cancel_tone = NoteFrequency::NOTE_A5;
    constexpr NoteFrequency navigation_tone = NoteFrequency::NOTE_E5;

    struct Note {
        NoteFrequency frequency; // Frequency in Hz
        uint16_t duration;  // Duration in milliseconds
    };

    // Initialize the sound system and related tasks
    void init();

    // Plays a melody consisting of an array of Notes. This function blocks until the melody is complete.
    void play_melody(const Note* melody, size_t length);

    // Asynchronously plays a melody consisting of an array of Notes.
    void async_play_melody(const Note* melody, size_t length);
    
    // Plays a melody that can be interrupted by pressing the A button. This function blocks until the melody is complete or interrupted.
    // Returns true if the melody was interrupted, false if it completed normally.
    bool play_interruptible_melody(const Note* melody, size_t length);

    // Plays a single tone of the specified frequency for the specified duration. This function blocks until the tone is complete.
    void play_tone(NoteFrequency frequency, uint16_t duration);

    // Asynchronously plays a single tone of the specified frequency for the specified duration.
    void async_play_tone(NoteFrequency frequency, uint16_t duration);

    void play_confirm_tone();

    void play_cancel_tone();

    void play_navigation_tone();
}