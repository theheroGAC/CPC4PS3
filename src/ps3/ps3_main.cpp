#include "ps3_platform.h"
#include "ps3_btn_icons.h"

#include <SDL.h>
#include <sys/process.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <algorithm>
#include <string>
#include <vector>

#include "cap32.h"
#include "crtc.h"
#include "disk.h"
#include "tape.h"
#include "z80.h"
#include "asic.h"
#include "cartridge.h"
#include "slotshandler.h"
#include "keyboard.h"
#include "video.h"
#include "zip.h"

SYS_PROCESS_PARAM(1001, 0x200000);

extern t_CPC CPC;
extern char chAppPath[];
extern byte keyboard_matrix[16];
extern dword dwSndBufferCopied;
extern dword dwFrameCountOverall;
extern SDL_Surface *back_surface;
extern t_drive driveA;
extern t_drive driveB;
extern t_VDU VDU;
extern t_GateArray GateArray;
extern t_z80regs z80;
void emulator_shutdown();
void video_display();
void update_timings();
void ps3_video_stats(uint32_t *blit_ms, uint32_t *present_ms);

enum {
    TAB_LIBRARY = 0,
    TAB_BROWSER,
    TAB_STATES,
    TAB_SETTINGS,
    TAB_CPC_DISK,
    TAB_HELP_EXIT,
    TAB_COUNT
};

static const char *g_tab_names[TAB_COUNT] = {
    "LIBRARY",
    "BROWSER",
    "SAVE STATES",
    "SETTINGS",
    "CPC & DISK",
    "HELP / EXIT"
};

struct Entry {
    std::string name;
    bool is_dir;
    off_t size;
};

struct LibEntry {
    std::string full;
    std::string name;
    std::string folder;
    std::string kind;
    off_t size;
};

struct DiskFileEntry {
    std::string full_name;
    std::string stem;
    std::string ext;
    int user;
    bool system_attr;
    int first_block;
};

struct DiskCatalogInfo {
    bool valid;
    bool has_cpm_boot;
    bool is_system_fmt;
    std::vector<DiskFileEntry> files;
};

struct KbdQueuedItem {
    bool is_wait;
    CPC_KEYS key;
    bool with_shift;
    bool with_ctrl;
    int frames;
    int gap;
};

enum {
    VK_ACT_NONE = 0,
    VK_ACT_SHIFT,
    VK_ACT_CTRL,
    VK_ACT_CAPS,
    VK_ACT_RUN,
    VK_ACT_CAT,
    VK_ACT_CPM
};

struct VKeyCell {
    const char *label;
    const char *shift_label;
    int normal_key;
    int shift_key;
    int action;
};

#define VK_ROWS 6
#define VK_COLS 12

static const VKeyCell g_vkbd_grid[VK_ROWS][VK_COLS] = {
    {
        {"ESC",  "ESC",  CPC_ESC,     CPC_ESC,     VK_ACT_NONE},
        {"RUN\"", "RUN\"", 0,         0,           VK_ACT_RUN},
        {"CAT",  "CAT",  0,           0,           VK_ACT_CAT},
        {"|CPM", "|CPM", 0,           0,           VK_ACT_CPM},
        {"F1",   "F1",   CPC_F1,      CPC_F1,      VK_ACT_NONE},
        {"F2",   "F2",   CPC_F2,      CPC_F2,      VK_ACT_NONE},
        {"F3",   "F3",   CPC_F3,      CPC_F3,      VK_ACT_NONE},
        {"F4",   "F4",   CPC_F4,      CPC_F4,      VK_ACT_NONE},
        {"F5",   "F5",   CPC_F5,      CPC_F5,      VK_ACT_NONE},
        {"F6",   "F6",   CPC_F6,      CPC_F6,      VK_ACT_NONE},
        {"CLR",  "CLR",  CPC_CLR,     CPC_CLR,     VK_ACT_NONE},
        {"DEL",  "DEL",  CPC_DEL,     CPC_DEL,     VK_ACT_NONE}
    },
    {
        {"1",    "!",    CPC_1,       CPC_EXCLAMATN, VK_ACT_NONE},
        {"2",    "\"",   CPC_2,       CPC_DBLQUOTE,  VK_ACT_NONE},
        {"3",    "#",    CPC_3,       CPC_HASH,      VK_ACT_NONE},
        {"4",    "$",    CPC_4,       CPC_DOLLAR,    VK_ACT_NONE},
        {"5",    "%",    CPC_5,       CPC_PERCENT,   VK_ACT_NONE},
        {"6",    "&",    CPC_6,       CPC_AMPERSAND, VK_ACT_NONE},
        {"7",    "'",    CPC_7,       CPC_QUOTE,     VK_ACT_NONE},
        {"8",    "(",    CPC_8,       CPC_LEFTPAREN, VK_ACT_NONE},
        {"9",    ")",    CPC_9,       CPC_RIGHTPAREN,VK_ACT_NONE},
        {"0",    "_",    CPC_0,       CPC_UNDERSCORE,VK_ACT_NONE},
        {"-",    "=",    CPC_MINUS,   CPC_EQUAL,     VK_ACT_NONE},
        {"^",    "#",    CPC_POWER,   CPC_POUND,     VK_ACT_NONE}
    },
    {
        {"q",    "Q",    CPC_q,       CPC_Q,       VK_ACT_NONE},
        {"w",    "W",    CPC_w,       CPC_W,       VK_ACT_NONE},
        {"e",    "E",    CPC_e,       CPC_E,       VK_ACT_NONE},
        {"r",    "R",    CPC_r,       CPC_R,       VK_ACT_NONE},
        {"t",    "T",    CPC_t,       CPC_T,       VK_ACT_NONE},
        {"y",    "Y",    CPC_y,       CPC_Y,       VK_ACT_NONE},
        {"u",    "U",    CPC_u,       CPC_U,       VK_ACT_NONE},
        {"i",    "I",    CPC_i,       CPC_I,       VK_ACT_NONE},
        {"o",    "O",    CPC_o,       CPC_O,       VK_ACT_NONE},
        {"p",    "P",    CPC_p,       CPC_P,       VK_ACT_NONE},
        {"@",    "|",    CPC_AT,      CPC_PIPE,    VK_ACT_NONE},
        {"[",    "{",    CPC_LBRACKET,CPC_LCBRACE, VK_ACT_NONE}
    },
    {
        {"CAPS", "CAPS", CPC_CAPSLOCK,CPC_CAPSLOCK,VK_ACT_CAPS},
        {"a",    "A",    CPC_a,       CPC_A,       VK_ACT_NONE},
        {"s",    "S",    CPC_s,       CPC_S,       VK_ACT_NONE},
        {"d",    "D",    CPC_d,       CPC_D,       VK_ACT_NONE},
        {"f",    "F",    CPC_f,       CPC_F,       VK_ACT_NONE},
        {"g",    "G",    CPC_g,       CPC_G,       VK_ACT_NONE},
        {"h",    "H",    CPC_h,       CPC_H,       VK_ACT_NONE},
        {"j",    "J",    CPC_j,       CPC_J,       VK_ACT_NONE},
        {"k",    "K",    CPC_k,       CPC_K,       VK_ACT_NONE},
        {"l",    "L",    CPC_l,       CPC_L,       VK_ACT_NONE},
        {":",    "*",    CPC_COLON,   CPC_ASTERISK,VK_ACT_NONE},
        {"RET",  "RET",  CPC_RETURN,  CPC_RETURN,  VK_ACT_NONE}
    },
    {
        {"SHFT", "SHFT", CPC_LSHIFT,  CPC_LSHIFT,  VK_ACT_SHIFT},
        {"z",    "Z",    CPC_z,       CPC_Z,       VK_ACT_NONE},
        {"x",    "X",    CPC_x,       CPC_X,       VK_ACT_NONE},
        {"c",    "C",    CPC_c,       CPC_C,       VK_ACT_NONE},
        {"v",    "V",    CPC_v,       CPC_V,       VK_ACT_NONE},
        {"b",    "B",    CPC_b,       CPC_B,       VK_ACT_NONE},
        {"n",    "N",    CPC_n,       CPC_N,       VK_ACT_NONE},
        {"m",    "M",    CPC_m,       CPC_M,       VK_ACT_NONE},
        {",",    "<",    CPC_COMMA,   CPC_LESS,    VK_ACT_NONE},
        {".",    ">",    CPC_PERIOD,  CPC_GREATER, VK_ACT_NONE},
        {"/",    "?",    CPC_SLASH,   CPC_QUESTION,VK_ACT_NONE},
        {"UP",   "UP",   CPC_CUR_UP,  CPC_CUR_UP,  VK_ACT_NONE}
    },
    {
        {"CTRL", "CTRL", CPC_CONTROL, CPC_CONTROL, VK_ACT_CTRL},
        {"COPY", "COPY", CPC_COPY,    CPC_COPY,    VK_ACT_NONE},
        {"F7",   "F7",   CPC_F7,      CPC_F7,      VK_ACT_NONE},
        {"F8",   "F8",   CPC_F8,      CPC_F8,      VK_ACT_NONE},
        {"F9",   "F9",   CPC_F9,      CPC_F9,      VK_ACT_NONE},
        {"F0",   "F0",   CPC_F0,      CPC_F0,      VK_ACT_NONE},
        {"SPACE","SPACE",CPC_SPACE,   CPC_SPACE,   VK_ACT_NONE},
        {";",    "+",    CPC_SEMICOLON,CPC_PLUS,   VK_ACT_NONE},
        {"ENT",  "ENT",  CPC_ENTER,   CPC_ENTER,   VK_ACT_NONE},
        {"LEFT", "LEFT", CPC_CUR_LEFT,CPC_CUR_LEFT,VK_ACT_NONE},
        {"DOWN", "DOWN", CPC_CUR_DOWN,CPC_CUR_DOWN,VK_ACT_NONE},
        {"RGHT", "RGHT", CPC_CUR_RIGHT,CPC_CUR_RIGHT,VK_ACT_NONE}
    }
};

static const int g_ram_choices[6] = {64, 128, 192, 256, 512, 576};
static const char *g_ram_names[6] = {"64K", "128K", "192K", "256K", "512K", "576K"};
static const char *g_model_names[4] = {"CPC 464", "CPC 664", "CPC 6128", "CPC 6128+"};
static const char *g_aspect_names[3] = {"4:3 CRT", "Pixel 1:1", "16:9 Full"};
static const char *g_target_names[5] = {"DRIVE A", "DRIVE B", "TAPE", "CARTRIDGE", "SNAPSHOT"};
static const char *g_vk_style_names[4] = {"Classic Blue", "Dark Graphite", "Cool Slate", "Amstrad Amber"};
static const char *g_vk_alpha_names[4] = {"92% Solid", "80% Clear", "64% Glass", "48% Ghost"};
static const char *g_vk_pos_names[3] = {"Bottom", "Center", "Top"};

static int g_menu_active = 0;
static int g_tab = TAB_LIBRARY;
static int g_browse_sel = 0;
static int g_browse_scroll = 0;
static int g_settings_sel = 0;
static int g_settings_scroll = 0;
static int g_state_sel = 0;
static int g_cpc_sel = 0;
static int g_cpc_cat_sel = 0;
static int g_cpc_cat_scroll = 0;
static int g_cpc_focus_cat = 0;
static int g_help_sel = 0;
static int g_browse_target = 0;

static std::string g_dir;
static std::vector<Entry> g_entries;

static std::vector<LibEntry> g_lib;
static std::vector<std::string> g_usb;
static std::vector<std::string> g_lib_roots;
static int g_lib_sel = 0;
static int g_lib_scroll = 0;
static int g_lib_root = 0;

static DiskCatalogInfo g_disk_cat;
static std::string g_dskA_save_path;
static std::string g_dskB_save_path;

static std::vector<KbdQueuedItem> g_kbd_q;
static int g_loader_seen = 0;
extern unsigned g_fdc_reads;
static CPCScancode g_kbd_held_sc = 0;
static bool g_kbd_held_shift = false;
static bool g_kbd_held_ctrl = false;
static int g_kbd_timer = 0;
static int g_kbd_gap = 0;

static int g_vk_x = 0;
static int g_vk_y = 0;
static int g_vk_shift = 0;
static int g_vk_ctrl = 0;
static int g_vk_caps = 0;
static int g_vk_pulse = 0;
static int g_vkbd_pick = 0;
static int g_vkbd_picked = -1;
static char g_vkbd_pick_title[64];
static int g_remap_open = 0;
static int g_remap_sel = 0;
static int g_remap_scroll = 0;
static int g_about_open = 0;
#define REMAP_SLOTS 10
#define REMAP_ROWS 12

static int s_exit_condition = 0;
static int emulation_step(void);
static void setup_cpc(void);

static bool file_exists(const char *p)
{
    if (!p || !*p) return false;
    struct stat st;
    return stat(p, &st) == 0 && S_ISREG(st.st_mode);
}

static bool dir_exists(const char *p)
{
    if (!p || !*p) return false;
    struct stat st;
    return stat(p, &st) == 0 && S_ISDIR(st.st_mode);
}

static bool rom_probe(const char *dir)
{
    if (!dir_exists(dir)) return false;
    char probe[512];
    snprintf(probe, sizeof(probe), "%s/cpc6128.rom", dir);
    return file_exists(probe);
}

static bool res_probe(const char *dir)
{
    if (!dir_exists(dir)) return false;
    char probe[512];
    snprintf(probe, sizeof(probe), "%s/keymap_us.map", dir);
    return file_exists(probe);
}

static void make_dir(const char *p)
{
    mkdir(p, 0777);
}

static bool has_ext(const char *n, const char *e)
{
    if (!n || !e) return false;
    size_t ln = strlen(n);
    size_t le = strlen(e);
    return ln >= le && strcasecmp(n + ln - le, e) == 0;
}

static int media_kind_code(const char *name)
{
    if (!name) return 0;
    if (has_ext(name, ".cdt") || has_ext(name, ".voc")) return 1;
    if (has_ext(name, ".cpr")) return 2;
    if (has_ext(name, ".sna")) return 3;
    return 0;
}

static bool ext_media(const char *n)
{
    return has_ext(n, ".dsk") || has_ext(n, ".ipf") || has_ext(n, ".raw") ||
           has_ext(n, ".cdt") || has_ext(n, ".voc") || has_ext(n, ".cpr") ||
           has_ext(n, ".sna") || has_ext(n, ".zip");
}

static void toast(const char *msg)
{
    ps3_video_toast(msg, 120);
}

static std::string base_name(const std::string &p)
{
    size_t s = p.find_last_of("/\\");
    return (s == std::string::npos) ? p : p.substr(s + 1);
}

static std::string strip_ext(const std::string &p)
{
    size_t d = p.find_last_of('.');
    return (d == std::string::npos) ? p : p.substr(0, d);
}

static std::string sanitize_stem(const std::string &s)
{
    std::string out = strip_ext(base_name(s));
    for (size_t i = 0; i < out.size(); i++) {
        unsigned char c = (unsigned char)out[i];
        if (!isalnum(c) && c != '_' && c != '-') out[i] = '_';
    }
    if (out.empty()) out = "disk";
    return out;
}

static std::string game_key(void)
{
    std::string f = CPC.driveA.file;
    if (f.empty()) f = CPC.driveB.file;
    if (f.empty()) f = CPC.tape.file;
    if (f.empty()) f = CPC.cartridge.file;
    if (f.empty()) f = CPC.snapshot.file;
    if (f.empty()) return "default";
    return sanitize_stem(f);
}

static std::string make_dsk_save_path(const std::string &src_path, char drv_letter)
{
    char out[640];
    snprintf(out, sizeof(out), PS3_SAVES_DIR "/%s_%c.dsk", sanitize_stem(src_path).c_str(), drv_letter);
    return std::string(out);
}

static void flush_altered_disks(void)
{
    if (!g_cfg.auto_save_dsk) return;
    if (!g_dskA_save_path.empty() && driveA.altered && driveA.tracks > 0 && !driveA.eject_hook) {
        if (dsk_save(g_dskA_save_path, &driveA) == 0) {
            driveA.altered = false;
            ps3_log("Auto-saved Drive A -> %s\n", g_dskA_save_path.c_str());
        }
    }
    if (!g_dskB_save_path.empty() && driveB.altered && driveB.tracks > 0 && !driveB.eject_hook) {
        if (dsk_save(g_dskB_save_path, &driveB) == 0) {
            driveB.altered = false;
            ps3_log("Auto-saved Drive B -> %s\n", g_dskB_save_path.c_str());
        }
    }
}

static void kbd_queue_clear(void)
{
    g_kbd_q.clear();
    if (g_kbd_held_sc) {
        applyKeypress(g_kbd_held_sc, keyboard_matrix, false, true);
        g_kbd_held_sc = 0;
    }
    if (g_kbd_held_shift) {
        applyKeypress(CPC.InputMapper->CPCscancodeFromCPCkey(CPC_LSHIFT), keyboard_matrix, false, false);
        g_kbd_held_shift = false;
    }
    if (g_kbd_held_ctrl) {
        applyKeypress(CPC.InputMapper->CPCscancodeFromCPCkey(CPC_CONTROL), keyboard_matrix, false, false);
        g_kbd_held_ctrl = false;
    }
    g_kbd_timer = 0;
    g_kbd_gap = 0;
}

