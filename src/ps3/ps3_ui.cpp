#include "ps3_platform.h"
#include "ps3_btn_icons.h"

#include <SDL.h>
#include <SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const uint8_t g_font5x7[96][7] = {
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x04,0x04,0x04,0x04,0x04,0x00,0x04},
    {0x0A,0x0A,0x0A,0x00,0x00,0x00,0x00},
    {0x0A,0x1F,0x0A,0x0A,0x1F,0x0A,0x00},
    {0x04,0x0F,0x14,0x0E,0x05,0x1E,0x04},
    {0x18,0x19,0x02,0x04,0x08,0x13,0x03},
    {0x0C,0x12,0x14,0x08,0x15,0x12,0x0D},
    {0x0C,0x04,0x08,0x00,0x00,0x00,0x00},
    {0x02,0x04,0x08,0x08,0x08,0x04,0x02},
    {0x08,0x04,0x02,0x02,0x02,0x04,0x08},
    {0x04,0x15,0x0E,0x1F,0x0E,0x15,0x04},
    {0x00,0x04,0x04,0x1F,0x04,0x04,0x00},
    {0x00,0x00,0x00,0x00,0x0C,0x04,0x08},
    {0x00,0x00,0x00,0x1F,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x0C,0x0C},
    {0x01,0x02,0x02,0x04,0x08,0x08,0x10},
    {0x0E,0x11,0x13,0x15,0x19,0x11,0x0E},
    {0x04,0x0C,0x04,0x04,0x04,0x04,0x0E},
    {0x0E,0x11,0x01,0x06,0x08,0x10,0x1F},
    {0x1F,0x02,0x04,0x06,0x01,0x11,0x0E},
    {0x02,0x06,0x0A,0x12,0x1F,0x02,0x02},
    {0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E},
    {0x06,0x08,0x10,0x1E,0x11,0x11,0x0E},
    {0x1F,0x01,0x02,0x04,0x08,0x08,0x08},
    {0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E},
    {0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C},
    {0x00,0x0C,0x0C,0x00,0x0C,0x0C,0x00},
    {0x00,0x0C,0x0C,0x00,0x0C,0x04,0x08},
    {0x02,0x04,0x08,0x10,0x08,0x04,0x02},
    {0x00,0x00,0x1F,0x00,0x1F,0x00,0x00},
    {0x08,0x04,0x02,0x01,0x02,0x04,0x08},
    {0x0E,0x11,0x02,0x04,0x04,0x00,0x04},
    {0x0E,0x11,0x17,0x15,0x17,0x10,0x0E},
    {0x0E,0x11,0x11,0x1F,0x11,0x11,0x11},
    {0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E},
    {0x0E,0x11,0x10,0x10,0x10,0x11,0x0E},
    {0x1E,0x11,0x11,0x11,0x11,0x11,0x1E},
    {0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F},
    {0x1F,0x10,0x10,0x1E,0x10,0x10,0x10},
    {0x0E,0x11,0x10,0x17,0x11,0x11,0x0F},
    {0x11,0x11,0x11,0x1F,0x11,0x11,0x11},
    {0x0E,0x04,0x04,0x04,0x04,0x04,0x0E},
    {0x07,0x02,0x02,0x02,0x12,0x12,0x0C},
    {0x11,0x12,0x14,0x18,0x14,0x12,0x11},
    {0x10,0x10,0x10,0x10,0x10,0x10,0x1F},
    {0x11,0x1B,0x15,0x15,0x11,0x11,0x11},
    {0x11,0x19,0x15,0x13,0x11,0x11,0x11},
    {0x0E,0x11,0x11,0x11,0x11,0x11,0x0E},
    {0x1E,0x11,0x11,0x1E,0x10,0x10,0x10},
    {0x0E,0x11,0x11,0x11,0x15,0x12,0x0D},
    {0x1E,0x11,0x11,0x1E,0x14,0x12,0x11},
    {0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E},
    {0x1F,0x04,0x04,0x04,0x04,0x04,0x04},
    {0x11,0x11,0x11,0x11,0x11,0x11,0x0E},
    {0x11,0x11,0x11,0x11,0x11,0x0A,0x04},
    {0x11,0x11,0x11,0x15,0x15,0x1B,0x11},
    {0x11,0x11,0x0A,0x04,0x0A,0x11,0x11},
    {0x11,0x11,0x0A,0x04,0x04,0x04,0x04},
    {0x1F,0x01,0x02,0x04,0x08,0x10,0x1F},
    {0x0E,0x08,0x08,0x08,0x08,0x08,0x0E},
    {0x10,0x08,0x08,0x04,0x02,0x02,0x01},
    {0x0E,0x02,0x02,0x02,0x02,0x02,0x0E},
    {0x04,0x0A,0x11,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x1F},
    {0x08,0x04,0x02,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x0E,0x01,0x0F,0x11,0x0F},
    {0x10,0x10,0x16,0x19,0x11,0x11,0x1E},
    {0x00,0x00,0x0E,0x11,0x10,0x11,0x0E},
    {0x01,0x01,0x0D,0x13,0x11,0x11,0x0F},
    {0x00,0x00,0x0E,0x11,0x1F,0x10,0x0E},
    {0x06,0x09,0x08,0x1C,0x08,0x08,0x08},
    {0x00,0x00,0x0F,0x11,0x0F,0x01,0x0E},
    {0x10,0x10,0x16,0x19,0x11,0x11,0x11},
    {0x04,0x00,0x0C,0x04,0x04,0x04,0x0E},
    {0x02,0x00,0x06,0x02,0x02,0x12,0x0C},
    {0x10,0x10,0x12,0x14,0x18,0x14,0x12},
    {0x0C,0x04,0x04,0x04,0x04,0x04,0x0E},
    {0x00,0x00,0x1A,0x15,0x15,0x11,0x11},
    {0x00,0x00,0x16,0x19,0x11,0x11,0x11},
    {0x00,0x00,0x0E,0x11,0x11,0x11,0x0E},
    {0x00,0x00,0x1E,0x11,0x1E,0x10,0x10},
    {0x00,0x00,0x0F,0x11,0x0F,0x01,0x01},
    {0x00,0x00,0x16,0x19,0x10,0x10,0x10},
    {0x00,0x00,0x0E,0x10,0x0E,0x01,0x1E},
    {0x08,0x08,0x1C,0x08,0x08,0x09,0x06},
    {0x00,0x00,0x11,0x11,0x11,0x13,0x0D},
    {0x00,0x00,0x11,0x11,0x11,0x0A,0x04},
    {0x00,0x00,0x11,0x11,0x15,0x15,0x0A},
    {0x00,0x00,0x11,0x0A,0x04,0x0A,0x11},
    {0x00,0x00,0x11,0x11,0x0F,0x01,0x0E},
    {0x00,0x00,0x1F,0x02,0x04,0x08,0x1F},
    {0x06,0x08,0x08,0x10,0x08,0x08,0x06},
    {0x04,0x04,0x04,0x00,0x04,0x04,0x04},
    {0x0C,0x04,0x04,0x02,0x04,0x04,0x0C},
    {0x08,0x15,0x02,0x00,0x00,0x00,0x00},
    {0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F}
};

#define TTF_SLOT_COUNT 4
#define TTF_CACHE_MAX 256

