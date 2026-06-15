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

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>  /* WebAssembly build drives the frame loop */
#endif

/* --- Window constants ------------------------------------------------- */
#define WIN_W  1400
#define WIN_H   820

/* --- Globals (read/written by ui.c) ----------------------------------- */
static Font gFonts[1];
static bool gCustomFont = false;

/* active sidebar item: index into gPlayer.numofSubjectType */
static int  gActiveNav  = 1;

/* current window size; updated every frame for correct backdrop sizing */
static int  gScreenW    = WIN_W;
static int  gScreenH    = WIN_H;

/* responsive state — recomputed each frame from gScreenW (see UpdateDrawFrame) */
static bool gIsMobile   = false;  /* gScreenW < BP_MOBILE                         */
static bool gDrawerOpen = false;  /* mobile sidebar drawer visibility            */
static bool gRowHover   = false;  /* a table row is hovered this frame → pointer */
static bool gIsTouch    = false;  /* touch device (web only) → HTML keyboard bridge */

/* per-frame dynamic string arena */
#define DYN_BUF_SIZE 32768
static char gDynBuf[DYN_BUF_SIZE];
static int  gDynPos;

/* command palette state */
static bool  gPopupOpen       = false;
static char  gCmdBuf[256]     = {0};  /* text being typed            */
static int   gCmdLen          = 0;

/* row-edit popup state */
static bool  gEditOpen        = false;
static char  gEditCode[MAXSIZEID] = {0};  /* subject code being edited     */
static char  gEditMidBuf[32]  = {0};
static int   gEditMidLen      = 0;
static char  gEditFinBuf[32]  = {0};
static int   gEditFinLen      = 0;
static int   gEditField       = 0;    /* 0 = mid, 1 = final            */
static int   gEditRatio       = 3;    /* 1=50/50  2=40/60  3=30/70     */
static char  gEditSubjectName[MAXSIZENAME] = {0};
static char  gEditError[64]   = {0};  /* inline popup validation msg ("" = none) */
static bool  gHasResult       = false;
static float gResultShowUntil = -1.f; /* GetTime() expiry for toast  */

/* command execution results (written by ExecuteCommand in cmd.h) */
static char  gFilterDept[64]  = {0};  /* kept for compat; not used in student view */
static char  gResultMsg[256]  = {0};  /* message shown in toast                    */

/* UI scale (loaded from assets/ui.cfg) */
static float gFontScale  = 1.8f;          /* multiplier for all font sizes         */
static int   gTargetFPS  = 60;            /* target FPS (60–240, set in ui.cfg)    */

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
    for(int i = 0; i < sizeSubjectType; i++)
      gTypeName[i][0] = 0;
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
    DB_SeedGradRules();   /* INSERT OR IGNORE — safe for existing DBs too */
    DB_LoadGradRules();
    DB_ApplyMinPassRule();/* migrate pre-rule grades: sub-3 mid/final → F */
    DB_ValidateData();
    /* Auto-reload: if validation found problems, re-parse the asset files so
     * that any fixes the user made since the last run are picked up.       */
    if (gDataWarnCount > 0) DB_ReloadData();
    memset(&gPlayer, 0, sizeof(gPlayer));
    strncpy(gPlayer.name_player, gUserName, MAXSIZENAME - 1);
    RefreshPlayer();
    gDBReady   = true;
    gNameInput = false;
    DB_Persist();   /* web: persist a freshly seeded DB right away */
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

    /* Edit popup takes input priority after F1 */
    if (gEditOpen) {
        if (IsKeyPressed(KEY_ESCAPE)) {
            gEditError[0] = '\0';
            gEditOpen = false;
            return;
        }
        /* Enter saves (validation + persist handled by EditTrySave) */
        if (IsKeyPressed(KEY_ENTER)) {
            EditTrySave();
            return;
        }
        /* Tab cycles between fields */
        if (IsKeyPressed(KEY_TAB)) {
            gEditField = (gEditField + 1) % 2;
            return;
        }
        /* Digit / dot / backspace routed through the shared keypad helper */
        if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE))
            EditKeyInput(8);
        int ch;
        while ((ch = GetCharPressed()) != 0)
            EditKeyInput(ch);
        return;
    }

    /* Ctrl+K -- toggle palette */
    if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_K)) {
        gPopupOpen = !gPopupOpen;
        if (!gPopupOpen) { gCmdLen = 0; gCmdBuf[0] = '\0'; }
        return;
    }

    if (gPopupOpen && gEditOpen) gPopupOpen = false;
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
            /* mobile: top bar above content; desktop: sidebar beside content */
            .layoutDirection = gIsMobile ? CLAY_TOP_TO_BOTTOM : CLAY_LEFT_TO_RIGHT,
        },
        .backgroundColor = C_BG,
    }) {
        if (!gNameInput) {
            if (gIsMobile) RenderTopBar();
            else           RenderSidebar();
            if (gActiveNav == 0)
                RenderDashboard();
            else
                RenderMainContent();
        }
    }

    if (gNameInput) {
        RenderNameInput();
    } else {
        /* Floating overlays rendered on top via zIndex */
        if (gIsMobile && gDrawerOpen) RenderDrawer();
        if (gEditOpen)   RenderEditPopup();
        if (gPopupOpen)  RenderCommandPopup();
        if (gHasResult)  RenderResultToast();
    }
}

