/*
 * Floe Slide: a seal on the sea ice can't steer, so every push is a belly slide that only stops
 * at a snow bank, a rock or a block. Collect the fish, then slide into the exit hole.
 * Breathing holes come in pairs, blocks can be pushed (and fill open water), cracked ice gives
 * way after one crossing. 32 floes in 4 worlds, undo any move.
 * Top-down puzzle for Zeal 8-bit OS on ZGDK.
 */
#include <zos_vfs.h>
#include <stdint.h>
#include <zos_errors.h>
#include <zos_sys.h>
#include <zos_video.h>
#include "game.h"
#include "controls.h"
#include "assets.h"
#include "zgdk/tilemap/scroll.h"
#include <zgdk/sprites.h>

#ifndef START_FLOE
#define START_FLOE 0
#endif
#ifndef SAVE_PATH
#ifdef EMULATOR
#define SAVE_PATH "H:/floe.sav"   // HostFS: the game folder
#else
#define SAVE_PATH "B:/floe.sav"
#endif
#endif
#define CLEAR_FRAMES 150
#define SAVE_MAGIC   0xF1

gfx_context vctx;
uint16_t input_now;
uint16_t input_pressed;
uint8_t frame_count;
static uint16_t input_last;
static gfx_sprite arena[ARENA_SIZE];
#ifdef AUTOPILOT
static uint8_t autopilot_on;
#endif

/* progress: floes unlocked, fewest moves per floe (0 = not cleared) */
struct {
    uint8_t magic;
    uint8_t unlocked;
    uint8_t best[FLOE_COUNT];
} save;   // global: the fps tools watch _save

static void fatal(zos_err_t err, const char *what) {
    // printf would pull in ~2.6 KB of formatting code
    uint16_t len = 0;
    char code[4] = { ' ', '0' + (err / 10) % 10, '0' + err % 10, '\n' };
    ioctl(DEV_STDOUT, CMD_RESET_SCREEN, NULL);
    while (what[len]) len++;
    write(DEV_STDOUT, what, &len);
    len = sizeof(code);
    write(DEV_STDOUT, code, &len);
    exit(err);
}

static void save_load(void) {
    uint16_t size = sizeof(save);
    zos_dev_t dev = open(SAVE_PATH, O_RDONLY);
    save.magic = 0;
    if (dev >= 0) {
        if (read(dev, &save, &size) != ERR_SUCCESS || size != sizeof(save)) save.magic = 0;
        close(dev);
    }
    if (save.magic != SAVE_MAGIC || save.unlocked == 0 || save.unlocked > FLOE_COUNT) {
        uint8_t i;
        save.magic = SAVE_MAGIC;
        save.unlocked = 1;
        for (i = 0; i < FLOE_COUNT; i++) save.best[i] = 0;
    }
}

static void save_write(void) {
    uint16_t size = sizeof(save);
    // no O_TRUNC: the file is always sizeof(save), so it is rewritten in place and keeps its disk space
    zos_dev_t dev = open(SAVE_PATH, O_WRONLY | O_CREAT);
    if (dev < 0) return;   // no disk: progress lasts until the game quits
    write(dev, &save, &size);
    close(dev);
}

static void read_input(void) {
    input_now = input_read();
#ifdef AUTOPILOT
    if (autopilot_on) input_now = (input_now & (IN_START | IN_QUIT)) | autopilot_input();
#endif
    input_pressed = input_now & ~input_last;
    input_last = input_now;
}

/* sea waves (palette 1-3) roll every 16 frames, the open exit glows (4-5) every 8 */
static const uint16_t WAVES[3] = { 0x2B55, 0x3C18, 0x5D1A };
static const uint16_t GLOW[2] = { 0xFEEB, 0xFD07 };
static void palette_cycle(void) {
    uint16_t pal[3];
    uint8_t k;
    if ((frame_count & 15) == 0) {
        k = (frame_count >> 4) % 3;
        pal[0] = WAVES[k];
        pal[1] = WAVES[(k + 1) % 3];
        pal[2] = WAVES[(k + 2) % 3];
        gfx_palette_load(&vctx, pal, 6, 1);
    } else if ((frame_count & 7) == 0) {
        k = (frame_count >> 3) & 1;
        pal[0] = GLOW[k];
        pal[1] = GLOW[k ^ 1];
        gfx_palette_load(&vctx, pal, 4, 4);
    }
}

static void frame_end(void) {
    gfx_wait_vblank(&vctx);
    sprites_render(&vctx);
    palette_cycle();
    gfx_wait_end_vblank(&vctx);
    bgm_tick();
    sfx_tick();
    frame_count++;
}

