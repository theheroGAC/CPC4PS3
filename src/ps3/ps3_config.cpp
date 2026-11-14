#include "ps3_platform.h"

#include <sysutil/sysutil.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdarg.h>
#include <ctype.h>

ps3_config_t g_cfg;
volatile int g_app_quit = 0;

static FILE *g_log = nullptr;

static void boot_trace_write(const char *path, s32 flags, const char *msg)
{
    s32 fd = -1;
    if (sysLv2FsOpen(path, flags, &fd, 0666, NULL, 0) == 0 && fd >= 0) {
        if (flags & SYS_O_CREAT) {
            sysLv2FsChmod(path, 0666);
        }
        u64 written = 0;
        sysLv2FsWrite(fd, msg, strlen(msg), &written);
        sysLv2FsClose(fd);
    }
}

__attribute__((constructor(110))) static void ps3_early_ctor(void)
{
    boot_trace_write("/dev_hdd0/tmp/cpc4ps3_boot.log", SYS_O_WRONLY | SYS_O_CREAT | SYS_O_TRUNC, "ctor110\n");
    boot_trace_write("/dev_usb000/cpc4ps3_boot.log", SYS_O_WRONLY | SYS_O_CREAT | SYS_O_TRUNC, "ctor110\n");
}

void ps3_boot_mark(const char *msg)
{
    if (!msg) return;
    char line[256];
    snprintf(line, sizeof(line), "%s\n", msg);
    boot_trace_write("/dev_hdd0/tmp/cpc4ps3_boot.log", SYS_O_WRONLY | SYS_O_CREAT | SYS_O_APPEND, line);
    boot_trace_write("/dev_usb000/cpc4ps3_boot.log", SYS_O_WRONLY | SYS_O_CREAT | SYS_O_APPEND, line);
    if (g_log) {
        fputs(line, g_log);
        fflush(g_log);
    }
}

void ps3_log_init(void)
{
    g_log = fopen(PS3_LOG_PATH, "w");
    if (!g_log) g_log = fopen("/dev_hdd0/tmp/cpc4ps3.log", "w");
    if (g_log) fflush(g_log);
}

void ps3_log(const char *fmt, ...)
{
    if (!g_log) g_log = fopen(PS3_LOG_PATH, "a");
    if (!g_log) g_log = fopen("/dev_hdd0/tmp/cpc4ps3.log", "a");
    if (!g_log) return;
    va_list ap;
    va_start(ap, fmt);
    vfprintf(g_log, fmt, ap);
    va_end(ap);
    fflush(g_log);
}

static void mk(const char *p)
{
    mkdir(p, 0777);
}

static void media_readme(void)
{
    FILE *f = fopen(PS3_ROMS_DIR "/README.txt", "r");
    if (f) {
        fclose(f);
        return;
    }
    f = fopen(PS3_ROMS_DIR "/README.txt", "w");
    if (!f) return;
    fputs("CPC4PS3 media folder\n\nAmstrad CPC disk, tape, cartridge and snapshot images go here.\n\n  disks/      .dsk  .ipf  .zip\n  tapes/      .cdt  .voc\n  carts/      .cpr\n  snapshots/  .sna\n\nAll folders are reachable from the in-game file browser.\n", f);
    fclose(f);
}

void ps3_paths_init(void)
{
    mk(PS3_ROOT);
    mk(PS3_TMP_DIR);
    mk(PS3_ROMS_DIR);
    mk(PS3_DISKS_DIR);
    mk(PS3_TAPES_DIR);
    mk(PS3_CARTS_DIR);
    mk(PS3_SNAPS_DIR);
    mk(PS3_STATES_DIR);
    mk(PS3_SAVES_DIR);
    mk(PS3_SHOTS_DIR);
    media_readme();
}

static void sysutil_cb(uint64_t status, uint64_t param, void *userdata)
{
    (void)param;
    (void)userdata;
    if (status == SYSUTIL_EXIT_GAME) {
        g_app_quit = 1;
    }
}

void ps3_sysutil_register(void)
{
    sysUtilUnregisterCallback(SYSUTIL_EVENT_SLOT0);
    sysUtilUnregisterCallback(SYSUTIL_EVENT_SLOT1);
    sysUtilRegisterCallback(SYSUTIL_EVENT_SLOT0, sysutil_cb, nullptr);
}

void ps3_sysutil_check(void)
{
    sysUtilCheckCallback();
}

void ps3_sysutil_unregister(void)
{
    sysUtilUnregisterCallback(SYSUTIL_EVENT_SLOT0);
}

void ps3_config_defaults(void)
{
    memset(&g_cfg, 0, sizeof(g_cfg));
    g_cfg.model = 2;
    g_cfg.ram_size = 128;
    g_cfg.aspect = PS3_ASPECT_4_3;
    g_cfg.scanlines = 0;
    g_cfg.volume = 80;
    g_cfg.show_fps = 0;
    g_cfg.joystick_port = 0;
    g_cfg.green_screen = 0;
    g_cfg.auto_run = 1;
    g_cfg.auto_save_dsk = 1;
    g_cfg.vkbd_style = 0;
    g_cfg.vkbd_alpha = 2;
    g_cfg.vkbd_pos = 0;
    for (int i = 0; i < 10; i++) g_cfg.padmap[i] = -1;
    snprintf(g_cfg.last_dir, sizeof(g_cfg.last_dir), "%s", PS3_ROMS_DIR);
}

static void trim(char *s)
{
    size_t n = strlen(s);
    while (n > 0 && (s[n - 1] == '\r' || s[n - 1] == '\n' || isspace((unsigned char)s[n - 1]))) s[--n] = 0;
}