static const int g_ttf_sizes[TTF_SLOT_COUNT] = { 14, 18, 22, 30 };
static TTF_Font *g_fonts[TTF_SLOT_COUNT] = { nullptr, nullptr, nullptr, nullptr };
static int g_ttf_ready = 0;

typedef struct {
    uint32_t hash;
    int slot;
    int w;
    int h;
    uint32_t last_use;
    uint8_t *alpha;
    char text[128];
} ttf_cache_t;

static ttf_cache_t g_ttf_cache[TTF_CACHE_MAX];
static uint32_t g_ttf_tick = 1;

static int scale_to_slot(int scale)
{
    if (scale <= 0) return 0;
    if (scale == 1) return 1;
    if (scale == 2) return 2;
    return 3;
}

static uint32_t fnv1a_str(const char *s, int slot)
{
    uint32_t h = 2166136261u ^ (uint32_t)(slot * 16777619u);
    while (*s) {
        h ^= (uint8_t)(*s++);
        h *= 16777619u;
    }
    return h;
}

static void cache_clear_all(void)
{
    for (int i = 0; i < TTF_CACHE_MAX; i++) {
        if (g_ttf_cache[i].alpha) {
            free(g_ttf_cache[i].alpha);
            g_ttf_cache[i].alpha = nullptr;
        }
        g_ttf_cache[i].text[0] = 0;
        g_ttf_cache[i].last_use = 0;
    }
}

void ps3_ui_init(void)
{
    if (g_ttf_ready) return;
    ps3_boot_mark("ttf_init_enter");
    if (TTF_Init() != 0) {
        ps3_log("TTF_Init failed: %s\n", TTF_GetError());
        ps3_boot_mark("ttf_init_fail");
        return;
    }

    static const char *paths[] = {
        PS3_PKG_USRDIR "/font.ttf",
        PS3_ROOT "/font.ttf",
        "./font.ttf",
        "font.ttf",
        PS3_PKG_USRDIR "/data/font.ttf",
        "./assets/font.ttf",
        nullptr
    };

    const char *found = nullptr;
    for (int i = 0; paths[i]; i++) {
        FILE *fp = fopen(paths[i], "rb");
        if (!fp) continue;
        fclose(fp);
        TTF_Font *probe = TTF_OpenFont(paths[i], g_ttf_sizes[1]);
        if (probe) {
            TTF_CloseFont(probe);
            found = paths[i];
            break;
        }
    }

    if (!found) {
        ps3_log("font.ttf not found, using bitmap fallback\n");
        ps3_boot_mark("ttf_no_font");
        TTF_Quit();
        return;
    }

    for (int i = 0; i < TTF_SLOT_COUNT; i++) {
        g_fonts[i] = TTF_OpenFont(found, g_ttf_sizes[i]);
    }
    memset(g_ttf_cache, 0, sizeof(g_ttf_cache));
    g_ttf_tick = 1;
    g_ttf_ready = (g_fonts[1] != nullptr) ? 1 : 0;
    if (!g_ttf_ready) {
        for (int i = 0; i < TTF_SLOT_COUNT; i++) {
            if (g_fonts[i]) {
                TTF_CloseFont(g_fonts[i]);
                g_fonts[i] = nullptr;
            }
        }
        TTF_Quit();
        ps3_log("TTF open failed for %s\n", found);
        ps3_boot_mark("ttf_open_fail");
        return;
    }
    ps3_log("TTF loaded from %s (ready=%d)\n", found, g_ttf_ready);
    ps3_boot_mark("ttf_ok");
}

void ps3_ui_shutdown(void)
{
    cache_clear_all();
    for (int i = 0; i < TTF_SLOT_COUNT; i++) {
        if (g_fonts[i]) {
            TTF_CloseFont(g_fonts[i]);
            g_fonts[i] = nullptr;
        }
    }
    if (g_ttf_ready) {
        TTF_Quit();
        g_ttf_ready = 0;
    }
}

static inline uint32_t blend_argb(uint32_t dst, uint32_t src, uint32_t a)
{
    if (a == 0) return dst;
    if (a >= 255) return 0xFF000000u | (src & 0x00FFFFFFu);
    uint32_t ia = 255u - a;
    uint32_t rb = (((src & 0x00FF00FFu) * a) + ((dst & 0x00FF00FFu) * ia)) >> 8;
    uint32_t g = (((src & 0x0000FF00u) * a) + ((dst & 0x0000FF00u) * ia)) >> 8;
    return 0xFF000000u | (rb & 0x00FF00FFu) | (g & 0x0000FF00u);
}

static inline uint32_t lerp_rgb(uint32_t c0, uint32_t c1, int num, int den)
{
    if (den <= 0) return c0;
    uint32_t a0 = (c0 >> 24) & 0xFFu;
    uint32_t r0 = (c0 >> 16) & 0xFFu;
    uint32_t g0 = (c0 >> 8) & 0xFFu;
    uint32_t b0 = c0 & 0xFFu;
    uint32_t a1 = (c1 >> 24) & 0xFFu;
    uint32_t r1 = (c1 >> 16) & 0xFFu;
    uint32_t g1 = (c1 >> 8) & 0xFFu;
    uint32_t b1 = c1 & 0xFFu;
    uint32_t a = a0 + (uint32_t)(((int)(a1 - a0) * num) / den);
    uint32_t r = r0 + (uint32_t)(((int)(r1 - r0) * num) / den);
    uint32_t g = g0 + (uint32_t)(((int)(g1 - g0) * num) / den);
    uint32_t b = b0 + (uint32_t)(((int)(b1 - b0) * num) / den);
    return (a << 24) | (r << 16) | (g << 8) | b;
}

void ps3_ui_fill(int x, int y, int w, int h, uint32_t argb)
{
    uint32_t *fb = ps3_video_fb();
    if (!fb || w <= 0 || h <= 0) return;
    int x0 = x < 0 ? 0 : x;
    int y0 = y < 0 ? 0 : y;
    int x1 = (x + w > PS3_SCREEN_W) ? PS3_SCREEN_W : (x + w);
    int y1 = (y + h > PS3_SCREEN_H) ? PS3_SCREEN_H : (y + h);
    uint32_t c = 0xFF000000u | (argb & 0x00FFFFFFu);
    for (int py = y0; py < y1; py++) {
        uint32_t *row = fb + py * PS3_SCREEN_W;
        for (int px = x0; px < x1; px++) {
            row[px] = c;
        }
    }
}

void ps3_ui_blend(int x, int y, int w, int h, uint32_t argb)
{
    uint32_t *fb = ps3_video_fb();
    if (!fb || w <= 0 || h <= 0) return;
    uint32_t a = (argb >> 24) & 0xFFu;
    if (a == 0) return;
    if (a == 255) {
        ps3_ui_fill(x, y, w, h, argb);
        return;
    }
    int x0 = x < 0 ? 0 : x;
    int y0 = y < 0 ? 0 : y;
    int x1 = (x + w > PS3_SCREEN_W) ? PS3_SCREEN_W : (x + w);
    int y1 = (y + h > PS3_SCREEN_H) ? PS3_SCREEN_H : (y + h);
    for (int py = y0; py < y1; py++) {
        uint32_t *row = fb + py * PS3_SCREEN_W;
        for (int px = x0; px < x1; px++) {
            row[px] = blend_argb(row[px], argb, a);
        }
    }
}