static void init(void) {
    zos_err_t err;

    err = input_init(1);
    if (err) fatal(err, "input");

    gfx_enable_screen(0);
    err = gfx_initialize(ZVB_CTRL_VID_MODE_GFX_320_8BIT, &vctx);
    if (err) fatal(err, "gfx");
    err = sprites_clear(&vctx);   // a previous program may have left sprites in the table
    if (err) fatal(err, "sprites");
    err = zar_file_open(PACK_FILE, &pack);   // floeslide.zar, next to the .bin
    if (err) fatal(err, PACK_FILE);
    err = load_palette(&vctx);
    if (err) fatal(err, "palette");
    err = load_tileset(&vctx);
    if (err) fatal(err, "tileset");
    tilemap_fill(&vctx, BG_LAYER, BG_SEA, 0, 0, 80, 40);
    tilemap_fill(&vctx, OBJ_LAYER, TILE_EMPTY, 0, 0, 80, 40);
    tilemap_scroll(BG_LAYER, 0, 0);
    tilemap_scroll(OBJ_LAYER, 0, 0);

    sprites_register_arena(arena, ARENA_SIZE);
    err = hud_init();
    if (err) fatal(err, "hud");

    sound_init_hardware();       // resets every voice, so before the tracker and the effects
    err = bgm_init();
    if (err) fatal(err, "music");
    game_sfx_init();
    save_load();

    gfx_enable_screen(1);
}

static void deinit(void) {
    bgm_stop();
    sound_deinit();
    zar_file_close(&pack);
    (void)sprites_clear(&vctx);   // best-effort cleanup before returning to ZOS
    ioctl(DEV_STDOUT, CMD_RESET_SCREEN, NULL);
}

static void title_info(uint8_t floe) {
    char buf[24];
    char *p;
    play_draw_preview(floe);
    hud_row(HUD_TOP, "     FLOE SLIDE");
    p = fmt_floe(buf, floe);
    *p++ = ' ';
    {
        const char *n = WORLD_NAMES[FLOES[floe].world];
        while (*n) *p++ = *n++;
    }
    *p = 0;
    panel(12, 13);
    text_show(0, buf, 192);
    if (floe >= save.unlocked) {
        text_show(1, "LOCKED", 208);
    } else if (save.best[floe]) {
        p = buf;
        p[0] = 'B'; p[1] = 'E'; p[2] = 'S'; p[3] = 'T'; p[4] = ' ';
        p = fmt_num(p + 5, save.best[floe]);
        p[0] = ' '; p[1] = ' '; p[2] = 'P'; p[3] = 'A'; p[4] = 'R'; p[5] = ' ';
        fmt_num(p + 6, FLOES[floe].par);
        text_show(1, buf, 208);
    } else {
        p = buf;
        p[0] = 'P'; p[1] = 'A'; p[2] = 'R'; p[3] = ' ';
        fmt_num(p + 4, FLOES[floe].par);
        text_show(1, buf, 208);
    }
    spr_arrow_l->y = floe ? 192 + 16 : SPRITE_OFF;
    spr_arrow_r->y = floe + 1 < FLOE_COUNT ? 192 + 16 : SPRITE_OFF;
}

static void title_controls(uint8_t page) {
    const char *text;
    if (page == 0)
        text = CONTROL_PROMPT(" ARROWS: PICK FLOE", " " PAD_DPAD ": PICK FLOE");
    else if (page == 1)
        text = CONTROL_PROMPT(" ENTER: PLAY", " " PAD_START ": PLAY");
    else
        text = CONTROL_PROMPT(" Z: UNDO  X: RESTART", " " PAD_B ": UNDO  " PAD_A ": RESTART");
    hud_row(HUD_BOTTOM, text);
}

/* Title: the floe picker. Returns the floe to play, 0xFF to quit. */
static uint8_t title(void) {
    uint8_t floe = save.unlocked - 1;
    uint16_t t = 0;
#ifdef AUTOPILOT
    floe = START_FLOE;
#endif
    input_flush();
    controls_menu_begin();
    text_hide_all();
    spr_arrow_l->x = 16 + 4;
    spr_arrow_r->x = 16 + 300;
    title_info(floe);
    title_controls(0);
    input_last = 0;
    bgm_play(BGM_TITLE);

    while (1) {
        uint8_t display_changed;
        read_input();
        display_changed = controls_menu_update();
        t++;
        if ((input_pressed & IN_LEFT) && floe > 0) { floe--; title_info(floe); game_sfx_tick(); }
        if ((input_pressed & IN_RIGHT) && floe + 1 < FLOE_COUNT) { floe++; title_info(floe); game_sfx_tick(); }
        if ((input_pressed & IN_UP) && floe >= FLOES_PER_WORLD) { floe -= FLOES_PER_WORLD; title_info(floe); game_sfx_tick(); }
        if ((input_pressed & IN_DOWN) && floe + FLOES_PER_WORLD < FLOE_COUNT) { floe += FLOES_PER_WORLD; title_info(floe); game_sfx_tick(); }
        if ((input_pressed & IN_START) && floe < save.unlocked) break;
        if (input_pressed & IN_QUIT) return 0xFF;
#ifdef AUTOPILOT
        if (t == 90) break;
#endif
        // the bottom row takes turns: how to pick, how to play
        if (display_changed || (t & 127) == 0)
            title_controls((t >> 7) % 3);
        frame_end();
    }
    text_hide_all();
    spr_arrow_l->y = SPRITE_OFF;
    spr_arrow_r->y = SPRITE_OFF;
    return floe;
}

