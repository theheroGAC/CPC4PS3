#include "ps3_platform.h"

#include <SDL.h>
#include <audio/audio.h>
#include <sys/thread.h>
#include <sys/systime.h>
#include <sys/event_queue.h>

#include <stdlib.h>
#include <string.h>

extern void audio_update(void *userdata, unsigned char *stream, int len);
extern uint32_t dwSndBufferCopied;

static u32 g_port = 0xFFFFFFFFu;
static audioPortConfig g_port_cfg;
static sys_ppu_thread_t g_tid = 0;
static sys_event_queue_t g_eq = 0;
static sys_ipc_key_t g_key = 0;
static volatile int g_run = 0;
static volatile int g_paused = 1;
static int g_ready = 0;
static int g_blocks = 0;
static int g_write_block = 4;
static int g_blocks_per_copy = 4;
static int16_t *g_stage = nullptr;
static int g_stage_frames = 0;
static float *g_ring = nullptr;
static int g_stream_logged = 0;

static void audio_thread_entry(void *arg)
{
    (void)arg;
    if (!g_ring) {
        ps3_log("audio: ring buffer missing\n");
        sysThreadExit(0);
    }

    while (g_run) {
        sys_event_t ev;
        if (sysEventQueueReceive(g_eq, &ev, 25000) != 0) {
            dwSndBufferCopied = 1;
            continue;
        }
        if (++g_blocks >= g_blocks_per_copy) {
            g_blocks = 0;
            float *dst = g_ring + (size_t)g_write_block * PS3_AUDIO_BLOCK * 2;
            if (g_paused || !g_stage) {
                memset(dst, 0, (size_t)g_blocks_per_copy * PS3_AUDIO_BLOCK * 2 * sizeof(float));
            } else {
                audio_update(nullptr, (unsigned char *)g_stage, g_stage_frames * 4);
                const float scale = 1.0f / 32768.0f;
                int n = g_stage_frames * 2;
                for (int i = 0; i < n; i++) dst[i] = (float)g_stage[i] * scale;
                if (!g_stream_logged) {
                    ps3_log("audio: streaming\n");
                    g_stream_logged = 1;
                }
            }
            dwSndBufferCopied = 1;
            g_write_block = (g_write_block + g_blocks_per_copy) % AUDIO_BLOCK_8;
        }
    }
    sysThreadExit(0);
}

static void audio_close_port(void)
{
    if (g_port != 0xFFFFFFFFu) {
        audioPortStop(g_port);
    }
    if (g_key) {
        audioRemoveNotifyEventQueue(g_key);
        g_key = 0;
    }
    if (g_eq) {
        sysEventQueueDestroy(g_eq, 0);
        g_eq = 0;
    }
    if (g_port != 0xFFFFFFFFu) {
        audioPortClose(g_port);
        g_port = 0xFFFFFFFFu;
    }
    audioQuit();
}

SDL_AudioDeviceID ps3_cell_open_audio(const char *device, int iscapture, const SDL_AudioSpec *desired, SDL_AudioSpec *obtained, int allowed_changes)
{
    (void)device;
    (void)iscapture;
    (void)allowed_changes;

    if (g_ready) return 1;
    if (!desired || !obtained) return 0;

    if (audioInit() != 0) {
        ps3_log("audioInit failed\n");
        return 0;
    }

    audioPortParam param;
    memset(&param, 0, sizeof(param));
    param.numChannels = AUDIO_PORT_2CH;
    param.numBlocks = AUDIO_BLOCK_8;
    param.attrib = AUDIO_PORT_INITLEVEL;
    param.level = 1.0f;

    if (audioPortOpen(&param, &g_port) != 0) {
        ps3_log("audioPortOpen failed\n");
        audioQuit();
        return 0;
    }

    if (audioCreateNotifyEventQueue(&g_eq, &g_key) != 0) {
        ps3_log("audioCreateNotifyEventQueue failed\n");
        audio_close_port();
        return 0;
    }

    if (audioSetNotifyEventQueue(g_key) != 0) {
        ps3_log("audioSetNotifyEventQueue failed\n");
        audio_close_port();
        return 0;
    }

    if (audioPortStart(g_port) != 0) {
        ps3_log("audioPortStart failed\n");
        audio_close_port();
        return 0;
    }

    if (audioGetPortConfig(g_port, &g_port_cfg) != 0 || !g_port_cfg.audioDataStart) {
        ps3_log("audio: ring buffer unavailable\n");
        audio_close_port();
        return 0;
    }
    g_ring = (float *)(uintptr_t)g_port_cfg.audioDataStart;
    memset(g_ring, 0, AUDIO_BLOCK_8 * PS3_AUDIO_BLOCK * 2 * sizeof(float));
    sysEventQueueDrain(g_eq);

    *obtained = *desired;
    obtained->size = (u32)(desired->samples * desired->channels * 2);

    g_blocks_per_copy = (int)(obtained->size / (PS3_AUDIO_BLOCK * 4));
    if (g_blocks_per_copy < 1) g_blocks_per_copy = 1;
    if (g_blocks_per_copy > 8) g_blocks_per_copy = 8;

    g_stage_frames = (int)(obtained->size / 4);
    g_stage = (int16_t *)malloc((size_t)obtained->size);
    if (!g_stage) {
        audio_close_port();
        return 0;
    }
    memset(g_stage, 0, obtained->size);

    g_blocks = 0;
    g_write_block = 4 % AUDIO_BLOCK_8;
    g_paused = 1;
    g_stream_logged = 0;
    g_run = 1;

    if (sysThreadCreate(&g_tid, audio_thread_entry, nullptr, 1500, 0x10000, THREAD_JOINABLE, "cpc_audio") != 0) {
        ps3_log("audio thread create failed\n");
        g_run = 0;
        g_tid = 0;
        free(g_stage);
        g_stage = nullptr;
        audio_close_port();
        return 0;
    }
    g_ready = 1;

    ps3_log("cellAudio ready (port=%u size=%u)\n", (unsigned)g_port, (unsigned)obtained->size);
    return 1;
}

void ps3_cell_close_audio(SDL_AudioDeviceID id)
{
    (void)id;
    if (!g_ready) return;
    g_run = 0;
    if (g_tid) {
        u64 retval = 0;
        sysThreadJoin(g_tid, &retval);
        g_tid = 0;
    }
    audio_close_port();
    if (g_stage) {
        free(g_stage);
        g_stage = nullptr;
    }
    g_ring = nullptr;
    g_ready = 0;
}

void ps3_cell_pause_audio_device(SDL_AudioDeviceID id, int pause_on)
{
    (void)id;
    g_paused = pause_on ? 1 : 0;
    if (g_paused) dwSndBufferCopied = 1;
}

void ps3_cell_pause_audio(int pause_on)
{
    g_paused = pause_on ? 1 : 0;
    if (g_paused) dwSndBufferCopied = 1;
}

void ps3_audio_pause(int paused)
{
    g_paused = paused ? 1 : 0;
    if (g_paused) dwSndBufferCopied = 1;
}
