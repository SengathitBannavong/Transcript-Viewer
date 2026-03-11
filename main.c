/*
 * main.c -- Clay DB Viewer (entry point + orchestration)
 *
 * Build:   make
 *
 * File layout
 * -----------
 *   app_data.h  -- Employee struct & static dataset
 *   ui.c        -- colors, helpers, all Clay Render* functions
 *   main.c      -- globals, keyboard handling, layout root, main()
 *
 * ui.c is NOT compiled separately; it is #include'd here after all
 * globals are defined so it can reference them directly.
 */

#define CLAY_IMPLEMENTATION
#include "clay.h"
#include "raylib.h"
#include "clay_renderer_raylib.c"   /* Raylib rendering backend  */

#include "app_data.h"               /* Employee + gEmployees + EMP_COUNT */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

/* --- Window constants ------------------------------------------------- */
#define WIN_W  1400
#define WIN_H   820

/* --- Globals (read/written by ui.c) ----------------------------------- */
static Font gFonts[1];
static bool gCustomFont = false;

/* active sidebar item (0-4); ui.c mutates this on click */
static int  gActiveNav  = 1;

/* current window size; updated every frame for correct backdrop sizing */
static int  gScreenW    = WIN_W;
static int  gScreenH    = WIN_H;

/* per-frame dynamic string arena */
#define DYN_BUF_SIZE 32768
static char gDynBuf[DYN_BUF_SIZE];
static int  gDynPos;

/* command palette state */
static bool  gPopupOpen       = false;
static char  gCmdBuf[256]     = {0};  /* text being typed            */
static int   gCmdLen          = 0;
static bool  gHasResult       = false;
static float gResultShowUntil = -1.f; /* GetTime() expiry for toast  */

/* command execution results (written by ExecuteCommand in cmd.h) */
static char  gFilterDept[64]  = {0};  /* active dept filter; "" = all */
static char  gResultMsg[256]  = {0};  /* message shown in toast       */

/* --- Command dispatcher (includes easyargs.h) ------------------------- */
#include "cmd.h"

/* --- UI rendering layer (sees all globals above) ---------------------- */
#include "ui.c"

/* --- Keyboard handler -------------------------------------------------- */
static void HandleKeyboard(void)
{
    /* Ctrl+K -- toggle palette */
    if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_K)) {
        gPopupOpen = !gPopupOpen;
        if (!gPopupOpen) { gCmdLen = 0; gCmdBuf[0] = '\0'; }
        return;
    }

    if (!gPopupOpen) return;

    /* Escape -- dismiss */
    if (IsKeyPressed(KEY_ESCAPE)) {
        gPopupOpen = false;
        gCmdLen    = 0;
        gCmdBuf[0] = '\0';
        return;
    }

    /* Enter -- submit */
    if (IsKeyPressed(KEY_ENTER) && gCmdLen > 0) {
        ExecuteCommand(gCmdBuf, &gActiveNav, gFilterDept, gResultMsg, sizeof(gResultMsg));
        gHasResult       = true;
        gResultShowUntil = (float)GetTime() + 5.f;
        gPopupOpen       = false;
        gCmdLen          = 0;
        gCmdBuf[0]       = '\0';
        return;
    }

    /* Backspace (with key-repeat) */
    if ((IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE))
            && gCmdLen > 0) {
        gCmdBuf[--gCmdLen] = '\0';
    }

    /* Printable ASCII */
    int ch;
    while ((ch = GetCharPressed()) != 0) {
        if (ch >= 32 && ch < 127 && gCmdLen < 253) {
            gCmdBuf[gCmdLen++] = (char)ch;
            gCmdBuf[gCmdLen]   = '\0';
        }
    }
}

/* --- Root layout ------------------------------------------------------- */
static void BuildLayout(void)
{
    CLAY(CLAY_ID("Root"), {
        .layout = {
            .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
        },
        .backgroundColor = C_BG,
    }) {
        RenderSidebar();
        RenderMainContent();
    }

    /* Floating overlays rendered on top via zIndex */
    if (gPopupOpen) RenderCommandPopup();
    if (gHasResult) RenderResultToast();
}

/* --- Clay error handler ------------------------------------------------ */
static void HandleClayError(Clay_ErrorData err)
{
    fprintf(stderr, "[Clay error %d] %.*s\n",
            err.errorType, err.errorText.length, err.errorText.chars);
}

/* --- Entry point ------------------------------------------------------- */
int main(void)
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(WIN_W, WIN_H, "Clay DB Viewer  --  Employee Dashboard");
    SetTargetFPS(60);

    /* Font loading: try bundled fonts first, fall back to Raylib default */
    const char *fontCandidates[] = {
        "Font/Space_Mono/SpaceMono-Bold.ttf",          /* bundled         */
        "Font/Quicksand/static/Quicksand-Regular.ttf",
        "Font/Outfit/static/Outfit-Regular.ttf",
        "/usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
        "/usr/share/fonts/opentype/noto/NotoSans-Regular.ttf",
        NULL
    };
    gFonts[0] = (Font){0};
    for (int i = 0; fontCandidates[i]; i++) {
        if (FileExists(fontCandidates[i])) {
            Font f = LoadFontEx(fontCandidates[i], 72, NULL, 250);
            if (f.texture.id != 0) {
                gFonts[0]   = f;
                gCustomFont = true;
                SetTextureFilter(gFonts[0].texture, TEXTURE_FILTER_BILINEAR);
                break;
            }
        }
    }
    if (!gCustomFont) gFonts[0] = GetFontDefault();

    /* Clay initialisation */
    size_t memSize = Clay_MinMemorySize();
    void  *mem     = malloc(memSize);
    Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(memSize, mem);
    Clay_Initialize(arena,
                    (Clay_Dimensions){ (float)WIN_W, (float)WIN_H },
                    (Clay_ErrorHandler){ HandleClayError, NULL });
    Clay_SetMeasureTextFunction(Raylib_MeasureText, gFonts);

    /* Main loop */
    while (!WindowShouldClose()) {
        gScreenW = GetScreenWidth();
        gScreenH = GetScreenHeight();

        Clay_SetLayoutDimensions((Clay_Dimensions){ (float)gScreenW, (float)gScreenH });
        Clay_SetPointerState(
            (Clay_Vector2){ (float)GetMouseX(), (float)GetMouseY() },
            IsMouseButtonDown(MOUSE_LEFT_BUTTON));
        Clay_UpdateScrollContainers(
            true,
            (Clay_Vector2){ 0.f, -GetMouseWheelMove() * 50.f },
            GetFrameTime());

        HandleKeyboard();

        /* expire toast */
        if (gHasResult && (float)GetTime() >= gResultShowUntil)
            gHasResult = false;

        gDynPos = 0;  /* reset per-frame string arena */

        Clay_BeginLayout();
        BuildLayout();
        Clay_RenderCommandArray cmds = Clay_EndLayout();

        BeginDrawing();
        ClearBackground((Color){ 11, 11, 20, 255 });
        Clay_Raylib_Render(cmds, gFonts);
        /* FPS counter */
        char fps[32];
        snprintf(fps, sizeof(fps), "FPS: %d", GetFPS());
        DrawTextEx(gFonts[0], fps,
                   (Vector2){ (float)(gScreenW - 80), 6.f }, 14.f, 1.f,
                   (Color){ 80, 80, 120, 180 });
        EndDrawing();
    }

    if (gCustomFont) UnloadFont(gFonts[0]);
    free(mem);
    Clay_Raylib_Close();
    return 0;
}