static void kbd_push_wait(int frames)
{
    if (frames <= 0) return;
    KbdQueuedItem it;
    it.is_wait = true;
    it.key = CPC_SPACE;
    it.with_shift = false;
    it.with_ctrl = false;
    it.frames = frames;
    it.gap = 0;
    g_kbd_q.push_back(it);
}

static void kbd_push_key(CPC_KEYS key, int hold_frames = 3, int gap_frames = 2, bool with_shift = false, bool with_ctrl = false)
{
    KbdQueuedItem it;
    it.is_wait = false;
    it.key = key;
    it.with_shift = with_shift;
    it.with_ctrl = with_ctrl;
    it.frames = hold_frames;
    it.gap = gap_frames;
    g_kbd_q.push_back(it);
}

static bool map_ascii_to_cpc(char c, CPC_KEYS *out)
{
    static const CPC_KEYS k_lower[26] = {
        CPC_a, CPC_b, CPC_c, CPC_d, CPC_e, CPC_f, CPC_g, CPC_h, CPC_i,
        CPC_j, CPC_k, CPC_l, CPC_m, CPC_n, CPC_o, CPC_p, CPC_q, CPC_r,
        CPC_s, CPC_t, CPC_u, CPC_v, CPC_w, CPC_x, CPC_y, CPC_z
    };
    static const CPC_KEYS k_upper[26] = {
        CPC_A, CPC_B, CPC_C, CPC_D, CPC_E, CPC_F, CPC_G, CPC_H, CPC_I,
        CPC_J, CPC_K, CPC_L, CPC_M, CPC_N, CPC_O, CPC_P, CPC_Q, CPC_R,
        CPC_S, CPC_T, CPC_U, CPC_V, CPC_W, CPC_X, CPC_Y, CPC_Z
    };
    static const CPC_KEYS k_digit[10] = {
        CPC_0, CPC_1, CPC_2, CPC_3, CPC_4, CPC_5, CPC_6, CPC_7, CPC_8, CPC_9
    };
    if (c >= 'a' && c <= 'z') { *out = k_lower[c - 'a']; return true; }
    if (c >= 'A' && c <= 'Z') { *out = k_upper[c - 'A']; return true; }
    if (c >= '0' && c <= '9') { *out = k_digit[c - '0']; return true; }
    switch (c) {
        case ' ': *out = CPC_SPACE; return true;
        case '\r':
        case '\n': *out = CPC_RETURN; return true;
        case '"': *out = CPC_DBLQUOTE; return true;
        case '|': *out = CPC_PIPE; return true;
        case ':': *out = CPC_COLON; return true;
        case ';': *out = CPC_SEMICOLON; return true;
        case '.': *out = CPC_PERIOD; return true;
        case ',': *out = CPC_COMMA; return true;
        case '-': *out = CPC_MINUS; return true;
        case '+': *out = CPC_PLUS; return true;
        case '*': *out = CPC_ASTERISK; return true;
        case '/': *out = CPC_SLASH; return true;
        case '?': *out = CPC_QUESTION; return true;
        case '!': *out = CPC_EXCLAMATN; return true;
        case '(': *out = CPC_LEFTPAREN; return true;
        case ')': *out = CPC_RIGHTPAREN; return true;
        case '=': *out = CPC_EQUAL; return true;
        case '@': *out = CPC_AT; return true;
        case '_': *out = CPC_UNDERSCORE; return true;
        case '#': *out = CPC_HASH; return true;
        case '$': *out = CPC_DOLLAR; return true;
        case '%': *out = CPC_PERCENT; return true;
        case '&': *out = CPC_AMPERSAND; return true;
        case '\'': *out = CPC_QUOTE; return true;
        case '[': *out = CPC_LBRACKET; return true;
        case ']': *out = CPC_RBRACKET; return true;
        case '{': *out = CPC_LCBRACE; return true;
        case '}': *out = CPC_RCBRACE; return true;
        case '<': *out = CPC_LESS; return true;
        case '>': *out = CPC_GREATER; return true;
        case '^': *out = CPC_POWER; return true;
        default: return false;
    }
}

static void kbd_push_string(const std::string &s)
{
    for (size_t i = 0; i < s.size(); i++) {
        CPC_KEYS k;
        if (map_ascii_to_cpc(s[i], &k)) {
            kbd_push_key(k, 3, 3, false, false);
        }
    }
}

static void kbd_queue_tick(void)
{
    if (g_kbd_timer > 0) {
        g_kbd_timer--;
        if (g_kbd_timer > 0) return;
    }

    if (g_kbd_held_sc || g_kbd_held_shift || g_kbd_held_ctrl) {
        if (g_kbd_held_sc) {
            applyKeypress(g_kbd_held_sc, keyboard_matrix, false, true);
            g_kbd_held_sc = 0;
        }
        if (g_kbd_held_shift) {
            applyKeypress(CPC.InputMapper->CPCscancodeFromCPCkey(CPC_LSHIFT), keyboard_matrix, false, false);
            g_kbd_held_shift = false;
        }
        if (g_kbd_held_ctrl) {
            applyKeypress(CPC.InputMapper->CPCscancodeFromCPCkey(CPC_CONTROL), keyboard_matrix, false, false);
            g_kbd_held_ctrl = false;
        }
        if (g_kbd_gap > 0) {
            g_kbd_timer = g_kbd_gap;
            g_kbd_gap = 0;
            return;
        }
    }

    while (!g_kbd_q.empty()) {
        KbdQueuedItem it = g_kbd_q.front();
        g_kbd_q.erase(g_kbd_q.begin());
        if (it.is_wait) {
            g_kbd_timer = it.frames;
            return;
        }
        CPCScancode sc = CPC.InputMapper->CPCscancodeFromCPCkey(it.key);
        if ((sc & 0xff) == 0xff) continue;
        if (it.with_shift) {
            applyKeypress(CPC.InputMapper->CPCscancodeFromCPCkey(CPC_LSHIFT), keyboard_matrix, true, false);
            g_kbd_held_shift = true;
        }
        if (it.with_ctrl) {
            applyKeypress(CPC.InputMapper->CPCscancodeFromCPCkey(CPC_CONTROL), keyboard_matrix, true, false);
            g_kbd_held_ctrl = true;
        }
        applyKeypress(sc, keyboard_matrix, true, false);
        g_kbd_held_sc = sc;
        g_kbd_timer = it.frames > 0 ? it.frames : 3;
        g_kbd_gap = it.gap > 0 ? it.gap : 2;
        return;
    }
}

static const byte *find_sector(t_drive *drv, int trk, int side, byte sector_id, dword *out_sz)
{
    if (!drv || trk < 0 || trk >= (int)drv->tracks || side < 0 || side > (int)drv->sides) return nullptr;
    t_track *t = &drv->track[trk][side];
    for (dword i = 0; i < t->sectors; i++) {
        if (t->sector[i].CHRN[2] == sector_id) {
            if (out_sz) *out_sz = t->sector[i].getTotalSize();
            return t->sector[i].getDataForRead();
        }
    }
    return nullptr;
}

static bool is_boot_sector_code(const byte *sec, dword sz)
{
    if (!sec || sz < 128) return false;
    byte first = sec[0];
    if (first == 0xE5 || first == 0x00 || first == 0xF6) return false;
    bool all_same = true;
    for (dword i = 1; i < 64 && i < sz; i++) {
        if (sec[i] != first) {
            all_same = false;
            break;
        }
    }
    return !all_same;
}

static void parse_dir_sector(const byte *sec, dword sz, DiskCatalogInfo *cat)
{
    if (!sec || sz < 32) return;
    for (dword off = 0; off + 32 <= sz; off += 32) {
        const byte *e = sec + off;
        if (e[0] >= 16) continue;
        std::string stem;
        bool bad = false;
        for (int i = 1; i <= 8; i++) {
            byte ch = e[i] & 0x7Fu;
            if (ch < 0x20 || ch > 0x7Eu) { bad = true; break; }
            if (ch != ' ') stem.push_back((char)toupper(ch));
        }
        if (bad || stem.empty()) continue;
        std::string ext;
        for (int i = 9; i <= 11; i++) {
            byte ch = e[i] & 0x7Fu;
            if (ch < 0x20 || ch > 0x7Eu) { bad = true; break; }
            if (ch != ' ') ext.push_back((char)toupper(ch));
        }
        if (bad) continue;
        bool sys_flag = (e[10] & 0x80u) != 0;
        std::string full = ext.empty() ? stem : (stem + "." + ext);

        bool dup = false;
        for (size_t k = 0; k < cat->files.size(); k++) {
            if (cat->files[k].full_name == full) { dup = true; break; }
        }
        if (dup) continue;

        DiskFileEntry dfe;
        dfe.full_name = full;
        dfe.stem = stem;
        dfe.ext = ext;
        dfe.user = (int)e[0];
        dfe.system_attr = sys_flag;
        dfe.first_block = (int)e[16];
        cat->files.push_back(dfe);
    }
}

static void scan_disk_catalog(t_drive *drv)
{
    g_disk_cat.valid = false;
    g_disk_cat.has_cpm_boot = false;
    g_disk_cat.is_system_fmt = false;
    g_disk_cat.files.clear();
    g_cpc_cat_sel = 0;
    g_cpc_cat_scroll = 0;

    if (!drv || drv->tracks == 0) return;

    dword sz = 0;
    if (find_sector(drv, 0, 0, 0xC1, &sz) != nullptr) {
        g_disk_cat.valid = true;
        for (byte sid = 0xC1; sid <= 0xC4; sid++) {
            const byte *s = find_sector(drv, 0, 0, sid, &sz);
            if (s) parse_dir_sector(s, sz, &g_disk_cat);
        }
        return;
    }

    const byte *boot41 = find_sector(drv, 0, 0, 0x41, &sz);
    if (boot41 != nullptr) {
        g_disk_cat.valid = true;
        g_disk_cat.is_system_fmt = true;
        g_disk_cat.has_cpm_boot = is_boot_sector_code(boot41, sz);
        for (byte sid = 0x41; sid <= 0x44; sid++) {
            const byte *s = find_sector(drv, 2, 0, sid, &sz);
            if (s) parse_dir_sector(s, sz, &g_disk_cat);
        }
        return;
    }

    const byte *boot01 = find_sector(drv, 0, 0, 0x01, &sz);
    if (boot01 != nullptr) {
        g_disk_cat.valid = true;
        g_disk_cat.is_system_fmt = true;
        g_disk_cat.has_cpm_boot = is_boot_sector_code(boot01, sz);
        for (byte sid = 0x01; sid <= 0x04; sid++) {
            const byte *s = find_sector(drv, 1, 0, sid, &sz);
            if (s) parse_dir_sector(s, sz, &g_disk_cat);
        }
    }
}

static bool has_ci(const std::string &a, const char *b)
{
    return strcasecmp(a.c_str(), b) == 0;
}

static bool read_disk_block(t_drive *drv, int blk, byte *out)
{
    if (!drv || blk <= 0 || blk > 720) return false;
    int trk = blk / 4;
    if (trk >= (int)drv->tracks) return false;
    byte sid = (byte)(0xC1 + (blk % 4) * 2);
    dword s1 = 0, s2 = 0;
    const byte *a = find_sector(drv, trk, 0, sid, &s1);
    const byte *b = find_sector(drv, trk, 0, (byte)(sid + 1), &s2);
    if (!a || !b || s1 < 512 || s2 < 512) return false;
    memcpy(out, a, 512);
    memcpy(out + 512, b, 512);
    return true;
}

static int disk_bin_low_refs(const byte *blk, int lim)
{
    const byte *body = blk + 128;
    int hits = 0;
    for (int i = 0; i + 2 < lim; i++) {
        byte op = body[i];
        if (op == 0x21 || op == 0x11 || op == 0xCD || op == 0xC3) {
            int v = body[i + 1] | (body[i + 2] << 8);
            if (v >= 0x400 && v < 0x800) hits++;
        }
    }
    return hits;
}

static void queue_disk_autorun(void)
{
    if (!g_cfg.auto_run || CPC.driveA.file.empty()) return;
    kbd_queue_clear();
    g_loader_seen = 0;
    kbd_push_wait(82);

    const DiskFileEntry *best = nullptr;
    int best_score = -1000;
    bool has_com = false;

    for (size_t i = 0; i < g_disk_cat.files.size(); i++) {
        const DiskFileEntry &f = g_disk_cat.files[i];
        if (has_ci(f.ext, "COM")) has_com = true;
        if (has_ci(f.ext, "SCR") || has_ci(f.ext, "DAT") || has_ci(f.ext, "MAP") ||
            has_ci(f.ext, "TXT") || has_ci(f.ext, "DOC") || has_ci(f.ext, "BAK") ||
            has_ci(f.ext, "GFX") || has_ci(f.ext, "CHR")) {
            continue;
        }
        int sc = 0;
        if (has_ci(f.ext, "BAS")) sc += 60;
        else if (f.ext.empty()) sc += 45;
        else if (has_ci(f.ext, "BIN")) sc += 30;
        else continue;

        if (!f.system_attr) sc += 15;
        if (has_ci(f.stem, "GO") || has_ci(f.stem, "BOOT")) sc += 130;
        if (has_ci(f.stem, "DISC") || has_ci(f.stem, "DISK")) sc += 120;
        else if (has_ci(f.stem, "MENU") || has_ci(f.stem, "RUN") || has_ci(f.stem, "LOAD") || has_ci(f.stem, "LOADER") || has_ci(f.stem, "START") || has_ci(f.stem, "GAME")) sc += 100;

        if (sc > best_score) {
            best_score = sc;
            best = &f;
        }
    }

    if (best) {
        byte blk1k[1024];
        int low_hits = 0;
        int load_addr = 0;
        if (has_ci(best->ext, "BIN") && read_disk_block(&driveA, best->first_block, blk1k)) {
            load_addr = blk1k[0x15] | (blk1k[0x16] << 8);
            if (load_addr == 0x8000) low_hits = disk_bin_low_refs(blk1k, 128);
        }
        if (low_hits >= 2) {
            std::string c1 = "LOAD\"" + best->full_name + "\",&400\n";
            kbd_push_string(c1);
            kbd_push_wait(50);
            kbd_push_string("CALL &400\n");
            ps3_log("Auto-run low origin: %sCALL &400 (load=%04X hits=%d)\n", c1.c_str(), load_addr, low_hits);
            toast(("Auto-run: LOAD\"" + best->full_name + "\",400").c_str());
            return;
        }
        std::string cmd = "RUN\"" + best->full_name + "\n";
        kbd_push_string(cmd);
        ps3_log("Auto-run queued: %s", cmd.c_str());
        toast(("Auto-run: RUN\"" + best->full_name).c_str());
    } else if (g_disk_cat.has_cpm_boot || has_com || g_disk_cat.is_system_fmt) {
        kbd_push_string("|CPM\n");
        ps3_log("Auto-run queued: |CPM\n");
        toast("Auto-run: |CPM");
    } else {
        kbd_push_string("RUN\"DISC\n");
        ps3_log("Auto-run fallback queued: RUN\"DISC\n");
        toast("Auto-run: RUN\"DISC");
    }
}

static void queue_tape_autorun(void)
{
    if (!g_cfg.auto_run || CPC.tape.file.empty()) return;
    kbd_queue_clear();
    kbd_push_wait(90);
    kbd_push_string("|TAPE\n");
    kbd_push_wait(20);
    kbd_push_string("RUN\"\n");
    toast("Auto-run: |TAPE -> RUN\"");
}

static void state_path(int slot, char *out, size_t n, const char *kind)
{
    snprintf(out, n, PS3_STATES_DIR "/%s.%s%d", game_key().c_str(), kind, slot);
}

static void thumb_save(int slot)
{
    ps3_video_draw_game();
    uint32_t *fb = ps3_video_fb();
    if (!fb) return;
    char tp[512];
    state_path(slot, tp, sizeof(tp), "th");
    FILE *f = fopen(tp, "wb");
    if (!f) return;
    uint32_t hdr[2] = {160u, 90u};
    fwrite(hdr, sizeof(uint32_t), 2, f);
    uint32_t line[160];
    for (int y = 0; y < 90; y++) {
        int sy = y * 8;
        for (int x = 0; x < 160; x++) line[x] = fb[(size_t)sy * PS3_SCREEN_W + x * 8];
        fwrite(line, sizeof(uint32_t), 160, f);
    }
    fclose(f);
}

static bool thumb_load(int slot, uint32_t *out)
{
    char tp[512];
    state_path(slot, tp, sizeof(tp), "th");
    FILE *f = fopen(tp, "rb");
    if (!f) return false;
    uint32_t hdr[2] = {0, 0};
    bool ok = fread(hdr, sizeof(uint32_t), 2, f) == 2 && hdr[0] == 160u && hdr[1] == 90u;
    if (ok) ok = fread(out, sizeof(uint32_t), 160 * 90, f) == (160u * 90u);
    fclose(f);
    return ok;
}

