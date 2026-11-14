#ifndef PS3_PLATFORM_H
#define PS3_PLATFORM_H

#include <stdint.h>
#include <stddef.h>

#define PS3_APP_NAME "CPC4PS3"
#define PS3_APP_VERSION "1.0"
#define PS3_TITLE_ID "CPC4PS300"

#define PS3_ROOT "/dev_hdd0/CPC"
#define PS3_TMP_DIR PS3_ROOT "/tmp"
#define PS3_ROMS_DIR PS3_ROOT "/roms"
#define PS3_DISKS_DIR PS3_ROMS_DIR "/disks"
#define PS3_TAPES_DIR PS3_ROMS_DIR "/tapes"
#define PS3_CARTS_DIR PS3_ROMS_DIR "/carts"
#define PS3_SNAPS_DIR PS3_ROMS_DIR "/snapshots"
#define PS3_STATES_DIR PS3_ROOT "/states"
#define PS3_SAVES_DIR PS3_ROOT "/saves"
#define PS3_SHOTS_DIR PS3_ROOT "/screenshots"
#define PS3_CONFIG_PATH PS3_ROOT "/cpc4ps3.ini"
#define PS3_LOG_PATH PS3_ROOT "/cpc4ps3.log"
#define PS3_PKG_USRDIR "/dev_hdd0/game/" PS3_TITLE_ID "/USRDIR"

#define PS3_SCREEN_W 1280
#define PS3_SCREEN_H 720
#define PS3_AUDIO_RATE 48000
#define PS3_AUDIO_BLOCK 256

#define PS3_ACT_UP (1u << 0)
#define PS3_ACT_DOWN (1u << 1)
#define PS3_ACT_LEFT (1u << 2)
#define PS3_ACT_RIGHT (1u << 3)
#define PS3_ACT_CONFIRM (1u << 4)
#define PS3_ACT_CANCEL (1u << 5)
#define PS3_ACT_SQUARE (1u << 6)
#define PS3_ACT_TRIANGLE (1u << 7)
#define PS3_ACT_L1 (1u << 8)
#define PS3_ACT_R1 (1u << 9)
#define PS3_ACT_L2 (1u << 10)
#define PS3_ACT_R2 (1u << 11)
#define PS3_ACT_START (1u << 12)
#define PS3_ACT_SELECT (1u << 13)
#define PS3_ACT_MENU (1u << 14)
#define PS3_ACT_L3 (1u << 15)
#define PS3_ACT_R3 (1u << 16)

enum {
    PS3_ASPECT_4_3 = 0,
    PS3_ASPECT_PIXEL,
    PS3_ASPECT_16_9,
    PS3_ASPECT_COUNT
};

typedef struct {
    int model;
    int ram_size;
    int aspect;
    int scanlines;
    int volume;
    int show_fps;
    int joystick_port;
    int green_screen;
    int auto_run;
    int auto_save_dsk;
    int vkbd_style;
    int vkbd_alpha;
    int vkbd_pos;
    int padmap[10];
    char last_dir[512];
} ps3_config_t;

extern ps3_config_t g_cfg;
extern volatile int g_app_quit;

void ps3_log_init(void);
void ps3_log(const char *fmt, ...);
void ps3_boot_mark(const char *msg);
void ps3_paths_init(void);
void ps3_sysutil_register(void);
void ps3_sysutil_check(void);
void ps3_sysutil_unregister(void);

void ps3_config_defaults(void);
void ps3_config_load(void);
void ps3_config_save(void);

int ps3_video_init(void);
void ps3_video_shutdown(void);
void ps3_video_present(void);
void ps3_video_draw_game(void);
void ps3_video_set_manual_flip(int on);
uint32_t *ps3_video_fb(void);
void ps3_video_set_ui_target(uint32_t *p);
void ps3_video_toast(const char *msg, int frames);
void ps3_video_set_title(const char *title);
const char *ps3_video_title(void);
void ps3_screenshot(void);

void ps3_audio_pause(int paused);

struct SDL_AudioSpec;
unsigned int ps3_cell_open_audio(const char *device, int iscapture, const struct SDL_AudioSpec *desired, struct SDL_AudioSpec *obtained, int allowed_changes);
void ps3_cell_close_audio(unsigned int id);
void ps3_cell_pause_audio_device(unsigned int id, int pause_on);
void ps3_cell_pause_audio(int pause_on);

void ps3_input_init(void);
void ps3_input_shutdown(void);
uint32_t ps3_input_read(uint32_t *pressed);
void ps3_input_apply_game(uint32_t cur);
void ps3_input_release_all(void);

void ps3_ui_init(void);
void ps3_ui_shutdown(void);
void ps3_ui_fill(int x, int y, int w, int h, uint32_t argb);
void ps3_ui_blend(int x, int y, int w, int h, uint32_t argb);
void ps3_ui_frame(int x, int y, int w, int h, uint32_t argb);
void ps3_ui_text(int x, int y, const char *s, uint32_t argb, int scale);
void ps3_ui_text_shadow(int x, int y, const char *s, uint32_t argb, int scale);
void ps3_ui_text_center(int cx, int y, const char *s, uint32_t argb, int scale);
void ps3_ui_text_clip(int x, int y, int maxw, const char *s, uint32_t argb, int scale);
int ps3_ui_text_w(const char *s, int scale);
int ps3_ui_text_h(int scale);
void ps3_ui_panel(int x, int y, int w, int h, uint32_t argb);
void ps3_ui_panel_grad(int x, int y, int w, int h, uint32_t top_argb, uint32_t bot_argb, uint32_t border_argb);
void ps3_ui_bevel(int x, int y, int w, int h, uint32_t hi, uint32_t lo);
void ps3_ui_hline(int x, int y, int w, uint32_t argb);
void ps3_ui_bg(void);
void ps3_ui_card_row(int x, int y, int w, int h, int sel);
void ps3_ui_badge(int x, int y, int w, int h, const char *txt, uint32_t bg, uint32_t fg);
void ps3_ui_selector_row(int x, int y, int w, int h, const char *label, const char *val, int sel);
void ps3_ui_switch_row(int x, int y, int w, int h, const char *label, int on, int sel);
void ps3_ui_slider_row(int x, int y, int w, int h, const char *label, int val, int vmin, int vmax, const char *unit, int sel);
void ps3_ui_btn_icon(int x, int y, int icon_id, int size);
int ps3_ui_btn_label(int x, int y, int icon_id, const char *txt, uint32_t col);
void ps3_ui_tab_bar(const char **tabs, int ntabs, int active, const char *right_info);
void ps3_ui_footer_btns(const int *icons, const char **labels, int n);
void ps3_ui_media_card(int x, int y, int w, int h, int media_type, const char *title, const char *sub, int active);
void ps3_ui_thumb_box(int x, int y, int w, int h, const uint32_t *px, int iw, int ih, int has_data, const char *empty_lbl);
void ps3_ui_splash(int pct, const char *stage, const char *rom_info, const char *disk_info);
void ps3_ui_image(int x, int y, int w, int h, const uint32_t *px, int iw, int ih);
void ps3_ui_header(const char *title, const char *sub);
void ps3_ui_footer(const char *hints);

#endif
