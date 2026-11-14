#include "ps3_platform.h"

#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cap32.h"
#include "video.h"

SDL_Window *mainSDLWindow = nullptr;
SDL_Renderer *renderer = nullptr;
SDL_Texture *texture = nullptr;
SDL_Surface *vid = nullptr;
SDL_Surface *scaled = nullptr;
SDL_Surface *pub = nullptr;
extern SDL_Surface *back_surface;
extern video_plugin *vid_plugin;

static uint32_t *g_fb = nullptr;
static uint32_t g_lut[32];
static int g_xmap[PS3_SCREEN_W];
static int g_xend[1024];
static int g_xmap_dw = 0;
static int g_xmap_sw = 0;
static char g_toast[160];
static int g_toast_frames = 0;
static char g_title[160];
static int g_sdl_ready = 0;
static int g_out_w = PS3_SCREEN_W;
static int g_out_h = PS3_SCREEN_H;

static uint32_t g_blit_ms = 0;
static uint32_t g_present_ms = 0;

void ps3_video_stats(uint32_t *blit_ms, uint32_t *present_ms)
{
    if (blit_ms) *blit_ms = g_blit_ms;
    if (present_ms) *present_ms = g_present_ms;
    g_blit_ms = 0;
    g_present_ms = 0;
}

static void blit_game_impl(void)
{
    if (!g_fb) return;
    if (!vid || !vid->pixels || !vid->format) {
        memset(g_fb, 0, (size_t)PS3_SCREEN_W * PS3_SCREEN_H * 4);
        return;
    }
    if (vid->w <= 0 || vid->h <= 0) return;

    const int bpp = vid->format->BytesPerPixel;
    const SDL_Color *pal = vid->format->palette ? vid->format->palette->colors : nullptr;
    if (pal) {
        for (int i = 0; i < 32; i++) {
            g_lut[i] = 0xFF000000u | ((uint32_t)pal[i].r << 16) | ((uint32_t)pal[i].g << 8) | pal[i].b;
        }
    }

    static int blit_logs = 0;
    if (blit_logs < 3) {
        blit_logs++;
        const uint8_t *crow = (const uint8_t *)vid->pixels + (size_t)(vid->h / 2) * vid->pitch;
        if (bpp == 4) {
            const uint32_t *crow32 = (const uint32_t *)crow;
            ps3_log("blit %dx%d bpp=4 px=%06X,%06X,%06X\n", vid->w, vid->h,
                    (unsigned)(crow32[0] & 0xFFFFFFu), (unsigned)(crow32[vid->w / 2] & 0xFFFFFFu),
                    (unsigned)(crow32[vid->w - 1] & 0xFFFFFFu));
        } else {
            ps3_log("blit %dx%d bpp=%d pal=%02X%02X%02X,%02X%02X%02X px=%02X,%02X,%02X\n",
                    vid->w, vid->h, bpp,
                    pal ? pal[0].r : 0, pal ? pal[0].g : 0, pal ? pal[0].b : 0,
                    pal ? pal[1].r : 0, pal ? pal[1].g : 0, pal ? pal[1].b : 0,
                    (unsigned)crow[0], (unsigned)crow[vid->w / 2],
                    (unsigned)crow[vid->w - 1]);
        }
    }

    int dw, dh, dx, dy;
    if (g_cfg.aspect == PS3_ASPECT_PIXEL) {
        dw = 768; dh = 540; dx = (PS3_SCREEN_W - dw) / 2; dy = (PS3_SCREEN_H - dh) / 2;
    } else if (g_cfg.aspect == PS3_ASPECT_16_9) {
        dw = PS3_SCREEN_W; dh = PS3_SCREEN_H; dx = 0; dy = 0;
    } else {
        dw = 960; dh = 720; dx = (PS3_SCREEN_W - dw) / 2; dy = 0;
    }

    if (dw != g_xmap_dw || vid->w != g_xmap_sw) {
        int lim = (int)(sizeof(g_xend) / sizeof(g_xend[0]));
        int sw = vid->w < lim ? vid->w : lim;
        for (int i = 0; i < dw; i++) g_xmap[i] = (i * vid->w) / dw;
        int s = 0;
        for (int x = 0; x < dw && s < sw; x++) {
            int m = (x * vid->w) / dw;
            while (s < m) { g_xend[s] = x; s++; }
        }
        while (s < sw) { g_xend[s] = dw; s++; }
        g_xmap_dw = dw;
        g_xmap_sw = vid->w;
    }

    if (dw < PS3_SCREEN_W || dh < PS3_SCREEN_H) {
        if (dx > 0) {
            for (int y = 0; y < dh; y++) {
                uint32_t *row = g_fb + (size_t)(dy + y) * PS3_SCREEN_W;
                memset(row, 0, (size_t)dx * 4);
                memset(row + dx + dw, 0, (size_t)(PS3_SCREEN_W - dx - dw) * 4);
            }
        }
        if (dy > 0) {
            memset(g_fb, 0, (size_t)dy * PS3_SCREEN_W * 4);
            memset(g_fb + (size_t)(dy + dh) * PS3_SCREEN_W, 0,
                   (size_t)(PS3_SCREEN_H - dy - dh) * PS3_SCREEN_W * 4);
        }
    }

    const uint8_t *src = (const uint8_t *)vid->pixels;
    const int pitch = vid->pitch;
    const int sh = vid->h;
    uint32_t *const fb = g_fb;

    for (int y = 0; y < dh; y++) {
        int sy = (y * sh) / dh;
        const uint8_t *row = src + (size_t)sy * pitch;
        uint32_t *dst = fb + (size_t)(dy + y) * PS3_SCREEN_W + dx;
        if (bpp == 4) {
            const uint32_t *row32 = (const uint32_t *)row;
            int lim = (int)(sizeof(g_xend) / sizeof(g_xend[0]));
            int sw = vid->w < lim ? vid->w : lim;
            int x = 0;
            for (int s = 0; s < sw && x < dw; s++) {
                uint32_t c = 0xFF000000u | (row32[s] & 0x00FFFFFFu);
                int xe = g_xend[s];
                if (xe > dw) xe = dw;
                while (x < xe) dst[x++] = c;
            }
            if (x < dw) {
                uint32_t c = 0xFF000000u | (row32[sw - 1] & 0x00FFFFFFu);
                while (x < dw) dst[x++] = c;
            }
        } else {
            for (int x = 0; x < dw; x++) {
                dst[x] = g_lut[row[g_xmap[x]] & 31];
            }
        }
        if (g_cfg.scanlines && (y & 1)) {
            for (int x = 0; x < dw; x++) {
                uint32_t c = dst[x];
                dst[x] = 0xFF000000u | (((c >> 1) & 0x7F7F7Fu));
            }
        }
    }
}

