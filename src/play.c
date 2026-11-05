/*
 * The puzzle: a floe on a 20x12 grid, the seal's slide, holes, blocks, cracks and the undo log.
 * Same rules, in the same order, as tools/floe.py (the solver that sets each floe's par).
 *
 * Layer 0 draws the floe (ice with edges, the cliff face below it) and the sea, layer 1 the
 * things on the ice. Moves are animated one cell at a time at SPEED px per frame; every rule is
 * applied when the seal (or a block) starts or finishes a cell.
 */
#include <core.h>
#include "game.h"
#include "assets.h"

#define SPEED        4       // px per frame: a cell every 4 frames
#define MAX_STEPS    200     // a slide that long is a hole loop: stop it
#define DIVE_FRAMES  8       // under the ice between two holes, per half
#define FALL_FRAMES  40      // splash, then the move is undone
#define WIN_FRAMES   24
#define LOG_SIZE     1024    // undo log: (cell, old code) pairs, (LOG_MARK, seal cell) starts a move
#define LOG_MARK     0xFF

#define ST_IDLE   0
#define ST_SLIDE  1
#define ST_PUSH   2
#define ST_DIVE   3
#define ST_FALL   4
#define ST_WIN    5

static uint8_t cells[GRID_CELLS];
static uint8_t log_buf[LOG_SIZE];
static uint16_t log_len;
static uint16_t moves;
static uint8_t fish, par, cur_floe;

static uint8_t state;
static uint8_t seal;          // cell
static int8_t step;           // cell delta of the slide: +-1 or +-GRID_W
static uint8_t facing;        // IN_* bit of the last direction
static uint8_t queued;        // a direction pressed while busy
static uint8_t sub;           // px moved into the next cell
static uint8_t target;        // the cell being entered
static uint8_t steps;         // cells slid this move
static uint8_t move_open;     // the move has changed something (counts, undoable)
static uint8_t timer;
static uint8_t dive_to;
static uint8_t block_cell, block_sub;
static uint8_t fx_timer;
static uint8_t idle_timer;

#define CX(i) ((uint8_t)((i) % GRID_W))
#define CY(i) ((uint8_t)((i) / GRID_W))

/* ---------------------------------------------------------------- drawing */

static uint8_t obj_tile_xy(uint8_t c, uint8_t x, uint8_t y) {
    switch (c) {
        case C_SNOW:  return ((x + y) & 1) ? TILE_SNOW2 : TILE_SNOW;
        case C_ROCK:  return TILE_ROCK;
        case C_FISH:  return TILE_FISH;
        case C_BLOCK: return TILE_BLOCK;
        case C_CRACK: return TILE_CRACK;
        case C_WATER: return TILE_WATER;
        case C_SLUSH: return TILE_SLUSH;
        case C_EXIT:  return fish ? TILE_EXIT_SHUT : TILE_EXIT_OPEN;
    }
    if (IS_HOLE(c)) return TILE_HOLE_A + ((c - C_HOLE) >> 1);
    return TILE_EMPTY;
}

static uint8_t obj_tile(uint8_t i) {
    return obj_tile_xy(cells[i], CX(i), CY(i));
}

static void draw_cell(uint8_t i) {
    gfx_tilemap_place(&vctx, obj_tile(i), OBJ_LAYER, CX(i), CY(i) + GRID_Y);
}


/* Called only just after unpack, before any move changes cells: the fresh
 * cells also describe the background shape, so no separate mask is needed.
 * Walk the grid with running x, y and index, no divides. */