void ps3_ui_frame(int x, int y, int w, int h, uint32_t argb)
{
    if (w <= 0 || h <= 0) return;
    uint32_t a = (argb >> 24) & 0xFFu;
    if (a == 255) {
        ps3_ui_fill(x, y, w, 1, argb);
        ps3_ui_fill(x, y + h - 1, w, 1, argb);
        ps3_ui_fill(x, y, 1, h, argb);
        ps3_ui_fill(x + w - 1, y, 1, h, argb);
    } else if (a > 0) {
        ps3_ui_blend(x, y, w, 1, argb);
        if (h > 1) ps3_ui_blend(x, y + h - 1, w, 1, argb);
        if (h > 2) {
            ps3_ui_blend(x, y + 1, 1, h - 2, argb);
            if (w > 1) ps3_ui_blend(x + w - 1, y + 1, 1, h - 2, argb);
        }
    }
}

void ps3_ui_panel(int x, int y, int w, int h, uint32_t argb)
{
    ps3_ui_blend(x, y, w, h, argb);
}

void ps3_ui_panel_grad(int x, int y, int w, int h, uint32_t top_argb, uint32_t bot_argb, uint32_t border_argb)
{
    uint32_t *fb = ps3_video_fb();
    if (!fb || w <= 0 || h <= 0) return;
    int x0 = x < 0 ? 0 : x;
    int y0 = y < 0 ? 0 : y;
    int x1 = (x + w > PS3_SCREEN_W) ? PS3_SCREEN_W : (x + w);
    int y1 = (y + h > PS3_SCREEN_H) ? PS3_SCREEN_H : (y + h);
    int den = h > 1 ? (h - 1) : 1;
    for (int py = y0; py < y1; py++) {
        uint32_t c = lerp_rgb(top_argb, bot_argb, py - y, den);
        uint32_t a = (c >> 24) & 0xFFu;
        uint32_t *row = fb + py * PS3_SCREEN_W;
        if (a == 255) {
            uint32_t oc = 0xFF000000u | (c & 0x00FFFFFFu);
            for (int px = x0; px < x1; px++) row[px] = oc;
        } else if (a > 0) {
            for (int px = x0; px < x1; px++) row[px] = blend_argb(row[px], c, a);
        }
    }
    if ((border_argb >> 24) & 0xFFu) {
        ps3_ui_frame(x, y, w, h, border_argb);
        if (w > 4 && h > 4) {
            ps3_ui_blend(x + 1, y + 1, w - 2, 1, 0x20FFFFFFu);
        }
    }
}

void ps3_ui_bevel(int x, int y, int w, int h, uint32_t hi, uint32_t lo)
{
    if (w < 2 || h < 2) return;
    ps3_ui_blend(x, y, w, 1, hi);
    ps3_ui_blend(x, y + 1, 1, h - 2, hi);
    ps3_ui_blend(x, y + h - 1, w, 1, lo);
    ps3_ui_blend(x + w - 1, y + 1, 1, h - 1, lo);
}

void ps3_ui_hline(int x, int y, int w, uint32_t argb)
{
    ps3_ui_blend(x, y, w, 1, argb);
}

static ttf_cache_t *ttf_get_glyphs(const char *s, int slot)
{
    if (!g_ttf_ready || !g_fonts[slot] || !s || !*s) return nullptr;
    size_t len = strlen(s);
    if (len >= 127) return nullptr;

    uint32_t h = fnv1a_str(s, slot);
    int victim = 0;
    uint32_t min_use = 0xFFFFFFFFu;
    for (int i = 0; i < TTF_CACHE_MAX; i++) {
        ttf_cache_t *e = &g_ttf_cache[i];
        if (e->alpha && e->hash == h && e->slot == slot && strcmp(e->text, s) == 0) {
            e->last_use = ++g_ttf_tick;
            return e;
        }
        if (!e->alpha) {
            victim = i;
            min_use = 0;
        } else if (min_use != 0 && e->last_use < min_use) {
            min_use = e->last_use;
            victim = i;
        }
    }

    SDL_Color white = { 255, 255, 255, 255 };
    SDL_Surface *raw = TTF_RenderUTF8_Blended(g_fonts[slot], s, white);
    if (!raw || !raw->format) {
        if (raw) SDL_FreeSurface(raw);
        return nullptr;
    }

    SDL_Surface *conv = nullptr;
    SDL_Surface *src = raw;
    if (raw->format->format != SDL_PIXELFORMAT_ARGB8888) {
        conv = SDL_ConvertSurfaceFormat(raw, SDL_PIXELFORMAT_ARGB8888, 0);
        if (conv) src = conv;
    }

    if (!src->format || src->format->BytesPerPixel != 4 || !src->pixels || src->w <= 0 || src->h <= 0 || src->format->Amask == 0) {
        if (conv) SDL_FreeSurface(conv);
        SDL_FreeSurface(raw);
        return nullptr;
    }

    int locked = 0;
    if (SDL_MUSTLOCK(src)) {
        if (SDL_LockSurface(src) == 0) locked = 1;
    }

    int w = src->w;
    int hpx = src->h;
    uint8_t *alpha = (uint8_t *)malloc((size_t)(w * hpx + 1));
    if (!alpha) {
        if (locked) SDL_UnlockSurface(src);
        if (conv) SDL_FreeSurface(conv);
        SDL_FreeSurface(raw);
        return nullptr;
    }

    uint32_t amask = src->format->Amask;
    uint8_t ashift = src->format->Ashift;
    for (int py = 0; py < hpx; py++) {
        const uint32_t *row = (const uint32_t *)((const uint8_t *)src->pixels + py * src->pitch);
        uint8_t *dst = alpha + py * w;
        for (int px = 0; px < w; px++) {
            dst[px] = (uint8_t)((row[px] & amask) >> ashift);
        }
    }

    if (locked) SDL_UnlockSurface(src);
    if (conv) SDL_FreeSurface(conv);
    SDL_FreeSurface(raw);

    ttf_cache_t *out = &g_ttf_cache[victim];
    if (out->alpha) free(out->alpha);
    out->hash = h;
    out->slot = slot;
    out->w = w;
    out->h = hpx;
    out->last_use = ++g_ttf_tick;
    out->alpha = alpha;
    memcpy(out->text, s, len + 1);
    return out;
}

static void draw_text_bitmap(int x, int y, const char *s, uint32_t argb, int scale, int maxx)
{
    uint32_t *fb = ps3_video_fb();
    if (!fb || !s) return;
    int sc = scale <= 0 ? 2 : scale;
    uint32_t src_a = (argb >> 24) & 0xFFu;
    if (src_a == 0) return;

    int cx = x;
    while (*s) {
        unsigned char c = (unsigned char)*s++;
        if (c < 32 || c > 127) c = '?';
        int adv = 6 * sc;
        if (maxx > 0 && cx + 5 * sc > maxx) break;
        const uint8_t *g = g_font5x7[c - 32];
        for (int r = 0; r < 7; r++) {
            uint8_t bits = g[r];
            for (int col = 0; col < 5; col++) {
                if (!(bits & (0x10 >> col))) continue;
                int rx = cx + col * sc;
                int ry = y + r * sc;
                if (src_a == 255) {
                    ps3_ui_fill(rx, ry, sc, sc, argb);
                } else {
                    ps3_ui_blend(rx, ry, sc, sc, argb);
                }
            }
        }
        cx += adv;
    }
}