static void plugin_blit_game(void)
{
    uint32_t t0 = SDL_GetTicks();
    blit_game_impl();
    g_blit_ms += SDL_GetTicks() - t0;
}

static void draw_toast(void)
{
    if (g_toast_frames <= 0 || !g_toast[0]) return;
    int tw = ps3_ui_text_w(g_toast, 2);
    int th = ps3_ui_text_h(2);
    int bh = th + 24;
    if (bh < 48) bh = 48;
    int bw = tw + 56;
    if (bw > PS3_SCREEN_W - 60) bw = PS3_SCREEN_W - 60;
    int bx = (PS3_SCREEN_W - bw) / 2;
    int by = PS3_SCREEN_H - 134;
    ps3_ui_panel_grad(bx, by, bw, bh, 0xEC1B2638u, 0xEC0F1622u, 0xFF3E8EE0u);
    ps3_ui_fill(bx + 2, by + 2, 4, bh - 4, 0xFFFF8800u);
    ps3_ui_text_center(PS3_SCREEN_W / 2, by + (bh - th) / 2, g_toast, 0xFFF3F6FAu, 2);
}

static SDL_Surface *plugin_init(video_plugin *t, int scale, bool fs)
{
    (void)scale;
    (void)fs;

    if (!vid) {
        vid = SDL_CreateRGBSurface(0, CPC_VISIBLE_SCR_WIDTH, CPC_VISIBLE_SCR_HEIGHT, 32,
                                   0x00FF0000u, 0x0000FF00u, 0x000000FFu, 0xFF000000u);
        if (!vid) {
            vid = SDL_CreateRGBSurface(0, CPC_VISIBLE_SCR_WIDTH, CPC_VISIBLE_SCR_HEIGHT, 8, 0, 0, 0, 0);
        }
        if (!vid) return nullptr;
        SDL_FillRect(vid, nullptr, 0);
    }

    t->half_pixels = 1;
    t->x_offset = 0;
    t->y_offset = 0;
    t->x_scale = 1.0f;
    t->y_scale = 1.0f;
    t->width = CPC_VISIBLE_SCR_WIDTH;
    t->height = CPC_VISIBLE_SCR_HEIGHT;

    SDL_ShowCursor(SDL_DISABLE);
    return vid;
}

static void plugin_setpal(SDL_Color *c)
{
    if (vid && vid->format && vid->format->palette) {
        SDL_SetPaletteColors(vid->format->palette, c, 0, 32);
    }
}