/* --- Clay error handler ------------------------------------------------ */
static void HandleClayError(Clay_ErrorData err)
{
    fprintf(stderr, "[Clay error %d] %.*s\n",
            err.errorType, err.errorText.length, err.errorText.chars);
}

/* --- Entry point ------------------------------------------------------- */
/* One frame of update + render. Defined after main(); on web it is invoked
 * by emscripten's requestAnimationFrame loop, on desktop by the while-loop. */
static void UpdateDrawFrame(void);

int main(void)
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(WIN_W, WIN_H, "Transcript Viewer");
    SetTargetFPS(gTargetFPS);  /* will be updated after ui.cfg loads */

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
            ClearBackground((Color){ 241, 237, 229, 255 });
            DrawText(msg1, 40, 120, 28, (Color){ 178, 52, 42, 255 });
            DrawText(msg2, 40, 165, 20, (Color){ 27, 26, 23, 255 });
            DrawText(msg3, 40, 210, 16, (Color){ 122, 116, 104, 255 });
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
                    if (strcmp(key, "target_fps") == 0) {
                        int fps = (int)val;
                        if (fps < 60)  fps = 60;
                        if (fps > 240) fps = 240;
                        gTargetFPS = fps;
                    }
                }
            }
            fclose(uiCfg);
        }
        SetTargetFPS(gTargetFPS);  /* apply after ui.cfg parsed */
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

    /* Web: mount IndexedDB-backed storage and load any saved DB before the
     * user can open one. No-op on desktop. */
    DB_PersistInit();

#if defined(PLATFORM_WEB)
    /* Detect touch devices (set by the shell script) → enable the soft-keyboard
     * bridge for text fields. Desktop web keeps the GLFW keyboard path. */
    gIsTouch = EM_ASM_INT({ return window.TV_isTouch ? 1 : 0; });
#endif

    /* Main loop — the per-frame body lives in UpdateDrawFrame() so the
     * WebAssembly build can let the browser drive it one frame at a time. */
#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(UpdateDrawFrame, 0, 1);  /* never returns */
#else
    while (!WindowShouldClose())
        UpdateDrawFrame();

    if (gCustomFont) UnloadFont(gFonts[0]);
    free(mem);
    DB_Close();
    Clay_Raylib_Close();
#endif
    return 0;
}

#if defined(PLATFORM_WEB)
/* ── Soft-keyboard bridge (touch web only) ────────────────────────────────
 * The hidden HTML <input> in shell.html owns the text while focused; we seed
 * it on open and copy its value back into the C buffer each frame. ASCII is
 * copied directly into HEAPU8 so we depend on no exported runtime helpers. */
enum { KBD_NONE = 0, KBD_NAME = 1, KBD_CMD = 2 };
static int gKbdTarget = KBD_NONE;

static void tv_kbd_open(const char *text)
{
    EM_ASM({ if (window.TV_kbdOpen) TV_kbdOpen(UTF8ToString($0), "text"); }, text);
}
static void tv_kbd_close(void)
{
    EM_ASM({ if (window.TV_kbdClose) TV_kbdClose(); });
}
/* Copy window.__tvKbdValue (ASCII) into buf[cap]; returns strlen written. */
static int tv_kbd_pull(char *buf, int cap)
{
    return EM_ASM_INT({
        var s = window.__tvKbdValue || "";
        var n = 0;
        for (var k = 0; k < s.length && n < $1 - 1; k++) {
            var c = s.charCodeAt(k);
            if (c >= 32 && c < 127) { HEAPU8[$0 + n] = c; n++; }
        }
        HEAPU8[$0 + n] = 0;
        return n;
    }, buf, cap);
}
static int tv_kbd_take_enter(void)
{
    return EM_ASM_INT({ var e = window.__tvKbdEnter; window.__tvKbdEnter = 0; return e; });
}