static void draw_ttf_cached(int x, int y, const ttf_cache_t *e, uint32_t argb, int maxx)
{
    uint32_t *fb = ps3_video_fb();
    if (!fb || !e || !e->alpha) return;
    uint32_t top_a = (argb >> 24) & 0xFFu;
    if (top_a == 0) return;

    int x0 = x < 0 ? 0 : x;
    int y0 = y < 0 ? 0 : y;
    int rx1 = x + e->w;
    if (maxx > 0 && rx1 > maxx) rx1 = maxx;
    int x1 = rx1 > PS3_SCREEN_W ? PS3_SCREEN_W : rx1;
    int y1 = (y + e->h > PS3_SCREEN_H) ? PS3_SCREEN_H : (y + e->h);

    for (int py = y0; py < y1; py++) {
        const uint8_t *src = e->alpha + (py - y) * e->w;
        uint32_t *dst = fb + py * PS3_SCREEN_W;
        for (int px = x0; px < x1; px++) {
            uint32_t a = src[px - x];
            if (!a) continue;
            if (top_a < 255) a = (a * top_a) / 255u;
            dst[px] = blend_argb(dst[px], argb, a);
        }
    }
}

int ps3_ui_text_w(const char *s, int scale)
{
    if (!s || !*s) return 0;
    int slot = scale_to_slot(scale);
    if (g_ttf_ready && g_fonts[slot]) {
        int w = 0, h = 0;
        if (TTF_SizeUTF8(g_fonts[slot], s, &w, &h) == 0) return w;
    }
    int sc = scale <= 0 ? 2 : scale;
    return (int)strlen(s) * 6 * sc;
}

int ps3_ui_text_h(int scale)
{
    int slot = scale_to_slot(scale);
    if (g_ttf_ready && g_fonts[slot]) {
        return TTF_FontHeight(g_fonts[slot]);
    }
    int sc = scale <= 0 ? 2 : scale;
    return 7 * sc;
}

void ps3_ui_text(int x, int y, const char *s, uint32_t argb, int scale)
{
    if (!s || !*s) return;
    int slot = scale_to_slot(scale);
    ttf_cache_t *e = ttf_get_glyphs(s, slot);
    if (e) {
        draw_ttf_cached(x, y, e, argb, 0);
        return;
    }
    draw_text_bitmap(x, y, s, argb, scale, 0);
}

void ps3_ui_text_shadow(int x, int y, const char *s, uint32_t argb, int scale)
{
    if (!s || !*s) return;
    uint32_t a = (argb >> 24) & 0xFFu;
    uint32_t sa = (a * 170u) / 255u;
    if (sa > 0) ps3_ui_text(x + 1, y + 1, s, sa << 24, scale);
    ps3_ui_text(x, y, s, argb, scale);
}

void ps3_ui_text_center(int cx, int y, const char *s, uint32_t argb, int scale)
{
    if (!s || !*s) return;
    int w = ps3_ui_text_w(s, scale);
    ps3_ui_text(cx - w / 2, y, s, argb, scale);
}

void ps3_ui_text_clip(int x, int y, int maxw, const char *s, uint32_t argb, int scale)
{
    if (!s || !*s || maxw <= 0) return;
    int slot = scale_to_slot(scale);
    if (g_ttf_ready && g_fonts[slot]) {
        if (ps3_ui_text_w(s, scale) <= maxw) {
            ttf_cache_t *e = ttf_get_glyphs(s, slot);
            if (e) {
                draw_ttf_cached(x, y, e, argb, x + maxw);
                return;
            }
        }
        char buf[192];
        snprintf(buf, sizeof(buf), "%s", s);
        size_t len = strlen(buf);
        if (len > 118) {
            len = 118;
            buf[len] = 0;
        }
        while (len > 2) {
            char tmp[132];
            memcpy(tmp, buf, len);
            tmp[len] = '.';
            tmp[len + 1] = '.';
            tmp[len + 2] = '.';
            tmp[len + 3] = 0;
            if (ps3_ui_text_w(tmp, scale) <= maxw) {
                memcpy(buf, tmp, len + 4);
                break;
            }
            len--;
        }
        if (len <= 2) {
            buf[0] = '.';
            buf[1] = '.';
            buf[2] = '.';
            buf[3] = 0;
        }
        ttf_cache_t *e = ttf_get_glyphs(buf, slot);
        if (e) {
            draw_ttf_cached(x, y, e, argb, x + maxw);
            return;
        }
    }
    draw_text_bitmap(x, y, s, argb, scale, x + maxw);
}

void ps3_ui_image(int x, int y, int w, int h, const uint32_t *px, int iw, int ih)
{
    uint32_t *fb = ps3_video_fb();
    if (!fb || !px || w <= 0 || h <= 0 || iw <= 0 || ih <= 0) return;
    for (int dy = 0; dy < h; dy++) {
        int py = y + dy;
        if (py < 0 || py >= PS3_SCREEN_H) continue;
        int sy = (dy * ih) / h;
        const uint32_t *srow = px + sy * iw;
        uint32_t *drow = fb + py * PS3_SCREEN_W;
        for (int dx = 0; dx < w; dx++) {
            int pxx = x + dx;
            if (pxx < 0 || pxx >= PS3_SCREEN_W) continue;
            int sx = (dx * iw) / w;
            drow[pxx] = 0xFF000000u | (srow[sx] & 0x00FFFFFFu);
        }
    }
}

void ps3_ui_bg(void)
{
    uint32_t *fb = ps3_video_fb();
    if (!fb) return;
    for (int y = 0; y < PS3_SCREEN_H; y++) {
        uint32_t c = lerp_rgb(0xFF0C1320u, 0xFF060A12u, y, PS3_SCREEN_H - 1);
        uint32_t *row = fb + y * PS3_SCREEN_W;
        for (int x = 0; x < PS3_SCREEN_W; x++) {
            uint32_t out = c;
            if (((x & 63) == 0) || ((y & 63) == 0)) {
                out = blend_argb(c, 0xFF1F324Eu, 34);
            }
            row[x] = out;
        }
    }
    ps3_ui_panel_grad(0, 0, PS3_SCREEN_W, 90, 0x26FF8000u, 0x00000000u, 0);
}

void ps3_ui_card_row(int x, int y, int w, int h, int sel)
{
    if (sel) {
        ps3_ui_panel_grad(x, y, w, h, 0xFF1F3E64u, 0xFF142842u, 0xFF4CA4FFu);
        ps3_ui_fill(x + 1, y + 1, 4, h - 2, 0xFFFF8800u);
    } else {
        ps3_ui_panel_grad(x, y, w, h, 0xFF101927u, 0xFF0C121Du, 0xFF1D2C42u);
    }
}

void ps3_ui_badge(int x, int y, int w, int h, const char *txt, uint32_t bg, uint32_t fg)
{
    uint32_t top = lerp_rgb(bg, 0xFFFFFFFFu, 22, 100);
    uint32_t bot = lerp_rgb(bg, 0xFF000000u, 22, 100);
    uint32_t brd = lerp_rgb(bg, 0xFFFFFFFFu, 45, 100);
    ps3_ui_panel_grad(x, y, w, h, top | 0xFF000000u, bot | 0xFF000000u, brd | 0xFF000000u);
    int th = ps3_ui_text_h(0);
    ps3_ui_text_center(x + w / 2, y + (h - th) / 2, txt, fg, 0);
}