static void draw_layers(void) {
    uint8_t row[GRID_W];
    uint8_t x, y;
    uint16_t i;   // runs to 260 over the extra sea row
    for (x = 0; x < GRID_W; x++) row[x] = BG_BAND;
    gfx_tilemap_load(&vctx, row, GRID_W, BG_LAYER, 0, HUD_TOP);
    gfx_tilemap_load(&vctx, row, GRID_W, BG_LAYER, 0, HUD_BOTTOM);
    // grid rows plus the sea row under the grid (screen row 13)
    i = 0;
    for (y = 0; y <= GRID_H; y++) {
        for (x = 0; x < GRID_W; x++, i++) {
            uint8_t t;
            uint8_t up = y > 0 && cells[i - GRID_W] != C_SEA;
            if (y < GRID_H && cells[i] != C_SEA) {
                t = BG_ICE;
                if (!up) t += 1;
                if (x == GRID_W - 1 || cells[i + 1] == C_SEA) t += 2;
                if (y == GRID_H - 1 || cells[i + GRID_W] == C_SEA) t += 4;
                if (x == 0 || cells[i - 1] == C_SEA) t += 8;
            } else if (up) {
                t = BG_FACE;
            } else {
                t = BG_SEA + ((x + y) & 1);
            }
            row[x] = t;
        }
        gfx_tilemap_load(&vctx, row, GRID_W, BG_LAYER, 0, y + GRID_Y);
    }
    i = 0;
    for (y = 0; y < GRID_H; y++) {
        for (x = 0; x < GRID_W; x++, i++) row[x] = obj_tile_xy(cells[i], x, y);
        gfx_tilemap_load(&vctx, row, GRID_W, OBJ_LAYER, 0, y + GRID_Y);
    }
    for (x = 0; x < GRID_W; x++) row[x] = TILE_EMPTY;
    gfx_tilemap_load(&vctx, row, GRID_W, OBJ_LAYER, 0, GRID_H + GRID_Y);
}

static void place(gfx_sprite *s, uint8_t cell, int8_t dx, int8_t dy) {
    s->x = (uint16_t)CX(cell) * 16 + 16 + dx;
    s->y = (uint16_t)(CY(cell) + GRID_Y) * 16 + 16 + dy;
}

static void seal_sprite(uint8_t tile_up, uint8_t tile_side) {
    spr_seal->flags = SPRITE_NONE;
    switch (facing) {
        case IN_UP:    spr_seal->tile = tile_up; break;
        case IN_DOWN:  spr_seal->tile = tile_up; spr_seal->flags = SPRITE_FLIP_Y; break;
        case IN_RIGHT: spr_seal->tile = tile_side; break;
        default:       spr_seal->tile = tile_side; spr_seal->flags = SPRITE_FLIP_X; break;
    }
}

static void fx(gfx_sprite *s, uint8_t cell, uint8_t tile) {
    place(s, cell, 0, 0);
    s->tile = tile;
    s->flags = SPRITE_NONE;
}

/* ---------------------------------------------------------------- load */

/* A preview or load discards the old puzzle. Its undo log is free as packed
 * input scratch until unpack finishes; no extra load buffer is needed. */

/* Unpack a floe into the grid. Returns nothing drawn. */
static void unpack(uint8_t floe) {
    const floe_t *f = &FLOES[floe];
    const uint8_t *src = log_buf;
    zar_entry_t file;
    uint16_t size = ((uint16_t)f->w * f->h + 1) >> 1;
    uint8_t ox = (GRID_W - f->w) >> 1;
    uint8_t oy = (GRID_H - f->h) >> 1;
    uint8_t x, y, k = 0;
    log_len = 0;
    mem_set(cells, C_SEA, sizeof(cells));
    if (zar_entry_from_name(&pack, "floes.bin", &file) != ERR_SUCCESS ||
        zar_entry_read_at(&pack, &file, f->offset, log_buf, &size) != ERR_SUCCESS) {
        mem_set(log_buf, 0, GRID_CELLS / 2);   // unreadable: open sea
    }
    for (y = 0; y < f->h; y++) {
        uint8_t at = (uint8_t)((oy + y) * GRID_W + ox);
        for (x = 0; x < f->w; x++, at++) {
            uint8_t c = (k & 1) ? (*src++ >> 4) : (*src & 0x0F);
            k++;
            cells[at] = c;
        }
    }
    seal = (uint8_t)((oy + f->start / f->w) * GRID_W + ox + f->start % f->w);
    fish = f->fish;
    par = f->par;
}

void play_draw_preview(uint8_t floe) {
    unpack(floe);
    draw_layers();
    facing = IN_UP;
    place(spr_seal, seal, 0, 0);
    seal_sprite(SPR_SEAL_UP, SPR_SEAL_SIDE_UP);
    spr_block->y = SPRITE_OFF;
    spr_fx1->y = SPRITE_OFF;
    spr_fx2->y = SPRITE_OFF;
}

void play_load(uint8_t floe) {
    cur_floe = floe;
    play_draw_preview(floe);
    log_len = 0;
    moves = 0;
    state = ST_IDLE;
    queued = 0;
    idle_timer = 0;
    hud_floe_name(floe);
    hud_stats(fish, moves, par);
}

