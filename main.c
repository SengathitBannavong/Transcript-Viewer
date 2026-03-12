/*
 * main.c -- Student Transcript Viewer (entry point + orchestration)
 *
 * Build:   make
 *
 * File layout
 * -----------
 *   Struct_Table.h  -- Player / Subject_Node / Subject_Type structs
 *   app_data.h      -- gPlayer global + gTypeName[]
 *   db.h            -- SQLite backend (DB_Open/Close/Query/Update)
 *   cmd.h           -- command palette dispatcher
 *   ui.c            -- all Clay rendering functions
 *   main.c          -- globals, keyboard handler, layout root, main()
 *
 * ui.c is NOT compiled separately; it is #include'd here after all
 * globals are defined so it can reference them directly.
 */

#define CLAY_IMPLEMENTATION
#include "clay.h"
#include "raylib.h"
#include "clay_renderer_raylib.c"   /* Raylib rendering backend  */

#include "app_data.h"               /* gPlayer + gTypeName                */
#include "db.h"                     /* SQLite backend                     */
#include "score_logic.h"            /* CPA / graduation / alert logic     */

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

/* active sidebar item: index into gPlayer.numofSubjectType */
static int  gActiveNav  = 0;

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
static char  gFilterDept[64]  = {0};  /* kept for compat; not used in student view */
static char  gResultMsg[256]  = {0};  /* message shown in toast                    */

/* UI scale (loaded from assets/ui.cfg) */
static float gFontScale  = 1.8f;          /* multiplier for all font sizes         */

/* name-input screen state */
static bool  gNameInput  = true;      /* true = show name-input screen             */
static bool  gDBReady    = false;     /* true = DB loaded, show main UI            */
static char  gUserName[26] = {0};    /* entered username (max 25 chars + NUL)     */
static int   gNameLen    = 0;

/* --- Score/player refresh helper ------------------------------------- */
/* Calls DB_Query then recomputes graduation status + alert level.       */
static void RefreshPlayer(void)
{
    DB_Query(&gPlayer);
    update_player_status(&gPlayer);
}

/* --- Command dispatcher ----------------------------------------------- */
#include "cmd.h"

/* --- UI rendering layer (sees all globals above) ---------------------- */
#include "ui.c"

/* --- DB initializer (called when user confirms name on startup) -------- */
static void InitPlayerDB(void)
{
    bool isNew = !DB_Exists(gUserName);
    if (!DB_Open(gUserName)) {
        snprintf(gResultMsg, sizeof(gResultMsg),
                 "Error: cannot open db_%s.db", gUserName);
        gHasResult       = true;
        gResultShowUntil = (float)GetTime() + 5.f;
        return;
    }
    DB_CreateSchema();
    if (isNew) {
        DB_SeedFromDat();
        if (strcmp(gUserName, "test") == 0) DB_LoadScores_Test();
    }
    memset(&gPlayer, 0, sizeof(gPlayer));
    strncpy(gPlayer.name_player, gUserName, MAXSIZENAME - 1);
    RefreshPlayer();
    gDBReady   = true;
    gNameInput = false;
    char title[64];
    snprintf(title, sizeof(title), "Transcript Viewer  --  %s", gUserName);
    SetWindowTitle(title);
}

/* --- Keyboard handler -------------------------------------------------- */
static void HandleKeyboard(void)
{
    /* Name-input phase: capture username before anything else */
    if (gNameInput) {
        if (IsKeyPressed(KEY_ESCAPE)) { CloseWindow(); return; }
        if (IsKeyPressed(KEY_ENTER) && gNameLen > 0) { InitPlayerDB(); return; }
        if ((IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE))
                && gNameLen > 0)
            gUserName[--gNameLen] = '\0';
        int ch;
        while ((ch = GetCharPressed()) != 0)
            if (ch >= 32 && ch < 127 && gNameLen < 25) {
                gUserName[gNameLen++] = (char)ch;
                gUserName[gNameLen]   = '\0';
            }
        return;
    }

    /* F1 -- toggle Clay debug overlay */
    if (IsKeyPressed(KEY_F1)) {
        Clay_SetDebugModeEnabled(!Clay_IsDebugModeEnabled());
        return;
    }

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
        if (!gNameInput) {
            RenderSidebar();
            RenderMainContent();
        }
    }

    if (gNameInput) {
        RenderNameInput();
    } else {
        /* Floating overlays rendered on top via zIndex */
        if (gPopupOpen) RenderCommandPopup();
        if (gHasResult) RenderResultToast();
    }
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
    InitWindow(WIN_W, WIN_H, "Transcript Viewer");
    SetTargetFPS(60);

    /* Font loading: read paths from assets/fonts.cfg, fall back to Raylib default */