void ps3_ui_selector_row(int x, int y, int w, int h, const char *label, const char *val, int sel)
{
    ps3_ui_card_row(x, y, w, h, sel);
    int th1 = ps3_ui_text_h(1);
    ps3_ui_text_shadow(x + 18, y + (h - th1) / 2, label, sel ? 0xFFFFFFFFu : 0xFFC8D6EAu, 1);

    int pw = 230;
    int ph = h - 14;
    if (ph < 26) ph = 26;
    int px = x + w - pw - 16;
    int py = y + (h - ph) / 2;
    ps3_ui_panel_grad(px, py, pw, ph,
                      sel ? 0xFF214068u : 0xFF162234u,
                      sel ? 0xFF152942u : 0xFF101926u,
                      sel ? 0xFFFF9820u : 0xFF2E4362u);
    int thp = ps3_ui_text_h(1);
    int ty = py + (ph - thp) / 2;
    ps3_ui_text(px + 10, ty, "<", sel ? 0xFFFFA834u : 0xFF7590B4u, 1);
    ps3_ui_text(px + pw - 22, ty, ">", sel ? 0xFFFFA834u : 0xFF7590B4u, 1);
    ps3_ui_text_center(px + pw / 2, ty, val, sel ? 0xFFFFFFFFu : 0xFFE2ECF8u, 1);
}

void ps3_ui_switch_row(int x, int y, int w, int h, const char *label, int on, int sel)
{
    ps3_ui_card_row(x, y, w, h, sel);
    int th1 = ps3_ui_text_h(1);
    ps3_ui_text_shadow(x + 18, y + (h - th1) / 2, label, sel ? 0xFFFFFFFFu : 0xFFC8D6EAu, 1);

    int sw = 96;
    int sh = 28;
    int sx = x + w - sw - 16;
    int sy = y + (h - sh) / 2;
    if (on) {
        ps3_ui_panel_grad(sx, sy, sw, sh, 0xFF1C7C46u, 0xFF12522Eu, sel ? 0xFF52E590u : 0xFF2DA862u);
        int th0 = ps3_ui_text_h(0);
        ps3_ui_text(sx + 12, sy + (sh - th0) / 2, "ON", 0xFFFFFFFFu, 0);
        ps3_ui_panel_grad(sx + sw - 28, sy + 3, 24, sh - 6, 0xFFF2FFF8u, 0xFFAEECC6u, 0xFFFFFFFFu);
    } else {
        ps3_ui_panel_grad(sx, sy, sw, sh, 0xFF222D3Fu, 0xFF161D29u, sel ? 0xFFFF9820u : 0xFF394B66u);
        int th0 = ps3_ui_text_h(0);
        ps3_ui_text(sx + sw - 38, sy + (sh - th0) / 2, "OFF", 0xFF8EA2BEu, 0);
        ps3_ui_panel_grad(sx + 4, sy + 3, 24, sh - 6, 0xFF98A9C2u, 0xFF66768Eu, 0xFFB0C0D8u);
    }
}

void ps3_ui_slider_row(int x, int y, int w, int h, const char *label, int val, int vmin, int vmax, const char *unit, int sel)
{
    ps3_ui_card_row(x, y, w, h, sel);
    int th1 = ps3_ui_text_h(1);
    ps3_ui_text_shadow(x + 18, y + (h - th1) / 2, label, sel ? 0xFFFFFFFFu : 0xFFC8D6EAu, 1);

    int rw = 160;
    int rh = 10;
    int rx = x + w - rw - 82;
    int ry = y + (h - rh) / 2;
    ps3_ui_panel_grad(rx, ry, rw, rh, 0xFF0A101Au, 0xFF141E2Eu, 0xFF2F4462u);

    int span = (vmax > vmin) ? (vmax - vmin) : 1;
    int clamped = val;
    if (clamped < vmin) clamped = vmin;
    if (clamped > vmax) clamped = vmax;
    int fw = ((clamped - vmin) * (rw - 2)) / span;
    if (fw > 0) {
        ps3_ui_panel_grad(rx + 1, ry + 1, fw, rh - 2, 0xFFFFB830u, 0xFFE66A00u, 0);
    }
    int kx = rx + fw - 4;
    if (kx < rx) kx = rx;
    if (kx > rx + rw - 8) kx = rx + rw - 8;
    ps3_ui_panel_grad(kx, ry - 4, 8, rh + 8, 0xFFFFFFFFu, 0xFFD0DCEAu, sel ? 0xFFFF8800u : 0xFF607898u);

    char vtxt[32];
    snprintf(vtxt, sizeof(vtxt), "%d%s", val, unit ? unit : "");
    ps3_ui_text_center(x + w - 42, y + (h - th1) / 2, vtxt, sel ? 0xFFFFC060u : 0xFFD8E4F4u, 1);
}

void ps3_ui_btn_icon(int x, int y, int icon_id, int size)
{
    uint32_t *fb = ps3_video_fb();
    if (!fb || icon_id < 0 || icon_id >= BTN_ICON_COUNT || size <= 0) return;
    const uint32_t *src = ps3_btn_icons_data[icon_id];
    for (int dy = 0; dy < size; dy++) {
        int py = y + dy;
        if (py < 0 || py >= PS3_SCREEN_H) continue;
        int sy = (size == PS3_BTN_ICON_SIZE) ? dy : (dy * PS3_BTN_ICON_SIZE) / size;
        const uint32_t *srow = src + sy * PS3_BTN_ICON_SIZE;
        uint32_t *drow = fb + py * PS3_SCREEN_W;
        for (int dx = 0; dx < size; dx++) {
            int pxx = x + dx;
            if (pxx < 0 || pxx >= PS3_SCREEN_W) continue;
            int sx = (size == PS3_BTN_ICON_SIZE) ? dx : (dx * PS3_BTN_ICON_SIZE) / size;
            uint32_t p = srow[sx];
            uint32_t a = (p >> 24) & 0xFFu;
            if (a) drow[pxx] = blend_argb(drow[pxx], p, a);
        }
    }
}

int ps3_ui_btn_label(int x, int y, int icon_id, const char *txt, uint32_t col)
{
    ps3_ui_btn_icon(x, y, icon_id, 24);
    int tx = x + 30;
    int th = ps3_ui_text_h(0);
    int ty = y + (24 - th) / 2;
    ps3_ui_text_shadow(tx, ty, txt, col, 0);
    return 30 + ps3_ui_text_w(txt, 0) + 18;
}