/* Reconcile the soft keyboard with the current focused text field. */
static void WebKbdSync(void)
{
    if (!gIsTouch) return;

    int desired = gNameInput ? KBD_NAME : (gPopupOpen ? KBD_CMD : KBD_NONE);

    if (desired != gKbdTarget) {
        if (desired == KBD_NONE)      tv_kbd_close();
        else if (desired == KBD_NAME) tv_kbd_open(gUserName);
        else                          tv_kbd_open(gCmdBuf);
        gKbdTarget = desired;
    }
    if (gKbdTarget == KBD_NONE) return;

    if (gKbdTarget == KBD_NAME) {
        gNameLen = tv_kbd_pull(gUserName, 26);
        if (gNameLen > 25) gNameLen = 25;
        if (tv_kbd_take_enter() && gNameLen > 0) InitPlayerDB();
    } else { /* KBD_CMD */
        gCmdLen = tv_kbd_pull(gCmdBuf, 256);
        if (tv_kbd_take_enter() && gCmdLen > 0) {
            ExecuteCommand(gCmdBuf, &gActiveNav, gFilterDept,
                           gResultMsg, sizeof(gResultMsg));
            gHasResult       = true;
            gResultShowUntil = (float)GetTime() + 5.f;
            gPopupOpen       = false;
            gCmdLen          = 0;
            gCmdBuf[0]       = '\0';
        }
    }
}
#endif

