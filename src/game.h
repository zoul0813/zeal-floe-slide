#include <stdint.h>
#include <zos_errors.h>
#include <zvb_gfx.h>
/* only the ZGDK headers used: <zgdk.h> pulls rect.h/ascii.h/music.h, whose non-inline static
 * functions get compiled into every .c that includes them */
#include "zgdk/input/button_map.h"
#include "zgdk/sprites.h"
#include "zgdk/tilemap.h"
#include "zgdk/sound/sounds.h"
#include "zgdk/sound/tracker.h"
#include <zgdk/input.h>
#include <zgdk/sound/sfx.h>
#include "gfx.h"
#include "levels.h"

#ifndef GAME_H
#define GAME_H

#define BG_LAYER    0   // sea and the floe itself
#define OBJ_LAYER   1   // snow, rocks, fish, holes... and the HUD text

/* the puzzle grid: screen rows 1..12, 20 columns; a floe is centered in it */
#define GRID_W      20
#define GRID_H      12
#define GRID_Y      1
#define GRID_CELLS  (GRID_W * GRID_H)
#define HUD_TOP     0
#define HUD_BOTTOM  14

/* cell codes, same as tools/floe.py */
#define C_SEA    0
#define C_ICE    1
#define C_SNOW   2
#define C_ROCK   3
#define C_FISH   4
#define C_BLOCK  5
#define C_CRACK  6
#define C_WATER  7
#define C_EXIT   8
#define C_HOLE   9    // 9..14, pairs (9,10) (11,12) (13,14)
#define C_SLUSH  15   // a hole a block filled: plain ice to every rule (C only, the solver calls it ice)
#define IS_ICE(c)      ((c) == C_ICE || (c) == C_SLUSH)
#define IS_HOLE(c)     ((uint8_t)((c) - C_HOLE) < 6)
#define HOLE_PARTNER(c) ((uint8_t)(((c) - C_HOLE) ^ 1) + C_HOLE)

/* input */
#define IN_LEFT    BUTTON_LEFT
#define IN_RIGHT   BUTTON_RIGHT
#define IN_UP      BUTTON_UP
#define IN_DOWN    BUTTON_DOWN
#define IN_DIRS    (BUTTON_LEFT | BUTTON_RIGHT | BUTTON_UP | BUTTON_DOWN)
#define IN_UNDO    (BUTTON_B | BUTTON_Y)      // Z / Space / A key, SNES B or Y
#define IN_RESTART (BUTTON_A | BUTTON_X)      // X / S key, SNES A or X
#define IN_START   BUTTON_START               // Enter, SNES Start
#define IN_QUIT    BUTTON_SELECT              // Right Shift, SNES Select

typedef struct {
    uint16_t offset;   // into floes.bin (floeslide.zar), cells packed two per byte, low nibble first
    uint8_t w, h;
    uint8_t start;     // cell index inside the w x h box
    uint8_t fish;
    uint8_t par;       // fewest moves (the solver's)
    uint8_t world;     // 0 based
} floe_t;

extern const floe_t FLOES[FLOE_COUNT];
extern const char * const WORLD_NAMES[WORLD_COUNT];
#ifdef AUTOPILOT
extern const char * const SOLUTIONS[FLOE_COUNT];
#endif

/* play.c: the puzzle */
#define PLAY_IDLE   0
#define PLAY_BUSY   1   // sliding, pushing, diving
#define PLAY_WON    2   // dove into the open exit
void play_load(uint8_t floe);       // draws both layers
void play_draw_preview(uint8_t floe);
uint8_t play_update(void);          // once per frame, PLAY_*
uint16_t play_moves(void);
uint8_t play_fish_left(void);
uint8_t play_idle(void);
void play_hide(void);
void play_redraw_rows(uint8_t row0, uint8_t row1);   // screen rows, layer 1 objects
void panel(uint8_t row0, uint8_t row1);              // dark band on layer 1 behind banners

/* hud.c */
#define TEXT_LINES 3
#define TEXT_LEN   20
/* Character glyphs mapped to icon tiles by hud_init(). */
#define HUD_FISH_ICON '@'

zos_err_t hud_init(void);
void hud_row(uint8_t row, const char *str);   // layer 1 text row, padded to 20
void hud_floe_name(uint8_t floe);
void hud_stats(uint8_t fish, uint16_t moves, uint8_t par);
void text_show(uint8_t line, const char *str, uint8_t y);
void text_hide(uint8_t line);
void text_hide_all(void);
char *fmt_floe(char *p, uint8_t floe);   // "1-3"
char *fmt_num(char *p, uint16_t n);       // no leading zeros

/* sprites: seal, block, two effects, two arrows, then the text */
#define SPR_COUNT   6
#define ARENA_SIZE  (SPR_COUNT + TEXT_LINES * TEXT_LEN)
extern gfx_sprite *spr_seal, *spr_block, *spr_fx1, *spr_fx2, *spr_arrow_l, *spr_arrow_r;
#define SPRITE_OFF 0

extern gfx_context vctx;
extern uint16_t input_now;
extern uint16_t input_pressed;
extern uint8_t frame_count;

/* audio */
#define MUSIC_PATTERNS 3
#define BGM_NONE   0xFF
#define BGM_TITLE  0
#define BGM_FLOE   1
#define BGM_END    2
zos_err_t bgm_init(void);
void *bgm_scratch(uint16_t *size);   // the pattern buffers, free until the first bgm_play()
void bgm_play(uint8_t song);
void bgm_stop(void);
void bgm_tick(void);
void bgm_pause(void);
void bgm_resume(void);
void game_sfx_init(void);
void game_sfx_slide(void);
void game_sfx_bump(void);
void game_sfx_fish(void);
void game_sfx_open(void);
void game_sfx_dive(void);
void game_sfx_splash(void);
void game_sfx_crack(void);
void game_sfx_push(void);
void game_sfx_thud(void);
void game_sfx_undo(void);
void game_sfx_win(void);
void game_sfx_tick(void);
void game_sfx_clear(void);

#ifdef AUTOPILOT
uint16_t autopilot_input(void);
void autopilot_reset(uint8_t floe);
#endif

#endif
