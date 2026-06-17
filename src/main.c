#define CLAY_IMPLEMENTATION
#include "clay.h"
#include "raylib.h"
#include "clay_renderer_raylib.h"
#include "app_data.h"
#include "app_config.h"
#include "db.h"
#include "score_logic.h"
#include "cmd.h"
#include "ui.h"

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

/* Global Singleton Instance definition */
AppContext gApp;

/* Convert a theme Clay_Color into a Raylib Color so charts drawn directly
   with Raylib (donuts, radar, comparison bars) follow the active theme. */
static inline Color Rc(Clay_Color c)
{
    return (Color){ (unsigned char)c.r, (unsigned char)c.g,
                    (unsigned char)c.b, (unsigned char)c.a };
}
/* Same, but with an explicit alpha (for translucent gridlines/fills). */
static inline Color RcA(Clay_Color c, unsigned char a)
{
    return (Color){ (unsigned char)c.r, (unsigned char)c.g,
                    (unsigned char)c.b, a };
}

/* Macro mappings to gApp members */
#define gScreenW (gApp.screen_w)
#define gScreenH (gApp.screen_h)
#define gActiveNav (gApp.active_nav)
#define gDynBuf (gApp.dyn_buf)
#define gDynPos (gApp.dyn_pos)
#define gPopupOpen (gApp.popup_open)
#define gCmdBuf (gApp.cmd_buf)
#define gCmdLen (gApp.cmd_len)
#define gHasResult (gApp.has_result)
#define gResultShowUntil (gApp.result_show_until)
#define gResultMsg (gApp.result_msg)
#define gPlayer (gApp.player)
#define gFonts (gApp.fonts)
#define gCustomFont (gApp.custom_font)
#define gIsMobile (gApp.is_mobile)
#define gDrawerOpen (gApp.drawer_open)
#define gRowHover (gApp.row_hover)
#define gIsTouch (gApp.is_touch)
#define gPlanTarget (gApp.plan_target)
#define gPlanFlex (gApp.plan_flex)
#define gEditOpen (gApp.edit_open)
#define gEditCode (gApp.edit_code)
#define gEditMidBuf (gApp.edit_mid_buf)
#define gEditMidLen (gApp.edit_mid_len)
#define gEditFinBuf (gApp.edit_fin_buf)
#define gEditFinLen (gApp.edit_fin_len)
#define gEditField (gApp.edit_field)
#define gEditRatio (gApp.edit_ratio)
#define gEditSubjectName (gApp.edit_subject_name)
#define gEditError (gApp.edit_error)
#define gFontScale (gApp.font_scale)
#define gTargetFPS (gApp.target_fps)
#define gNameInput (gApp.name_input)
#define gDBReady (gApp.db_ready)
#define gUserName (gApp.user_name)
#define gNameLen (gApp.name_len)
#define gTypeName (gApp.type_name)
#define gGradRules (gApp.grad_rules)
#define gDataWarningsBuf (gApp.data_warnings_buf)
#define gDataWarnCount (gApp.data_warn_count)
#define gFilterDept (gApp.filter_dept)

/* --- Score/player refresh helper ------------------------------------- */
/* Calls DB_Query then recomputes graduation status + alert level.       */
void RefreshPlayer(void)
{
    DB_Query(&gPlayer);
    update_player_status(&gPlayer);
}

void ShowToastFor(float seconds)
{
    gHasResult       = true;
    gResultShowUntil = (float)GetTime() + seconds;
}

static void ResetCommandInput(void)
{
    gCmdLen = 0;
    gCmdBuf[0] = '\0';
}

void ReturnToNameInput(void)
{
    memset(&gPlayer, 0, sizeof(gPlayer));
    gNameLen = 0;
    gUserName[0] = '\0';
    gDBReady = false;
    gNameInput = true;
    gApp.sandbox_override_count = 0;
    gApp.draft_override_count = 0;
    gApp.sandbox_dirty = false;
    gApp.compare_focused = false;
    gApp.compare_input[0] = '\0';
    gApp.compare_input_len = 0;
    ClearCompareState();
}