static void save_state(int slot)
{
    char p[512];
    state_path(slot, p, sizeof(p), "st");
    if (snapshot_save(p) == 0) {
        thumb_save(slot);
        char msg[64];
        snprintf(msg, sizeof(msg), "Saved state -> Slot %d", slot + 1);
        toast(msg);
    } else {
        toast("Save state failed");
    }
}

static void load_state(int slot)
{
    char p[512];
    state_path(slot, p, sizeof(p), "st");
    if (!file_exists(p)) {
        char msg[64];
        snprintf(msg, sizeof(msg), "Slot %d is empty", slot + 1);
        toast(msg);
        return;
    }
    if (snapshot_load(p) == 0) {
        kbd_queue_clear();
        char msg[64];
        snprintf(msg, sizeof(msg), "Loaded state <- Slot %d", slot + 1);
        toast(msg);
    } else {
        toast("Load state failed");
    }
}

static void delete_state(int slot)
{
    char p[512];
    state_path(slot, p, sizeof(p), "st");
    remove(p);
    state_path(slot, p, sizeof(p), "th");
    remove(p);
    char msg[64];
    snprintf(msg, sizeof(msg), "Deleted Slot %d", slot + 1);
    toast(msg);
}

static void restart_emulator(void)
{
    std::string saved_a = CPC.driveA.file;
    std::string saved_b = CPC.driveB.file;
    std::string saved_t = CPC.tape.file;
    flush_altered_disks();
    kbd_queue_clear();
    emulator_shutdown();
    delete CPC.InputMapper;
    CPC.InputMapper = nullptr;
    if (g_cfg.model < 3) {
        CPC.cartridge.file.clear();
    }
    setup_cpc();
    if (emulator_init() == 0) {
        if (!saved_b.empty()) {
            CPC.driveB.file = saved_b;
            CPC.driveB.drive = DRIVE::DSK_B;
            file_load(CPC.driveB);
        }
        if (!saved_a.empty()) {
            CPC.driveA.file = saved_a;
            CPC.driveA.drive = DRIVE::DSK_A;
            if (file_load(CPC.driveA) == 0) scan_disk_catalog(&driveA);
        }
        if (!saved_t.empty()) {
            CPC.tape.file = saved_t;
            CPC.tape.drive = DRIVE::TAPE;
            file_load(CPC.tape);
        }
        video_set_palette();
        toast("CPC restarted");
    } else {
        toast("Restart failed");
    }
}

static void setup_cpc(void)
{
    CPC.model = (unsigned)g_cfg.model;
    CPC.jumpers = 0x1e;
    CPC.ram_size = (unsigned)g_cfg.ram_size;
    if (CPC.model < 2) CPC.ram_size = 64;
    else if (CPC.ram_size < 128) CPC.ram_size = 128;
    CPC.speed = DEF_SPEED_SETTING;
    CPC.limit_speed = 1;
    CPC.auto_pause = 0;
    CPC.boot_time = 42;
    CPC.printer = 0;
    CPC.mf2 = 0;
    CPC.net4cpc = 0;
    CPC.keyboard = 0;
    CPC.joystick_emulation = JoystickEmulation::Keyboard;
    CPC.joysticks = 0;
    CPC.joystick_menu_button = 9;
    CPC.joystick_vkeyboard_button = 10;
    CPC.scr_scale = 1;
    CPC.scr_preserve_aspect_ratio = 1;
    CPC.scr_style = 0;
    CPC.scr_oglfilter = 0;
    CPC.scr_oglscanlines = 0;
    CPC.scr_led = 0;
    CPC.scr_fps = 0;
    CPC.scr_tube = g_cfg.green_screen ? 1u : 0u;
    CPC.scr_intensity = 10;
    CPC.scr_remanency = 0;
    CPC.scr_window = 0;
    CPC.scr_green_mode = 0;
    CPC.scr_green_blue_percent = 0;
    CPC.snd_enabled = 1;
    CPC.snd_ready = false;
    CPC.snd_playback_rate = 3;
    CPC.snd_bits = 1;
    CPC.snd_stereo = 1;
    CPC.snd_volume = (unsigned)g_cfg.volume;
    CPC.snd_pp_device = 0;
    CPC.kbd_layout = "keymap_us.map";
    CPC.max_tracksize = 6144 - 154;
    CPC.dsk_path = PS3_DISKS_DIR;
    CPC.tape_path = PS3_TAPES_DIR;
    CPC.snap_path = PS3_SNAPS_DIR;
    CPC.cart_path = PS3_CARTS_DIR;
    CPC.sdump_dir = PS3_SHOTS_DIR;
    CPC.printer_file = PS3_ROOT "/printer.dat";

    static const char *rom_candidates[] = {
        PS3_PKG_USRDIR "/rom",
        PS3_PKG_USRDIR "/roms",
        PS3_ROOT "/rom",
        PS3_ROMS_DIR,
        "/app_home/rom",
        "rom",
        "./rom",
        nullptr
    };
    CPC.rom_path = PS3_PKG_USRDIR "/rom";
    for (int i = 0; rom_candidates[i]; i++) {
        if (rom_probe(rom_candidates[i])) {
            CPC.rom_path = rom_candidates[i];
            break;
        }
    }
    ps3_log("CPC.rom_path = %s\n", CPC.rom_path.c_str());
    {
        char bmsg[160];
        snprintf(bmsg, sizeof(bmsg), "rom_path:%s", CPC.rom_path.c_str());
        ps3_boot_mark(bmsg);
    }
    CPC.rom_file[7] = "amsdos.rom";
    CPC.rom_mf2 = "MF2.rom";
    if (CPC.model >= 3) {
        if (CPC.cartridge.file.empty()) CPC.cartridge.file = CPC.rom_path + "/system.cpr";
    } else {
        CPC.cartridge.file.clear();
    }

    static const char *res_candidates[] = {
        PS3_PKG_USRDIR "/resources",
        "/app_home/resources",
        "resources",
        "./resources",
        PS3_ROOT "/resources",
        nullptr
    };
    CPC.resources_path = PS3_PKG_USRDIR "/resources";
    for (int i = 0; res_candidates[i]; i++) {
        if (res_probe(res_candidates[i])) {
            CPC.resources_path = res_candidates[i];
            break;
        }
    }
    ps3_log("CPC.resources_path = %s\n", CPC.resources_path.c_str());

    CPC.paused = false;
    CPC.tape_motor = 0;
    CPC.tape_play_button = 0;
    CPC.printer_port = 0xff;
    if (!CPC.InputMapper) {
        CPC.InputMapper = new InputMapper(&CPC);
    }
}

static int mount_file(const char *path, int target)
{
    if (!path || !*path) return 0;
    flush_altered_disks();

    if (CPC.model != 2 && target != 3 && !has_ext(path, ".cpr")) {
        g_cfg.model = 2;
        g_cfg.ram_size = 128;
        restart_emulator();
        kbd_queue_clear();
        toast("CPC 6128 selected for this media");
    }

    if (has_ext(path, ".cdt") || has_ext(path, ".voc") || target == 2) {
        if (has_ext(path, ".cdt") || has_ext(path, ".voc")) {
            CPC.tape.file = path;
            CPC.tape.zip_index = 0;
            CPC.tape.drive = DRIVE::TAPE;
            if (file_load(CPC.tape) != 0) return 0;
            Tape_Rewind();
            CPC.tape_play_button = 0x10;
            emulator_reset();
            queue_tape_autorun();
            return 1;
        }
    }

    if (has_ext(path, ".cpr") || target == 3) {
        if (has_ext(path, ".cpr")) {
            CPC.cartridge.file = path;
            CPC.cartridge.zip_index = 0;
            CPC.cartridge.drive = DRIVE::CARTRIDGE;
            if (CPC.model < 3) {
                g_cfg.model = 3;
                if (g_cfg.ram_size < 128) g_cfg.ram_size = 128;
                restart_emulator();
                kbd_queue_clear();
                return 1;
            }
            if (file_load(CPC.cartridge) != 0) return 0;
            emulator_reset();
            kbd_queue_clear();
            return 1;
        }
    }

    if (has_ext(path, ".sna") || target == 4) {
        if (has_ext(path, ".sna")) {
            CPC.snapshot.file = path;
            CPC.snapshot.zip_index = 0;
            CPC.snapshot.drive = DRIVE::SNAPSHOT;
            kbd_queue_clear();
            return snapshot_load(std::string(path)) == 0;
        }
    }

    int use_b = (target == 1) ? 1 : 0;
    if (CPC.model >= 3 && !CPC.rom_path.empty()) {
        std::string sys_cpr = CPC.rom_path + "/system.cpr";
        if (CPC.cartridge.file != sys_cpr) {
            CPC.cartridge.file = sys_cpr;
            CPC.cartridge.zip_index = 0;
            CPC.cartridge.drive = DRIVE::CARTRIDGE;
            file_load(CPC.cartridge);
        }
    }
    t_slot &slot = use_b ? CPC.driveB : CPC.driveA;
    t_drive &drv = use_b ? driveB : driveA;
    std::string &savep = use_b ? g_dskB_save_path : g_dskA_save_path;

    dsk_eject(&drv);
    slot.file = path;
    slot.zip_index = 0;
    slot.drive = use_b ? DRIVE::DSK_B : DRIVE::DSK_A;
    savep = make_dsk_save_path(path, use_b ? 'B' : 'A');

    int loaded = 0;
    if (g_cfg.auto_save_dsk && file_exists(savep.c_str()) && has_ext(path, ".dsk")) {
        if (dsk_load(savep, &drv) == 0) {
            loaded = 1;
            ps3_log("Loaded disk save overlay: %s\n", savep.c_str());
        }
    }
    if (!loaded) {
        if (file_load(slot) != 0) return 0;
    }

    if (!use_b) {
        scan_disk_catalog(&driveA);
        emulator_reset();
        video_set_palette();
        queue_disk_autorun();
        ps3_log("mounted %s tracks=%d cat=%d files=%d boot=%d sys=%d model=%d ram=%d int=%d\n", path, driveA.tracks,
                g_disk_cat.valid ? 1 : 0, (int)g_disk_cat.files.size(),
                g_disk_cat.has_cpm_boot ? 1 : 0, g_disk_cat.is_system_fmt ? 1 : 0,
                CPC.model, CPC.ram_size, CPC.scr_intensity);

    } else {
        toast("Mounted Drive B");
    }
    return 1;
}

static void next_archive_image(void)
{
    t_slot *slot = nullptr;
    t_drive *drv = nullptr;
    if (!CPC.driveA.file.empty()) { slot = &CPC.driveA; drv = &driveA; }
    else if (!CPC.driveB.file.empty()) { slot = &CPC.driveB; drv = &driveB; }
    if (!slot || !has_ext(slot->file.c_str(), ".zip")) {
        toast("No ZIP disk mounted");
        return;
    }
    flush_altered_disks();
    slot->zip_index++;
    dsk_eject(drv);
    if (file_load(*slot) != 0) {
        slot->zip_index = 0;
        dsk_eject(drv);
        if (file_load(*slot) != 0) {
            toast("ZIP switch failed");
            return;
        }
    }
    if (drv == &driveA) {
        scan_disk_catalog(&driveA);
        emulator_reset();
        queue_disk_autorun();
    } else {
        toast("Next image in Drive B");
    }
}

static void usb_scan(void)
{
    g_usb.clear();
    const char *devs[] = {"/dev_usb000", "/dev_usb001", "/dev_usb002", "/dev_usb003"};
    for (int i = 0; i < 4; i++) {
        DIR *d = opendir(devs[i]);
        if (d) {
            closedir(d);
            g_usb.push_back(devs[i]);
        }
    }
}

static void lib_scan_dir(const std::string &path, int depth, int *count)
{
    if (depth > 4 || *count >= 800) return;
    DIR *d = opendir(path.c_str());
    if (!d) return;
    std::vector<std::string> subdirs;
    struct dirent *de;
    while ((de = readdir(d)) != nullptr && *count < 800) {
        const char *n = de->d_name;
        if (!strcmp(n, ".") || !strcmp(n, "..")) continue;
        std::string full = path + "/" + n;
        struct stat st;
        if (stat(full.c_str(), &st) != 0) continue;
        if (S_ISDIR(st.st_mode)) {
            subdirs.push_back(full);
        } else if (ext_media(n)) {
            LibEntry le;
            le.full = full;
            le.name = strip_ext(n);
            le.folder = base_name(path);
            if (has_ext(n, ".cdt") || has_ext(n, ".voc")) le.kind = "CDT";
            else if (has_ext(n, ".cpr")) le.kind = "CPR";
            else if (has_ext(n, ".sna")) le.kind = "SNA";
            else if (has_ext(n, ".zip")) le.kind = "ZIP";
            else if (has_ext(n, ".ipf")) le.kind = "IPF";
            else le.kind = "DSK";
            le.size = st.st_size;
            g_lib.push_back(le);
            (*count)++;
        }
    }
    closedir(d);
    std::sort(subdirs.begin(), subdirs.end());
    for (size_t i = 0; i < subdirs.size() && *count < 800; i++) {
        lib_scan_dir(subdirs[i], depth + 1, count);
    }
}

static void lib_refresh(void)
{
    usb_scan();
    g_lib_roots.clear();
    g_lib_roots.push_back(PS3_ROMS_DIR);
    for (size_t i = 0; i < g_usb.size(); i++) g_lib_roots.push_back(g_usb[i]);
    if (g_lib_root >= (int)g_lib_roots.size()) g_lib_root = 0;
    g_lib.clear();
    int count = 0;
    lib_scan_dir(g_lib_roots[g_lib_root], 0, &count);
    std::sort(g_lib.begin(), g_lib.end(), [](const LibEntry &a, const LibEntry &b) {
        return strcasecmp(a.name.c_str(), b.name.c_str()) < 0;
    });
    if (g_lib_sel >= (int)g_lib.size()) g_lib_sel = g_lib.empty() ? 0 : (int)g_lib.size() - 1;
    g_lib_scroll = 0;
}

static void browse_refresh(const std::string &path)
{
    g_dir = path;
    g_entries.clear();
    g_browse_sel = 0;
    g_browse_scroll = 0;
    if (g_dir != "/") {
        Entry up;
        up.name = "..";
        up.is_dir = true;
        up.size = 0;
        g_entries.push_back(up);
    }
    DIR *d = opendir(g_dir.c_str());
    if (!d) return;
    std::vector<Entry> dirs, files;
    struct dirent *de;
    while ((de = readdir(d)) != nullptr) {
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) continue;
        std::string full = (g_dir == "/") ? (std::string("/") + de->d_name) : (g_dir + "/" + de->d_name);
        struct stat st;
        if (stat(full.c_str(), &st) != 0) continue;
        Entry e;
        e.name = de->d_name;
        e.is_dir = S_ISDIR(st.st_mode);
        e.size = st.st_size;
        if (e.is_dir) dirs.push_back(e);
        else if (ext_media(de->d_name)) files.push_back(e);
    }
    closedir(d);
    auto cmp = [](const Entry &a, const Entry &b) { return strcasecmp(a.name.c_str(), b.name.c_str()) < 0; };
    std::sort(dirs.begin(), dirs.end(), cmp);
    std::sort(files.begin(), files.end(), cmp);
    for (size_t i = 0; i < dirs.size(); i++) g_entries.push_back(dirs[i]);
    for (size_t i = 0; i < files.size(); i++) g_entries.push_back(files[i]);
    snprintf(g_cfg.last_dir, sizeof(g_cfg.last_dir), "%s", g_dir.c_str());
}

static uint32_t badge_color_for_kind(const char *k)
{
    if (!strcmp(k, "DSK") || !strcmp(k, "IPF")) return 0xFF1E5694u;
    if (!strcmp(k, "CDT")) return 0xFF8C4A12u;
    if (!strcmp(k, "CPR")) return 0xFF5A2E84u;
    if (!strcmp(k, "SNA")) return 0xFF1D6E4Au;
    return 0xFF3E4E68u;
}

static void draw_header_and_tabs(void)
{
    char info[128];
    const char *drv = CPC.driveA.file.empty() ?
                      (CPC.tape.file.empty() ? "READY" : base_name(CPC.tape.file).c_str()) :
                      base_name(CPC.driveA.file).c_str();
    snprintf(info, sizeof(info), "%s  |  %uK  |  %s",
             g_model_names[CPC.model & 3], CPC.ram_size, drv);
    ps3_ui_tab_bar(g_tab_names, TAB_COUNT, g_tab, info);
}