static int g_manual_flip = 0;

void ps3_video_set_manual_flip(int on)
{
    g_manual_flip = on;
}

static void plugin_flip(video_plugin *t)
{
    (void)t;
    if (g_manual_flip) return;
    plugin_blit_game();
    ps3_video_present();
}

static void plugin_close(void)
{
    if (vid) { SDL_FreeSurface(vid); vid = nullptr; }
    if (scaled) { SDL_FreeSurface(scaled); scaled = nullptr; }
    if (pub) { SDL_FreeSurface(pub); pub = nullptr; }
    back_surface = nullptr;
}

std::vector<video_plugin> video_plugin_list;

int ps3_video_init(void)
{
    if (video_plugin_list.empty()) {
        video_plugin plugin;
        memset(&plugin, 0, sizeof(plugin));
        plugin.name = "PS3";
        plugin.hidden = false;
        plugin.init = plugin_init;
        plugin.set_palette = plugin_setpal;
        plugin.flip = plugin_flip;
        plugin.close = plugin_close;
        plugin.half_pixels = 1;
        plugin.x_offset = 0;
        plugin.y_offset = 0;
        plugin.x_scale = 1.0f;
        plugin.y_scale = 1.0f;
        plugin.width = CPC_VISIBLE_SCR_WIDTH;
        plugin.height = CPC_VISIBLE_SCR_HEIGHT;
        video_plugin_list.push_back(plugin);
    }

    ps3_boot_mark("video_sdl_init");
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        ps3_log("SDL_Init video failed: %s\n", SDL_GetError());
        ps3_boot_mark("video_sdl_init_fail");
        return -1;
    }
    g_sdl_ready = 1;
    ps3_boot_mark("video_sdl_init_ok");

    if (!g_fb) {
        g_fb = (uint32_t *)calloc((size_t)PS3_SCREEN_W * PS3_SCREEN_H, sizeof(uint32_t));
        if (!g_fb) return -1;
    }
    for (size_t i = 0; i < (size_t)PS3_SCREEN_W * PS3_SCREEN_H; i++) {
        g_fb[i] = 0xFF080C18u;
    }

    SDL_DisplayMode dm;
    if (SDL_GetDesktopDisplayMode(0, &dm) == 0 && dm.w >= 640 && dm.h >= 480) {
        g_out_w = dm.w;
        g_out_h = dm.h;
    } else {
        g_out_w = PS3_SCREEN_W;
        g_out_h = PS3_SCREEN_H;
    }

    const Uint32 wflags[] = { 0, SDL_WINDOW_SHOWN };
    for (size_t i = 0; i < sizeof(wflags) / sizeof(wflags[0]) && !mainSDLWindow; i++) {
        mainSDLWindow = SDL_CreateWindow(PS3_APP_NAME, 0, 0, g_out_w, g_out_h, wflags[i]);
    }
    if (!mainSDLWindow) {
        g_out_w = PS3_SCREEN_W;
        g_out_h = PS3_SCREEN_H;
        mainSDLWindow = SDL_CreateWindow(PS3_APP_NAME, 0, 0, g_out_w, g_out_h, 0);
    }
    if (!mainSDLWindow) {
        ps3_log("SDL_CreateWindow failed: %s\n", SDL_GetError());
        ps3_boot_mark("video_window_fail");
        return -1;
    }
    ps3_boot_mark("video_window_ok");

    int actual_w = 0, actual_h = 0;
    SDL_GetWindowSize(mainSDLWindow, &actual_w, &actual_h);
    if (actual_w >= 640 && actual_h >= 480) {
        g_out_w = actual_w;
        g_out_h = actual_h;
    }

    const Uint32 rflags[] = {
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE,
        SDL_RENDERER_ACCELERATED,
        SDL_RENDERER_SOFTWARE,
        0
    };
    for (size_t i = 0; i < sizeof(rflags) / sizeof(rflags[0]) && !renderer; i++) {
        renderer = SDL_CreateRenderer(mainSDLWindow, -1, rflags[i]);
    }
    if (!renderer) {
        ps3_log("SDL_CreateRenderer failed: %s\n", SDL_GetError());
        ps3_boot_mark("video_renderer_fail");
        return -1;
    }
    ps3_boot_mark("video_renderer_ok");

    SDL_SetRenderDrawColor(renderer, 8, 12, 24, 255);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
    ps3_boot_mark("video_clear_present_ok");

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, PS3_SCREEN_W, PS3_SCREEN_H);
    if (!texture) {
        ps3_log("SDL_CreateTexture failed: %s\n", SDL_GetError());
        ps3_boot_mark("video_texture_fail");
        return -1;
    }
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_NONE);
    ps3_boot_mark("video_texture_ok");

    ps3_ui_init();
    ps3_sysutil_register();

    return 0;
}

