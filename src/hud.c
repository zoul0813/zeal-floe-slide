/*
 * HUD and banners. Nothing scrolls, so the HUD is plain layer 1 text in rows 0 and 14 (the font
 * sits at the ASCII codes). Banners are sprites, so they can sit over the floe.
 */
#include "game.h"
#include <zgdk/ascii.h>
#include <core.h>
#include <zgdk/hud/sprite.h>

gfx_sprite *spr_seal, *spr_block, *spr_fx1, *spr_fx2, *spr_arrow_l, *spr_arrow_r;
static gfx_sprite *text;
#define TEXT(line, i) (&text[(line) * TEXT_LEN + (i)])

static gfx_sprite *add(void) {
    gfx_sprite s = { .y = SPRITE_OFF, .x = 0, .tile = 0, .flags = SPRITE_NONE, .options = SPRITE_OPTION_NONE };
    return sprites_register(s);
}

zos_err_t hud_init(void) {
    uint8_t i;
    gfx_sprite *last;
    // This game's font occupies tiles 32-95; unsupported characters are blank.
    mem_set(ASCII_MAP, 32, sizeof(ASCII_MAP));
    ascii_map(' ', 64, 32);
    ascii_map(HUD_FISH_ICON, 1, TILE_FISH);

    // slot order is draw order: higher slots on top
    spr_block = add();
    spr_seal = add();
    spr_fx1 = add();
    spr_fx2 = add();
    spr_arrow_l = add();
    spr_arrow_r = add();
    text = add();
    for (i = 1; i < TEXT_LINES * TEXT_LEN; i++) last = add();
    if (!last) return ERR_NO_MORE_MEMORY;
    spr_arrow_l->tile = SPR_ARROW;
    spr_arrow_l->flags = SPRITE_FLIP_X;
    spr_arrow_r->tile = SPR_ARROW;
    return ERR_SUCCESS;
}

void hud_row(uint8_t row, const char *str) {
    uint8_t line[GRID_W];
    uint8_t i = 0;
    while (i < GRID_W && str[i]) { line[i] = ascii_to((uint8_t)str[i]); i++; }
    while (i < GRID_W) line[i++] = ascii_to(' ');
    gfx_tilemap_load(&vctx, line, GRID_W, OBJ_LAYER, 0, row);
}

char *fmt_num(char *p, uint16_t n) {
    char digits[5];
    uint8_t i = 0;
    do { digits[i++] = '0' + n % 10; n /= 10; } while (n);
    while (i) *p++ = digits[--i];
    *p = 0;
    return p;
}

char *fmt_floe(char *p, uint8_t floe) {
    *p++ = '1' + floe / FLOES_PER_WORLD;
    *p++ = '-';
    *p++ = '1' + floe % FLOES_PER_WORLD;
    *p = 0;
    return p;
}

/* row 0: "1-3 BREATHING HOLES" */
void hud_floe_name(uint8_t floe) {
    char buf[24];
    const char *n = WORLD_NAMES[FLOES[floe].world];
    char *p = fmt_floe(buf, floe);
    *p++ = ' ';
    while (*n) *p++ = *n++;
    *p = 0;
    hud_row(HUD_TOP, buf);
}

/* row 14: " <fish>3  MOVES 12  PAR 9", "M 123" from 100 moves */
void hud_stats(uint8_t fish, uint16_t moves, uint8_t par) {
    char buf[24];
    char *p = buf;
    *p++ = HUD_FISH_ICON;
    *p++ = '0' + fish;
    *p++ = ' ';
    *p++ = ' ';
    if (moves >= 100) { *p++ = 'M'; *p++ = ' '; }   // keeps a 2-digit par on the 20-tile row
    else { p[0] = 'M'; p[1] = 'O'; p[2] = 'V'; p[3] = 'E'; p[4] = 'S'; p[5] = ' '; p += 6; }
    p = fmt_num(p, moves);
    *p++ = ' ';
    *p++ = ' ';
    p[0] = 'P'; p[1] = 'A'; p[2] = 'R'; p[3] = ' ';
    fmt_num(p + 4, par);
    hud_row(HUD_BOTTOM, buf);
}

void text_show(uint8_t line, const char *str, uint8_t y) {
    hud_sprite_text_t row = { TEXT(line, 0), TEXT_LEN, 12 };
    // Preserve this font's visual centering (12 px advance, 2 px correction).
    hud_sprite_put_s_centered(&row, str, 158, y);
}

void text_hide(uint8_t line) {
    hud_sprite_hide(TEXT(line, 0), TEXT_LEN);
}

void text_hide_all(void) {
    hud_sprite_hide(TEXT(0, 0), TEXT_LINES * TEXT_LEN);
}