static void draw_tab_library(void)
{
    ps3_ui_panel_grad(28, 118, 748, 536, 0xFF101926u, 0xFF0B111Bu, 0xFF223652u);
    char hdr[160];
    snprintf(hdr, sizeof(hdr), "SOURCE: %s   (%d GAMES)", g_lib_roots[g_lib_root].c_str(), (int)g_lib.size());
    ps3_ui_text_clip(44, 130, 710, hdr, 0xFF8EC8FFu, 0);
    ps3_ui_hline(42, 152, 720, 0xFF223652u);

    const int vis = 9;
    if (g_lib_sel < g_lib_scroll) g_lib_scroll = g_lib_sel;
    if (g_lib_sel >= g_lib_scroll + vis) g_lib_scroll = g_lib_sel - vis + 1;

    for (int i = 0; i < vis && g_lib_scroll + i < (int)g_lib.size(); i++) {
        int idx = g_lib_scroll + i;
        const LibEntry &le = g_lib[idx];
        int sel = (idx == g_lib_sel);
        int ry = 160 + i * 54;
        ps3_ui_card_row(40, ry, 724, 48, sel);
        ps3_ui_badge(54, ry + 11, 56, 26, le.kind.c_str(), badge_color_for_kind(le.kind.c_str()), 0xFFFFFFFFu);
        ps3_ui_text_clip(124, ry + 6, 624, le.name.c_str(), sel ? 0xFFFFFFFFu : 0xFFD4E2F4u, 1);
        ps3_ui_text_clip(124, ry + 28, 624, le.folder.c_str(), 0xFF7D94B4u, 0);
    }
    if (g_lib.empty()) {
        ps3_ui_text_center(402, 340, "NO CPC MEDIA FOUND IN THIS SOURCE", 0xFFFFB038u, 2);
        ps3_ui_text_center(402, 380, "Copy .dsk / .cdt / .cpr / .zip into /dev_hdd0/CPC/roms", 0xFF8EA4C4u, 1);
    }

    ps3_ui_panel_grad(790, 118, 462, 536, 0xFF101926u, 0xFF0B111Bu, 0xFF223652u);
    ps3_ui_text(808, 130, "MEDIA INSPECTOR", 0xFF8EC8FFu, 0);
    ps3_ui_hline(806, 152, 430, 0xFF223652u);

    if (!g_lib.empty() && g_lib_sel < (int)g_lib.size()) {
        const LibEntry &le = g_lib[g_lib_sel];
        int mcode = media_kind_code(le.full.c_str());
        ps3_ui_media_card(806, 166, 430, 108, mcode, le.name.c_str(), le.folder.c_str(), 1);

        char sbuf[64];
        snprintf(sbuf, sizeof(sbuf), "%ld KB  (%s)", (long)(le.size / 1024), le.kind.c_str());
        ps3_ui_badge(808, 294, 118, 24, "FILE SIZE", 0xFF1B2B42u, 0xFF9AC4F4u);
        ps3_ui_text(940, 297, sbuf, 0xFFE8F2FFu, 0);

        ps3_ui_badge(808, 328, 118, 24, "AUTO-RUN", 0xFF1B2B42u, 0xFF9AC4F4u);
        ps3_ui_text(940, 331, g_cfg.auto_run ? "ENABLED (RUN\" / |CPM)" : "MANUAL PROMPT", g_cfg.auto_run ? 0xFF52E590u : 0xFFFFB038u, 0);

        ps3_ui_badge(808, 362, 118, 24, "DISK SAVE", 0xFF1B2B42u, 0xFF9AC4F4u);
        ps3_ui_text(940, 365, g_cfg.auto_save_dsk ? "WRITABLE (/CPC/saves)" : "READ-ONLY", 0xFFD8E6F8u, 0);

        ps3_ui_panel_grad(806, 412, 430, 224, 0xFF0C1420u, 0xFF090E18u, 0xFF1D2E46u);
        ps3_ui_text(822, 428, "QUICK TIPS", 0xFFFFA834u, 0);
        ps3_ui_text(822, 458, "- Press X to mount & auto-launch", 0xFFC4D6EEu, 0);
        ps3_ui_text(822, 484, "- Press L2 in-game for CPC Keyboard", 0xFFC4D6EEu, 0);
        ps3_ui_text(822, 510, "- Use CPC & DISK tab to run specific", 0xFFC4D6EEu, 0);
        ps3_ui_text(822, 534, "  files from multi-game compilations", 0xFFC4D6EEu, 0);
        ps3_ui_text(822, 574, "- System ROMs are already bundled", 0xFF6AE098u, 0);
    }

    static const int fi[4] = {BTN_ICON_CROSS, BTN_ICON_SQUARE, BTN_ICON_TRIANGLE, BTN_ICON_CIRCLE};
    static const char *fl[4] = {"Mount & Run", "Rescan Media", "Switch Source", "Resume"};
    ps3_ui_footer_btns(fi, fl, 4);
}

static void draw_tab_browser(void)
{
    ps3_ui_panel_grad(28, 118, 1224, 44, 0xFF142238u, 0xFF0E1624u, 0xFF2B466Cu);
    ps3_ui_text_clip(44, 131, 860, g_dir.c_str(), 0xFFEAF4FFu, 1);
    char tbuf[48];
    snprintf(tbuf, sizeof(tbuf), "TARGET: %s", g_target_names[g_browse_target]);
    ps3_ui_badge(996, 126, 240, 28, tbuf, 0xFF7A4414u, 0xFFFFF4E0u);

    ps3_ui_panel_grad(28, 168, 764, 486, 0xFF101926u, 0xFF0B111Bu, 0xFF223652u);
    const int vis = 9;
    if (g_browse_sel < g_browse_scroll) g_browse_scroll = g_browse_sel;
    if (g_browse_sel >= g_browse_scroll + vis) g_browse_scroll = g_browse_sel - vis + 1;

    for (int i = 0; i < vis && g_browse_scroll + i < (int)g_entries.size(); i++) {
        int idx = g_browse_scroll + i;
        const Entry &e = g_entries[idx];
        int sel = (idx == g_browse_sel);
        int ry = 178 + i * 52;
        ps3_ui_card_row(40, ry, 740, 46, sel);
        if (e.is_dir) {
            ps3_ui_badge(54, ry + 10, 62, 26, "DIR", 0xFF28486Eu, 0xFFFFFFFFu);
        } else {
            const char *k = has_ext(e.name.c_str(), ".cdt") || has_ext(e.name.c_str(), ".voc") ? "CDT" :
                            (has_ext(e.name.c_str(), ".cpr") ? "CPR" :
                            (has_ext(e.name.c_str(), ".sna") ? "SNA" :
                            (has_ext(e.name.c_str(), ".zip") ? "ZIP" : "DSK")));
            ps3_ui_badge(54, ry + 10, 62, 26, k, badge_color_for_kind(k), 0xFFFFFFFFu);
        }
        ps3_ui_text_clip(130, ry + 11, 530, e.name.c_str(), sel ? 0xFFFFFFFFu : 0xFFD4E2F4u, 1);
        if (!e.is_dir) {
            char sz[32];
            snprintf(sz, sizeof(sz), "%ld KB", (long)(e.size / 1024));
            ps3_ui_text(678, ry + 13, sz, 0xFF8AA4C8u, 0);
        }
    }

    ps3_ui_panel_grad(806, 168, 446, 486, 0xFF101926u, 0xFF0B111Bu, 0xFF223652u);
    ps3_ui_text(824, 182, "FILE DETAILS", 0xFF8EC8FFu, 0);
    ps3_ui_hline(822, 204, 414, 0xFF223652u);
    if (!g_entries.empty() && g_browse_sel < (int)g_entries.size()) {
        const Entry &e = g_entries[g_browse_sel];
        int mc = e.is_dir ? 3 : media_kind_code(e.name.c_str());
        ps3_ui_media_card(822, 218, 414, 108, mc, e.name.c_str(), e.is_dir ? "Directory" : g_target_names[g_browse_target], 1);
    }
    ps3_ui_panel_grad(822, 344, 414, 290, 0xFF0C1420u, 0xFF090E18u, 0xFF1D2E46u);
    ps3_ui_text(840, 362, "CURRENT SLOTS", 0xFFFFA834u, 0);
    char line[128];
    snprintf(line, sizeof(line), "A: %s", CPC.driveA.file.empty() ? "(empty)" : base_name(CPC.driveA.file).c_str());
    ps3_ui_text_clip(840, 396, 378, line, 0xFFD8E6F8u, 0);
    snprintf(line, sizeof(line), "B: %s", CPC.driveB.file.empty() ? "(empty)" : base_name(CPC.driveB.file).c_str());
    ps3_ui_text_clip(840, 424, 378, line, 0xFFD8E6F8u, 0);
    snprintf(line, sizeof(line), "T: %s", CPC.tape.file.empty() ? "(empty)" : base_name(CPC.tape.file).c_str());
    ps3_ui_text_clip(840, 452, 378, line, 0xFFD8E6F8u, 0);
    snprintf(line, sizeof(line), "C: %s", CPC.cartridge.file.empty() ? "(empty)" : base_name(CPC.cartridge.file).c_str());
    ps3_ui_text_clip(840, 480, 378, line, 0xFFD8E6F8u, 0);

    static const int fi[4] = {BTN_ICON_CROSS, BTN_ICON_TRIANGLE, BTN_ICON_SQUARE, BTN_ICON_CIRCLE};
    static const char *fl[4] = {"Open / Mount", "Change Target", "HDD/USB Root", "Resume"};
    ps3_ui_footer_btns(fi, fl, 4);
}

static void draw_tab_states(void)
{
    ps3_ui_panel_grad(28, 118, 672, 536, 0xFF101926u, 0xFF0B111Bu, 0xFF223652u);
    char hdr[128];
    snprintf(hdr, sizeof(hdr), "GAME STATES: %s", game_key().c_str());
    ps3_ui_text_clip(44, 130, 640, hdr, 0xFF8EC8FFu, 0);
    ps3_ui_hline(42, 152, 644, 0xFF223652u);

    for (int i = 0; i < 8; i++) {
        char p[512];
        state_path(i, p, sizeof(p), "st");
        struct stat st;
        bool exists = stat(p, &st) == 0;
        int sel = (i == g_state_sel);
        int ry = 160 + i * 60;
        ps3_ui_card_row(40, ry, 648, 54, sel);

        char sname[32];
        snprintf(sname, sizeof(sname), "SLOT %d", i + 1);
        ps3_ui_text_shadow(56, ry + 15, sname, sel ? 0xFFFFFFFFu : 0xFFD8E6F8u, 1);

        ps3_ui_badge(152, ry + 14, 78, 26, exists ? "SAVED" : "EMPTY", exists ? 0xFF1D6E4Au : 0xFF253246u, 0xFFFFFFFFu);

        if (exists) {
            char tstr[64];
            struct tm *tmv = localtime(&st.st_mtime);
            if (tmv) {
                snprintf(tstr, sizeof(tstr), "%04d-%02d-%02d %02d:%02d  (%ldK)",
                         tmv->tm_year + 1900, tmv->tm_mon + 1, tmv->tm_mday,
                         tmv->tm_hour, tmv->tm_min, (long)(st.st_size / 1024));
            } else {
                snprintf(tstr, sizeof(tstr), "%ld KB", (long)(st.st_size / 1024));
            }
            ps3_ui_text(248, ry + 17, tstr, 0xFFB8CCE4u, 0);
        } else {
            ps3_ui_text(248, ry + 17, "No snapshot saved in this slot", 0xFF667C9Au, 0);
        }

        if (i == 0) {
            ps3_ui_badge(568, ry + 15, 106, 24, "QUICK L1/R1", 0xFF244068u, 0xFF9AC8FFu);
        }
    }

    ps3_ui_panel_grad(716, 118, 536, 536, 0xFF101926u, 0xFF0B111Bu, 0xFF223652u);
    ps3_ui_text(736, 130, "SLOT PREVIEW", 0xFF8EC8FFu, 0);
    ps3_ui_hline(734, 152, 500, 0xFF223652u);

    static uint32_t thumb[160 * 90];
    char p[512];
    state_path(g_state_sel, p, sizeof(p), "st");
    bool exists = file_exists(p);
    bool has_thumb = exists && thumb_load(g_state_sel, thumb);
    ps3_ui_thumb_box(736, 168, 496, 279, thumb, 160, 90, has_thumb, exists ? "NO PREVIEW IMAGE" : "EMPTY STATE SLOT");

    ps3_ui_panel_grad(736, 464, 496, 172, 0xFF0C1420u, 0xFF090E18u, 0xFF1D2E46u);
    ps3_ui_text(754, 480, "SAVE STORAGE STATUS", 0xFFFFA834u, 0);
    ps3_ui_text(754, 512, g_cfg.auto_save_dsk ? "Auto Disk Save: ON (/dev_hdd0/CPC/saves)" : "Auto Disk Save: OFF",
                g_cfg.auto_save_dsk ? 0xFF6AE098u : 0xFFFFB038u, 0);
    ps3_ui_text(754, 540, driveA.altered ? "Drive A has unsaved disk writes (will auto-save)" : "Drive A clean / synced",
                driveA.altered ? 0xFFFFD060u : 0xFFA4BAD6u, 0);
    ps3_ui_text(754, 568, "Press L1 / R1 during game to Quick-Save / Load Slot 1", 0xFF8AA4C8u, 0);
    ps3_ui_text(754, 594, "Export .SNA to /snapshots from CPC & DISK tab", 0xFF8AA4C8u, 0);

    static const int fi[4] = {BTN_ICON_CROSS, BTN_ICON_SQUARE, BTN_ICON_TRIANGLE, BTN_ICON_CIRCLE};
    static const char *fl[4] = {"Load State", "Save State", "Delete Slot", "Resume"};
    ps3_ui_footer_btns(fi, fl, 4);
}

#define SETTINGS_COUNT 15

struct PadPreset {
    const char *name;
    const char *desc;
    int map[REMAP_SLOTS];
};

static const PadPreset g_pad_presets[5] = {
    {"Joy + Cursors", "Joystick Port A/B + Cursor Keys + X:Fire1 O:Fire2 Sq:SPACE Tri:1 Start:RET", {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1}},
    {"QAOP + Space",  "Up:Q  Down:A  Left:O  Right:P  X:SPACE  O:M  Sq:SPACE  Tri:1  Start:RET",  {CPC_q, CPC_a, CPC_o, CPC_p, CPC_SPACE, CPC_m, CPC_SPACE, CPC_1, CPC_RETURN, CPC_ESC}},
    {"WASD + Space",  "Up:W  Down:S  Left:A  Right:D  X:SPACE  O:Z  Sq:X  Tri:1  Start:RET",      {CPC_w, CPC_s, CPC_a, CPC_d, CPC_SPACE, CPC_z, CPC_x, CPC_1, CPC_RETURN, CPC_ESC}},
    {"Cursors + ZX",  "Arrows  X:SPACE  O:Z  Sq:X  Tri:1  Start:RETURN  R2:ESC",                  {CPC_CUR_UP, CPC_CUR_DOWN, CPC_CUR_LEFT, CPC_CUR_RIGHT, CPC_SPACE, CPC_z, CPC_x, CPC_1, CPC_RETURN, CPC_ESC}},
    {"QAZX + Space",  "Up:Q  Down:A  Left:Z  Right:X  X:SPACE  O:RET  Sq:SHIFT  Tri:2  Start:RET", {CPC_q, CPC_a, CPC_z, CPC_x, CPC_SPACE, CPC_RETURN, CPC_LSHIFT, CPC_2, CPC_RETURN, CPC_ESC}}
};

static int padmap_detect_preset(void)
{
    for (int p = 0; p < 5; p++) {
        bool match = true;
        for (int i = 0; i < REMAP_SLOTS; i++) {
            if (g_cfg.padmap[i] != g_pad_presets[p].map[i]) {
                match = false;
                break;
            }
        }
        if (match) return p;
    }
    return -1;
}

static const char *padmap_preset_name(void)
{
    int p = padmap_detect_preset();
    return (p >= 0) ? g_pad_presets[p].name : "Custom";
}

static void padmap_apply_preset(int p)
{
    p = (p % 5 + 5) % 5;
    for (int i = 0; i < REMAP_SLOTS; i++) g_cfg.padmap[i] = g_pad_presets[p].map[i];
    ps3_config_save();
    char msg[128];
    snprintf(msg, sizeof(msg), "Preset: %s", g_pad_presets[p].name);
    toast(msg);
}

static const char *padmap_slot_name(int i)
{
    static const char *names[REMAP_SLOTS] = {
        "D-PAD UP", "D-PAD DOWN", "D-PAD LEFT", "D-PAD RIGHT",
        "CROSS (X)", "CIRCLE (O)", "SQUARE", "TRIANGLE",
        "START", "R2"
    };
    return (i >= 0 && i < REMAP_SLOTS) ? names[i] : "?";
}

static const char *padmap_default_name(int i)
{
    switch (i) {
        case 0: return "JOY + CURSOR UP";
        case 1: return "JOY + CURSOR DOWN";
        case 2: return "JOY + CURSOR LEFT";
        case 3: return "JOY + CURSOR RIGHT";
        case 4: return "JOY FIRE 1";
        case 5: return "JOY FIRE 2";
        case 6: return "SPACE";
        case 7: return "KEY 1";
        case 8: return "RETURN";
        case 9: return "ESC";
        default: return "DEFAULT";
    }
}