/* --- DB initializer (called when user confirms name on startup) -------- */
static void InitPlayerDB(void)
{
    for(int i = 0; i < sizeSubjectType; i++)
      gTypeName[i][0] = 0;
    bool isNew = !DB_Exists(gUserName);
    if (!DB_Open(gUserName)) {
        snprintf(gResultMsg, sizeof(gResultMsg),
                 "Error: cannot open db_%s.db", gUserName);
        ShowToastFor(5.f);
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

    /* Portal Import popup handler */
    if (gApp.import_open) {
        if (IsKeyPressed(KEY_ESCAPE)) {
            gApp.import_open = false;
            return;
        }
        if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_V)) {
            const char *clipText = GetClipboardText();
            if (clipText) {
                strncpy(gApp.import_buf, clipText, sizeof(gApp.import_buf) - 1);
                gApp.import_buf[sizeof(gApp.import_buf) - 1] = '\0';
                gApp.import_len = (int)strlen(gApp.import_buf);
            }
            return;
        }
        return;
    }

    /* Keyboard input for Comparison page custom user input box */
    if (gActiveNav == NAV_COMPARE && gApp.compare_focused) {
        if (IsKeyPressed(KEY_ESCAPE)) {
            gApp.compare_focused = false;
            return;
        }
        if (IsKeyPressed(KEY_ENTER)) {
            CompareTryAddCustom();
            gApp.compare_focused = false;
            return;
        }
        if ((IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE))
                && gApp.compare_input_len > 0) {
            gApp.compare_input[--gApp.compare_input_len] = '\0';
        }
        int ch;
        while ((ch = GetCharPressed()) != 0) {
            if (ch >= 32 && ch < 127 && gApp.compare_input_len < 25) {
                gApp.compare_input[gApp.compare_input_len++] = (char)ch;
                gApp.compare_input[gApp.compare_input_len] = '\0';
            }
        }
        return;
    }

    /* Ctrl+K -- toggle palette */
    if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_K)) {
        gPopupOpen = !gPopupOpen;
        if (!gPopupOpen) ResetCommandInput();
        return;
    }

    if (gPopupOpen && gEditOpen) gPopupOpen = false;
    if (!gPopupOpen) return;

    /* Escape -- dismiss */
    if (IsKeyPressed(KEY_ESCAPE)) {
        gPopupOpen = false;
        ResetCommandInput();
        return;
    }

    /* Enter -- submit */
    if (IsKeyPressed(KEY_ENTER) && gCmdLen > 0) {
        ExecuteCommand(gCmdBuf, &gActiveNav, gFilterDept, gResultMsg, sizeof(gResultMsg));
        ShowToastFor(5.f);
        gPopupOpen       = false;
        ResetCommandInput();
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
            else if (gActiveNav == NAV_PLANNER)
                RenderPlanner();
            else if (gActiveNav == NAV_SETTINGS)
                RenderSettings();
            else if (gActiveNav == NAV_COMPARE)
                RenderCompare();
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
        if (gApp.import_open) RenderImportPopup();
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
    memset(&gApp, 0, sizeof(gApp));
    gApp.font_scale = 1.8f;
    gApp.target_fps = 60;
    gApp.name_input = true;
    gApp.active_nav = 1;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(WIN_W, WIN_H, "Transcript Viewer");
    SetTargetFPS(gTargetFPS);  /* will be updated after ui.cfg loads */

    AppConfig appConfig = AppConfig_Load("assets/ui.cfg");
    gFontScale = appConfig.font_scale;
    gTargetFPS = appConfig.target_fps;
    gApp.theme_id = appConfig.theme_id;
    Theme_Apply(gApp.theme_id);

    FontLoadResult fontLoad = AppConfig_LoadFont("assets/fonts.cfg");
    if (!fontLoad.has_configured_candidates) {
        AppConfig_DrawMissingFontScreen();
        CloseWindow();
        return 1;
    }
    gFonts[0] = fontLoad.font;
    gCustomFont = fontLoad.custom_font;
    SetTargetFPS(gTargetFPS);

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
            ShowToastFor(5.f);
            gPopupOpen       = false;
            ResetCommandInput();
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
                ShowToastFor(5.f);
            } else if (imp == -1) {
                snprintf(gResultMsg, sizeof(gResultMsg),
                         "Import failed or cancelled");
                ShowToastFor(5.f);
            }
        }