/* ── Per-frame update + render ─────────────────────────────────────────── */
static void UpdateDrawFrame(void)
{
    {
        (void)gIsTouch;  /* used only by the web soft-keyboard bridge */
        gScreenW = GetScreenWidth();
        gScreenH = GetScreenHeight();

        bool wasMobile = gIsMobile;
        gIsMobile = gScreenW < BP_MOBILE;
        if (!gIsMobile && wasMobile) gDrawerOpen = false;  /* leaving mobile closes drawer */

        Clay_SetLayoutDimensions((Clay_Dimensions){ (float)gScreenW, (float)gScreenH });
        Clay_SetPointerState(
            (Clay_Vector2){ (float)GetMouseX(), (float)GetMouseY() },
            IsMouseButtonDown(MOUSE_LEFT_BUTTON));
        Clay_UpdateScrollContainers(
            true,
            (Clay_Vector2){ 0.f, GetMouseWheelMove() * 50.f },
            GetFrameTime());

        HandleKeyboard();

#if defined(PLATFORM_WEB)
        /* Mirror the focused text field into the mobile soft keyboard. */
        WebKbdSync();
#endif

#if defined(PLATFORM_WEB)
        /* Finish an async .db import once the browser has written the file. */
        if (gDBReady) {
            int imp = DB_ImportPoll(gUserName);
            if (imp == 1) {
                RefreshPlayer();
                snprintf(gResultMsg, sizeof(gResultMsg),
                         "Imported database for %s", gUserName);
                gHasResult = true; gResultShowUntil = (float)GetTime() + 5.f;
            } else if (imp == -1) {
                snprintf(gResultMsg, sizeof(gResultMsg),
                         "Import failed or cancelled");
                gHasResult = true; gResultShowUntil = (float)GetTime() + 5.f;
            }
        }
#endif

        /* expire toast */
        if (gHasResult && (float)GetTime() >= gResultShowUntil)
            gHasResult = false;

        gDynPos = 0;  /* reset per-frame string arena */

        gRowHover = false;  /* set true by RenderTableRow when a row is hovered */
        Clay_BeginLayout();
        BuildLayout();
        Clay_RenderCommandArray cmds = Clay_EndLayout();

        /* clickable rows get the pointing-hand cursor */
        SetMouseCursor(gRowHover ? MOUSE_CURSOR_POINTING_HAND : MOUSE_CURSOR_DEFAULT);

        BeginDrawing();
        ClearBackground((Color){ 241, 237, 229, 255 });
        Clay_Raylib_Render(cmds, gFonts);

        /* ── Draw donut charts on top of Clay output (dashboard only) ── */
        if (gDBReady && !gNameInput && gActiveNav == 0) {
            /* Fetch bounding boxes from last frame */
            Clay_ElementData gd = Clay_GetElementData(
                Clay_GetElementId(CLAY_STRING(DONUT_GRADE_ID)));
            Clay_ElementData cd = Clay_GetElementData(
                Clay_GetElementId(CLAY_STRING(DONUT_CPA_ID)));

            /* ── Grade distribution donut ── */
            if (gd.found) {
                float cx = gd.boundingBox.x + gd.boundingBox.width  * 0.5f;
                float cy = gd.boundingBox.y + gd.boundingBox.height * 0.5f;
                float r  = (gd.boundingBox.width  < gd.boundingBox.height
                            ? gd.boundingBox.width : gd.boundingBox.height) * 0.5f - 6.f;
                if (r < 10.f) r = 10.f;
                float inner = r * 0.58f;

                /* count grades */
                int cA=0,cB=0,cC=0,cD=0,cF=0,cX=0, tot=0;
                for (int t=1; t<sizeSubjectType; t++) {
                    Subject_Node *nd = gPlayer.numofSubjectType[t].head;
                    while (nd) {
                        if (nd->status_ever_been_study & 1) {
                            switch(nd->score_letter){
                                case 'A': cA++; break; case 'B': cB++; break;
                                case 'C': cC++; break; case 'D': cD++; break;
                                case 'F': cF++; break; default: cX++; break;
                            }
                            tot++;
                        }  //else cX++;
                        nd = nd->next;
                    }
                }
                if (tot == 0) tot = 1;

                typedef struct { int cnt; Color col; } Seg;
                Seg segs[6] = {
                    {cA, (Color){ 47,120, 75,255}},   /* forest       */
                    {cB, (Color){ 52, 88,138,255}},   /* navy         */
                    {cC, (Color){166,124, 36,255}},   /* ochre        */
                    {cD, (Color){190,108, 50,255}},   /* burnt orange */
                    {cF, (Color){178, 52, 42,255}},   /* brick        */
                    {cX, (Color){180,172,158,255}},   /* faint paper  */
                };
                float angle = -90.f;
                for (int s=0; s<6; s++) {
                    if (segs[s].cnt == 0) continue;
                    float sweep = 360.f * segs[s].cnt / (float)tot;
                    DrawRing((Vector2){cx,cy}, inner, r, angle, angle+sweep, 36,
                             segs[s].col);
                    angle += sweep;
                }
                /* centre text */
                char ctxt[16];
                snprintf(ctxt, sizeof(ctxt), "%d", tot);
                float tw = MeasureTextEx(gFonts[0], ctxt, 14.f, 1.f).x;
                DrawTextEx(gFonts[0], ctxt,
                           (Vector2){cx - tw*0.5f, cy - 10.f},
                           12.f*gFontScale, 1.f, (Color){27,26,23,255});
                DrawTextEx(gFonts[0], "subj",
                           (Vector2){cx - MeasureTextEx(gFonts[0],"subj",10.f,1.f).x*0.5f,
                                     cy + 2.f},
                           12.f*gFontScale, 1.f, (Color){122,116,104,255});
            }

            /* ── CPA gauge (arc from -135° to 135°, 0..4 scale) ── */
            if (cd.found) {
                float cx = cd.boundingBox.x + cd.boundingBox.width  * 0.5f;
                float cy = cd.boundingBox.y + cd.boundingBox.height * 0.5f;
                float r  = (cd.boundingBox.width  < cd.boundingBox.height
                            ? cd.boundingBox.width : cd.boundingBox.height) * 0.5f - 6.f;
                if (r < 10.f) r = 10.f;
                float inner = r * 0.58f;

                float cpa_v = calc_cpa(&gPlayer, 0);
                if (cpa_v > 4.f) cpa_v = 4.f;

                /* background arc */
                DrawRing((Vector2){cx,cy}, inner, r, -135.f, 135.f, 36,
                         (Color){221,214,201,255});
                /* filled arc */
                float fillEnd = -135.f + 270.f * (cpa_v / 4.f);
                Color fillCol = cpa_v >= 3.5f ? (Color){ 47,120, 75,255}
                              : cpa_v >= 2.5f ? (Color){ 52, 88,138,255}
                              : cpa_v >= 2.0f ? (Color){166,124, 36,255}
                                              : (Color){178, 52, 42,255};
                if (cpa_v > 0.01f)
                    DrawRing((Vector2){cx,cy}, inner, r, -135.f, fillEnd, 36,
                             fillCol);
                /* centre label */
                char cpatxt[16];
                snprintf(cpatxt, sizeof(cpatxt), "%.2f", cpa_v);
                float tw2 = MeasureTextEx(gFonts[0], cpatxt, 16.f, 1.f).x;
                DrawTextEx(gFonts[0], cpatxt,
                           (Vector2){cx - tw2*0.5f, cy - 10.f},
                           12.f*gFontScale, 1.f, fillCol);
                DrawTextEx(gFonts[0], "/ 4.00",
                           (Vector2){cx - MeasureTextEx(gFonts[0],"/ 4.00",10.f,1.f).x*0.5f,
                                     cy + 8.f},
                           12.f*gFontScale, 1.f, (Color){122,116,104,255});
            }

            /* ── Grade-distribution radar (A+ A B+ B C+ C D+ D F) ── */
            Clay_ElementData radd = Clay_GetElementData(
                Clay_GetElementId(CLAY_STRING(RADAR_GRADE_ID)));
            if (radd.found && radd.boundingBox.width > 20.f) {
                float cx  = radd.boundingBox.x + radd.boundingBox.width  * 0.5f;
                float cy  = radd.boundingBox.y + radd.boundingBox.height * 0.5f;
                float box = radd.boundingBox.width < radd.boundingBox.height
                            ? radd.boundingBox.width : radd.boundingBox.height;
                float R   = box * 0.5f - 40.f;          /* room for labels */
                if (R < 24.f) R = 24.f;

                int gc[9];
                calc_grade_counts(gc);
                int gmax = 1;
                for (int i = 0; i < 9; i++) if (gc[i] > gmax) gmax = gc[i];

                const float TAU = 6.2831853f;
                Vector2 dir[9];
                for (int i = 0; i < 9; i++) {
                    float a = -TAU * 0.25f + (float)i * (TAU / 9.f);  /* top, clockwise */
                    dir[i] = (Vector2){ cosf(a), sinf(a) };
                }

                Color gridCol   = (Color){221, 214, 201, 255};
                Color spokeCol  = (Color){208, 200, 186, 255};
                Color axisTxt   = (Color){122, 116, 104, 255};
                Color dataLine  = (Color){140,  47,  42, 255};
                Color dataFill  = (Color){140,  47,  42,  46};

                /* concentric grid rings */
                for (int k = 1; k <= 4; k++) {
                    float rr = R * (float)k / 4.f;
                    for (int i = 0; i < 9; i++) {
                        Vector2 p1 = { cx + dir[i].x*rr,       cy + dir[i].y*rr };
                        Vector2 p2 = { cx + dir[(i+1)%9].x*rr, cy + dir[(i+1)%9].y*rr };
                        DrawLineEx(p1, p2, 1.f, gridCol);
                    }
                }
                /* spokes */
                for (int i = 0; i < 9; i++)
                    DrawLineEx((Vector2){cx, cy},
                               (Vector2){cx + dir[i].x*R, cy + dir[i].y*R},
                               1.f, spokeCol);

                /* data polygon */
                Vector2 pts[9];
                for (int i = 0; i < 9; i++) {
                    float rr = R * ((float)gc[i] / (float)gmax);
                    if (rr < 1.f) rr = 1.f;
                    pts[i] = (Vector2){ cx + dir[i].x*rr, cy + dir[i].y*rr };
                }
                /* translucent fill */
                Vector2 fan[11];
                fan[0] = (Vector2){ cx, cy };
                for (int i = 0; i < 9; i++) fan[i+1] = pts[i];
                fan[10] = pts[0];
                DrawTriangleFan(fan, 11, dataFill);
                /* outline + vertex dots */
                for (int i = 0; i < 9; i++)
                    DrawLineEx(pts[i], pts[(i+1)%9], 2.f, dataLine);
                for (int i = 0; i < 9; i++)
                    if (gc[i] > 0) DrawCircleV(pts[i], 3.f, dataLine);

                /* axis labels with counts */
                float lsz = 9.f * gFontScale;
                for (int i = 0; i < 9; i++) {
                    char buf[16];
                    snprintf(buf, sizeof(buf), "%s %d", kGradeLabels[i], gc[i]);
                    float lr = R + 18.f;
                    float lx = cx + dir[i].x * lr;
                    float ly = cy + dir[i].y * lr;
                    Vector2 ts = MeasureTextEx(gFonts[0], buf, lsz, 1.f);
                    DrawTextEx(gFonts[0], buf,
                               (Vector2){ lx - ts.x*0.5f, ly - ts.y*0.5f },
                               lsz, 1.f, axisTxt);
                }
            }
        }

        /* FPS counter */
        char fps[32];
        snprintf(fps, sizeof(fps), "FPS: %d", GetFPS());
        DrawTextEx(gFonts[0], fps,
                   (Vector2){ (float)(gScreenW - 80), 6.f }, 14.f, 1.f,
                   (Color){ 122, 116, 104, 150 });
        EndDrawing();
    }
}