struct PadKeyName { int key; const char *name; };
static const PadKeyName g_padkey_names[] = {
    {CPC_0, "0"}, {CPC_1, "1"}, {CPC_2, "2"}, {CPC_3, "3"}, {CPC_4, "4"},
    {CPC_5, "5"}, {CPC_6, "6"}, {CPC_7, "7"}, {CPC_8, "8"}, {CPC_9, "9"},
    {CPC_A, "A"}, {CPC_B, "B"}, {CPC_C, "C"}, {CPC_D, "D"}, {CPC_E, "E"},
    {CPC_F, "F"}, {CPC_G, "G"}, {CPC_H, "H"}, {CPC_I, "I"}, {CPC_J, "J"},
    {CPC_K, "K"}, {CPC_L, "L"}, {CPC_M, "M"}, {CPC_N, "N"}, {CPC_O, "O"},
    {CPC_P, "P"}, {CPC_Q, "Q"}, {CPC_R, "R"}, {CPC_S, "S"}, {CPC_T, "T"},
    {CPC_U, "U"}, {CPC_V, "V"}, {CPC_W, "W"}, {CPC_X, "X"}, {CPC_Y, "Y"},
    {CPC_Z, "Z"},
    {CPC_a, "a"}, {CPC_b, "b"}, {CPC_c, "c"}, {CPC_d, "d"}, {CPC_e, "e"},
    {CPC_f, "f"}, {CPC_g, "g"}, {CPC_h, "h"}, {CPC_i, "i"}, {CPC_j, "j"},
    {CPC_k, "k"}, {CPC_l, "l"}, {CPC_m, "m"}, {CPC_n, "n"}, {CPC_o, "o"},
    {CPC_p, "p"}, {CPC_q, "q"}, {CPC_r, "r"}, {CPC_s, "s"}, {CPC_t, "t"},
    {CPC_u, "u"}, {CPC_v, "v"}, {CPC_w, "w"}, {CPC_x, "x"}, {CPC_y, "y"},
    {CPC_z, "z"},
    {CPC_SPACE, "SPACE"}, {CPC_RETURN, "RETURN"}, {CPC_ENTER, "ENTER"},
    {CPC_ESC, "ESC"}, {CPC_TAB, "TAB"}, {CPC_DEL, "DEL"}, {CPC_CLR, "CLR"},
    {CPC_COPY, "COPY"}, {CPC_CAPSLOCK, "CAPS LOCK"}, {CPC_LSHIFT, "SHIFT"},
    {CPC_RSHIFT, "SHIFT R"}, {CPC_CONTROL, "CTRL"},
    {CPC_CUR_UP, "CURSOR UP"}, {CPC_CUR_DOWN, "CURSOR DOWN"},
    {CPC_CUR_LEFT, "CURSOR LEFT"}, {CPC_CUR_RIGHT, "CURSOR RIGHT"},
    {CPC_F0, "F0"}, {CPC_F1, "F1"}, {CPC_F2, "F2"}, {CPC_F3, "F3"},
    {CPC_F4, "F4"}, {CPC_F5, "F5"}, {CPC_F6, "F6"}, {CPC_F7, "F7"},
    {CPC_F8, "F8"}, {CPC_F9, "F9"},
    {CPC_MINUS, "-"}, {CPC_EQUAL, "="}, {CPC_PLUS, "+"},
    {CPC_ASTERISK, "*"}, {CPC_SLASH, "/"}, {CPC_COLON, ":"},
    {CPC_SEMICOLON, ";"}, {CPC_COMMA, ","}, {CPC_PERIOD, "."},
    {CPC_QUESTION, "?"}, {CPC_EXCLAMATN, "!"}, {CPC_DBLQUOTE, "\""},
    {CPC_QUOTE, "'"}, {CPC_HASH, "#"}, {CPC_DOLLAR, "$"},
    {CPC_PERCENT, "%"}, {CPC_AMPERSAND, "&"}, {CPC_LEFTPAREN, "("},
    {CPC_RIGHTPAREN, ")"}, {CPC_AT, "@"}, {CPC_PIPE, "|"},
    {CPC_UNDERSCORE, "_"}, {CPC_POWER, "^"}, {CPC_POUND, "POUND"},
    {CPC_LBRACKET, "["}, {CPC_LCBRACE, "{"}, {CPC_RBRACKET, "]"},
    {CPC_RCBRACE, "}"}, {CPC_LESS, "<"}, {CPC_GREATER, ">"},
    {CPC_BACKSLASH, "\\"}, {CPC_BACKQUOTE, "`"}
};

static void padmap_value_name(int slot, char *out, size_t n)
{
    int v = (slot >= 0 && slot < REMAP_SLOTS) ? g_cfg.padmap[slot] : -1;
    if (v < 0) {
        snprintf(out, n, "%s", padmap_default_name(slot));
        return;
    }
    const char *base = nullptr;
    int key = v & 0xffff;
    for (size_t i = 0; i < sizeof(g_padkey_names) / sizeof(g_padkey_names[0]); i++) {
        if (g_padkey_names[i].key == key) { base = g_padkey_names[i].name; break; }
    }
    if (!base) {
        snprintf(out, n, "CODE %d", key);
        return;
    }
    if (v & 0x10000) snprintf(out, n, "SHIFT+%s", base);
    else if (v & 0x20000) snprintf(out, n, "CTRL+%s", base);
    else snprintf(out, n, "%s", base);
}

static int padmap_custom_count(void)
{
    int n = 0;
    for (int i = 0; i < REMAP_SLOTS; i++) if (g_cfg.padmap[i] >= 0) n++;
    return n;
}

static void draw_remap_editor(void)
{
    ps3_ui_panel_grad(28, 118, 1224, 536, 0xFF101926u, 0xFF0B111Bu, 0xFF223652u);
    ps3_ui_text(46, 130, "CONTROL PRESETS & PAD REMAP  |  UNIVERSAL FOR ALL GAMES", 0xFF8EC8FFu, 0);
    ps3_ui_hline(44, 152, 1190, 0xFF223652u);

    const int vis = 6;
    if (g_remap_sel < g_remap_scroll) g_remap_scroll = g_remap_sel;
    if (g_remap_sel >= g_remap_scroll + vis) g_remap_scroll = g_remap_sel - vis + 1;
    for (int i = 0; i < vis && g_remap_scroll + i < REMAP_ROWS; i++) {
        int idx = g_remap_scroll + i;
        int sel = (idx == g_remap_sel);
        int ry = 162 + i * 62;
        if (idx == 0) {
            ps3_ui_selector_row(44, ry, 1192, 54, "Universal Control Preset", padmap_preset_name(), sel);
        } else if (idx <= REMAP_SLOTS) {
            int slot = idx - 1;
            ps3_ui_card_row(44, ry, 1192, 54, sel);
            char val[64];
            padmap_value_name(slot, val, sizeof(val));
            ps3_ui_text_shadow(62, ry + 15, padmap_slot_name(slot), sel ? 0xFFFFFFFFu : 0xFFD2E0F4u, 1);
            int vw = ps3_ui_text_w(val, 0);
            ps3_ui_text(44 + 1192 - 18 - vw, ry + 15, val,
                        g_cfg.padmap[slot] < 0 ? 0xFF8EC8FFu : 0xFF7DFFA8u, 0);
        } else {
            ps3_ui_card_row(44, ry, 1192, 54, sel);
            ps3_ui_text_shadow(62, ry + 15, "RESET ALL TO DEFAULT (JOY + CURSORS)", sel ? 0xFFFFFFFFu : 0xFFD2E0F4u, 1);
        }
    }

    int curp = padmap_detect_preset();
    ps3_ui_panel_grad(44, 542, 1192, 98, 0xFF0E1D30u, 0xFF0B1422u, 0xFF2B507Eu);
    char pdesc[160];
    snprintf(pdesc, sizeof(pdesc), "ACTIVE PROFILE: %s  |  %s",
             padmap_preset_name(),
             (curp >= 0) ? g_pad_presets[curp].desc : "Custom global mapping saved in /dev_hdd0/CPC/cpc4ps3.ini");
    ps3_ui_text_clip(60, 554, 1160, pdesc, 0xFF7DFFA8u, 0);
    ps3_ui_text(60, 582, "Amstrad CPC games have no hardware control standard: many older titles ignore Joystick and use QAOP, WASD, or Z/X.", 0xFFAAC2E0u, 0);
    ps3_ui_text(60, 606, "Switch presets here or remap single buttons via Virtual Keyboard; settings apply universally to every game.", 0xFFAAC2E0u, 0);

    static const int fi[3] = {BTN_ICON_CROSS, BTN_ICON_LEFT, BTN_ICON_CIRCLE};
    static const char *fl[3] = {"Select / Preset", "Prev / Reset Key", "Back"};
    ps3_ui_footer_btns(fi, fl, 3);
}

static void draw_tab_settings(void)
{
    ps3_ui_panel_grad(28, 118, 1224, 536, 0xFF101926u, 0xFF0B111Bu, 0xFF223652u);

    const int vis = 9;
    if (g_settings_sel < g_settings_scroll) g_settings_scroll = g_settings_sel;
    if (g_settings_sel >= g_settings_scroll + vis) g_settings_scroll = g_settings_sel - vis + 1;

    bool need_restart = ((int)CPC.model != g_cfg.model) || ((int)CPC.ram_size != g_cfg.ram_size);

    for (int i = 0; i < vis && g_settings_scroll + i < SETTINGS_COUNT; i++) {
        int idx = g_settings_scroll + i;
        int sel = (idx == g_settings_sel);
        int ry = 132 + i * 56;
        int rx = 44;
        int rw = 1192;
        int rh = 50;

        switch (idx) {
            case 0:
                ps3_ui_selector_row(rx, ry, rw, rh, "CPC Hardware Model", g_model_names[g_cfg.model & 3], sel);
                break;
            case 1: {
                const char *rname = "128K";
                for (int r = 0; r < 6; r++) if (g_ram_choices[r] == g_cfg.ram_size) rname = g_ram_names[r];
                ps3_ui_selector_row(rx, ry, rw, rh, "System RAM Expansion", rname, sel);
                break;
            }
            case 2:
                ps3_ui_selector_row(rx, ry, rw, rh, "Monitor Display Mode", g_cfg.green_screen ? "Green Phosphor" : "Color CRT", sel);
                break;
            case 3:
                ps3_ui_selector_row(rx, ry, rw, rh, "Screen Aspect Ratio", g_aspect_names[g_cfg.aspect % 3], sel);
                break;
            case 4:
                ps3_ui_switch_row(rx, ry, rw, rh, "CRT Scanlines Overlay", g_cfg.scanlines, sel);
                break;
            case 5:
                ps3_ui_slider_row(rx, ry, rw, rh, "PSG Audio Volume", g_cfg.volume, 0, 100, "%", sel);
                break;
            case 6:
                ps3_ui_switch_row(rx, ry, rw, rh, "Auto-Run Disk / Tape on Launch", g_cfg.auto_run, sel);
                break;
            case 7:
                ps3_ui_switch_row(rx, ry, rw, rh, "Auto-Save Writable Disks (.dsk)", g_cfg.auto_save_dsk, sel);
                break;
            case 8:
                ps3_ui_selector_row(rx, ry, rw, rh, "Active Joystick Port", g_cfg.joystick_port ? "Port B (Joy 1)" : "Port A (Joy 0)", sel);
                break;
            case 9:
                ps3_ui_switch_row(rx, ry, rw, rh, "Show On-Screen FPS Counter", g_cfg.show_fps, sel);
                break;
            case 10:
                ps3_ui_selector_row(rx, ry, rw, rh, "Virtual Keyboard Theme", g_vk_style_names[g_cfg.vkbd_style & 3], sel);
                break;
            case 11:
                ps3_ui_selector_row(rx, ry, rw, rh, "Virtual Keyboard Opacity", g_vk_alpha_names[g_cfg.vkbd_alpha & 3], sel);
                break;
            case 12:
                ps3_ui_selector_row(rx, ry, rw, rh, "Control Preset (Universal)", padmap_preset_name(), sel);
                break;
            case 13:
                ps3_ui_card_row(rx, ry, rw, rh, sel);
                ps3_ui_text_shadow(rx + 18, ry + 13, "Remap Pad Buttons (Universal)", 0xFFE2ECF8u, 1);
                {
                    char rinfo[64];
                    int pr = padmap_detect_preset();
                    if (pr == 0) snprintf(rinfo, sizeof(rinfo), "ALL DEFAULT");
                    else if (pr > 0) snprintf(rinfo, sizeof(rinfo), "PRESET");
                    else snprintf(rinfo, sizeof(rinfo), "%d CUSTOM", padmap_custom_count());
                    int rvw = ps3_ui_text_w(rinfo, 1);
                    ps3_ui_text(rx + rw - 18 - rvw, ry + 13, rinfo, pr == 0 ? 0xFF8EC8FFu : 0xFF7DFFA8u, 1);
                }
                break;
            case 14:
                ps3_ui_card_row(rx, ry, rw, rh, sel);
                ps3_ui_text_shadow(rx + 18, ry + 13, "Apply Model / RAM & Restart Core", need_restart ? 0xFFFFD060u : 0xFFE2ECF8u, 1);
                if (need_restart) {
                    ps3_ui_badge(rx + rw - 196, ry + 11, 180, 28, "RESTART REQUIRED", 0xFF944808u, 0xFFFFFFFFu);
                } else {
                    ps3_ui_badge(rx + rw - 140, ry + 11, 124, 28, "SYNCED", 0xFF1A5A3Eu, 0xFFD8FFE8u);
                }
                break;
        }
    }

    static const int fi[4] = {BTN_ICON_CROSS, BTN_ICON_LEFT, BTN_ICON_RIGHT, BTN_ICON_CIRCLE};
    static const char *fl[4] = {"Select / Toggle", "Prev Value", "Next Value", "Resume"};
    ps3_ui_footer_btns(fi, fl, 4);
}

#define CPC_ACTIONS_COUNT 11

static void draw_tab_cpc_disk(void)
{
    std::string a_sub = CPC.driveA.file.empty() ? "No floppy mounted" : (driveA.altered ? "Modified (auto-save pending)" : "Mounted & ready");
    std::string b_sub = CPC.driveB.file.empty() ? "No floppy mounted" : (driveB.altered ? "Modified (auto-save pending)" : "Mounted & ready");
    std::string t_name = !CPC.tape.file.empty() ? base_name(CPC.tape.file) :
                         (!CPC.cartridge.file.empty() ? base_name(CPC.cartridge.file) : "Empty");
    int t_type = !CPC.tape.file.empty() ? 1 : 2;
    std::string t_sub = !CPC.tape.file.empty() ? (CPC.tape_play_button ? "Cassette Play active" : "Cassette loaded") : "Cartridge slot";

    ps3_ui_media_card(28, 118, 396, 94, 0,
                      CPC.driveA.file.empty() ? "DRIVE A: EMPTY" : base_name(CPC.driveA.file).c_str(),
                      a_sub.c_str(), !CPC.driveA.file.empty());
    ps3_ui_media_card(442, 118, 396, 94, 0,
                      CPC.driveB.file.empty() ? "DRIVE B: EMPTY" : base_name(CPC.driveB.file).c_str(),
                      b_sub.c_str(), !CPC.driveB.file.empty());
    ps3_ui_media_card(856, 118, 396, 94, t_type,
                      t_name.c_str(),
                      t_sub.c_str(), !CPC.tape.file.empty() || !CPC.cartridge.file.empty());

    ps3_ui_panel_grad(28, 224, 594, 430, 0xFF101926u, 0xFF0B111Bu, g_cpc_focus_cat ? 0xFF223652u : 0xFF3E7ED0u);
    static const char *actions[CPC_ACTIONS_COUNT] = {
        "Open CPC Virtual Keyboard (L2)",
        "Auto-Run Current Disk / Tape",
        "Type CAT  (Directory Listing)",
        "Type |CPM (Boot CP/M System)",
        "Next Disk Image in ZIP Archive",
        "Browse Media for Drive / Tape",
        "Save Writable Disks (.dsk to /saves)",
        "Export .SNA Snapshot (/snapshots)",
        "Eject All Disks & Tape",
        "Hard Reset CPC Machine",
        "Capture PNG Screenshot"
    };

    const int vis = 8;
    int scroll = 0;
    if (g_cpc_sel >= vis) scroll = g_cpc_sel - vis + 1;
    for (int i = 0; i < vis && scroll + i < CPC_ACTIONS_COUNT; i++) {
        int idx = scroll + i;
        int sel = (!g_cpc_focus_cat) && (idx == g_cpc_sel);
        int ry = 236 + i * 50;
        ps3_ui_card_row(40, ry, 570, 44, sel);
        ps3_ui_text_clip(58, ry + 10, 534, actions[idx], sel ? 0xFFFFFFFFu : 0xFFD2E0F4u, 1);
    }

    ps3_ui_panel_grad(638, 224, 614, 430, 0xFF101926u, 0xFF0B111Bu, g_cpc_focus_cat ? 0xFF3E7ED0u : 0xFF223652u);
    char ctitle[96];
    snprintf(ctitle, sizeof(ctitle), "DRIVE A CATALOG  (%d FILES)%s", (int)g_disk_cat.files.size(),
             g_disk_cat.has_cpm_boot ? "  [CP/M BOOT]" : "");
    ps3_ui_text(656, 236, ctitle, 0xFF8EC8FFu, 0);
    ps3_ui_badge(1078, 232, 158, 24, g_cpc_focus_cat ? "CATALOG ACTIVE" : "TRIANGLE: FOCUS",
                 g_cpc_focus_cat ? 0xFF1E6A44u : 0xFF20324Cu, 0xFFFFFFFFu);
    ps3_ui_hline(654, 260, 580, 0xFF223652u);

    if (CPC.driveA.file.empty()) {
        ps3_ui_text_center(945, 410, "Mount a .dsk / .zip disk in Drive A to inspect files", 0xFF7D94B4u, 0);
    } else if (g_disk_cat.files.empty()) {
        ps3_ui_text_center(945, 396, g_disk_cat.has_cpm_boot ? "Protected / CP/M boot sector detected" : "Custom track loader or empty catalog", 0xFFFFB848u, 1);
        ps3_ui_text_center(945, 432, "Try Type |CPM or RUN\"DISC from the actions column", 0xFF90A8C8u, 0);
    } else {
        const int cvis = 7;
        if (g_cpc_cat_sel < g_cpc_cat_scroll) g_cpc_cat_scroll = g_cpc_cat_sel;
        if (g_cpc_cat_sel >= g_cpc_cat_scroll + cvis) g_cpc_cat_scroll = g_cpc_cat_sel - cvis + 1;
        for (int i = 0; i < cvis && g_cpc_cat_scroll + i < (int)g_disk_cat.files.size(); i++) {
            int idx = g_cpc_cat_scroll + i;
            const DiskFileEntry &f = g_disk_cat.files[idx];
            int sel = g_cpc_focus_cat && (idx == g_cpc_cat_sel);
            int ry = 272 + i * 52;
            ps3_ui_card_row(654, ry, 580, 46, sel);
            ps3_ui_badge(668, ry + 10, 62, 26, f.ext.empty() ? "BIN" : f.ext.c_str(), badge_color_for_kind(f.ext.c_str()), 0xFFFFFFFFu);
            ps3_ui_text(746, ry + 11, f.full_name.c_str(), sel ? 0xFFFFFFFFu : 0xFFD8E6F8u, 1);
            if (f.system_attr) {
                ps3_ui_badge(1148, ry + 11, 70, 24, "SYS", 0xFF3E4E68u, 0xFFD8E6F8u);
            } else {
                ps3_ui_text(1118, ry + 14, "RUN\"", 0xFF68B4FFu, 0);
            }
        }
    }

    static const int fi[4] = {BTN_ICON_CROSS, BTN_ICON_TRIANGLE, BTN_ICON_L2, BTN_ICON_CIRCLE};
    static const char *fl[4] = {g_cpc_focus_cat ? "RUN\" Selected" : "Execute Action", "Switch Panel", "Virtual Keyboard", "Resume"};
    ps3_ui_footer_btns(fi, fl, 4);
}