void ps3_video_shutdown(void)
{
    ps3_ui_shutdown();
    if (texture) { SDL_DestroyTexture(texture); texture = nullptr; }
    if (renderer) { SDL_DestroyRenderer(renderer); renderer = nullptr; }
    if (mainSDLWindow) { SDL_DestroyWindow(mainSDLWindow); mainSDLWindow = nullptr; }
    if (g_fb) { free(g_fb); g_fb = nullptr; }
    if (g_sdl_ready) {
        SDL_Quit();
        g_sdl_ready = 0;
    }
}

static uint32_t *g_ui_target = nullptr;

void ps3_video_set_ui_target(uint32_t *p)
{
    g_ui_target = p;
}

uint32_t *ps3_video_fb(void)
{
    return g_ui_target ? g_ui_target : g_fb;
}

void ps3_video_draw_game(void)
{
    plugin_blit_game();
}

void ps3_video_present(void)
{
    if (!renderer || !texture || !g_fb) return;
    if (g_toast_frames > 0) {
        draw_toast();
        g_toast_frames--;
    }
    SDL_Rect dst;
    dst.x = 0;
    dst.y = 0;
    dst.w = g_out_w;
    dst.h = g_out_h;
    uint32_t t0 = SDL_GetTicks();
    SDL_UpdateTexture(texture, nullptr, g_fb, PS3_SCREEN_W * 4);
    SDL_RenderCopy(renderer, texture, nullptr, &dst);
    SDL_RenderPresent(renderer);
    g_present_ms += SDL_GetTicks() - t0;
    ps3_sysutil_check();
}

void ps3_video_toast(const char *msg, int frames)
{
    if (!msg) return;
    snprintf(g_toast, sizeof(g_toast), "%s", msg);
    g_toast_frames = frames > 0 ? frames : 90;
}

void ps3_video_set_title(const char *title)
{
    if (title) snprintf(g_title, sizeof(g_title), "%s", title);
}

const char *ps3_video_title(void)
{
    return g_title;
}

static void put_le16(FILE *f, unsigned v)
{
    unsigned char b[2] = {(unsigned char)(v & 0xFF), (unsigned char)((v >> 8) & 0xFF)};
    fwrite(b, 1, 2, f);
}

static void put_le32(FILE *f, unsigned v)
{
    unsigned char b[4] = {(unsigned char)(v & 0xFF), (unsigned char)((v >> 8) & 0xFF),
                          (unsigned char)((v >> 16) & 0xFF), (unsigned char)((v >> 24) & 0xFF)};
    fwrite(b, 1, 4, f);
}

void ps3_screenshot(void)
{
    if (!g_fb) return;
    char path[256];
    snprintf(path, sizeof(path), PS3_SHOTS_DIR "/cpc_%u.bmp", (unsigned)SDL_GetTicks());
    FILE *f = fopen(path, "wb");
    if (!f) {
        ps3_video_toast("Screenshot failed", 120);
        return;
    }
    const int w = PS3_SCREEN_W;
    const int h = PS3_SCREEN_H;
    const int row = (w * 3 + 3) & ~3;
    const unsigned datasz = (unsigned)(row * h);
    unsigned char zero[3] = {0, 0, 0};

    fputc('B', f); fputc('M', f);
    put_le32(f, 54 + datasz);
    put_le16(f, 0); put_le16(f, 0);
    put_le32(f, 54);
    put_le32(f, 40);
    put_le32(f, w);
    put_le32(f, h);
    put_le16(f, 1);
    put_le16(f, 24);
    put_le32(f, 0);
    put_le32(f, datasz);
    put_le32(f, 2835);
    put_le32(f, 2835);
    put_le32(f, 0);
    put_le32(f, 0);

    for (int y = h - 1; y >= 0; y--) {
        const uint32_t *srcrow = g_fb + (size_t)y * w;
        int pad = row - w * 3;
        for (int x = 0; x < w; x++) {
            uint32_t c = srcrow[x];
            unsigned char bgr[3] = {(unsigned char)(c & 0xFF), (unsigned char)((c >> 8) & 0xFF), (unsigned char)((c >> 16) & 0xFF)};
            fwrite(bgr, 1, 3, f);
        }
        if (pad > 0) fwrite(zero, 1, (size_t)pad, f);
    }
    fclose(f);
    ps3_video_toast("Screenshot saved", 120);
}
