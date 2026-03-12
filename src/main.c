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
    DB_ValidateData();
    /* Auto-reload: if validation found problems, re-parse the asset files so
     * that any fixes the user made since the last run are picked up.       */
    if (gDataWarnCount > 0) DB_ReloadData();
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

    /* Edit popup takes input priority after F1 */
    if (gEditOpen) {
        if (IsKeyPressed(KEY_ESCAPE)) {
            gEditOpen = false;
            return;
        }
        /* Tab / Shift+Tab cycle between fields */
        if (IsKeyPressed(KEY_TAB)) {
            gEditField = (gEditField + 1) % 2;
            return;
        }
        /* Backspace */
        char *activeBuf = gEditField == 0 ? gEditMidBuf : gEditFinBuf;
        int  *activeLen = gEditField == 0 ? &gEditMidLen : &gEditFinLen;
        if ((IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE))
                && *activeLen > 0) {
            activeBuf[--(*activeLen)] = '\0';
        }
        /* Printable: digits and single '.' */
        int ch;
        while ((ch = GetCharPressed()) != 0) {
            bool isDot   = (ch == '.');
            bool isDigit = (ch >= '0' && ch <= '9');
            if ((isDigit || isDot) && *activeLen < 6) {
                /* only one dot allowed */
                if (isDot && strchr(activeBuf, '.')) continue;
                activeBuf[(*activeLen)++] = (char)ch;
                activeBuf[*activeLen]     = '\0';
            }
        }
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
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
        },
        .backgroundColor = C_BG,
    }) {
        if (!gNameInput) {
            RenderSidebar();
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
                    {cA, (Color){ 34,197, 94,255}},
                    {cB, (Color){ 99,102,241,255}},
                    {cC, (Color){234,179,  8,255}},
                    {cD, (Color){249,115, 22,255}},
                    {cF, (Color){239, 68, 68,255}},
                    {cX, (Color){ 60, 60, 90,180}},
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
                           12.f*gFontScale, 1.f, (Color){224,224,242,255});
                DrawTextEx(gFonts[0], "subj",
                           (Vector2){cx - MeasureTextEx(gFonts[0],"subj",10.f,1.f).x*0.5f,
                                     cy + 2.f},
                           12.f*gFontScale, 1.f, (Color){110,110,155,255});
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
                         (Color){36,36,68,255});
                /* filled arc */
                float fillEnd = -135.f + 270.f * (cpa_v / 4.f);
                Color fillCol = cpa_v >= 3.5f ? (Color){ 34,197, 94,255}
                              : cpa_v >= 2.5f ? (Color){ 99,102,241,255}
                              : cpa_v >= 2.0f ? (Color){234,179,  8,255}
                                              : (Color){239, 68, 68,255};
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
                           12.f*gFontScale, 1.f, (Color){110,110,155,255});
            }
        }

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