#define HELP_ACTIONS_COUNT 6

static void draw_tab_help_exit(void)
{
    ps3_ui_panel_grad(28, 118, 760, 536, 0xFF101926u, 0xFF0B111Bu, 0xFF223652u);
    ps3_ui_text(46, 130, "CONTROLLER GUIDE & UNIVERSAL CONTROLS", 0xFF8EC8FFu, 0);
    ps3_ui_hline(44, 152, 728, 0xFF223652u);

    struct HelpBind { int icon; const char *lbl; };
    static const HelpBind binds[] = {
        {BTN_ICON_SELECT,   "SELECT or L3+R3 : Open / Close Emulator Menu"},
        {BTN_ICON_L2,       "L2 : Open / Close Amstrad CPC Virtual Keyboard"},
        {BTN_ICON_L1,       "L1 : Quick Save State (Slot 1)"},
        {BTN_ICON_R1,       "R1 : Quick Load State (Slot 1)"},
        {BTN_ICON_R3,       "R3 : Swap Joystick Port A / Port B"},
        {BTN_ICON_DPAD,     "D-Pad / Left Stick : Joystick / Preset Directions"},
        {BTN_ICON_CROSS,    "Cross : Joystick Fire 1 (or Preset) / Menu Confirm"},
        {BTN_ICON_CIRCLE,   "Circle : Joystick Fire 2 (or Preset) / Menu Back"},
        {BTN_ICON_SQUARE,   "Square : CPC SPACE key (or Preset Action)"},
        {BTN_ICON_TRIANGLE, "Triangle : CPC Key 1 (or Preset Key)"},
        {BTN_ICON_START,    "Start : CPC RETURN  |  R2 : CPC ESC"},
        {BTN_ICON_CROSS,    "Switch Control Presets or Remap Keys: Settings"}
    };
    for (int i = 0; i < 12; i++) {
        ps3_ui_btn_label(52, 166 + i * 34, binds[i].icon, binds[i].lbl, 0xFFDCE8F8u);
    }

    ps3_ui_panel_grad(44, 582, 728, 58, 0xFF0E1D30u, 0xFF0B1422u, 0xFF2B507Eu);
    ps3_ui_text(60, 591, "UNIVERSAL CONTROLS: CPC games vary between Joystick, QAOP, WASD, or Cursors.", 0xFF6AE098u, 0);
    ps3_ui_text(60, 613, "Choose a Control Preset or customize buttons in Settings (applied to all games).", 0xFFAAC2E0u, 0);

    ps3_ui_panel_grad(804, 118, 448, 536, 0xFF101926u, 0xFF0B111Bu, 0xFF223652u);
    ps3_ui_text(822, 130, "SESSION ACTIONS", 0xFF8EC8FFu, 0);
    ps3_ui_hline(820, 152, 414, 0xFF223652u);

    static const char *hacts[HELP_ACTIONS_COUNT] = {
        "Resume Game",
        "Reset CPC Machine",
        "Take PNG Screenshot",
        "Save Config & Flush Disks",
        "About",
        "Exit CPC4PS3 to XMB"
    };
    for (int i = 0; i < HELP_ACTIONS_COUNT; i++) {
        int sel = (i == g_help_sel);
        int ry = 170 + i * 68;
        ps3_ui_card_row(822, ry, 410, 56, sel);
        ps3_ui_text_shadow(844, ry + 16, hacts[i], (i == 5 && sel) ? 0xFFFF8080u : (sel ? 0xFFFFFFFFu : 0xFFD4E2F4u), 1);
    }

    ps3_ui_panel_grad(822, 592, 410, 46, 0xFF0D1624u, 0xFF09101Au, 0xFF203450u);
    char ver_foot[96];
    snprintf(ver_foot, sizeof(ver_foot), "CPC4PS3 v%s  |  Select About for ROM Copyright", PS3_APP_VERSION);
    ps3_ui_text_center(1027, 606, ver_foot, 0xFF94B4DAu, 0);

    static const int fi[2] = {BTN_ICON_CROSS, BTN_ICON_CIRCLE};
    static const char *fl[2] = {"Activate", "Resume"};
    ps3_ui_footer_btns(fi, fl, 2);
}

static void draw_about_modal(void)
{
    ps3_ui_blend(0, 0, PS3_SCREEN_W, PS3_SCREEN_H, 0xC8040810u);

    int mx = 180;
    int my = 112;
    int mw = 920;
    int mh = 496;
    ps3_ui_panel_grad(mx, my, mw, mh, 0xFF132036u, 0xFF0A111Du, 0xFF4CA4FFu);

    static const uint32_t bar[5] = {0xFF2894FFu, 0xFF24E0F8u, 0xFF34D878u, 0xFFFFB024u, 0xFFFF3848u};
    for (int i = 0; i < 5; i++) {
        int bx0 = mx + 2 + ((mw - 4) * i) / 5;
        int bx1 = mx + 2 + ((mw - 4) * (i + 1)) / 5;
        ps3_ui_fill(bx0, my + 2, bx1 - bx0, 4, bar[i]);
    }

    ps3_ui_text_shadow(mx + 30, my + 24, "ABOUT CPC4PS3", 0xFFFFFFFFu, 2);
    char vbadge[32];
    snprintf(vbadge, sizeof(vbadge), "VERSION %s", PS3_APP_VERSION);
    ps3_ui_badge(mx + mw - 166, my + 22, 136, 28, vbadge, 0xFF7A4414u, 0xFFFFF4E0u);
    ps3_ui_hline(mx + 28, my + 62, mw - 56, 0xFF264066u);

    ps3_ui_text_shadow(mx + 30, my + 82, "Amstrad CPC 464 / 664 / 6128 / 6128+ Emulator for PlayStation 3", 0xFFEAF4FFu, 1);
    ps3_ui_text(mx + 30, my + 114, "Emulation Core: Caprice32 (Open Source)", 0xFFAEC8E8u, 1);
    ps3_ui_text(mx + 30, my + 144, "PlayStation 3 Port & Interface: thheroGAC", 0xFF7DFFA8u, 1);
    ps3_ui_text(mx + 30, my + 176, "Thanks to the Caprice32 authors, testers, PSL1GHT and SDL communities.", 0xFF94B4DAu, 0);

    ps3_ui_panel_grad(mx + 28, my + 218, mw - 56, 186, 0xFF0E1D30u, 0xFF0B1422u, 0xFF2B507Eu);
    ps3_ui_text(mx + 48, my + 234, "SYSTEM ROM COPYRIGHT NOTICE", 0xFF8EC8FFu, 0);
    ps3_ui_hline(mx + 46, my + 256, mw - 92, 0xFF223C5Eu);
    ps3_ui_text(mx + 48, my + 272, "AMSTRAD CPC ROMS ARE COPYRIGHTED BY AMSTRAD AND LOCOMOTIVE SOFTWARE.", 0xFF6AE098u, 0);
    ps3_ui_text(mx + 48, my + 296, "AMSTRAD HAS KINDLY GRANTED PERMISSION FOR THE REDISTRIBUTION OF ITS", 0xFFAAC2E0u, 0);
    ps3_ui_text(mx + 48, my + 320, "COPYRIGHTED MATERIAL FOR USE WITH EMULATORS. ALL ORIGINAL COPYRIGHT", 0xFFAAC2E0u, 0);
    ps3_ui_text(mx + 48, my + 344, "NOTICES ARE PRESERVED. ALL RIGHTS REMAIN WITH THEIR RESPECTIVE", 0xFFAAC2E0u, 0);
    ps3_ui_text(mx + 48, my + 368, "COPYRIGHT HOLDERS.", 0xFFAAC2E0u, 0);

    ps3_ui_hline(mx + 28, my + 426, mw - 56, 0xFF203450u);
    int fx = mx + 30;
    fx += ps3_ui_btn_label(fx, my + 446, BTN_ICON_CROSS, "Close", 0xFFDCE8F8u);
    ps3_ui_btn_label(fx, my + 446, BTN_ICON_CIRCLE, "Back to Help / Exit", 0xFFDCE8F8u);
}

static void draw_menu(void)
{
    ps3_ui_bg();
    draw_header_and_tabs();
    switch (g_tab) {
        case TAB_LIBRARY: draw_tab_library(); break;
        case TAB_BROWSER: draw_tab_browser(); break;
        case TAB_STATES: draw_tab_states(); break;
        case TAB_SETTINGS: if (g_remap_open) draw_remap_editor(); else draw_tab_settings(); break;
        case TAB_CPC_DISK: draw_tab_cpc_disk(); break;
        case TAB_HELP_EXIT: draw_tab_help_exit(); break;
    }
    if (g_about_open) draw_about_modal();
}

static void mount_file_ui(const char *path, int target)
{
    if (mount_file(path, target)) {
        g_menu_active = 0;
    } else {
        toast("Unsupported or damaged media file");
    }
}

static void settings_change(int dir)
{
    switch (g_settings_sel) {
        case 0:
            g_cfg.model = (g_cfg.model + dir + 4) % 4;
            if (g_cfg.model >= 2 && g_cfg.ram_size < 128) g_cfg.ram_size = 128;
            break;
        case 1: {
            int idx = 1;
            for (int i = 0; i < 6; i++) if (g_ram_choices[i] == g_cfg.ram_size) idx = i;
            idx = (idx + dir + 6) % 6;
            g_cfg.ram_size = g_ram_choices[idx];
            break;
        }
        case 2:
            g_cfg.green_screen ^= 1;
            CPC.scr_tube = g_cfg.green_screen ? 1u : 0u;
            video_set_palette();
            break;
        case 3:
            g_cfg.aspect = (g_cfg.aspect + dir + PS3_ASPECT_COUNT) % PS3_ASPECT_COUNT;
            break;
        case 4:
            g_cfg.scanlines ^= 1;
            break;
        case 5:
            g_cfg.volume += dir * 5;
            if (g_cfg.volume < 0) g_cfg.volume = 0;
            if (g_cfg.volume > 100) g_cfg.volume = 100;
            CPC.snd_volume = (unsigned)g_cfg.volume;
            break;
        case 6:
            g_cfg.auto_run ^= 1;
            break;
        case 7:
            g_cfg.auto_save_dsk ^= 1;
            break;
        case 8:
            g_cfg.joystick_port ^= 1;
            break;
        case 9:
            g_cfg.show_fps ^= 1;
            break;
        case 10:
            g_cfg.vkbd_style = (g_cfg.vkbd_style + dir + 4) % 4;
            break;
        case 11:
            g_cfg.vkbd_alpha = (g_cfg.vkbd_alpha + dir + 4) % 4;
            break;
        case 12: {
            int curp = padmap_detect_preset();
            int nextp = (curp < 0) ? ((dir >= 0) ? 0 : 4) : ((curp + dir + 5) % 5);
            padmap_apply_preset(nextp);
            return;
        }
    }
    ps3_config_save();
}

static void nav_list(int *sel, int delta, int count)
{
    if (count <= 0) return;
    *sel = (*sel + delta + count) % count;
}

static uint32_t *g_vk_layer = nullptr;
static int g_vk_layer_valid = 0;
static int g_vk_layer_state[7];
static int g_vk_px = 0, g_vk_py = 0, g_vk_pw = 0, g_vk_ph = 0;

static void draw_vkbd_overlay_direct(void)
{
    static const uint32_t bg_top[4]   = {0x001B2842u, 0x00181C24u, 0x0015293Eu, 0x00241A12u};
    static const uint32_t bg_bot[4]   = {0x0010182Au, 0x00101318u, 0x000E1B2Au, 0x0018110Cu};
    static const uint32_t brd_col[4]  = {0xFF4CA4FFu, 0xFF5CE0FFu, 0xFF56B8E6u, 0xFFFF9820u};
    static const uint32_t key_top[4]  = {0x00263A5Cu, 0x00242A36u, 0x001E3A58u, 0x0034251Au};
    static const uint32_t key_bot[4]  = {0x001A2840u, 0x00191D26u, 0x0014273Cu, 0x00241911u};
    static const uint32_t key_brd[4]  = {0xFF3E5E8Cu, 0xFF3E485Cu, 0xFF325E84u, 0xFF68482Cu};
    static const uint8_t  alpha_val[4]= {0xEA, 0xCC, 0xA4, 0x7A};

    int st = g_cfg.vkbd_style & 3;
    int al = g_cfg.vkbd_alpha & 3;
    uint32_t a_mask = ((uint32_t)alpha_val[al]) << 24;

    int kw = 88;
    int kh = 40;
    int kgap = 6;
    int panel_w = VK_COLS * kw + (VK_COLS - 1) * kgap + 32;
    int panel_h = VK_ROWS * kh + (VK_ROWS - 1) * kgap + 70;
    int px = (PS3_SCREEN_W - panel_w) / 2;
    int py;
    if (g_cfg.vkbd_pos == 1) py = (PS3_SCREEN_H - panel_h) / 2;
    else if (g_cfg.vkbd_pos == 2) py = 26;
    else py = PS3_SCREEN_H - panel_h - 26;
    g_vk_px = px;
    g_vk_py = py;
    g_vk_pw = panel_w;
    g_vk_ph = panel_h;

    ps3_ui_panel_grad(px, py, panel_w, panel_h, bg_top[st] | a_mask, bg_bot[st] | a_mask, brd_col[st]);

    char hdr[160];
    snprintf(hdr, sizeof(hdr), "AMSTRAD CPC KEYBOARD   |   %s%s%s",
             g_vk_shift ? "[SHIFT] " : "",
             g_vk_ctrl ? "[CTRL] " : "",
             g_vk_caps ? "[CAPS] " : "");
    ps3_ui_text_shadow(px + 18, py + 10, hdr, 0xFFF2F7FFu, 0);

    char stat_r[96];
    snprintf(stat_r, sizeof(stat_r), "THEME: %s  |  POS: %s  |  OPACITY: %s",
             g_vk_style_names[st], g_vk_pos_names[g_cfg.vkbd_pos % 3], g_vk_alpha_names[al]);
    int srw = ps3_ui_text_w(stat_r, 0);
    ps3_ui_text(px + panel_w - srw - 18, py + 10, stat_r, 0xFF9BC0EAu, 0);

    int gx = px + 16;
    int gy = py + 34;

    g_vk_pulse = (g_vk_pulse + 1) & 31;
    uint32_t pulse_hi = (g_vk_pulse < 16) ? 0xFF70C8FFu : 0xFFFFB038u;

    for (int r = 0; r < VK_ROWS; r++) {
        for (int c = 0; c < VK_COLS; c++) {
            const VKeyCell &cell = g_vkbd_grid[r][c];
            int kx = gx + c * (kw + kgap);
            int ky = gy + r * (kh + kgap);
            int sel = (c == g_vk_x && r == g_vk_y);
            int is_active_mod = (cell.action == VK_ACT_SHIFT && g_vk_shift) ||
                                (cell.action == VK_ACT_CTRL && g_vk_ctrl) ||
                                (cell.action == VK_ACT_CAPS && g_vk_caps);
            int is_macro = (cell.action == VK_ACT_RUN || cell.action == VK_ACT_CAT || cell.action == VK_ACT_CPM);

            uint32_t kt = sel ? 0x002E6AB0u : (is_active_mod ? 0x001D6E4Au : (is_macro ? 0x006A3E18u : key_top[st]));
            uint32_t kb = sel ? 0x001B4678u : (is_active_mod ? 0x0012462Eu : (is_macro ? 0x00462810u : key_bot[st]));
            uint32_t kbr = sel ? pulse_hi : (is_active_mod ? 0xFF4CE888u : key_brd[st]);

            ps3_ui_panel_grad(kx, ky, kw, kh, kt | a_mask, kb | a_mask, kbr);
            ps3_ui_bevel(kx + 1, ky + 1, kw - 2, kh - 2, 0x28FFFFFFu, 0x40000000u);

            const char *lbl = (g_vk_shift || g_vk_caps) ? cell.shift_label : cell.label;
            int th = ps3_ui_text_h(0);
            uint32_t tc = sel ? 0xFFFFFFFFu : (is_macro ? 0xFFFFDC98u : 0xFFE2EEFCu);
            ps3_ui_text_center(kx + kw / 2, ky + (kh - th) / 2, lbl, tc, 0);

            if (is_active_mod) {
                ps3_ui_fill(kx + kw - 10, ky + 5, 5, 5, 0xFF4CFF88u);
            }
        }
    }

    int fy = py + panel_h - 26;
    if (g_vkbd_pick) {
        ps3_ui_text(px + 18, fy, "PICK A KEY FOR THE PAD BUTTON   X: Select   Triangle: SHIFT   O: Cancel", 0xFF7DFFA8u, 0);
    } else {
        ps3_ui_text(px + 18, fy, "X: Type Key   Square: DEL   Triangle: SHIFT   Start: RETURN   Select: Opacity   R2: Pos   L1/R1: Theme   O: Close", 0xFFAEC8E8u, 0);
    }
}

