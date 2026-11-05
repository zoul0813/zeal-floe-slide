/*
 * Video recording: plays each floe's par solution from tools/make_levels.py (the BFS solver),
 * one slide at a time, with a pause between slides so the moves read on video.
 */
#include "game.h"

#define THINK_FRAMES 22

static const char *moves;
static uint8_t wait;
static uint8_t pressed;

void autopilot_reset(uint8_t floe) {
    moves = SOLUTIONS[floe];
    wait = 60;   // let the floe show first
    pressed = 0;
}

uint16_t autopilot_input(void) {
    if (pressed) {   // one frame down, then release so the next press is a new press
        pressed = 0;
        return 0;
    }
    if (!play_idle() || !moves || !*moves) return 0;
    if (wait) { wait--; return 0; }
    wait = THINK_FRAMES;
    pressed = 1;
    switch (*moves++) {
        case 'U': return IN_UP;
        case 'D': return IN_DOWN;
        case 'L': return IN_LEFT;
        default:  return IN_RIGHT;
    }
}