void ps3_ui_tab_bar(const char **tabs, int ntabs, int active, const char *right_info)
{
    ps3_ui_panel_grad(0, 0, PS3_SCREEN_W, 64, 0xFF152034u, 0xFF0D1422u, 0);
    ps3_ui_fill(0, 63, PS3_SCREEN_W, 2, 0xFF284266u);

    ps3_ui_panel_grad(28, 12, 74, 38, 0xFFFF9018u, 0xFFD95800u, 0xFFFFC060u);
    int th2 = ps3_ui_text_h(2);
    ps3_ui_text_center(65, 12 + (38 - th2) / 2, "CPC", 0xFFFFFFFFu, 2);

    ps3_ui_text_shadow(116, 11, "CPC4PS3", 0xFFF4F8FFu, 2);
    ps3_ui_text(116, 39, "AMSTRAD CPC / PLUS", 0xFF8AA8D0u, 0);

    if (right_info && *right_info) {
        int rw = ps3_ui_text_w(right_info, 1) + 28;
        if (rw < 180) rw = 180;
        int rx = PS3_SCREEN_W - rw - 28;
        ps3_ui_panel_grad(rx, 15, rw, 32, 0xFF1A2A42u, 0xFF111C2Eu, 0xFF32507Cu);
        int th1 = ps3_ui_text_h(1);
        ps3_ui_text_clip(rx + 12, 15 + (32 - th1) / 2, rw - 24, right_info, 0xFFE0ECFAu, 1);
    }

    ps3_ui_panel_grad(0, 65, PS3_SCREEN_W, 42, 0xFF0E1624u, 0xFF0B101Bu, 0);
    ps3_ui_fill(0, 106, PS3_SCREEN_W, 1, 0xFF223652u);

    int x = 28;
    ps3_ui_btn_icon(x, 74, BTN_ICON_L1, 24);
    x += 34;

    int avail = PS3_SCREEN_W - 28 - 34 - x;
    int gap = 8;
    int tw = (ntabs > 0) ? ((avail - gap * (ntabs - 1)) / ntabs) : 140;
    if (tw > 188) tw = 188;

    int th1 = ps3_ui_text_h(1);
    for (int i = 0; i < ntabs; i++) {
        int is_act = (i == active);
        if (is_act) {
            ps3_ui_panel_grad(x, 69, tw, 36, 0xFF243E62u, 0xFF162840u, 0xFF4CA4FFu);
            ps3_ui_fill(x + 1, 69, tw - 2, 3, 0xFFFF8800u);
        } else {
            ps3_ui_panel_grad(x, 71, tw, 32, 0xFF121C2Cu, 0xFF0D1420u, 0xFF203048u);
        }
        ps3_ui_text_center(x + tw / 2, 69 + (36 - th1) / 2, tabs[i], is_act ? 0xFFFFFFFFu : 0xFF8CA4C4u, is_act ? 1 : 0);
        x += tw + gap;
    }

    ps3_ui_btn_icon(x + 4, 74, BTN_ICON_R1, 24);
}

void ps3_ui_footer_btns(const int *icons, const char **labels, int n)
{
    ps3_ui_panel_grad(0, PS3_SCREEN_H - 54, PS3_SCREEN_W, 54, 0xFF0E1524u, 0xFF070A12u, 0);
    ps3_ui_fill(0, PS3_SCREEN_H - 54, PS3_SCREEN_W, 1, 0xFF294366u);
    int x = 32;
    int y = PS3_SCREEN_H - 39;
    for (int i = 0; i < n; i++) {
        if (!labels[i]) continue;
        x += ps3_ui_btn_label(x, y, icons[i], labels[i], 0xFFD4E0F0u);
    }
}

void ps3_ui_media_card(int x, int y, int w, int h, int media_type, const char *title, const char *sub, int active)
{
    ps3_ui_panel_grad(x, y, w, h,
                      active ? 0xFF182A44u : 0xFF101926u,
                      active ? 0xFF101C2Eu : 0xFF0B111Bu,
                      active ? 0xFF4292E8u : 0xFF21324Au);

    int ix = x + 14;
    int is = h - 22;
    if (is > 72) is = 72;
    if (is < 44) is = 44;
    int iy = y + (h - is) / 2;

    if (media_type == 0) {
        ps3_ui_panel_grad(ix, iy, is, is, 0xFF232B38u, 0xFF141820u, 0xFF4B5B74u);
        int sw = (is * 44) / 100;
        int sh = (is * 34) / 100;
        int sx = ix + (is - sw) / 2;
        ps3_ui_panel_grad(sx, iy + 2, sw, sh, 0xFFD4DFECu, 0xFF8C9BB0u, 0xFF5A687Cu);
        ps3_ui_fill(sx + 6, iy + 6, sw / 3, sh - 8, 0xFF1A222Eu);
        int lx = ix + 7;
        int lw = is - 14;
        int lh = (is * 42) / 100;
        int ly = iy + is - lh - 5;
        ps3_ui_fill(lx, ly, lw, lh, 0xFFEAF0F8u);
        ps3_ui_fill(lx, ly, lw, 4, 0xFFE62E2Du);
        ps3_ui_fill(lx, ly + 4, lw, 3, 0xFF27AE60u);
        ps3_ui_fill(lx, ly + 7, lw, 3, 0xFF2980B9u);
    } else if (media_type == 1) {
        int cw = is;
        int ch = (is * 70) / 100;
        int cy = iy + (is - ch) / 2;
        ps3_ui_panel_grad(ix, cy, cw, ch, 0xFF2B3544u, 0xFF1A202Au, 0xFF5A6C84u);
        ps3_ui_fill(ix + 6, cy + 5, cw - 12, ch - 18, 0xFFF2ECE0u);
        ps3_ui_fill(ix + 6, cy + 5, cw - 12, 4, 0xFFFF8800u);
        int win_w = cw - 20;
        ps3_ui_fill(ix + 10, cy + ch / 2 - 4, win_w, 12, 0xFF121822u);
        ps3_ui_fill(ix + 15, cy + ch / 2 - 1, 7, 7, 0xFFE0E8F4u);
        ps3_ui_fill(ix + cw - 22, cy + ch / 2 - 1, 7, 7, 0xFFE0E8F4u);
    } else if (media_type == 2) {
        ps3_ui_panel_grad(ix + 4, iy + 2, is - 8, is - 8, 0xFF323C4Au, 0xFF1F2630u, 0xFF6B7C94u);
        for (int g = 0; g < 3; g++) {
            ps3_ui_fill(ix + 8, iy + 5 + g * 4, is - 16, 2, 0xFF141A22u);
        }
        ps3_ui_panel_grad(ix + 10, iy + 18, is - 20, is - 28, 0xFF184A84u, 0xFF103058u, 0xFF5CA8FFu);
        ps3_ui_fill(ix + 10, iy + is - 6, is - 20, 4, 0xFFD4A030u);
    } else {
        ps3_ui_panel_grad(ix + 6, iy + 8, is - 12, is - 16, 0xFF1C2C42u, 0xFF101A28u, 0xFF4CA4FFu);
        for (int p = 0; p < 4; p++) {
            ps3_ui_fill(ix + 11 + p * 8, iy + is - 8, 4, 5, 0xFFE6B840u);
        }
        ps3_ui_fill(ix + 12, iy + 16, is - 24, is - 30, 0xFF2A5894u);
    }

    int tx = ix + is + 14;
    int tw = x + w - tx - 12;
    int th1 = ps3_ui_text_h(1);
    int th0 = ps3_ui_text_h(0);
    ps3_ui_text_clip(tx, y + h / 2 - th1 - 1, tw, title ? title : "", active ? 0xFFFFFFFFu : 0xFFA4B8D4u, 1);
    ps3_ui_text_clip(tx, y + h / 2 + 4, tw, sub ? sub : "", active ? 0xFF8EC8FFu : 0xFF6E829Eu, 0);
    (void)th0;
}