void play_hide(void) {
    spr_seal->y = SPRITE_OFF;
    spr_block->y = SPRITE_OFF;
    spr_fx1->y = SPRITE_OFF;
    spr_fx2->y = SPRITE_OFF;
}

uint16_t play_moves(void) { return moves; }
uint8_t play_fish_left(void) { return fish; }

/* ---------------------------------------------------------------- undo log */

/* drop the oldest move to make room */
static void log_trim(void) {
    uint16_t i = 2;
    while (i < log_len && log_buf[i] != LOG_MARK) i += 2;
    if (i >= log_len) { log_len = 0; return; }
    for (uint16_t j = i; j < log_len; j++) log_buf[j - i] = log_buf[j];
    log_len -= i;
}

static void log_push(uint8_t a, uint8_t b) {
    if (log_len + 2 > LOG_SIZE) log_trim();
    log_buf[log_len++] = a;
    log_buf[log_len++] = b;
}

/* change a cell, remembering the old code */
static void set_cell(uint8_t i, uint8_t c) {
    log_push(i, cells[i]);
    cells[i] = c;
}

static void exit_redraw(void) {
    uint8_t i;
    for (i = 0; i < GRID_CELLS; i++) {
        if (cells[i] == C_EXIT) { draw_cell(i); return; }
    }
}

/* Roll back to the last mark. Returns 0 when there was nothing to undo. */
static uint8_t undo(void) {
    uint8_t had = fish;
    if (log_len == 0) return 0;
    while (log_len) {
        uint8_t b = log_buf[--log_len];
        uint8_t a = log_buf[--log_len];
        if (a == LOG_MARK) {
            seal = b;
            break;
        }
        if (b == C_FISH) fish++;
        cells[a] = b;
        draw_cell(a);
    }
    if ((had == 0) != (fish == 0)) exit_redraw();
    return 1;
}

/* ---------------------------------------------------------------- moves */

static int8_t dir_step(uint8_t dir) {
    switch (dir) {
        case IN_UP:    return -GRID_W;
        case IN_DOWN:  return GRID_W;
        case IN_LEFT:  return -1;
        default:       return 1;
    }
}

/* cell next to i in the slide direction, or 0xFF off the grid (sea) */
static uint8_t next_cell(uint8_t i, int8_t d) {
    uint8_t x = CX(i);
    if (d == 1 && x == GRID_W - 1) return 0xFF;
    if (d == -1 && x == 0) return 0xFF;
    if (d < -1 && i < GRID_W) return 0xFF;
    if (d > 1 && i >= GRID_CELLS - GRID_W) return 0xFF;
    return (uint8_t)(i + d);
}

static uint8_t code_at(uint8_t i) {
    return i == 0xFF ? C_SEA : cells[i];
}

/* first change of a move: it counts */
static void move_begin(void) {
    if (move_open) return;
    move_open = 1;
    log_push(LOG_MARK, seal);
    if (moves < 65535) moves++;
    hud_stats(fish, moves, par);
}

static void start_move(uint8_t dir) {
    facing = dir;
    step = dir_step(dir);
    steps = 0;
    sub = 0;
    move_open = 0;
    state = ST_SLIDE;
}

static void end_move(void) {
    state = ST_IDLE;
    idle_timer = 0;
    seal_sprite(SPR_SEAL_UP, SPR_SEAL_SIDE_UP);
}

/* the seal is at the start of a cell: what's next? (floe.py: the top of the while loop) */
static void slide_decide(void) {
    uint8_t n = next_cell(seal, step);
    uint8_t c = code_at(n);
    if (c == C_SNOW || c == C_ROCK || (c == C_EXIT && fish)) {
        if (!move_open) game_sfx_bump(); else game_sfx_thud();
        end_move();
        return;
    }
    if (c == C_BLOCK) {
        uint8_t m = code_at(next_cell(n, step));
        if (!IS_ICE(m) && m != C_WATER && m != C_SEA) {
            if (!move_open) game_sfx_bump(); else game_sfx_thud();
            end_move();
            return;
        }
        move_begin();
        set_cell(n, C_ICE);
        draw_cell(n);
        block_cell = n;
        block_sub = 0;
        spr_block->tile = TILE_BLOCK;
        spr_block->flags = SPRITE_NONE;
        place(spr_block, n, 0, 0);
        game_sfx_push();
        state = ST_PUSH;
        return;
    }
    if (c == C_SEA || c == C_WATER) {
        // in it goes: splash, then undo the whole move
        move_begin();
        target = n;
        timer = FALL_FRAMES;
        spr_seal->y = SPRITE_OFF;
        if (n != 0xFF) fx(spr_fx1, n, SPR_SPLASH1);
        game_sfx_splash();
        state = ST_FALL;
        return;
    }
    move_begin();
    if (cells[seal] == C_CRACK) {
        set_cell(seal, C_WATER);
        draw_cell(seal);
        game_sfx_crack();
    }
    if (steps == 0) game_sfx_slide();
    target = n;
    sub = SPEED;
}

