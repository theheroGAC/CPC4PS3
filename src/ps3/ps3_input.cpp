#include "ps3_platform.h"

#include <io/pad.h>
#include <string.h>

#include "cap32.h"
#include "keyboard.h"

extern byte keyboard_matrix[16];
extern t_CPC CPC;

static padData g_last;
static int g_have_pad = 0;
static uint32_t g_prev = 0;
static uint32_t g_game_prev = 0;
static uint32_t g_rep = 0;

void ps3_input_init(void)
{
    ioPadInit(7);
    memset(&g_last, 0, sizeof(g_last));
    g_have_pad = 0;
    g_prev = 0;
    g_game_prev = 0;
    g_rep = 0;
}

void ps3_input_shutdown(void)
{
    ioPadEnd();
}

static int read_pad(void)
{
    padInfo info;
    padData tmp;
    if (ioPadGetInfo(&info) != 0) return 0;
    for (int p = 0; p < 7; p++) {
        if (!info.status[p]) {
            if (p == 0) {
                memset(&g_last, 0, sizeof(g_last));
                g_have_pad = 0;
            }
            continue;
        }
        if (ioPadGetData(p, &tmp) == 0 && tmp.len >= 8) {
            g_last = tmp;
            g_have_pad = 1;
        }
        if (g_have_pad) return 1;
    }
    return g_have_pad;
}

uint32_t ps3_input_read(uint32_t *pressed)
{
    uint32_t cur = 0;

    if (read_pad() && g_last.len) {
        if (g_last.BTN_UP || g_last.ANA_L_V < 0x40) cur |= PS3_ACT_UP;
        if (g_last.BTN_DOWN || g_last.ANA_L_V > 0xC0) cur |= PS3_ACT_DOWN;
        if (g_last.BTN_LEFT || g_last.ANA_L_H < 0x40) cur |= PS3_ACT_LEFT;
        if (g_last.BTN_RIGHT || g_last.ANA_L_H > 0xC0) cur |= PS3_ACT_RIGHT;
        if (g_last.BTN_CROSS) cur |= PS3_ACT_CONFIRM;
        if (g_last.BTN_CIRCLE) cur |= PS3_ACT_CANCEL;
        if (g_last.BTN_SQUARE) cur |= PS3_ACT_SQUARE;
        if (g_last.BTN_TRIANGLE) cur |= PS3_ACT_TRIANGLE;
        if (g_last.BTN_L1) cur |= PS3_ACT_L1;
        if (g_last.BTN_R1) cur |= PS3_ACT_R1;
        if (g_last.BTN_L2) cur |= PS3_ACT_L2;
        if (g_last.BTN_R2) cur |= PS3_ACT_R2;
        if (g_last.BTN_START) cur |= PS3_ACT_START;
        if (g_last.BTN_SELECT) cur |= PS3_ACT_SELECT;
        if (g_last.BTN_L3) cur |= PS3_ACT_L3;
        if (g_last.BTN_R3) cur |= PS3_ACT_R3;
        if (g_last.BTN_SELECT || (g_last.BTN_L3 && g_last.BTN_R3)) cur |= PS3_ACT_MENU;
    }

    uint32_t newly = cur & ~g_prev;
    uint32_t dirs = cur & (PS3_ACT_UP | PS3_ACT_DOWN | PS3_ACT_LEFT | PS3_ACT_RIGHT);
    if (dirs && dirs == (g_prev & (PS3_ACT_UP | PS3_ACT_DOWN | PS3_ACT_LEFT | PS3_ACT_RIGHT))) {
        g_rep++;
        if (g_rep > 10) {
            if (g_rep > 22) newly |= dirs;
            else if ((g_rep & 1u) == 0) newly |= dirs;
        }
    } else {
        g_rep = 0;
    }
    g_prev = cur;
    if (pressed) *pressed = newly;
    return cur;
}