void ps3_ui_thumb_box(int x, int y, int w, int h, const uint32_t *px, int iw, int ih, int has_data, const char *empty_lbl)
{
    ps3_ui_panel_grad(x, y, w, h, 0xFF121A28u, 0xFF0A0F18u, 0xFF324D74u);
    if (has_data && px && iw > 0 && ih > 0) {
        ps3_ui_image(x + 4, y + 4, w - 8, h - 8, px, iw, ih);
        ps3_ui_frame(x + 4, y + 4, w - 8, h - 8, 0xFF1E3250u);
    } else {
        for (int py = y + 6; py < y + h - 6; py += 4) {
            ps3_ui_fill(x + 6, py, w - 12, 1, 0xFF141E2Eu);
        }
        int th = ps3_ui_text_h(0);
        ps3_ui_text_center(x + w / 2, y + (h - th) / 2, empty_lbl ? empty_lbl : "EMPTY SLOT", 0xFF6B809Eu, 0);
    }
}

void ps3_ui_splash(int pct, const char *stage, const char *rom_info, const char *disk_info)
{
    uint32_t *fb = ps3_video_fb();
    if (!fb) return;

    for (int y = 0; y < PS3_SCREEN_H; y++) {
        uint32_t base = (y < 260)
            ? lerp_rgb(0xFF050912u, 0xFF0D182Cu, y, 260)
            : lerp_rgb(0xFF0D182Cu, 0xFF04070Eu, y - 260, PS3_SCREEN_H - 260);
        uint32_t *row = fb + y * PS3_SCREEN_W;
        int scan = (y % 3) == 0;
        int gy = (y & 63) == 0;
        for (int x = 0; x < PS3_SCREEN_W; x++) {
            uint32_t out = base;
            if (gy || ((x & 63) == 0)) out = blend_argb(out, 0xFF203A5Eu, 24);
            if (scan) out = blend_argb(out, 0xFF000000u, 28);
            row[x] = out;
        }
    }
    ps3_ui_panel_grad(0, 0, PS3_SCREEN_W, 180, 0x261E64C8u, 0x00000000u, 0);

    int px = 120;
    int py = 68;
    int pw = 1040;
    int ph = 584;
    ps3_ui_blend(px + 8, py + 10, pw, ph, 0x70000000u);
    ps3_ui_panel_grad(px, py, pw, ph, 0xF20F1A2Eu, 0xF5080D18u, 0xFF2B4870u);

    static const uint32_t cpc_bar[5] = {0xFF2894FFu, 0xFF24E0F8u, 0xFF34D878u, 0xFFFFB024u, 0xFFFF3848u};
    for (int i = 0; i < 5; i++) {
        int bx0 = px + 2 + ((pw - 4) * i) / 5;
        int bx1 = px + 2 + ((pw - 4) * (i + 1)) / 5;
        ps3_ui_fill(bx0, py + 2, bx1 - bx0, 5, cpc_bar[i]);
    }

    int blen = 22;
    ps3_ui_fill(px, py, blen, 2, 0xFF38D0FFu);
    ps3_ui_fill(px, py, 2, blen, 0xFF38D0FFu);
    ps3_ui_fill(px + pw - blen, py, blen, 2, 0xFF38D0FFu);
    ps3_ui_fill(px + pw - 2, py, 2, blen, 0xFF38D0FFu);
    ps3_ui_fill(px, py + ph - 2, blen, 2, 0xFFFF9820u);
    ps3_ui_fill(px, py + ph - blen, 2, blen, 0xFFFF9820u);
    ps3_ui_fill(px + pw - blen, py + ph - 2, blen, 2, 0xFFFF9820u);
    ps3_ui_fill(px + pw - 2, py + ph - blen, 2, blen, 0xFFFF9820u);

    int clamped = pct;
    if (clamped < 0) clamped = 0;
    if (clamped > 100) clamped = 100;

    int ex = 176;
    int ey = 104;
    ps3_ui_panel_grad(ex + 14, ey, 134, 94, 0xFF36445Au, 0xFF1D2534u, 0xFF566B8Eu);
    ps3_ui_fill(ex + 24, ey + 10, 114, 72, 0xFF0A1018u);
    ps3_ui_panel_grad(ex + 28, ey + 14, 106, 64, 0xFF0A2E1Eu, 0xFF051A10u, 0xFF287A50u);
    ps3_ui_fill(ex + 35, ey + 22, 68, 3, 0xFF5EFF9Eu);
    ps3_ui_fill(ex + 35, ey + 29, 86, 2, 0xFF3AD87Eu);
    ps3_ui_fill(ex + 35, ey + 36, 52, 2, 0xFF3AD87Eu);
    ps3_ui_fill(ex + 35, ey + 45, 38, 3, 0xFF90FFB8u);
    ps3_ui_fill(ex + 35, ey + 53, 8, 9, ((clamped / 10) & 1) ? 0xFFA4FFD0u : 0xFF287A4Eu);
    ps3_ui_fill(ex + 136, ey + 85, 6, 4, (clamped >= 80) ? 0xFF42F588u : 0xFFFF9820u);

    ps3_ui_panel_grad(ex + 60, ey + 94, 42, 6, 0xFF1A2230u, 0xFF0E131Cu, 0);
    ps3_ui_panel_grad(ex, ey + 100, 162, 34, 0xFF263042u, 0xFF151B26u, 0xFF485C7Eu);
    for (int k = 0; k < 10; k++) {
        uint32_t kcol = (k == 7) ? 0xFF28A4FFu : ((k == 8) ? 0xFF34D878u : ((k == 9) ? 0xFFFF4848u : 0xFF4A5A74u));
        ps3_ui_fill(ex + 10 + k * 9, ey + 110, 7, 13, kcol);
    }
    ps3_ui_fill(ex + 112, ey + 110, 40, 8, 0xFF090D14u);
    ps3_ui_fill(ex + 115, ey + 113, 34, 2, 0xFF324662u);
    ps3_ui_fill(ex + 114, ey + 122, 8, 3, (clamped >= 80) ? 0xFFFFB024u : 0xFF5A3814u);

    int tx = 368;
    ps3_ui_text_shadow(tx + 1, 102, "CPC4PS3", 0xFF060A12u, 3);
    ps3_ui_text_shadow(tx, 100, "CPC4PS3", 0xFFFFFFFFu, 3);
    int tw = ps3_ui_text_w("CPC4PS3", 3);
    ps3_ui_fill(tx, 142, tw, 3, 0xFFFF9820u);

    char vtxt[24];
    snprintf(vtxt, sizeof(vtxt), "v%s", PS3_APP_VERSION);
    ps3_ui_badge(tx + tw + 18, 106, 70, 28, vtxt, 0xFF8A4810u, 0xFFFFF4DCu);
    ps3_ui_badge(tx + tw + 98, 106, 174, 28, "AMSTRAD CPC / PLUS", 0xFF1A4672u, 0xFFDCEEFFu);

    ps3_ui_text(tx, 158, "AMSTRAD CPC 464 / 664 / 6128 / 6128+ EMULATOR FOR PLAYSTATION 3", 0xFF9AC4F8u, 1);

    ps3_ui_badge(tx,       198, 118, 26, "Z80A 4MHz",      0xFF18365Eu, 0xFFD8ECFFu);
    ps3_ui_badge(tx + 128, 198, 130, 26, "CRTC + ASIC",    0xFF184A4Eu, 0xFFD8FFF0u);
    ps3_ui_badge(tx + 268, 198, 148, 26, "AY-3-8912 PSG",  0xFF3E2A5Au, 0xFFF0E4FFu);
    ps3_ui_badge(tx + 426, 198, 146, 26, "64K - 576K RAM", 0xFF563418u, 0xFFFFECD8u);

    ps3_ui_hline(176, 262, 928, 0xFF243C60u);
    ps3_ui_fill(480, 262, 320, 1, 0xFF38D0FFu);

    const char *r_txt = rom_info ? rom_info : "464 / 664 / 6128 / AMSDOS / 6128+ System ROM Pack";
    const char *d_txt = disk_info ? disk_info : "/dev_hdd0/CPC/roms  •  /dev_hdd0/CPC/saves  •  /states  •  USB";
    const char *c_txt = "SELECT: Menu  •  L2: Virtual Keyboard  •  L1/R1: Save/Load  •  Universal Presets";

    ps3_ui_panel_grad(176, 282, 928, 46, 0xFF101C30u, 0xFF0B1322u, 0xFF243C60u);
    ps3_ui_badge(190, 292, 138, 26, "SYSTEM ROMS", 0xFF1C4E7Eu, 0xFFE8F4FFu);
    ps3_ui_text_clip(342, 297, 626, r_txt, 0xFFDCE8F8u, 0);
    ps3_ui_badge(980, 292, 110, 26,
                 clamped >= 80 ? "LOADED" : (clamped >= 40 ? "STAGING" : "READY"),
                 clamped >= 80 ? 0xFF1D6242u : 0xFF704218u,
                 clamped >= 80 ? 0xFFE4FFF0u : 0xFFFFE8C8u);

    ps3_ui_panel_grad(176, 338, 928, 46, 0xFF101C30u, 0xFF0B1322u, 0xFF243C60u);
    ps3_ui_badge(190, 348, 138, 26, "STORAGE", 0xFF1D5C48u, 0xFFE8FFF2u);
    ps3_ui_text_clip(342, 353, 626, d_txt, 0xFFC8EEDCu, 0);
    ps3_ui_badge(980, 348, 110, 26,
                 clamped >= 100 ? "INDEXED" : (clamped >= 92 ? "SCANNING" : "WAITING"),
                 clamped >= 100 ? 0xFF1D6242u : 0xFF284264u,
                 0xFFE4F4FFu);

    ps3_ui_panel_grad(176, 394, 928, 46, 0xFF101C30u, 0xFF0B1322u, 0xFF243C60u);
    ps3_ui_badge(190, 404, 138, 26, "CONTROLS", 0xFF6C3E18u, 0xFFFFF0DCu);
    ps3_ui_text_clip(342, 409, 626, c_txt, 0xFFF4DAB8u, 0);
    ps3_ui_badge(980, 404, 110, 26, "UNIVERSAL", 0xFF1D5C48u, 0xFFE4FFF0u);

    ps3_ui_text(178, 468, stage ? stage : "INITIALIZING...", 0xFFEAF4FFu, 1);
    char pct_txt[16];
    snprintf(pct_txt, sizeof(pct_txt), "%d%%", clamped);
    int pctw = ps3_ui_text_w(pct_txt, 1);
    ps3_ui_text(1102 - pctw, 468, pct_txt, 0xFFFFC048u, 1);

    int bx = 176;
    int by = 498;
    int bw = 928;
    int bh = 28;
    ps3_ui_panel_grad(bx, by, bw, bh, 0xFF070C15u, 0xFF10192Au, 0xFF345684u);
    int inner_w = bw - 6;
    int fw = (clamped * inner_w) / 100;
    if (fw > 0) {
        int fy0 = by + 3;
        int fy1 = by + bh - 3;
        for (int pyy = fy0; pyy < fy1; pyy++) {
            uint32_t *row = fb + pyy * PS3_SCREEN_W;
            int gloss = (pyy < fy0 + 3);
            for (int dx = 0; dx < fw; dx++) {
                uint32_t gcol = (dx < inner_w / 2)
                    ? lerp_rgb(0xFF24A8FFu, 0xFF34E088u, dx, inner_w / 2)
                    : lerp_rgb(0xFF34E088u, 0xFFFFA824u, dx - inner_w / 2, inner_w - inner_w / 2);
                if (gloss) gcol = blend_argb(gcol, 0xFFFFFFFFu, 70);
                row[bx + 3 + dx] = gcol;
            }
        }
    }
    for (int q = 1; q <= 3; q++) {
        ps3_ui_blend(bx + 3 + (inner_w * q) / 4, by + 3, 2, bh - 6, 0x48000000u);
    }

    static const uint32_t pal27[27] = {
        0xFF000000u, 0xFF000080u, 0xFF0000FFu, 0xFF800000u, 0xFF800080u, 0xFFFF0000u,
        0xFF7F00FFu, 0xFFFF00FFu, 0xFF008000u, 0xFF008080u, 0xFF0080FFu, 0xFF808000u,
        0xFF808080u, 0xFF8080FFu, 0xFFFF8000u, 0xFFFF8080u, 0xFF00FF00u, 0xFF00FF80u,
        0xFF00FFFFu, 0xFF80FF00u, 0xFF80FF80u, 0xFFFFFF00u, 0xFF80FFFFu, 0xFFFFFFFFu,
        0xFFFFFF80u, 0xFFFFCC66u, 0xFFFF6600u
    };
    for (int ci = 0; ci < 27; ci++) {
        int sx0 = 176 + (928 * ci) / 27;
        int sx1 = 176 + (928 * (ci + 1)) / 27;
        ps3_ui_fill(sx0, 558, sx1 - sx0, 6, pal27[ci]);
    }
    ps3_ui_fill(176, 558, 928, 1, 0x30FFFFFFu);

    ps3_ui_text_center(640, 584, "CAPRICE32 EMULATION ENGINE  •  PLAYSTATION 3 PORT BY THHEROGAC", 0xFF84A6CEu, 0);
}