static void draw_vkbd_overlay(void)
{
    int st = g_cfg.vkbd_style & 3;
    int al = g_cfg.vkbd_alpha & 3;
    int pos = g_cfg.vkbd_pos % 3;
    int state[7] = { g_vk_x, g_vk_y,
                     (g_vk_shift ? 1 : 0) | (g_vk_ctrl ? 2 : 0) | (g_vk_caps ? 4 : 0),
                     st, al, pos, g_vkbd_pick };
    if (!g_vk_layer) g_vk_layer = (uint32_t *)malloc((size_t)PS3_SCREEN_W * PS3_SCREEN_H * 4);
    if (!g_vk_layer) {
        draw_vkbd_overlay_direct();
        return;
    }
    if (!g_vk_layer_valid || memcmp(state, g_vk_layer_state, sizeof(state)) != 0) {
        memset(g_vk_layer, 0, (size_t)PS3_SCREEN_W * PS3_SCREEN_H * 4);
        ps3_video_set_ui_target(g_vk_layer);
        draw_vkbd_overlay_direct();
        ps3_video_set_ui_target(nullptr);
        memcpy(g_vk_layer_state, state, sizeof(state));
        g_vk_layer_valid = 1;
    }
    uint32_t *fb = ps3_video_fb();
    if (!fb) return;
    int x0 = g_vk_px < 0 ? 0 : g_vk_px;
    int y0 = g_vk_py < 0 ? 0 : g_vk_py;
    int x1 = g_vk_px + g_vk_pw;
    int y1 = g_vk_py + g_vk_ph;
    if (x1 > PS3_SCREEN_W) x1 = PS3_SCREEN_W;
    if (y1 > PS3_SCREEN_H) y1 = PS3_SCREEN_H;
    for (int y = y0; y < y1; y++) {
        const uint32_t *s = g_vk_layer + (size_t)y * PS3_SCREEN_W;
        uint32_t *d = fb + (size_t)y * PS3_SCREEN_W;
        for (int x = x0; x < x1; x++) {
            uint32_t c = s[x];
            uint32_t a = c >> 24;
            if (!a) continue;
            if (a == 255) { d[x] = c; continue; }
            uint32_t o = d[x];
            uint32_t ia = 255 - a;
            uint32_t r = ((((c >> 16) & 0xFF) * a + ((o >> 16) & 0xFF) * ia) >> 8);
            uint32_t g = ((((c >> 8) & 0xFF) * a + ((o >> 8) & 0xFF) * ia) >> 8);
            uint32_t b = (((c & 0xFF) * a + (o & 0xFF) * ia) >> 8);
            d[x] = 0xFF000000u | (r << 16) | (g << 8) | b;
        }
    }
    if (g_vkbd_pick) {
        ps3_ui_panel_grad(190, 56, 900, 96, 0xFF0E2A12u, 0xFF0A1A0Cu, 0xFF4CE888u);
        ps3_ui_text_center(PS3_SCREEN_W / 2, 72, g_vkbd_pick_title, 0xFF7DFFA8u, 2);
        ps3_ui_text_center(PS3_SCREEN_W / 2, 112, "X: CHOOSE KEY   TRIANGLE: SHIFT   O: CANCEL", 0xFFFFFFFFu, 0);
    }
}

static void run_vkbd(void)
{
    ps3_audio_pause(0);
    CPC.paused = false;
    ps3_input_release_all();
    ps3_video_set_manual_flip(1);
    int vk_frames = 0;
    uint32_t vk_t0 = SDL_GetTicks();
    uint32_t vk_blit = 0, vk_draw = 0, vk_pres = 0;

    while (!g_app_quit) {
        ps3_sysutil_check();
        uint32_t pressed = 0;
        ps3_input_read(&pressed);

        if (pressed & (PS3_ACT_CANCEL | PS3_ACT_L2)) {
            break;
        }
        if (pressed & PS3_ACT_UP) g_vk_y = (g_vk_y + VK_ROWS - 1) % VK_ROWS;
        if (pressed & PS3_ACT_DOWN) g_vk_y = (g_vk_y + 1) % VK_ROWS;
        if (pressed & PS3_ACT_LEFT) g_vk_x = (g_vk_x + VK_COLS - 1) % VK_COLS;
        if (pressed & PS3_ACT_RIGHT) g_vk_x = (g_vk_x + 1) % VK_COLS;
        if (pressed & PS3_ACT_TRIANGLE) g_vk_shift ^= 1;
        if (!g_vkbd_pick && (pressed & PS3_ACT_SQUARE)) kbd_push_key(CPC_DEL, 3, 2, false, false);
        if (!g_vkbd_pick && (pressed & PS3_ACT_START)) kbd_push_key(CPC_RETURN, 4, 2, false, false);
        if (!g_vkbd_pick && (pressed & PS3_ACT_SELECT)) {
            g_cfg.vkbd_alpha = (g_cfg.vkbd_alpha + 1) % 4;
            ps3_config_save();
        }
        if (!g_vkbd_pick && (pressed & PS3_ACT_R2)) {
            g_cfg.vkbd_pos = (g_cfg.vkbd_pos + 1) % 3;
            ps3_config_save();
        }
        if (!g_vkbd_pick && (pressed & PS3_ACT_R1)) {
            g_cfg.vkbd_style = (g_cfg.vkbd_style + 1) % 4;
            ps3_config_save();
        }
        if (!g_vkbd_pick && (pressed & PS3_ACT_L1)) {
            g_cfg.vkbd_style = (g_cfg.vkbd_style + 3) % 4;
            ps3_config_save();
        }

        if (pressed & PS3_ACT_CONFIRM) {
            const VKeyCell &cell = g_vkbd_grid[g_vk_y][g_vk_x];
            if (cell.action == VK_ACT_SHIFT) {
                g_vk_shift ^= 1;
            } else if (cell.action == VK_ACT_CTRL) {
                g_vk_ctrl ^= 1;
            } else if (cell.action == VK_ACT_CAPS) {
                g_vk_caps ^= 1;
                kbd_push_key(CPC_CAPSLOCK, 3, 2, false, false);
            } else if (cell.action == VK_ACT_RUN) {
                if (!g_vkbd_pick) kbd_push_string("RUN\"");
            } else if (cell.action == VK_ACT_CAT) {
                if (!g_vkbd_pick) {
                    kbd_push_string("CAT\n");
                    break;
                }
            } else if (cell.action == VK_ACT_CPM) {
                if (!g_vkbd_pick) {
                    kbd_push_string("|CPM\n");
                    break;
                }
            } else if (g_vkbd_pick) {
                int code = (g_vk_shift || g_vk_caps) ? cell.shift_key : cell.normal_key;
                bool extra_shift = g_vk_shift != 0 && (cell.shift_key == cell.normal_key);
                g_vkbd_picked = code | (extra_shift ? 0x10000 : 0) | (g_vk_ctrl ? 0x20000 : 0);
                g_vk_shift = 0;
                g_vk_ctrl = 0;
                break;
            } else {
                int code = (g_vk_shift || g_vk_caps) ? cell.shift_key : cell.normal_key;
                bool extra_shift = g_vk_shift != 0 && (cell.shift_key == cell.normal_key);
                kbd_push_key((CPC_KEYS)code, 4, 2, extra_shift, g_vk_ctrl != 0);
                g_vk_shift = 0;
                g_vk_ctrl = 0;
            }
        }

        int drew = 0;
        for (int spin = 0; spin < 40; spin++) {
            if (emulation_step()) {
                kbd_queue_tick();
                drew = 1;
                break;
            }
        }
        if (!drew) {
            SDL_Delay(1);
            continue;
        }

        uint32_t t0 = SDL_GetTicks();
        ps3_video_draw_game();
        uint32_t t1 = SDL_GetTicks();
        draw_vkbd_overlay();
        uint32_t t2 = SDL_GetTicks();
        ps3_video_present();
        uint32_t t3 = SDL_GetTicks();
        vk_blit += t1 - t0;
        vk_draw += t2 - t1;
        vk_pres += t3 - t2;
        vk_frames++;
    }

    ps3_video_set_manual_flip(0);
    ps3_log("vkbd closed frames=%d ms=%u blit=%u draw=%u pres=%u\n", vk_frames,
            (unsigned)(SDL_GetTicks() - vk_t0), vk_blit, vk_draw, vk_pres);
    ps3_input_read(nullptr);
}

static void remap_editor_input(uint32_t pressed)
{
    if (pressed & PS3_ACT_UP) nav_list(&g_remap_sel, -1, REMAP_ROWS);
    if (pressed & PS3_ACT_DOWN) nav_list(&g_remap_sel, 1, REMAP_ROWS);
    if (pressed & (PS3_ACT_CANCEL | PS3_ACT_TRIANGLE | PS3_ACT_MENU)) {
        g_remap_open = 0;
        return;
    }
    if (g_remap_sel == 0) {
        if (pressed & PS3_ACT_LEFT) {
            int curp = padmap_detect_preset();
            padmap_apply_preset(curp < 0 ? 4 : curp - 1);
        } else if (pressed & (PS3_ACT_CONFIRM | PS3_ACT_RIGHT)) {
            int curp = padmap_detect_preset();
            padmap_apply_preset(curp < 0 ? 0 : curp + 1);
        }
        return;
    }
    if ((pressed & PS3_ACT_LEFT) && g_remap_sel >= 1 && g_remap_sel <= REMAP_SLOTS) {
        int slot = g_remap_sel - 1;
        g_cfg.padmap[slot] = -1;
        ps3_config_save();
        return;
    }
    if (pressed & (PS3_ACT_CONFIRM | PS3_ACT_RIGHT)) {
        if (g_remap_sel >= 1 && g_remap_sel <= REMAP_SLOTS) {
            int slot = g_remap_sel - 1;
            g_vkbd_pick = 1;
            g_vkbd_picked = -1;
            snprintf(g_vkbd_pick_title, sizeof(g_vkbd_pick_title), "PICK KEY FOR: %s", padmap_slot_name(slot));
            g_vk_x = 6;
            g_vk_y = 5;
            run_vkbd();
            g_vkbd_pick = 0;
            CPC.paused = true;
            ps3_audio_pause(1);
            ps3_input_release_all();
            if (g_vkbd_picked >= 0) {
                g_cfg.padmap[slot] = g_vkbd_picked;
                ps3_config_save();
                char done_msg[128];
                char kval[64];
                padmap_value_name(slot, kval, sizeof(kval));
                snprintf(done_msg, sizeof(done_msg), "%s = %s", padmap_slot_name(slot), kval);
                toast(done_msg);
            }
        } else {
            for (int i = 0; i < REMAP_SLOTS; i++) g_cfg.padmap[i] = -1;
            ps3_config_save();
            toast("Pad mapping reset to Joy + Cursors");
        }
    }
}

