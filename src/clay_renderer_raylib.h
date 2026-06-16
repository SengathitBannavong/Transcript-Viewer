#pragma once

#include "clay.h"
#include "raylib.h"

extern Camera Raylib_camera;

Clay_Dimensions Raylib_MeasureText(Clay_StringSlice text, Clay_TextElementConfig *config, void *userData);
void Clay_Raylib_Render(Clay_RenderCommandArray renderCommands, Font *fonts);
void Clay_Raylib_Close(void);