void ps3_ui_header(const char *title, const char *sub)
{
    ps3_ui_panel_grad(0, 0, PS3_SCREEN_W, 84, 0xFF152034u, 0xFF0D1422u, 0);
    ps3_ui_fill(0, 84, PS3_SCREEN_W, 2, 0xFF284266u);
    ps3_ui_panel_grad(28, 18, 72, 44, 0xFFFF9018u, 0xFFD95800u, 0xFFFFC060u);
    ps3_ui_text_center(64, 18 + (44 - ps3_ui_text_h(2)) / 2, "CPC", 0xFFFFFFFFu, 2);
    ps3_ui_text_shadow(114, 16, title ? title : PS3_APP_NAME, 0xFFFFFFFFu, 2);
    ps3_ui_text(114, 48, sub ? sub : "", 0xFF8AA8D0u, 0);
}

void ps3_ui_footer(const char *hints)
{
    ps3_ui_panel_grad(0, PS3_SCREEN_H - 54, PS3_SCREEN_W, 54, 0xFF0E1524u, 0xFF070A12u, 0);
    ps3_ui_fill(0, PS3_SCREEN_H - 54, PS3_SCREEN_W, 1, 0xFF294366u);
    ps3_ui_text(32, PS3_SCREEN_H - 36, hints ? hints : "", 0xFFB8CCE4u, 0);
}