static void menu_run(void)
{
    static int s_first_menu_trace = 1;
    int first_trace = s_first_menu_trace;
    s_first_menu_trace = 0;

    g_menu_active = 1;
    g_remap_open = 0;
    g_about_open = 0;
    ps3_audio_pause(1);
    if (first_trace) ps3_boot_mark("menu_audio_paused");
    ps3_input_release_all();
    CPC.paused = true;

    int open_vkbd_after = 0;
    if (first_trace) ps3_boot_mark("menu_loop_enter");

    while (g_menu_active && !g_app_quit) {
        ps3_sysutil_check();
        uint32_t pressed = 0;
        ps3_input_read(&pressed);

        if (g_about_open) {
            if (pressed & (PS3_ACT_CONFIRM | PS3_ACT_CANCEL | PS3_ACT_TRIANGLE | PS3_ACT_MENU)) {
                g_about_open = 0;
            }
            pressed = 0;
        }
        if (g_remap_open) {
            remap_editor_input(pressed);
            pressed = 0;
        }

        if (pressed & (PS3_ACT_MENU | PS3_ACT_CANCEL)) {
            g_menu_active = 0;
            break;
        }
        if (pressed & PS3_ACT_L1) g_tab = (g_tab + TAB_COUNT - 1) % TAB_COUNT;
        if (pressed & PS3_ACT_R1) g_tab = (g_tab + 1) % TAB_COUNT;

        if (g_tab == TAB_LIBRARY) {
            if (pressed & PS3_ACT_UP) nav_list(&g_lib_sel, -1, (int)g_lib.size());
            if (pressed & PS3_ACT_DOWN) nav_list(&g_lib_sel, 1, (int)g_lib.size());
            if (pressed & PS3_ACT_SQUARE) { lib_refresh(); toast("Media rescan complete"); }
            if (pressed & PS3_ACT_TRIANGLE) {
                g_lib_root = (g_lib_root + 1) % (int)g_lib_roots.size();
                lib_refresh();
            }
            if ((pressed & PS3_ACT_CONFIRM) && !g_lib.empty()) {
                const LibEntry &le = g_lib[g_lib_sel];
                int t = has_ext(le.full.c_str(), ".cdt") || has_ext(le.full.c_str(), ".voc") ? 2 :
                        (has_ext(le.full.c_str(), ".cpr") ? 3 :
                        (has_ext(le.full.c_str(), ".sna") ? 4 : 0));
                mount_file_ui(le.full.c_str(), t);
            }
        } else if (g_tab == TAB_BROWSER) {
            if (pressed & PS3_ACT_UP) nav_list(&g_browse_sel, -1, (int)g_entries.size());
            if (pressed & PS3_ACT_DOWN) nav_list(&g_browse_sel, 1, (int)g_entries.size());
            if (pressed & PS3_ACT_TRIANGLE) g_browse_target = (g_browse_target + 1) % 5;
            if (pressed & PS3_ACT_SQUARE) {
                usb_scan();
                if (g_dir.rfind("/dev_hdd0", 0) == 0 && !g_usb.empty()) browse_refresh(g_usb[0]);
                else if (g_dir != PS3_ROOT && g_dir != PS3_ROMS_DIR) browse_refresh(PS3_ROMS_DIR);
                else browse_refresh("/");
            }
            if ((pressed & PS3_ACT_LEFT) && g_dir != "/") {
                size_t s = g_dir.find_last_of('/');
                browse_refresh((s == 0) ? "/" : g_dir.substr(0, s));
            }
            if ((pressed & PS3_ACT_CONFIRM) && !g_entries.empty()) {
                const Entry &e = g_entries[g_browse_sel];
                if (e.is_dir) {
                    if (e.name == "..") {
                        size_t s = g_dir.find_last_of('/');
                        browse_refresh((s == 0) ? "/" : g_dir.substr(0, s));
                    } else {
                        std::string next = (g_dir == "/") ? (std::string("/") + e.name) : (g_dir + "/" + e.name);
                        browse_refresh(next);
                    }
                } else {
                    std::string full = (g_dir == "/") ? (std::string("/") + e.name) : (g_dir + "/" + e.name);
                    mount_file_ui(full.c_str(), g_browse_target);
                }
            }
        } else if (g_tab == TAB_STATES) {
            if (pressed & PS3_ACT_UP) nav_list(&g_state_sel, -1, 8);
            if (pressed & PS3_ACT_DOWN) nav_list(&g_state_sel, 1, 8);
            if (pressed & PS3_ACT_CONFIRM) { load_state(g_state_sel); g_menu_active = 0; }
            if (pressed & PS3_ACT_SQUARE) { save_state(g_state_sel); }
            if (pressed & PS3_ACT_TRIANGLE) { delete_state(g_state_sel); }
        } else if (g_tab == TAB_SETTINGS) {
            if (pressed & PS3_ACT_UP) nav_list(&g_settings_sel, -1, SETTINGS_COUNT);
            if (pressed & PS3_ACT_DOWN) nav_list(&g_settings_sel, 1, SETTINGS_COUNT);
            if (pressed & PS3_ACT_LEFT) settings_change(-1);
            if (pressed & PS3_ACT_RIGHT) settings_change(1);
            if (pressed & PS3_ACT_CONFIRM) {
                if (g_settings_sel == 12) settings_change(1);
                else if (g_settings_sel == 13) g_remap_open = 1;
                else if (g_settings_sel == 14) restart_emulator();
                else settings_change(1);
            }
        } else if (g_tab == TAB_CPC_DISK) {
            if (pressed & (PS3_ACT_TRIANGLE | PS3_ACT_L2)) {
                if (pressed & PS3_ACT_L2) {
                    g_menu_active = 0;
                    open_vkbd_after = 1;
                    break;
                }
                g_cpc_focus_cat ^= 1;
            }
            if (!g_cpc_focus_cat && (pressed & PS3_ACT_RIGHT) && !g_disk_cat.files.empty()) {
                g_cpc_focus_cat = 1;
            } else if (g_cpc_focus_cat && (pressed & PS3_ACT_LEFT)) {
                g_cpc_focus_cat = 0;
            } else if (!g_cpc_focus_cat) {
                if (pressed & PS3_ACT_UP) nav_list(&g_cpc_sel, -1, CPC_ACTIONS_COUNT);
                if (pressed & PS3_ACT_DOWN) nav_list(&g_cpc_sel, 1, CPC_ACTIONS_COUNT);
                if (pressed & PS3_ACT_CONFIRM) {
                    switch (g_cpc_sel) {
                        case 0: g_menu_active = 0; open_vkbd_after = 1; break;
                        case 1: queue_disk_autorun(); g_menu_active = 0; break;
                        case 2: kbd_queue_clear(); kbd_push_string("CAT\n"); g_menu_active = 0; break;
                        case 3: kbd_queue_clear(); kbd_push_string("|CPM\n"); g_menu_active = 0; break;
                        case 4: next_archive_image(); g_menu_active = 0; break;
                        case 5: g_tab = TAB_BROWSER; break;
                        case 6: {
                            int saved = 0;
                            if (!g_dskA_save_path.empty() && driveA.tracks > 0 && !driveA.eject_hook && dsk_save(g_dskA_save_path, &driveA) == 0) {
                                driveA.altered = false;
                                saved++;
                            }
                            if (!g_dskB_save_path.empty() && driveB.tracks > 0 && !driveB.eject_hook && dsk_save(g_dskB_save_path, &driveB) == 0) {
                                driveB.altered = false;
                                saved++;
                            }
                            toast(saved ? "Disk saved to /dev_hdd0/CPC/saves" : "No writable disk mounted");
                            break;
                        }
                        case 7: {
                            char snap[512];
                            snprintf(snap, sizeof(snap), PS3_SNAPS_DIR "/%s.sna", game_key().c_str());
                            if (snapshot_save(snap) == 0) toast("Exported .SNA snapshot");
                            else toast("Snapshot export failed");
                            break;
                        }
                        case 8:
                            flush_altered_disks();
                            dsk_eject(&driveA);
                            dsk_eject(&driveB);
                            CPC.driveA.file.clear();
                            CPC.driveB.file.clear();
                            g_dskA_save_path.clear();
                            g_dskB_save_path.clear();
                            g_disk_cat.files.clear();
                            CPC.tape.file.clear();
                            tape_eject();
                            kbd_queue_clear();
                            emulator_reset();
                            toast("Ejected all media");
                            break;
                        case 9:
                            flush_altered_disks();
                            kbd_queue_clear();
                            emulator_reset();
                            g_menu_active = 0;
                            break;
                        case 10:
                            ps3_screenshot();
                            g_menu_active = 0;
                            break;
                    }
                }
            } else {
                if (pressed & PS3_ACT_UP) nav_list(&g_cpc_cat_sel, -1, (int)g_disk_cat.files.size());
                if (pressed & PS3_ACT_DOWN) nav_list(&g_cpc_cat_sel, 1, (int)g_disk_cat.files.size());
                if ((pressed & PS3_ACT_CONFIRM) && !g_disk_cat.files.empty()) {
                    const DiskFileEntry &f = g_disk_cat.files[g_cpc_cat_sel];
                    kbd_queue_clear();
                    if (has_ci(f.ext, "COM")) kbd_push_string("|CPM\n");
                    else kbd_push_string("RUN\"" + f.full_name + "\n");
                    g_menu_active = 0;
                }
            }
        } else if (g_tab == TAB_HELP_EXIT) {
            if (pressed & PS3_ACT_UP) nav_list(&g_help_sel, -1, HELP_ACTIONS_COUNT);
            if (pressed & PS3_ACT_DOWN) nav_list(&g_help_sel, 1, HELP_ACTIONS_COUNT);
            if (pressed & PS3_ACT_CONFIRM) {
                switch (g_help_sel) {
                    case 0: g_menu_active = 0; break;
                    case 1: flush_altered_disks(); kbd_queue_clear(); emulator_reset(); g_menu_active = 0; break;
                    case 2: ps3_screenshot(); g_menu_active = 0; break;
                    case 3: flush_altered_disks(); ps3_config_save(); toast("Settings & disks saved"); break;
                    case 4: g_about_open = 1; break;
                    case 5: g_app_quit = 1; g_menu_active = 0; break;
                }
            }
        }

        if (first_trace) ps3_boot_mark("menu_draw1_enter");
        draw_menu();
        if (first_trace) ps3_boot_mark("menu_draw1_ok");
        ps3_video_present();
        if (first_trace) {
            ps3_boot_mark("menu_present1_ok");
            first_trace = 0;
        }
    }

    ps3_input_release_all();
    ps3_config_save();

    if (open_vkbd_after && !g_app_quit) {
        run_vkbd();
    }

    if (!g_app_quit) {
        CPC.paused = false;
        ps3_audio_pause(0);
    } else {
        CPC.paused = true;
        ps3_audio_pause(1);
    }
}

static void show_fatal_error(const char *title, const char *msg)
{
    for (int i = 0; i < 600 && !g_app_quit; i++) {
        ps3_sysutil_check();
        uint32_t pressed = 0;
        ps3_input_read(&pressed);
        if (pressed & (PS3_ACT_CONFIRM | PS3_ACT_CANCEL | PS3_ACT_START)) break;
        ps3_ui_bg();
        ps3_ui_panel_grad(220, 190, 840, 320, 0xFF2E1216u, 0xFF1A0A0Cu, 0xFFFF4050u);
        ps3_ui_text_center(PS3_SCREEN_W / 2, 232, title, 0xFFFF5A68u, 3);
        ps3_ui_text_center(PS3_SCREEN_W / 2, 314, msg, 0xFFFFFFFFu, 2);
        ps3_ui_text_center(PS3_SCREEN_W / 2, 438, "Press X or START to exit", 0xFFD0DCEAu, 1);
        ps3_video_present();
    }
}

static uint32_t g_snd_stall_ms = 0;
static int g_null_surface_logged = 0;
static unsigned long long g_cyc_acc = 0;

static int emulation_step(void)
{
    if (CPC.limit_speed && CPC.snd_enabled && s_exit_condition == EC_SOUND_BUFFER) {
        if (!dwSndBufferCopied) {
            uint32_t now = SDL_GetTicks();
            if (!g_snd_stall_ms) g_snd_stall_ms = now;
            if (now - g_snd_stall_ms < 1500) {
                usleep(1000);
                return 0;
            }
            ps3_log("audio stall: sound disabled, emulation continues\n");
            CPC.snd_enabled = 0;
            ps3_audio_pause(1);
            g_snd_stall_ms = 0;
        } else {
            dwSndBufferCopied = 0;
            g_snd_stall_ms = 0;
        }
    }

    if (!back_surface || !back_surface->pixels) {
        if (!g_null_surface_logged) {
            ps3_log("emulation: back_surface missing\n");
            g_null_surface_logged = 1;
        }
        return 0;
    }

    dword off = (dword)(CPC.scr_pos - CPC.scr_base);
    if (VDU.scrln > 0) {
        CPC.scr_base = (byte *)back_surface->pixels + (VDU.scrln * CPC.scr_line_offs);
    } else {
        CPC.scr_base = (byte *)back_surface->pixels;
    }
    CPC.scr_pos = CPC.scr_base + off;

    int cyc_before = CPC.cycle_count;
    s_exit_condition = z80_execute();
    if (!g_loader_seen) {
        unsigned cur = z80.PC.w.l & 0xFFFFu;
        if (cur >= 0x8000 && cur < 0xA200) {
            g_loader_seen = 1;
            ps3_log("loader entry pc=%04X f=%u fdc=%u\n", cur, (unsigned)dwFrameCountOverall, g_fdc_reads);
        }
    }
    int cyc_after = CPC.cycle_count;
    int cyc_delta = cyc_before - cyc_after;
    if (cyc_delta < 0) cyc_delta += CYCLE_COUNT_INIT;
    if (cyc_delta >= 0 && cyc_delta < CYCLE_COUNT_INIT) g_cyc_acc += (unsigned)cyc_delta;

    if (s_exit_condition == EC_SOUND_BUFFER) {
        return 0;
    }
    if (s_exit_condition == EC_FRAME_COMPLETE) {
        dwFrameCountOverall++;
        asic_draw_sprites();
        video_display();
        if (dwFrameCountOverall <= 100 && (dwFrameCountOverall % 5) == 0) {
            ps3_log("boot f=%u pc=%04X ink=%u,%u,%u ms=%u\n", (unsigned)dwFrameCountOverall,
                    (unsigned)(z80.PC.w.l & 0xFFFFu), (unsigned)GateArray.ink_values[0],
                    (unsigned)GateArray.ink_values[1], (unsigned)GateArray.ink_values[2],
                    (unsigned)SDL_GetTicks());
        }
        if ((dwFrameCountOverall % 250) == 0) {
            const byte *vrow = (const byte *)back_surface->pixels +
                               (size_t)(back_surface->h / 2) * back_surface->pitch;
            uint32_t blit_ms = 0, present_ms = 0;
            ps3_video_stats(&blit_ms, &present_ms);
            ps3_log("emu frames=%u ms=%u cyc/f=%u blit=%u pres=%u line=%u scrln=%d cyc=%d paused=%d pc=%04X model=%d bps=%d ink=%u,%u,%u px=%02X,%02X,%02X\n",
                    (unsigned)dwFrameCountOverall, (unsigned)SDL_GetTicks(),
                    (unsigned)(g_cyc_acc / (dwFrameCountOverall ? dwFrameCountOverall : 1)),
                    (unsigned)blit_ms, (unsigned)present_ms,
                    (unsigned)VDU.scanline, (int)VDU.scrln, (int)CPC.cycle_count,
                    CPC.paused ? 1 : 0,
                    (unsigned)(z80.PC.w.l & 0xFFFFu), (int)CPC.model,
                    (int)CPC.scr_bps, (unsigned)GateArray.ink_values[0],
                    (unsigned)GateArray.ink_values[1], (unsigned)GateArray.ink_values[2],
                    (unsigned)vrow[0], (unsigned)vrow[back_surface->w / 2],
                    (unsigned)vrow[back_surface->w - 1]);
            ps3_log("fdc reads=%u\n", g_fdc_reads);
        }
        return 1;
    }
    return 0;
}

static void splash_step(int pct, const char *stage, const char *rom_info, const char *disk_info, int hold_frames)
{
    for (int i = 0; i < hold_frames && !g_app_quit; i++) {
        ps3_sysutil_check();
        ps3_input_read(nullptr);
        ps3_ui_splash(pct, stage, rom_info, disk_info);
        ps3_video_present();
    }
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    ps3_boot_mark("main_enter");

    z80_init_tables();

    ps3_paths_init();
    ps3_log_init();
    ps3_log("CPC4PS3 %s boot\n", PS3_APP_VERSION);
    ps3_boot_mark("paths_ok");

    ps3_sysutil_register();
    ps3_input_init();
    ps3_boot_mark("input_ok");

    ps3_config_load();
    make_dir(PS3_ROMS_DIR);
    make_dir(PS3_DISKS_DIR);
    make_dir(PS3_TAPES_DIR);
    make_dir(PS3_CARTS_DIR);
    make_dir(PS3_SNAPS_DIR);
    make_dir(PS3_STATES_DIR);
    make_dir(PS3_SAVES_DIR);
    make_dir(PS3_SHOTS_DIR);
    ps3_boot_mark("config_loaded");

    if (ps3_video_init() != 0) {
        ps3_log("video subsystem failed\n");
        ps3_boot_mark("video_fail");
        return 1;
    }

    splash_step(18, "PREPARING AMSTRAD CPC HARDWARE...", nullptr, nullptr, 12);
    ps3_boot_mark("splash1_ok");

    snprintf(chAppPath, 256, "%s", PS3_ROOT);
    setup_cpc();
    ps3_boot_mark("setup_cpc_ok");

    char rom_desc[128];
    snprintf(rom_desc, sizeof(rom_desc), "ROM PACK: %s  (464 / 664 / 6128 / 6128+)", CPC.rom_path.c_str());
    splash_step(40, "INITIALIZING VIDEO RENDERER...", rom_desc, nullptr, 10);

    if (video_init() != 0) {
        ps3_log("core video_init failed\n");
        ps3_boot_mark("core_video_fail");
        show_fatal_error("Video initialization failed", "Could not allocate CPC video surface");
        ps3_video_shutdown();
        return 1;
    }
    ps3_boot_mark("core_video_ok");

    splash_step(62, "INITIALIZING AUDIO SYNTHESIZER (AY-3-8912)...", rom_desc, nullptr, 10);
    if (audio_init() != 0) {
        ps3_log("audio_init failed, sound disabled\n");
        ps3_boot_mark("audio_fail");
        CPC.snd_enabled = 0;
    } else {
        ps3_boot_mark("audio_ok");
    }

    splash_step(80, "LOADING BUNDLED SYSTEM ROMS...", rom_desc, nullptr, 10);
    if (emulator_init() != 0) {
        ps3_log("emulator_init failed\n");
        ps3_boot_mark("emu_init_fail");
        char err_msg[384];
        snprintf(err_msg, sizeof(err_msg), "Failed to load ROM from: %s", CPC.rom_path.c_str());
        show_fatal_error("Missing CPC ROMs", err_msg);
        ps3_video_shutdown();
        return 1;
    }
    if (CPC.scr_intensity < 5 || CPC.scr_intensity > 15) CPC.scr_intensity = 10;
    update_timings();
    video_set_palette();
    ps3_boot_mark("emu_init_ok");

    splash_step(92, "SCANNING /dev_hdd0/CPC/roms & USB MEDIA...", rom_desc, nullptr, 8);
    lib_refresh();
    ps3_boot_mark("lib_refresh_ok");
    browse_refresh(dir_exists(g_cfg.last_dir) ? g_cfg.last_dir : PS3_ROMS_DIR);
    ps3_boot_mark("browse_refresh_ok");

    char lib_desc[128];
    snprintf(lib_desc, sizeof(lib_desc), "%d MEDIA FILES FOUND  |  WRITABLE SAVES READY", (int)g_lib.size());
    splash_step(100, "READY! OPENING MENU...", rom_desc, lib_desc, 16);
    ps3_boot_mark("splash_final_ok");

    ps3_input_release_all();
    ps3_input_read(nullptr);
    ps3_video_set_title("CPC4PS3");

    g_tab = g_lib.empty() ? TAB_BROWSER : TAB_LIBRARY;
    ps3_boot_mark("menu_enter");
    menu_run();
    ps3_boot_mark("loop_enter");

    while (!g_app_quit) {
        ps3_sysutil_check();

        uint32_t pressed = 0;
        uint32_t cur = ps3_input_read(&pressed);

        if (pressed & PS3_ACT_MENU) {
            menu_run();
            continue;
        }
        if (pressed & PS3_ACT_L1) save_state(0);
        if (pressed & PS3_ACT_R1) load_state(0);
        if (pressed & PS3_ACT_L2) {
            run_vkbd();
            continue;
        }
        if (pressed & PS3_ACT_R3) {
            g_cfg.joystick_port ^= 1;
            toast(g_cfg.joystick_port ? "Joystick: Port B" : "Joystick: Port A");
        }

        ps3_input_apply_game(cur);

        if (!emulation_step()) {
            SDL_Delay(1);
            continue;
        }

        kbd_queue_tick();
    }

    flush_altered_disks();
    emulator_shutdown();
    ps3_input_shutdown();
    ps3_config_save();
    ps3_sysutil_unregister();
    ps3_video_shutdown();
    return 0;
}