/* returns 1 to quit to the title */
static uint8_t pause(void) {
    uint8_t quit = 0;
    panel(5, 8);
    text_show(0, "PAUSED", 84);
    text_show(1, CONTROL_PROMPT("ENTER: GO ON", PAD_START ": GO ON"), 104);
    text_show(2, CONTROL_PROMPT("SHIFT: TITLE", PAD_SELECT ": TITLE"), 124);
    bgm_pause();
    input_flush();
    input_last = 0;
    do {
        read_input();
        if (input_pressed & IN_QUIT) { quit = 1; break; }
        frame_end();
    } while (!(input_pressed & IN_START));
    text_hide_all();
    play_redraw_rows(5, 8);
    bgm_resume();
    return quit;
}

static void floe_clear(uint8_t floe) {
    char buf[24];
    char *p = buf;
    uint16_t moves = play_moves();
    uint8_t par = FLOES[floe].par;
    p[0] = 'M'; p[1] = 'O'; p[2] = 'V'; p[3] = 'E'; p[4] = 'S'; p[5] = ' ';
    p = fmt_num(p + 6, moves);
    p[0] = ' '; p[1] = ' '; p[2] = 'P'; p[3] = 'A'; p[4] = 'R'; p[5] = ' ';
    fmt_num(p + 6, par);
    panel(5, 8);
    text_show(0, "FLOE CLEAR!", 84);
    text_show(1, buf, 104);
    if (moves <= par) text_show(2, "PERFECT!", 124);
    if (moves <= 255 && (!save.best[floe] || moves < save.best[floe])) save.best[floe] = (uint8_t)moves;
    if (save.unlocked < FLOE_COUNT && save.unlocked <= floe + 1) save.unlocked = floe + 2;
    save_write();
    // the jingle is a sound effect, so the floe music stays loaded (reparsing a track costs ~4 frames)
    bgm_pause();
    game_sfx_clear();
    input_flush();
    input_last = 0;
    for (uint8_t t = CLEAR_FRAMES; t; t--) {
        read_input();
        if (t < CLEAR_FRAMES - 30 && (input_pressed & (IN_START | IN_UNDO | IN_RESTART))) break;
        frame_end();
    }
    text_hide_all();
}

static void the_end(void) {
    play_hide();
    panel(4, 9);
    text_show(0, "ALL FLOES CLEAR!", 70);
    text_show(1, "THE SEAL IS FULL", 100);
    text_show(2, "THANK YOU", 130);
    bgm_play(BGM_END);
    for (uint16_t t = 0; t < 420; t++) {
        read_input();
        if (t > 60 && (input_pressed & IN_START)) break;
        frame_end();
    }
    text_hide_all();
}

static void play(uint8_t floe) {
#ifdef AUTOPILOT
    autopilot_on = 1;
#endif
    play_load(floe);
#ifdef AUTOPILOT
    autopilot_reset(floe);
#endif
    bgm_play(BGM_FLOE);
    input_flush();
    input_last = 0;

    while (1) {
        read_input();
        if (input_pressed & IN_QUIT) break;
        if (input_pressed & IN_START) {
            if (pause()) break;
        }
        if (play_update() == PLAY_WON) {
            floe_clear(floe);
            if (++floe == FLOE_COUNT) {
                the_end();
                break;
            }
            play_load(floe);
#ifdef AUTOPILOT
            autopilot_reset(floe);
#endif
            bgm_play(BGM_FLOE);
        }
        frame_end();
    }
    play_hide();
    text_hide_all();
    bgm_stop();
#ifdef AUTOPILOT
    autopilot_on = 0;
#endif
    frame_end();
    input_flush();
}

int main(void) {
    uint8_t floe;
    init();
    while ((floe = title()) != 0xFF) play(floe);
    deinit();
    return 0;
}