#define MAX_FONT_ENTRIES 32
#define MAX_FONT_PATH    512
    static char fontPathBuf[MAX_FONT_ENTRIES][MAX_FONT_PATH];
    const char *fontCandidates[MAX_FONT_ENTRIES + 1];
    int numFontCandidates = 0;

    FILE *fontCfg = fopen("assets/fonts.cfg", "r");
    if (fontCfg) {
        char line[MAX_FONT_PATH];
        while (fgets(line, sizeof(line), fontCfg) && numFontCandidates < MAX_FONT_ENTRIES) {
            /* strip newline */
            int ll = (int)strlen(line);
            while (ll > 0 && (line[ll-1] == '\n' || line[ll-1] == '\r' || line[ll-1] == ' '))
                line[--ll] = '\0';
            if (ll == 0 || line[0] == '#') continue;  /* blank / comment */
            snprintf(fontPathBuf[numFontCandidates], MAX_FONT_PATH, "%s", line);
            fontCandidates[numFontCandidates] = fontPathBuf[numFontCandidates];
            numFontCandidates++;
        }
        fclose(fontCfg);
    }
    // if no font found: show error on screen and exit
    if (numFontCandidates == 0) {
        fprintf(stderr,
            "[font] No font paths found in assets/fonts.cfg.\n"
            "       Add at least one valid .ttf path to that file and restart.\n");
        const char *msg1 = "No font found — cannot start.";
        const char *msg2 = "Add a .ttf path to  assets/fonts.cfg  and restart.";
        const char *msg3 = "Press any key or close this window to exit.";
        SetTargetFPS(60);
        while (!WindowShouldClose() && !GetKeyPressed()) {
            BeginDrawing();
            ClearBackground((Color){ 11, 11, 20, 255 });
            DrawText(msg1, 40, 120, 28, (Color){ 239, 68, 68, 255 });
            DrawText(msg2, 40, 165, 20, (Color){ 224, 224, 242, 255 });
            DrawText(msg3, 40, 210, 16, (Color){ 110, 110, 155, 255 });
            EndDrawing();
        }
        CloseWindow();
        return 1;
    }
    fontCandidates[numFontCandidates] = NULL;  /* sentinel */

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

    /* ── UI config: load font_scale from assets/ui.cfg ─────────────── */
    {
        FILE *uiCfg = fopen("assets/ui.cfg", "r");
        if (uiCfg) {
            char line[256];
            while (fgets(line, sizeof(line), uiCfg)) {
                if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;
                char key[64]; float val;
                if (sscanf(line, "%63s %f", key, &val) == 2) {
                    if (strcmp(key, "font_scale") == 0 && val > 0.1f && val < 10.f)
                        gFontScale = val;
                }
            }
            fclose(uiCfg);
        }
    }

    /* ── SQLite: opened via InitPlayerDB() after user enters name ───── */
    memset(&gPlayer, 0, sizeof(gPlayer));

    /* Clay initialisation */
    size_t memSize = Clay_MinMemorySize();
    void  *mem     = malloc(memSize);
    Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(memSize, mem);
    Clay_Initialize(arena,
                    (Clay_Dimensions){ (float)WIN_W, (float)WIN_H },
                    (Clay_ErrorHandler){ HandleClayError, NULL });
    Clay_SetMeasureTextFunction(Raylib_MeasureText, gFonts);
    Clay_SetDebugModeEnabled(false);  /* F1 toggles at runtime */

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
    DB_Close();
    Clay_Raylib_Close();
    return 0;
}