/* the seal finished entering target */
static void slide_arrive(void) {
    uint8_t c = cells[target];
    seal = target;
    sub = 0;
    if (++steps > MAX_STEPS) {
        // a hole loop: treated like falling in, as the solver does
        timer = FALL_FRAMES;
        spr_seal->y = SPRITE_OFF;
        fx(spr_fx1, seal, SPR_SPLASH1);
        state = ST_FALL;
        return;
    }
    if (c == C_FISH) {
        set_cell(seal, C_ICE);
        draw_cell(seal);
        fx(spr_fx2, seal, SPR_STAR1);
        fx_timer = 16;
        if (--fish == 0) {
            exit_redraw();
            game_sfx_open();
        } else {
            game_sfx_fish();
        }
        hud_stats(fish, moves, par);
    } else if (c == C_EXIT) {
        timer = WIN_FRAMES;
        spr_seal->tile = SPR_DIVE1;
        spr_seal->flags = SPRITE_NONE;
        place(spr_seal, seal, 0, 0);
        game_sfx_win();
        state = ST_WIN;
    } else if (IS_HOLE(c)) {
        uint8_t p = HOLE_PARTNER(c), i;
        for (i = 0; i < GRID_CELLS; i++) {
            if (cells[i] == p) break;
        }
        dive_to = i;
        timer = DIVE_FRAMES * 2;
        game_sfx_dive();
        state = ST_DIVE;
    }
}

static void push_update(void) {
    if (block_sub == 0) {
        uint8_t n = next_cell(block_cell, step);
        uint8_t m = code_at(n);
        if (IS_ICE(m) || m == C_WATER || m == C_SEA) {
            block_sub = SPEED;
            target = n;
        } else {
            set_cell(block_cell, C_BLOCK);
            draw_cell(block_cell);
            spr_block->y = SPRITE_OFF;
            game_sfx_thud();
            end_move();
            return;
        }
    } else {
        block_sub += SPEED;
    }
    if (block_sub >= 16) {
        uint8_t m = code_at(target);
        block_sub = 0;
        if (IS_ICE(m)) {
            block_cell = target;
        } else {
            // into water: a hole is filled, the open sea just swallows it
            if (m == C_WATER) {
                set_cell(target, C_SLUSH);
                draw_cell(target);
            }
            if (target != 0xFF) {
                fx(spr_fx1, target, SPR_SPLASH1);
                fx_timer = 16;
            }
            spr_block->y = SPRITE_OFF;
            game_sfx_splash();
            end_move();
            return;
        }
    }
    {
        int8_t dx = 0, dy = 0;
        if (step == 1) dx = block_sub;
        else if (step == -1) dx = -(int8_t)block_sub;
        else if (step > 0) dy = block_sub;
        else dy = -(int8_t)block_sub;
        place(spr_block, block_cell, dx, dy);
    }
}

static uint8_t dir_pressed(void) {
    uint16_t p = input_pressed & IN_DIRS;
    if (p & IN_UP) return IN_UP;
    if (p & IN_DOWN) return IN_DOWN;
    if (p & IN_LEFT) return IN_LEFT;
    if (p & IN_RIGHT) return IN_RIGHT;
    return 0;
}