#else
        /* Desktop: drag-and-drop a .db file onto the window to import it. */
        if (gDBReady && IsFileDropped()) {
            FilePathList dropped = LoadDroppedFiles();
            if (dropped.count > 0) {
                if (DB_ImportFile(gUserName, dropped.paths[0])) {
                    RefreshPlayer();
                    snprintf(gResultMsg, sizeof(gResultMsg),
                             "Imported %s", GetFileName(dropped.paths[0]));
                } else {
                    snprintf(gResultMsg, sizeof(gResultMsg),
                             "Drop failed: not a valid .db");
                }
                ShowToastFor(5.f);
            }
            UnloadDroppedFiles(dropped);
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
        ClearBackground(Rc(C_BG));
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
                           12.f*gFontScale, 1.f, Rc(C_TEXT));
                DrawTextEx(gFonts[0], "subj",
                           (Vector2){cx - MeasureTextEx(gFonts[0],"subj",10.f,1.f).x*0.5f,
                                     cy + 2.f},
                           12.f*gFontScale, 1.f, Rc(C_SUBTEXT));
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
                         Rc(C_BORDER));
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
                           12.f*gFontScale, 1.f, Rc(C_SUBTEXT));
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

                Color gridCol   = Rc(C_BORDER);
                Color spokeCol  = Rc(C_BORDER);
                Color axisTxt   = Rc(C_SUBTEXT);
                Color dataLine  = Rc(C_ACCENT);
                Color dataFill  = RcA(C_ACCENT, 46);

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

        /* ── Draw comparison charts on top of Clay output (compare page only) ── */
        if (gDBReady && !gNameInput && gActiveNav == NAV_COMPARE) {
            /* 1. CPA & GPA Comparison Chart */
            Clay_ElementData sd = Clay_GetElementData(
                Clay_GetElementId(CLAY_STRING(COMP_CHART_STATS_ID)));
            if (sd.found && sd.boundingBox.width > 20.f) {
                float x = sd.boundingBox.x;
                float y = sd.boundingBox.y;
                float w = sd.boundingBox.width;
                float h = sd.boundingBox.height;

                float padL = 50.f;
                float padR = 20.f;
                float padT = 30.f;
                float padB = 30.f;

                float chartW = w - padL - padR;
                float chartH = h - padT - padB;

                /* grid lines */
                for (float val = 0.f; val <= 4.f; val += 1.f) {
                    float y_pos = (y + padT + chartH) - (val / 4.0f) * chartH;
                    DrawLineV((Vector2){x + padL, y_pos}, (Vector2){x + padL + chartW, y_pos}, RcA(C_BORDER, 150));
                    char lbl[8];
                    snprintf(lbl, sizeof(lbl), "%.1f", val);
                    DrawTextEx(gFonts[0], lbl, (Vector2){x + padL - 30.f, y_pos - 6.f}, 10.f * gFontScale, 1.f, Rc(C_SUBTEXT));
                }

                /* draw bars for loaded students */
                float slotW = chartW / 3.0f;
                for (int i = 0; i < 3; i++) {
                    float slotCX = x + padL + i * slotW + slotW * 0.5f;
                    const char *name = NULL;
                    Player *player = NULL;
                    if (CompareGetStudent(i, &name, &player)) {
                        float val_cpa = calc_cpa(player, 0);
                        float val_gpa = calc_cpa(player, 1);
                        if (val_cpa > 4.f) val_cpa = 4.f;
                        if (val_gpa > 4.f) val_gpa = 4.f;

                        float barW = slotW * 0.25f;
                        float barGap = 4.f;

                        /* CPA bar */
                        float cpa_h = (val_cpa / 4.f) * chartH;
                        float bar_x = slotCX - barW - barGap * 0.5f;
                        float bar_y = (y + padT + chartH) - cpa_h;
                        DrawRectangleRec((Rectangle){bar_x, bar_y, barW, cpa_h}, (Color){ 238, 99, 66, 255 });

                        /* CPA value label */
                        char val_str[16];
                        snprintf(val_str, sizeof(val_str), "%.2f", val_cpa);
                        float tw = MeasureTextEx(gFonts[0], val_str, 9.f * gFontScale, 1.f).x;
                        DrawTextEx(gFonts[0], val_str, (Vector2){bar_x + barW*0.5f - tw*0.5f, bar_y - 12.f}, 9.f * gFontScale, 1.f, (Color){ 238, 99, 66, 255 });

                        /* GPA bar */
                        float gpa_h = (val_gpa / 4.f) * chartH;
                        float bar2_x = slotCX + barGap * 0.5f;
                        float bar2_y = (y + padT + chartH) - gpa_h;
                        DrawRectangleRec((Rectangle){bar2_x, bar2_y, barW, gpa_h}, (Color){ 102, 186, 183, 255 });

                        /* GPA value label */
                        snprintf(val_str, sizeof(val_str), "%.2f", val_gpa);
                        tw = MeasureTextEx(gFonts[0], val_str, 9.f * gFontScale, 1.f).x;
                        DrawTextEx(gFonts[0], val_str, (Vector2){bar2_x + barW*0.5f - tw*0.5f, bar2_y - 12.f}, 9.f * gFontScale, 1.f, (Color){ 102, 186, 183, 255 });

                        /* Student name */
                        tw = MeasureTextEx(gFonts[0], name, 11.f * gFontScale, 1.f).x;
                        DrawTextEx(gFonts[0], name, (Vector2){slotCX - tw*0.5f, y + padT + chartH + 8.f}, 11.f * gFontScale, 1.f, Rc(C_TEXT));
                    } else {
                        /* Empty slot text */
                        const char *emp = "(Empty Slot)";
                        float tw = MeasureTextEx(gFonts[0], emp, 10.f * gFontScale, 1.f).x;
                        DrawTextEx(gFonts[0], emp, (Vector2){slotCX - tw*0.5f, y + padT + chartH * 0.5f}, 10.f * gFontScale, 1.f, Rc(C_SUBTEXT));
                    }
                }

                /* Legend */
                float legX = x + w - 160.f;
                DrawRectangle(legX, y + 6.f, 10, 10, (Color){ 238, 99, 66, 255 });
                DrawTextEx(gFonts[0], "CPA", (Vector2){legX + 15.f, y + 6.f}, 10.f * gFontScale, 1.f, Rc(C_TEXT));
                DrawRectangle(legX + 60.f, y + 6.f, 10, 10, (Color){ 102, 186, 183, 255 });
                DrawTextEx(gFonts[0], "GPA", (Vector2){legX + 75.f, y + 6.f}, 10.f * gFontScale, 1.f, Rc(C_TEXT));
            }

            /* 2. Category Completion Progress Chart */
            Clay_ElementData cd = Clay_GetElementData(
                Clay_GetElementId(CLAY_STRING(COMP_CHART_CATEGORIES_ID)));
            if (cd.found && cd.boundingBox.width > 20.f) {
                float x = cd.boundingBox.x;
                float y = cd.boundingBox.y;
                float w = cd.boundingBox.width;
                float h = cd.boundingBox.height;

                float padL = 170.f;
                float padR = 72.f;
                float padT = 20.f;
                float padB = 30.f;

                float chartW = w - padL - padR;
                float chartH = h - padT - padB;

                /* active categories list */
                int active_categories[16];
                int active_cat_count = 0;
                for (int t = 1; t < sizeSubjectType; t++) {
                    if (gTypeName[t][0] == 0) continue;
                    active_categories[active_cat_count++] = t;
                }

                float slotH = chartH / (float)(active_cat_count > 0 ? active_cat_count : 1);
                Color student_colors[3] = {
                    (Color){ 41, 121, 255, 255 },  /* blue */
                    (Color){ 255, 152, 0, 255 },   /* orange */
                    (Color){ 76, 175, 80, 255 }    /* green */
                };

                /* find dynamic max value to scale chart */
                float max_val = 20.f;
                for (int c = 0; c < active_cat_count; c++) {
                    int t = active_categories[c];
                    for (int i = 0; i < 3; i++) {
                        const char *name = NULL;
                        Player *player = NULL;
                        if (CompareGetStudent(i, &name, &player)) {
                            /* Respect grad_config.cfg: requirement may be a fixed
                               limit (e.g. tu_chon = 9) rather than the full pool. */
                            float val = (float)_sl_resolve_pass(player, t);
                            if (val > max_val) max_val = val;
                            float limit = (float)_sl_resolve_limit(player, t);
                            if (limit > max_val) max_val = limit;
                        }
                    }
                }

                /* zebra row backgrounds to group each category's 3 bars */
                for (int c = 0; c < active_cat_count; c++) {
                    if (c % 2 == 1) {
                        float rowY = y + padT + c * slotH;
                        DrawRectangleRec((Rectangle){x + padL, rowY, chartW, slotH}, (Color){0, 0, 0, 8});
                    }
                }

                /* grid lines */
                for (float val = 5.f; val <= max_val; val += 5.f) {
                    float gridX = x + padL + (val / max_val) * chartW;
                    DrawLineV((Vector2){gridX, y + padT}, (Vector2){gridX, y + padT + chartH}, RcA(C_BORDER, 100));
                    char lbl[8];
                    snprintf(lbl, sizeof(lbl), "%.0f", val);
                    float tw = MeasureTextEx(gFonts[0], lbl, 9.f * gFontScale, 1.f).x;
                    DrawTextEx(gFonts[0], lbl, (Vector2){gridX - tw*0.5f, y + padT + chartH + 5.f}, 9.f * gFontScale, 1.f, Rc(C_SUBTEXT));
                }

                /* Y-axis line */
                DrawLineV((Vector2){x + padL, y + padT}, (Vector2){x + padL, y + padT + chartH}, Rc(C_BORDER));

                /* draw bars per category */
                for (int c = 0; c < active_cat_count; c++) {
                    int t = active_categories[c];
                    float slotCY = y + padT + c * slotH + slotH * 0.5f;

                    /* Category label */
                    float tw = MeasureTextEx(gFonts[0], gTypeName[t], 10.f * gFontScale, 1.f).x;
                    DrawTextEx(gFonts[0], gTypeName[t], (Vector2){x + padL - tw - 10.f, slotCY - 5.f}, 10.f * gFontScale, 1.f, Rc(C_TEXT));

                    /* Draw horizontal bars for each loaded student */
                    float barH = slotH * 0.22f;
                    if (barH > 11.f) barH = 11.f;
                    if (barH < 3.f) barH = 3.f;
                    float barGap = 3.f;
                    float groupH = 3.f * barH + 2.f * barGap;

                    for (int i = 0; i < 3; i++) {
                        const char *name = NULL;
                        Player *player = NULL;
                        if (CompareGetStudent(i, &name, &player)) {
                            /* Use grad_config.cfg requirement as the denominator,
                               so fixed-limit types (e.g. tu_chon = 9) and
                               subject-count types track toward their actual rule
                               instead of the full credit pool. */
                            float completed = (float)_sl_resolve_pass(player, t);
                            float total = (float)_sl_resolve_limit(player, t);

                            float barY = slotCY - groupH * 0.5f + i * (barH + barGap);

                            /* faint full-length track showing the credit requirement (total) */
                            float trackW = (total / max_val) * chartW;
                            if (total > 0.01f) {
                                DrawRectangleRec((Rectangle){x + padL, barY, trackW, barH}, RcA(C_BORDER, 200));
                            }

                            /* solid bar showing completed credits */
                            float barW = (completed / max_val) * chartW;
                            if (barW < 2.f && completed > 0.5f) barW = 2.f;
                            DrawRectangleRec((Rectangle){x + padL, barY, barW, barH}, student_colors[i]);

                            /* value label anchored just past the track end (aligns into a
                               neat column per category instead of piling up at the axis).
                               Skip near-zero values that would render as a cluttered "0/N". */
                            if (completed >= 0.5f) {
                                char val_str[16];
                                if (total > 0.01f) {
                                    snprintf(val_str, sizeof(val_str), "%.0f/%.0f", completed, total);
                                } else {
                                    snprintf(val_str, sizeof(val_str), "%.0f", completed);
                                }
                                float anchorW = trackW > barW ? trackW : barW;
                                float lblX = x + padL + anchorW + 6.f;
                                float ltw = MeasureTextEx(gFonts[0], val_str, 8.f * gFontScale, 1.f).x;
                                if (lblX + ltw > x + w - 4.f) lblX = x + w - 4.f - ltw;
                                Color lc = student_colors[i];
                                lc.r = (unsigned char)(lc.r * 0.72f);
                                lc.g = (unsigned char)(lc.g * 0.72f);
                                lc.b = (unsigned char)(lc.b * 0.72f);
                                DrawTextEx(gFonts[0], val_str, (Vector2){lblX, barY + barH*0.5f - 5.f}, 8.f * gFontScale, 1.f, lc);
                            }
                        }
                    }
                }

                /* Legend dynamically generated from loaded students */
                float legX = x + w - 30.f;
                for (int i = 2; i >= 0; i--) {
                    const char *name = NULL;
                    Player *player = NULL;
                    if (CompareGetStudent(i, &name, &player)) {
                        float tw = MeasureTextEx(gFonts[0], name, 10.f * gFontScale, 1.f).x;
                        legX -= (tw + 20.f);
                        DrawRectangle(legX, y + 6.f, 10, 10, student_colors[i]);
                        DrawTextEx(gFonts[0], name, (Vector2){legX + 15.f, y + 6.f}, 10.f * gFontScale, 1.f, Rc(C_TEXT));
                    }
                }
            }

            /* 3. Concentric Radial Progress Rings Chart */
            Clay_ElementData rd = Clay_GetElementData(
                Clay_GetElementId(CLAY_STRING(COMP_CHART_RINGS_ID)));
            if (rd.found && rd.boundingBox.width > 20.f) {
                float x = rd.boundingBox.x;
                float y = rd.boundingBox.y;
                float w = rd.boundingBox.width;
                float h = rd.boundingBox.height;

                Color student_colors[3] = {
                    (Color){ 41, 121, 255, 255 },  /* blue */
                    (Color){ 255, 152, 0, 255 },   /* orange */
                    (Color){ 76, 175, 80, 255 }    /* green */
                };

                /* Pre-build legend lines so we can size the panel to fit them
                   (prevents the heading/labels from being clipped at the edge). */
                const char *legHeading = "Overall Graduation Progress";
                char legLines[3][80];
                for (int i = 0; i < 3; i++) {
                    const char *nm = NULL;
                    Player *pl = NULL;
                    if (CompareGetStudent(i, &nm, &pl)) {
                        int comp = calc_effective_credits(pl);
                        int tot  = calc_required_credits(pl);
                        int pct  = tot > 0 ? (int)(((float)comp / (float)tot) * 100.f) : 0;
                        snprintf(legLines[i], sizeof(legLines[i]), "%s  %d/%d (%d%%)", nm, comp, tot, pct);
                    } else {
                        snprintf(legLines[i], sizeof(legLines[i]), "Slot %d  (Empty)", i + 1);
                    }
                }
                float legMaxW = MeasureTextEx(gFonts[0], legHeading, 12.f * gFontScale, 1.f).x;
                for (int i = 0; i < 3; i++) {
                    float lw = 20.f + MeasureTextEx(gFonts[0], legLines[i], 11.f * gFontScale, 1.f).x;
                    if (lw > legMaxW) legMaxW = lw;
                }

                float padL = 20.f;
                float padR = legMaxW + 56.f; /* legend panel width + breathing room */
                float padT = 20.f;
                float padB = 20.f;

                float chartW = w - padL - padR;
                float chartH = h - padT - padB;

                float cx = x + padL + chartW * 0.5f;
                float cy = y + padT + chartH * 0.5f;

                float maxR = (chartW < chartH ? chartW : chartH) * 0.46f;
                if (maxR < 20.f) maxR = 20.f;

                float ring_thickness = maxR * 0.16f;
                if (ring_thickness > 20.f) ring_thickness = 20.f;
                if (ring_thickness < 8.f)  ring_thickness = 8.f;
                float ring_gap = ring_thickness * 0.4f;

                /* Draw 3 background rings and progress rings */
                for (int i = 0; i < 3; i++) {
                    float R = maxR - i * (ring_thickness + ring_gap);
                    if (R - ring_thickness < 5.f) break;

                    /* 1. Background ring */
                    DrawRing((Vector2){cx, cy}, R - ring_thickness, R, 0.f, 360.f, 48, Rc(C_BORDER));

                    /* 2. Progress ring if student loaded */
                    const char *name = NULL;
                    Player *player = NULL;
                    if (CompareGetStudent(i, &name, &player)) {
                        int completed = calc_effective_credits(player);
                        int total = calc_required_credits(player);
                        float pct = total > 0 ? (float)completed / (float)total : 0.f;
                        if (pct > 1.f) pct = 1.f;
                        if (pct < 0.f) pct = 0.f;

                        float sweep = 360.f * pct;
                        if (pct > 0.01f) {
                            DrawRing((Vector2){cx, cy}, R - ring_thickness, R, -90.f, -90.f + sweep, 48, student_colors[i]);
                            
                            /* Draw rounded caps for the progress ring arc (starts at -90 deg) */
                            float r_mid = R - ring_thickness * 0.5f;
                            float rad_start = -90.f * (3.14159265f / 180.f);
                            float rad_end = (-90.f + sweep) * (3.14159265f / 180.f);
                            DrawCircleV((Vector2){ cx + cosf(rad_start) * r_mid, cy + sinf(rad_start) * r_mid }, ring_thickness * 0.5f, student_colors[i]);
                            DrawCircleV((Vector2){ cx + cosf(rad_end) * r_mid, cy + sinf(rad_end) * r_mid }, ring_thickness * 0.5f, student_colors[i]);
                        }
                    }
                }

                /* Legend panel, vertically centred against the rings */
                float legBlockH = 26.f + 3.f * 24.f;
                float legX = x + w - padR + 16.f;
                float legY = cy - legBlockH * 0.5f;
                DrawTextEx(gFonts[0], legHeading, (Vector2){legX, legY}, 12.f * gFontScale, 1.f, Rc(C_TEXT));
                legY += 26.f;

                for (int i = 0; i < 3; i++) {
                    const char *lnm = NULL;
                    Player *lpl = NULL;
                    int loaded = CompareGetStudent(i, &lnm, &lpl);
                    Color sw = loaded ? student_colors[i] : Rc(C_BORDER);
                    Color tx = loaded ? Rc(C_TEXT) : Rc(C_SUBTEXT);
                    DrawRectangle(legX, legY + 2.f, 12, 12, sw);
                    DrawTextEx(gFonts[0], legLines[i], (Vector2){legX + 20.f, legY}, 11.f * gFontScale, 1.f, tx);
                    legY += 24.f;
                }
            }
        }

        /* FPS counter */
        char fps[32];
        snprintf(fps, sizeof(fps), "FPS: %d", GetFPS());
        DrawTextEx(gFonts[0], fps,
                   (Vector2){ (float)(gScreenW - 80), 6.f }, 14.f, 1.f,
                   RcA(C_SUBTEXT, 150));
        EndDrawing();
    }
}