void ps3_input_apply_game(uint32_t cur)
{
    static const uint32_t kPadMasks[10] = {
        PS3_ACT_UP, PS3_ACT_DOWN, PS3_ACT_LEFT, PS3_ACT_RIGHT,
        PS3_ACT_CONFIRM, PS3_ACT_CANCEL, PS3_ACT_SQUARE, PS3_ACT_TRIANGLE,
        PS3_ACT_START, PS3_ACT_R2
    };
    uint32_t ch = cur ^ g_game_prev;
    if (!ch) return;
    uint32_t mapped = 0;
    for (int i = 0; i < 10; i++) {
        int v = g_cfg.padmap[i];
        if (v < 0) continue;
        uint32_t m = kPadMasks[i];
        if (ch & m) {
            bool dn = (cur & m) != 0;
            CPCScancode sc = CPC.InputMapper->CPCscancodeFromCPCkey((CPC_KEYS)(v & 0xffff));
            if ((sc & 0xff) != 0xff) {
                if (v & 0x10000) sc |= MOD_CPC_SHIFT;
                if (v & 0x20000) sc |= MOD_CPC_CTRL;
                applyKeypress(sc, keyboard_matrix, dn, (sc & (MOD_CPC_SHIFT | MOD_CPC_CTRL)) != 0);
            }
            mapped |= m;
        }
    }
    ch &= ~mapped;

    const CPC_KEYS up = (g_cfg.joystick_port == 1) ? CPC_J1_UP : CPC_J0_UP;
    const CPC_KEYS down = (g_cfg.joystick_port == 1) ? CPC_J1_DOWN : CPC_J0_DOWN;
    const CPC_KEYS left = (g_cfg.joystick_port == 1) ? CPC_J1_LEFT : CPC_J0_LEFT;
    const CPC_KEYS right = (g_cfg.joystick_port == 1) ? CPC_J1_RIGHT : CPC_J0_RIGHT;
    const CPC_KEYS fire1 = (g_cfg.joystick_port == 1) ? CPC_J1_FIRE1 : CPC_J0_FIRE1;
    const CPC_KEYS fire2 = (g_cfg.joystick_port == 1) ? CPC_J1_FIRE2 : CPC_J0_FIRE2;

    if (ch & PS3_ACT_UP) {
        bool dn = (cur & PS3_ACT_UP) != 0;
        applyKeypress(CPC.InputMapper->CPCscancodeFromCPCkey(up), keyboard_matrix, dn, false);
        applyKeypress(CPC.InputMapper->CPCscancodeFromCPCkey(CPC_CUR_UP), keyboard_matrix, dn, false);
    }
    if (ch & PS3_ACT_DOWN) {
        bool dn = (cur & PS3_ACT_DOWN) != 0;
        applyKeypress(CPC.InputMapper->CPCscancodeFromCPCkey(down), keyboard_matrix, dn, false);
        applyKeypress(CPC.InputMapper->CPCscancodeFromCPCkey(CPC_CUR_DOWN), keyboard_matrix, dn, false);
    }
    if (ch & PS3_ACT_LEFT) {
        bool dn = (cur & PS3_ACT_LEFT) != 0;
        applyKeypress(CPC.InputMapper->CPCscancodeFromCPCkey(left), keyboard_matrix, dn, false);
        applyKeypress(CPC.InputMapper->CPCscancodeFromCPCkey(CPC_CUR_LEFT), keyboard_matrix, dn, false);
    }
    if (ch & PS3_ACT_RIGHT) {
        bool dn = (cur & PS3_ACT_RIGHT) != 0;
        applyKeypress(CPC.InputMapper->CPCscancodeFromCPCkey(right), keyboard_matrix, dn, false);
        applyKeypress(CPC.InputMapper->CPCscancodeFromCPCkey(CPC_CUR_RIGHT), keyboard_matrix, dn, false);
    }
    if (ch & PS3_ACT_CONFIRM) applyKeypress(CPC.InputMapper->CPCscancodeFromCPCkey(fire1), keyboard_matrix, (cur & PS3_ACT_CONFIRM) != 0, false);
    if (ch & PS3_ACT_CANCEL) applyKeypress(CPC.InputMapper->CPCscancodeFromCPCkey(fire2), keyboard_matrix, (cur & PS3_ACT_CANCEL) != 0, false);
    if (ch & PS3_ACT_SQUARE) applyKeypress(CPC.InputMapper->CPCscancodeFromCPCkey(CPC_SPACE), keyboard_matrix, (cur & PS3_ACT_SQUARE) != 0, false);
    if (ch & PS3_ACT_TRIANGLE) applyKeypress(CPC.InputMapper->CPCscancodeFromCPCkey(CPC_1), keyboard_matrix, (cur & PS3_ACT_TRIANGLE) != 0, false);
    if (ch & PS3_ACT_START) applyKeypress(CPC.InputMapper->CPCscancodeFromCPCkey(CPC_RETURN), keyboard_matrix, (cur & PS3_ACT_START) != 0, false);
    if (ch & PS3_ACT_R2) applyKeypress(CPC.InputMapper->CPCscancodeFromCPCkey(CPC_ESC), keyboard_matrix, (cur & PS3_ACT_R2) != 0, false);

    g_game_prev = cur;
}

void ps3_input_release_all(void)
{
    memset(keyboard_matrix, 0xff, 16);
    g_game_prev = 0;
}