uint8_t play_update(void) {
    uint8_t d = dir_pressed();

    if (fx_timer && --fx_timer == 0) {
        spr_fx2->y = SPRITE_OFF;
        if (state != ST_FALL) spr_fx1->y = SPRITE_OFF;
    } else if (fx_timer == 8) {
        if (spr_fx2->tile == SPR_STAR1) spr_fx2->tile = SPR_STAR2;
        if (spr_fx1->tile == SPR_SPLASH1) spr_fx1->tile = SPR_SPLASH2;
    }

    switch (state) {
        case ST_IDLE:
            if (input_pressed & IN_UNDO) {
                if (undo()) {
                    moves--;
                    hud_stats(fish, moves, par);
                    game_sfx_undo();
                    place(spr_seal, seal, 0, 0);
                }
                queued = 0;
                break;
            }
            if (input_pressed & IN_RESTART) {
                play_load(cur_floe);
                game_sfx_undo();
                break;
            }
            if (!d) d = queued;
            queued = 0;
            if (d) {
                start_move(d);
                slide_decide();
            } else {
                // idle: a flipper wiggle now and then
                idle_timer++;
                seal_sprite((idle_timer & 0x40) && (idle_timer & 0x08) ? SPR_SEAL_UP2 : SPR_SEAL_UP,
                            (idle_timer & 0x40) && (idle_timer & 0x08) ? SPR_SEAL_SIDE_UP2 : SPR_SEAL_SIDE_UP);
            }
            break;

        case ST_SLIDE:
            if (d) queued = d;
            seal_sprite(SPR_SEAL_SLIDE, SPR_SEAL_SIDE_SLIDE);
            if (sub) {
                sub += SPEED;
                if (sub >= 16) {
                    slide_arrive();
                    if (state == ST_SLIDE) slide_decide();
                }
            }
            break;

        case ST_PUSH:
            if (d) queued = d;
            push_update();
            break;

        case ST_DIVE:
            if (d) queued = d;
            timer--;
            if (timer == DIVE_FRAMES) {
                seal = dive_to;
                fx(spr_fx1, seal, SPR_DIVE2);
                fx_timer = 12;
            }
            spr_seal->tile = timer > DIVE_FRAMES ? (timer > DIVE_FRAMES + 4 ? SPR_DIVE1 : SPR_DIVE2) :
                             (timer > 4 ? SPR_DIVE2 : SPR_DIVE1);
            spr_seal->flags = SPRITE_NONE;
            if (timer == 0) {
                state = ST_SLIDE;
                slide_decide();
            }
            break;

        case ST_FALL:
            if (timer == FALL_FRAMES - 10) spr_fx1->tile = SPR_SPLASH2;
            if (timer == FALL_FRAMES - 22) spr_fx1->y = SPRITE_OFF;
            if (--timer == 0) {
                spr_fx1->y = SPRITE_OFF;
                undo();
                moves--;
                hud_stats(fish, moves, par);
                queued = 0;
                end_move();
            }
            break;

        case ST_WIN:
            if (timer == WIN_FRAMES / 2) spr_seal->tile = SPR_DIVE2;
            if (--timer == 0) {
                spr_seal->y = SPRITE_OFF;
                return PLAY_WON;
            }
            return PLAY_BUSY;
    }

    if (state == ST_FALL) return PLAY_BUSY;
    {
        int8_t dx = 0, dy = 0;
        uint8_t s = (state == ST_SLIDE) ? sub : 0;
        if (step == 1) dx = s;
        else if (step == -1) dx = -(int8_t)s;
        else if (step > 0) dy = s;
        else dy = -(int8_t)s;
        place(spr_seal, seal, dx, dy);
    }
    return state == ST_IDLE ? PLAY_IDLE : PLAY_BUSY;
}

uint8_t play_idle(void) { return state == ST_IDLE; }

void play_redraw_rows(uint8_t row0, uint8_t row1) {
    uint8_t row[GRID_W];
    uint8_t x, y, i;
    for (y = row0; y <= row1; y++) {
        if (y >= GRID_Y && y < GRID_Y + GRID_H) {
            i = (uint8_t)((y - GRID_Y) * GRID_W);
            for (x = 0; x < GRID_W; x++, i++) row[x] = obj_tile_xy(cells[i], x, y - GRID_Y);
        } else {
            for (x = 0; x < GRID_W; x++) row[x] = TILE_EMPTY;
        }
        gfx_tilemap_load(&vctx, row, GRID_W, OBJ_LAYER, 0, y);
    }
}

void panel(uint8_t row0, uint8_t row1) {
    for (; row0 <= row1; row0++) tilemap_fill(&vctx, OBJ_LAYER, BG_BAND, 0, row0, GRID_W, 1);
}