void ps3_config_load(void)
{
    ps3_config_defaults();
    FILE *f = fopen(PS3_CONFIG_PATH, "r");
    if (!f) return;

    char line[640];
    while (fgets(line, sizeof(line), f)) {
        trim(line);
        if (!line[0] || line[0] == '#' || line[0] == ';' || line[0] == '[') continue;
        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq = 0;
        char *k = line;
        char *v = eq + 1;
        while (*k && isspace((unsigned char)*k)) k++;
        while (*v && isspace((unsigned char)*v)) v++;
        trim(k);
        trim(v);
        if (!strcasecmp(k, "model")) g_cfg.model = atoi(v);
        else if (!strcasecmp(k, "ram_size")) g_cfg.ram_size = atoi(v);
        else if (!strcasecmp(k, "aspect")) g_cfg.aspect = atoi(v);
        else if (!strcasecmp(k, "scanlines")) g_cfg.scanlines = atoi(v) ? 1 : 0;
        else if (!strcasecmp(k, "volume")) g_cfg.volume = atoi(v);
        else if (!strcasecmp(k, "show_fps")) g_cfg.show_fps = atoi(v) ? 1 : 0;
        else if (!strcasecmp(k, "joystick_port")) g_cfg.joystick_port = atoi(v) ? 1 : 0;
        else if (!strcasecmp(k, "green_screen")) g_cfg.green_screen = atoi(v) ? 1 : 0;
        else if (!strcasecmp(k, "auto_run")) g_cfg.auto_run = atoi(v) ? 1 : 0;
        else if (!strcasecmp(k, "auto_save_dsk")) g_cfg.auto_save_dsk = atoi(v) ? 1 : 0;
        else if (!strcasecmp(k, "vkbd_style")) g_cfg.vkbd_style = atoi(v);
        else if (!strcasecmp(k, "vkbd_alpha")) g_cfg.vkbd_alpha = atoi(v);
        else if (!strcasecmp(k, "vkbd_pos")) g_cfg.vkbd_pos = atoi(v);
        else if (!strcasecmp(k, "padmap0")) g_cfg.padmap[0] = atoi(v);
        else if (!strcasecmp(k, "padmap1")) g_cfg.padmap[1] = atoi(v);
        else if (!strcasecmp(k, "padmap2")) g_cfg.padmap[2] = atoi(v);
        else if (!strcasecmp(k, "padmap3")) g_cfg.padmap[3] = atoi(v);
        else if (!strcasecmp(k, "padmap4")) g_cfg.padmap[4] = atoi(v);
        else if (!strcasecmp(k, "padmap5")) g_cfg.padmap[5] = atoi(v);
        else if (!strcasecmp(k, "padmap6")) g_cfg.padmap[6] = atoi(v);
        else if (!strcasecmp(k, "padmap7")) g_cfg.padmap[7] = atoi(v);
        else if (!strcasecmp(k, "padmap8")) g_cfg.padmap[8] = atoi(v);
        else if (!strcasecmp(k, "padmap9")) g_cfg.padmap[9] = atoi(v);
        else if (!strcasecmp(k, "last_dir")) snprintf(g_cfg.last_dir, sizeof(g_cfg.last_dir), "%s", v);
    }
    fclose(f);

    if (g_cfg.model < 0 || g_cfg.model > 3) g_cfg.model = 2;
    if (g_cfg.ram_size < 64 || g_cfg.ram_size > 576) g_cfg.ram_size = 128;
    if (g_cfg.aspect < 0 || g_cfg.aspect >= PS3_ASPECT_COUNT) g_cfg.aspect = PS3_ASPECT_4_3;
    if (g_cfg.volume < 0) g_cfg.volume = 0;
    if (g_cfg.volume > 100) g_cfg.volume = 100;
    if (g_cfg.vkbd_style < 0 || g_cfg.vkbd_style > 3) g_cfg.vkbd_style = 0;
    if (g_cfg.vkbd_alpha < 0 || g_cfg.vkbd_alpha > 3) g_cfg.vkbd_alpha = 2;
    if (g_cfg.vkbd_pos < 0 || g_cfg.vkbd_pos > 2) g_cfg.vkbd_pos = 0;
}

void ps3_config_save(void)
{
    FILE *f = fopen(PS3_CONFIG_PATH, "w");
    if (!f) return;
    fprintf(f, "[cpc4ps3]\n");
    fprintf(f, "model=%d\n", g_cfg.model);
    fprintf(f, "ram_size=%d\n", g_cfg.ram_size);
    fprintf(f, "aspect=%d\n", g_cfg.aspect);
    fprintf(f, "scanlines=%d\n", g_cfg.scanlines);
    fprintf(f, "volume=%d\n", g_cfg.volume);
    fprintf(f, "show_fps=%d\n", g_cfg.show_fps);
    fprintf(f, "joystick_port=%d\n", g_cfg.joystick_port);
    fprintf(f, "green_screen=%d\n", g_cfg.green_screen);
    fprintf(f, "auto_run=%d\n", g_cfg.auto_run);
    fprintf(f, "auto_save_dsk=%d\n", g_cfg.auto_save_dsk);
    fprintf(f, "vkbd_style=%d\n", g_cfg.vkbd_style);
    fprintf(f, "vkbd_alpha=%d\n", g_cfg.vkbd_alpha);
    fprintf(f, "vkbd_pos=%d\n", g_cfg.vkbd_pos);
    for (int i = 0; i < 10; i++) fprintf(f, "padmap%d=%d\n", i, g_cfg.padmap[i]);
    fprintf(f, "last_dir=%s\n", g_cfg.last_dir);
    fclose(f);
}
