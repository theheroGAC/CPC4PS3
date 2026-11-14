#include "ps3_platform.h"
#include "cap32.h"

extern "C" {

void ps3_sdl_render_cp(const char *tag)
{
    (void)tag;
}

void ps3_sdl_flip_timeout(unsigned int ms)
{
    (void)ms;
}

void ps3_sdl_fifo_blocked(unsigned int ms)
{
    (void)ms;
}

}

void ShowCursor(bool show)
{
    (void)show;
}

void cleanExit(int returnCode, bool askIfUnsaved)
{
    (void)returnCode;
    (void)askIfUnsaved;
    g_app_quit = 1;
}
