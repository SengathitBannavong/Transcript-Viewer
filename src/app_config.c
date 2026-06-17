#include "app_config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define APP_MAX_FONT_ENTRIES 32
#define APP_MAX_FONT_PATH    512

AppConfig AppConfig_Default(void)
{
    return (AppConfig){
        .font_scale = 1.8f,
        .target_fps = 60,
        .theme_id = 0,
    };
}

AppConfig AppConfig_Load(const char *path)
{
    AppConfig config = AppConfig_Default();
    FILE *uiCfg = fopen(path, "r");
    if (!uiCfg) return config;

    char line[256];
    while (fgets(line, sizeof(line), uiCfg)) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;

        char key[64];
        float val;
        if (sscanf(line, "%63s %f", key, &val) != 2) continue;

        if (strcmp(key, "font_scale") == 0 && val > 0.1f && val < 10.f) {
            config.font_scale = val;
        } else if (strcmp(key, "target_fps") == 0) {
            int fps = (int)val;
            if (fps < 60) fps = 60;
            if (fps > 240) fps = 240;
            config.target_fps = fps;
        } else if (strcmp(key, "theme_id") == 0) {
            int tid = (int)val;
            if (tid < 0 || tid > 2) tid = 0;
            config.theme_id = tid;
        }
    }

    fclose(uiCfg);
    return config;
}

FontLoadResult AppConfig_LoadFont(const char *path)
{
    char fontPathBuf[APP_MAX_FONT_ENTRIES][APP_MAX_FONT_PATH];
    const char *fontCandidates[APP_MAX_FONT_ENTRIES + 1];
    int numFontCandidates = 0;

    FILE *fontCfg = fopen(path, "r");
    if (fontCfg) {
        char line[APP_MAX_FONT_PATH];
        while (fgets(line, sizeof(line), fontCfg) &&
               numFontCandidates < APP_MAX_FONT_ENTRIES) {
            int len = (int)strlen(line);
            while (len > 0 &&
                   (line[len - 1] == '\n' || line[len - 1] == '\r' ||
                    line[len - 1] == ' ')) {
                line[--len] = '\0';
            }
            if (len == 0 || line[0] == '#') continue;

            snprintf(fontPathBuf[numFontCandidates],
                     APP_MAX_FONT_PATH,
                     "%s",
                     line);
            fontCandidates[numFontCandidates] = fontPathBuf[numFontCandidates];
            numFontCandidates++;
        }
        fclose(fontCfg);
    }

    FontLoadResult result = {
        .font = {0},
        .custom_font = false,
        .has_configured_candidates = numFontCandidates > 0,
    };

    fontCandidates[numFontCandidates] = NULL;
    for (int i = 0; fontCandidates[i]; i++) {
        if (!FileExists(fontCandidates[i])) continue;

        Font font = LoadFontEx(fontCandidates[i], 72, NULL, 250);
        if (font.texture.id == 0) continue;

        result.font = font;
        result.custom_font = true;
        SetTextureFilter(result.font.texture, TEXTURE_FILTER_BILINEAR);
        return result;
    }

    result.font = GetFontDefault();
    return result;
}

void AppConfig_DrawMissingFontScreen(void)
{
    const char *msg1 = "No font found - cannot start.";
    const char *msg2 = "Add a .ttf path to  assets/fonts.cfg  and restart.";
    const char *msg3 = "Press any key or close this window to exit.";

    fprintf(stderr,
            "[font] No font paths found in assets/fonts.cfg.\n"
            "       Add at least one valid .ttf path to that file and restart.\n");

    SetTargetFPS(60);
    while (!WindowShouldClose() && !GetKeyPressed()) {
        BeginDrawing();
        ClearBackground((Color){ 241, 237, 229, 255 });
        DrawText(msg1, 40, 120, 28, (Color){ 178, 52, 42, 255 });
        DrawText(msg2, 40, 165, 20, (Color){ 27, 26, 23, 255 });
        DrawText(msg3, 40, 210, 16, (Color){ 122, 116, 104, 255 });
        EndDrawing();
    }
}

bool AppConfig_Save(const char *path, float font_scale, int target_fps, int theme_id)
{
    FILE *f = fopen(path, "r");
    char *lines[100];
    int line_count = 0;

    if (f) {
        char line[256];
        while (fgets(line, sizeof(line), f) && line_count < 100) {
            lines[line_count++] = strdup(line);
        }
        fclose(f);
    } else {
        lines[line_count++] = strdup("# ui.cfg\n");
        lines[line_count++] = strdup("font_scale 1.8\n");
        lines[line_count++] = strdup("target_fps 60\n");
        lines[line_count++] = strdup("theme_id 0\n");
    }

    FILE *out = fopen(path, "w");
    if (!out) {
        for (int i = 0; i < line_count; i++) free(lines[i]);
        return false;
    }

    bool wrote_theme = false;
    for (int i = 0; i < line_count; i++) {
        int start = 0;
        while (lines[i][start] == ' ' || lines[i][start] == '\t') start++;

        char key[64] = {0};
        float val = 0.f;

        if (lines[i][start] != '#' && lines[i][start] != '\0' &&
            lines[i][start] != '\n' && lines[i][start] != '\r') {
            if (sscanf(lines[i] + start, "%63s %f", key, &val) == 2) {
                if (strcmp(key, "font_scale") == 0) {
                    fprintf(out, "font_scale %.1f\n", font_scale);
                    free(lines[i]);
                    continue;
                } else if (strcmp(key, "target_fps") == 0) {
                    fprintf(out, "target_fps %d\n", target_fps);
                    free(lines[i]);
                    continue;
                } else if (strcmp(key, "theme_id") == 0) {
                    fprintf(out, "theme_id %d\n", theme_id);
                    wrote_theme = true;
                    free(lines[i]);
                    continue;
                }
            }
        }
        fprintf(out, "%s", lines[i]);
        free(lines[i]);
    }
    if (!wrote_theme) {
        fprintf(out, "theme_id %d\n", theme_id);
    }
    fclose(out);
    return true;
}
