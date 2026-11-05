/* Run the real puzzle rules, archive cells and generated par solutions. */
#include "game.h"
#include "assets.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

gfx_context vctx;
uint16_t input_now, input_pressed;
uint8_t frame_count;
static gfx_sprite sprites[SPR_COUNT];
gfx_sprite *spr_seal = &sprites[0], *spr_block = &sprites[1];
gfx_sprite *spr_fx1 = &sprites[2], *spr_fx2 = &sprites[3];
gfx_sprite *spr_arrow_l = &sprites[4], *spr_arrow_r = &sprites[5];
zar_file_t pack;
static FILE *floes;
static uint8_t vram[2][15][GRID_W];

void *mem_cpy(void *d, const void *s, size_t n) { return memmove(d, s, n); }
void *mem_set(void *d, uint8_t v, size_t n) { return memset(d, v, n); }
zos_err_t zar_entry_from_name(zar_file_t *p, const char *name, zar_entry_t *e) {
    (void)p; (void)e; assert(strcmp(name, "floes.bin") == 0); return ERR_SUCCESS;
}
zos_err_t zar_entry_read_at(zar_file_t *p, zar_entry_t *e, uint16_t off, void *dst, uint16_t *n) {
    (void)p; (void)e;
    assert(fseek(floes, off, SEEK_SET) == 0);
    assert(fread(dst, 1, *n, floes) == *n);
    return ERR_SUCCESS;
}
gfx_error gfx_tilemap_place(gfx_context *c, uint8_t tile, uint8_t layer, uint8_t x, uint8_t y) {
    (void)c; assert(layer < 2 && x < GRID_W && y < 15);
    vram[layer][y][x] = tile; return GFX_SUCCESS;
}
gfx_error gfx_tilemap_load(gfx_context *c, void *data, uint8_t n, uint8_t layer, uint8_t x, uint8_t y) {
    uint8_t *src = data;
    while (n--) gfx_tilemap_place(c, *src++, layer, x++, y);
    return GFX_SUCCESS;
}
gfx_error tilemap_fill(gfx_context *c, uint8_t layer, uint8_t tile, uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    uint16_t dx, dy;
    for (dy = 0; dy < h; dy++) for (dx = 0; dx < w; dx++)
        gfx_tilemap_place(c, tile, layer, x + dx, y + dy);
    return GFX_SUCCESS;
}
void hud_floe_name(uint8_t f) { (void)f; }
void hud_stats(uint8_t f, uint16_t m, uint8_t p) { (void)f; (void)m; (void)p; }
#define SILENT(name) void game_sfx_##name(void) {}
SILENT(bump) SILENT(crack) SILENT(dive) SILENT(fish) SILENT(open)
SILENT(push) SILENT(slide) SILENT(splash) SILENT(thud) SILENT(undo) SILENT(win)

/* Inclusion also exposes the undo state for state-restoration assertions. */
#include "../src/play.c"

static uint8_t run_move(uint16_t keys) {
    unsigned ticks = 0;
    uint8_t result;
    input_pressed = input_now = keys;
    do {
        result = play_update();
        input_pressed = input_now = 0;
        assert(++ticks < 2000);
    } while (result == PLAY_BUSY);
    return result;
}

int main(int argc, char **argv) {
    uint8_t f;
    assert(argc == 2);
    floes = fopen(argv[1], "rb"); assert(floes);
    for (f = 0; f < FLOE_COUNT; f++) {
        const char *s = SOLUTIONS[f];
        uint8_t result = PLAY_IDLE;
        uint8_t initial[GRID_CELLS], background[15][GRID_W], initial_seal;
        play_load(f);
        memcpy(initial, cells, sizeof(initial)); initial_seal = seal;
        memcpy(background, vram[BG_LAYER], sizeof(background));
        while (*s) {
            uint16_t keys = *s == 'U' ? IN_UP : *s == 'D' ? IN_DOWN : *s == 'L' ? IN_LEFT : IN_RIGHT;
            result = run_move(keys);
            if (s == SOLUTIONS[f] && result != PLAY_WON) {
                run_move(IN_UNDO);
                assert(memcmp(cells, initial, sizeof(initial)) == 0);
                assert(seal == initial_seal && play_moves() == 0 && fish == FLOES[f].fish);
                result = run_move(keys);
            }
            s++;
        }
        assert(result == PLAY_WON && fish == 0 && play_moves() == FLOES[f].par);
        assert(memcmp(background, vram[BG_LAYER], sizeof(background)) == 0);
        /* A new preview replaces cells/history; it must not retain old scratch. */
        play_draw_preview(f);
        assert(memcmp(cells, initial, sizeof(initial)) == 0 && log_len == 0);
    }
    fclose(floes);
    puts("floe: all 32 par solutions, undo and preview reloads passed");
    return 0;
}
