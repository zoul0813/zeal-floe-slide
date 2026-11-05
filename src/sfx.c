/*
 * Sound effects: tables for ZGDK's sequencer (zgdk/sound/sfx.h) on voice 3.
 * Voices 0-2 belong to the ZMT music (music.c), which never writes voice 3.
 */
#include "game.h"

static const sfx_seg S_SLIDE[]  = { SFX_SEG(WAV_NOISE, 900, 2200, 6), SFX_SEG(WAV_NOISE, 2200, 1400, 8), SFX_END };
static const sfx_seg S_BUMP[]   = { SFX_SEG(WAV_TRIANGLE, 140, 90, 4), SFX_END };
static const sfx_seg S_THUD[]   = { SFX_SEG(WAV_TRIANGLE, 200, 80, 5), SFX_SEG(WAV_NOISE, 500, 300, 3), SFX_END };
static const sfx_seg S_FISH[]   = { SFX_SEG(WAV_SQUARE, 988, 988, 3), SFX_SEG(WAV_SQUARE, 1319, 1319, 3), SFX_SEG(WAV_SQUARE, 1976, 1976, 5), SFX_END };
static const sfx_seg S_OPEN[]   = { SFX_SEG(WAV_SQUARE, 784, 784, 4), SFX_SEG(WAV_SQUARE, 988, 988, 4), SFX_SEG(WAV_SQUARE, 1175, 1175, 4), SFX_SEG(WAV_SQUARE, 1568, 1568, 10), SFX_END };
static const sfx_seg S_DIVE[]   = { SFX_SEG(WAV_TRIANGLE, 700, 200, 8), SFX_SEG(WAV_TRIANGLE, 200, 700, 8), SFX_END };
static const sfx_seg S_SPLASH[] = { SFX_SEG(WAV_NOISE, 3000, 500, 14), SFX_END };
static const sfx_seg S_CRACK[]  = { SFX_SEG(WAV_NOISE, 4000, 4000, 2), SFX_SEG(WAV_NOISE, 1500, 900, 5), SFX_END };
static const sfx_seg S_PUSH[]   = { SFX_SEG(WAV_SAWTOOTH, 160, 220, 6), SFX_END };
static const sfx_seg S_UNDO[]   = { SFX_SEG(WAV_SQUARE, 900, 500, 5), SFX_END };
static const sfx_seg S_WIN[]    = { SFX_SEG(WAV_TRIANGLE, 500, 1200, 10), SFX_SEG(WAV_NOISE, 2400, 600, 10), SFX_END };
static const sfx_seg S_CLEAR[]  = { SFX_SEG(WAV_SQUARE, 523, 523, 7), SFX_SEG(WAV_SQUARE, 659, 659, 7), SFX_SEG(WAV_SQUARE, 784, 784, 7),
                                    SFX_SEG(WAV_SQUARE, 1047, 1047, 12), SFX_SEG(WAV_SQUARE, 784, 784, 6), SFX_SEG(WAV_SQUARE, 1047, 1047, 6),
                                    SFX_SEG(WAV_SQUARE, 1319, 1319, 30), SFX_END };
static const sfx_seg S_TICK[]   = { SFX_SEG(WAV_SQUARE, 1200, 1200, 2), SFX_END };

void game_sfx_init(void) {
    sfx_init(VOICE3, VOL_75);
}

void game_sfx_tick(void)   { sfx_play_prio(S_TICK, 0); }
void game_sfx_slide(void)  { sfx_play_prio(S_SLIDE, 0); }
void game_sfx_bump(void)   { sfx_play_prio(S_BUMP, 0); }
void game_sfx_thud(void)   { sfx_play_prio(S_THUD, 1); }
void game_sfx_push(void)   { sfx_play_prio(S_PUSH, 1); }
void game_sfx_crack(void)  { sfx_play_prio(S_CRACK, 1); }
void game_sfx_undo(void)   { sfx_play_prio(S_UNDO, 1); }
void game_sfx_dive(void)   { sfx_play_prio(S_DIVE, 2); }
void game_sfx_fish(void)   { sfx_play_prio(S_FISH, 2); }
void game_sfx_splash(void) { sfx_play_prio(S_SPLASH, 2); }
void game_sfx_open(void)   { sfx_play_prio(S_OPEN, 3); }
void game_sfx_win(void)    { sfx_play_prio(S_WIN, 3); }
void game_sfx_clear(void)  { sfx_play_prio(S_CLEAR, 4); }
