# Clay UI Library — Practical Guide

> Based on clay.h source reading + building a real app with it.  
> Version: **clay.h main branch (03.2026)**

---

## Table of Contents

1. [What is Clay?](#1-what-is-clay)
2. [Setup](#2-setup)
3. [The Main Loop Pattern](#3-the-main-loop-pattern)
4. [The CLAY() Macro — Core Concept](#4-the-clay-macro--core-concept)
5. [Element Declaration Fields](#5-element-declaration-fields)
6. [Sizing](#6-sizing)
7. [Layout Direction & Alignment](#7-layout-direction--alignment)
8. [Padding & Gap](#8-padding--gap)
9. [Colors & Corner Radius](#9-colors--corner-radius)
10. [Borders](#10-borders)
11. [Text](#11-text)
12. [Scrollable Containers (Clip)](#12-scrollable-containers-clip)
13. [IDs — How to Name Elements](#13-ids--how-to-name-elements)
14. [Hover & Pointer Interaction](#14-hover--pointer-interaction)
15. [The Renderer — Drawing Render Commands](#15-the-renderer--drawing-render-commands)
16. [Text Measurement Function](#16-text-measurement-function)
17. [Dynamic Strings](#17-dynamic-strings)
18. [Common Patterns Cheatsheet](#18-common-patterns-cheatsheet)
19. [Gotchas & Lessons Learned](#19-gotchas--lessons-learned)

---

## 1. What is Clay?

Clay is a **single-header C library** (`clay.h`) for **UI layout**.

- It does **layout only** — it doesn't draw anything itself.
- It outputs a list of **render commands** you execute with your own renderer (Raylib, SDL, OpenGL, etc.).
- Think of it like CSS Flexbox, but in C with macros.
- Zero dependencies. Drop `clay.h` into your project and go.

```
Your Code
  │  declares UI with CLAY() macros
  ▼
Clay computes positions & sizes
  │  outputs Clay_RenderCommandArray
  ▼
Your Renderer
  │  reads each command and draws it
  ▼
Screen
```

---

## 2. Setup

### Step 1 — Include

In **exactly one** `.c` file, define the implementation before including:

```c
#define CLAY_IMPLEMENTATION
#include "clay.h"
```

In all other files just `#include "clay.h"` (no define).

### Step 2 — Allocate memory

Clay uses a single large memory arena. You size it, allocate it, give it to Clay.

```c
size_t memSize = Clay_MinMemorySize();       // ask Clay how much it needs
void  *mem     = malloc(memSize);
Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(memSize, mem);
```

### Step 3 — Initialize

```c
Clay_Initialize(
    arena,
    (Clay_Dimensions){ screenWidth, screenHeight },  // initial window size
    (Clay_ErrorHandler){ myErrorHandler, NULL }       // error callback + userdata
);
```

### Step 4 — Set text measurement function

Clay needs to know how wide/tall text will be for layout. You provide this:

```c
Clay_SetMeasureTextFunction(MyMeasureText, NULL);
```

---

## 3. The Main Loop Pattern

Every frame, do this in order:

```c
// 1. Update window size (if resizable)
Clay_SetLayoutDimensions((Clay_Dimensions){ screenW, screenH });

// 2. Update mouse position and button state
Clay_SetPointerState(
    (Clay_Vector2){ mouseX, mouseY },
    isMouseButtonDown
);

// 3. Update scrolling
Clay_UpdateScrollContainers(
    true,                              // enable drag scroll
    (Clay_Vector2){ 0, -scrollDelta }, // scroll offset this frame
    deltaTime                          // frame time in seconds
);

// 4. Build layout
Clay_BeginLayout();
    // ... declare your UI here with CLAY() macros ...
Clay_RenderCommandArray cmds = Clay_EndLayout();

// 5. Render
for (int i = 0; i < cmds.length; i++) {
    Clay_RenderCommand *cmd = Clay_RenderCommandArray_Get(&cmds, i);
    // ... handle cmd->commandType ...
}
```

---

## 4. The CLAY() Macro — Core Concept

`CLAY()` is a **for-loop trick** that opens an element, lets you declare children, then closes it automatically.

```c
CLAY(id, { ...config... }) {
    // children go here
    CLAY(...) { }
    CLAY_TEXT(...);
}
```

It expands to roughly:
```c
Clay__OpenElementWithId(id);
Clay__ConfigureOpenElement({ ...config... });
    // your children body runs here
Clay__CloseElement();
```

> **Mental model:** think of `CLAY(id, config) { children }` like an HTML `<div>`.

### Minimal element

```c
CLAY(CLAY_ID("Box"), {
    .layout = { .sizing = { CLAY_SIZING_FIXED(100), CLAY_SIZING_FIXED(50) } },
    .backgroundColor = { 255, 0, 0, 255 },
}) {}
```

---

## 5. Element Declaration Fields

The second argument to `CLAY()` is a `Clay_ElementDeclaration` struct:

```c
typedef struct Clay_ElementDeclaration {
    Clay_LayoutConfig        layout;          // sizing, padding, gap, direction, alignment
    Clay_Color               backgroundColor; // RGBA 0-255
    Clay_CornerRadius        cornerRadius;    // rounded corners
    Clay_BorderElementConfig border;          // border lines
    Clay_ClipElementConfig   clip;            // clipping / scrolling
    Clay_FloatingElementConfig floating;      // floating / overlay elements
    Clay_ImageElementConfig  image;           // image data pointer
    Clay_CustomElementConfig custom;          // custom render data
    void                    *userData;        // pass through to render commands
} Clay_ElementDeclaration;
```

You only set the fields you need. Unset fields default to zero/disabled.

---

## 6. Sizing

Sizing is set inside `.layout.sizing`:

```c
.layout = {
    .sizing = {
        .width  = CLAY_SIZING_GROW(0),    // fills available space
        .height = CLAY_SIZING_FIXED(48),  // exactly 48px
    }
}
```

### Sizing types

| Macro | Description |
|-------|-------------|
| `CLAY_SIZING_GROW(0)` | Expand to fill remaining parent space |
| `CLAY_SIZING_FIXED(px)` | Exact pixel size |
| `CLAY_SIZING_FIT(0)` | Shrink-wrap children (like CSS `width: fit-content`) |
| `CLAY_SIZING_PERCENT(0.5)` | 50% of parent. Use 0.0–1.0, **not** 0–100 |

### Min/Max constraints

```c
CLAY_SIZING_GROW(minPx)        // grow but never smaller than minPx
CLAY_SIZING_FIT(minPx)         // fit but never smaller than minPx
```

---

## 7. Layout Direction & Alignment

```c
.layout = {
    .layoutDirection = CLAY_LEFT_TO_RIGHT,  // default: horizontal row
    .layoutDirection = CLAY_TOP_TO_BOTTOM,  // vertical column

    .childAlignment = {
        .x = CLAY_ALIGN_X_LEFT,    // default
        .x = CLAY_ALIGN_X_CENTER,
        .x = CLAY_ALIGN_X_RIGHT,
        .y = CLAY_ALIGN_Y_TOP,     // default
        .y = CLAY_ALIGN_Y_CENTER,
        .y = CLAY_ALIGN_Y_BOTTOM,
    }
}
```

### Example: horizontally centered column

```c
CLAY(CLAY_ID("Column"), {
    .layout = {
        .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
        .layoutDirection = CLAY_TOP_TO_BOTTOM,
        .childAlignment  = { .x = CLAY_ALIGN_X_CENTER },
        .childGap        = 8,
    },
}) {
    // children stacked vertically, centered horizontally
}
```

---

## 8. Padding & Gap

```c
.layout = {
    .padding  = { .left=16, .right=16, .top=8, .bottom=8 },
    .childGap = 12,  // gap between each child element
}
```

Shortcut macro:
```c
.padding = CLAY_PADDING_ALL(16),  // same on all 4 sides
```

> `childGap` is space **between** children, not around them. For outer spacing use `padding`.

---

## 9. Colors & Corner Radius

### Color

`Clay_Color` is `{ r, g, b, a }` as `float`, conventionally 0–255.

```c
.backgroundColor = { 99, 102, 241, 255 }   // indigo
.backgroundColor = { 0, 0, 0, 0 }          // transparent (no rectangle drawn)
```

> An element only generates a `RECTANGLE` render command if `backgroundColor.a > 0`.

### Corner Radius

```c
.cornerRadius = CLAY_CORNER_RADIUS(8)        // all 4 corners = 8px radius
.cornerRadius = { .topLeft=8, .topRight=8,
                  .bottomLeft=0, .bottomRight=0 }  // individual corners
```

---

## 10. Borders

```c
.border = {
    .color = { 60, 60, 100, 255 },
    .width = { .left=1, .right=1, .top=1, .bottom=1 }  // all sides
}

// Or just one side:
.border = { .color = { 60,60,100,255 }, .width = { .bottom = 1 } }

// Shortcut — all sides same width:
.border = { .color = myColor, .width = CLAY_BORDER_ALL(1) }
```

`betweenChildren` adds dividers between child elements (like a table row separator):
```c
.width = { .betweenChildren = 1 }
```

---

## 11. Text

Text is **not** declared with `CLAY()`. Use the `CLAY_TEXT()` macro:

```c
CLAY_TEXT(textString, textConfigPtr);
```

### Clay_String

Clay uses its own string type (not `const char*` directly):

```c
// From a string literal (safe, statically allocated):
Clay_String s = CLAY_STRING("Hello World");

// From a runtime char* (you manage lifetime):
Clay_String s = { .isStaticallyAllocated = false,
                  .length = strlen(myStr),
                  .chars  = myStr };
```

### Clay_TextElementConfig

```c
Clay_TextElementConfig cfg = {
    .textColor     = { 220, 220, 240, 255 },
    .fontSize      = 14,
    .fontId        = 0,         // index you map in your renderer
    .letterSpacing = 1,
    .lineHeight    = 0,         // 0 = natural height from measurement
    .wrapMode      = CLAY_TEXT_WRAP_WORDS,   // default
                  // CLAY_TEXT_WRAP_NONE     — no wrap
                  // CLAY_TEXT_WRAP_NEWLINES — only on \n
    .textAlignment = CLAY_TEXT_ALIGN_LEFT,
};
```

**Important:** `CLAY_TEXT` takes a **pointer** to the config, not a copy.  
Use `Clay__StoreTextElementConfig()` to copy it into Clay's arena (safe, frame-valid):

```c
CLAY_TEXT(CLAY_STRING("Hello"),
          Clay__StoreTextElementConfig((Clay_TextElementConfig){
              .textColor = { 255, 255, 255, 255 },
              .fontSize  = 16,
          }));
```

Or make a helper macro:
```c
#define TC(color, size) \
    Clay__StoreTextElementConfig((Clay_TextElementConfig){ \
        .textColor = (color), .fontSize = (uint16_t)(size), \
        .wrapMode  = CLAY_TEXT_WRAP_NONE })

CLAY_TEXT(CLAY_STRING("Hello"), TC(white, 16));
```

---

## 12. Scrollable Containers (Clip)

To make a container scroll its children:

```c
CLAY(CLAY_ID("ScrollArea"), {
    .layout = {
        .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(400) },
        .layoutDirection = CLAY_TOP_TO_BOTTOM,
    },
    .clip = { .vertical = true },   // clip & scroll vertically
             // .horizontal = true  // or horizontally
}) {
    // items taller than 400px will scroll
}
```

And in your main loop, pass scroll input:
```c
Clay_UpdateScrollContainers(
    true,
    (Clay_Vector2){ 0.f, -GetMouseWheelMove() * 50.f },
    GetFrameTime()
);
```

---

## 13. IDs — How to Name Elements

Every element needs a unique ID per frame. There are several macros:

| Macro | Use when |
|-------|----------|
| `CLAY_ID("Name")` | Static string literal, unique in the layout |
| `CLAY_IDI("Name", index)` | Same base name, different integer index — **for loops** |
| `CLAY_SID(clayString)` | ID from a runtime `Clay_String` |
| `CLAY_SIDI(clayString, index)` | Runtime string + index |
| `CLAY_ID_LOCAL("Name")` | Scoped to parent element (avoids global uniqueness) |
| `CLAY_IDI_LOCAL("Name", i)` | Local + indexed |

### Rules

- `CLAY_ID("X")` can only be used **once** per frame — same ID twice = error.
- In a for loop always use `CLAY_IDI("Name", i)` so each iteration gets a different ID.
- **`CLAY_SIDI` requires a `Clay_String`, not a string literal directly.** Use `CLAY_IDI` for literals.

```c
// ✅ correct in a loop
for (int i = 0; i < count; i++) {
    CLAY(CLAY_IDI("Item", i), { ... }) { }
}

// ❌ wrong — duplicate ID every iteration
for (int i = 0; i < count; i++) {
    CLAY(CLAY_ID("Item"), { ... }) { }
}
```

---

## 14. Hover & Pointer Interaction

Call `Clay_Hovered()` **inside** a `CLAY()` block to check if the mouse is over the element being configured:

```c
CLAY(CLAY_IDI("Row", i), {
    .backgroundColor = Clay_Hovered() ? hoverColor : normalColor,
}) { }
```

For click events use `Clay_OnHover()`:
```c
void OnClick(Clay_ElementId id, Clay_PointerData pointer, void *userData) {
    if (pointer.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        // handle click
    }
}

CLAY(CLAY_ID("Button"), { ... }) {
    Clay_OnHover(OnClick, myUserData);
}
```

Check if pointer is over a specific element by ID:
```c
bool over = Clay_PointerOver(CLAY_ID("MyButton"));
```

---

## 15. The Renderer — Drawing Render Commands

`Clay_EndLayout()` returns `Clay_RenderCommandArray`. Iterate and draw:

```c
Clay_RenderCommandArray cmds = Clay_EndLayout();

for (int i = 0; i < cmds.length; i++) {
    Clay_RenderCommand *cmd = Clay_RenderCommandArray_Get(&cmds, i);
    Clay_BoundingBox b = cmd->boundingBox;  // x, y, width, height

    switch (cmd->commandType) {

    case CLAY_RENDER_COMMAND_TYPE_RECTANGLE: {
        Clay_RectangleRenderData *r = &cmd->renderData.rectangle;
        // draw filled rect at (b.x, b.y, b.width, b.height)
        // with color r->backgroundColor
        // with rounding r->cornerRadius.topLeft (etc.)
        break;
    }

    case CLAY_RENDER_COMMAND_TYPE_TEXT: {
        Clay_TextRenderData *t = &cmd->renderData.text;
        // t->stringContents is a Clay_StringSlice (NOT null-terminated!)
        // copy it to a buffer and null-terminate before passing to font renderer
        char buf[1024];
        int len = t->stringContents.length;
        memcpy(buf, t->stringContents.chars, len);
        buf[len] = '\0';
        // draw text at (b.x, b.y) with t->fontSize, t->textColor
        break;
    }

    case CLAY_RENDER_COMMAND_TYPE_BORDER: {
        Clay_BorderRenderData *brd = &cmd->renderData.border;
        // draw border lines using brd->width.top/bottom/left/right
        // with brd->color and brd->cornerRadius
        break;
    }

    case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START:
        // begin clipping to b.x, b.y, b.width, b.height
        break;

    case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END:
        // end clipping
        break;

    case CLAY_RENDER_COMMAND_TYPE_IMAGE:
        // cmd->renderData.image.imageData is your pointer
        break;

    case CLAY_RENDER_COMMAND_TYPE_CUSTOM:
        // cmd->renderData.custom.customData is your pointer
        break;
    }
}
```

> **Commands are already sorted by z-index** — draw them in array order and they will be correct.

---

## 16. Text Measurement Function

Clay cannot measure text itself — you provide a callback:

```c
Clay_Dimensions MyMeasureText(Clay_StringSlice text,
                               Clay_TextElementConfig *cfg,
                               void *userData)
{
    char buf[1024];
    int len = text.length < 1023 ? text.length : 1023;
    memcpy(buf, text.chars, len);
    buf[len] = '\0';

    // use your font API here, e.g. Raylib:
    Vector2 sz = MeasureTextEx(myFont, buf, cfg->fontSize, cfg->letterSpacing);
    return (Clay_Dimensions){ sz.x, sz.y };
}

// Register it once after Clay_Initialize:
Clay_SetMeasureTextFunction(MyMeasureText, NULL);
```

> Clay calls this heavily for caching — it's memoized internally, so performance is fine.

---

## 17. Dynamic Strings

`CLAY_STRING("literal")` only works with **compile-time string literals**.  
For runtime strings (formatted numbers, names from data, etc.) you need a `Clay_String` with a pointer to memory that stays valid **until after `Clay_EndLayout()`**.

The simplest approach — a per-frame ring buffer:

```c
static char dynBuf[32768];
static int  dynPos = 0;

Clay_String DS(const char *fmt, ...) {
    char   *start = dynBuf + dynPos;
    va_list ap;
    va_start(ap, fmt);
    int len = vsnprintf(start, 32767 - dynPos, fmt, ap);
    va_end(ap);
    dynPos += len + 1;
    return (Clay_String){ .isStaticallyAllocated = false,
                          .length = len, .chars = start };
}

// Reset at the start of each frame:
dynPos = 0;

// Usage:
CLAY_TEXT(DS("Score: %d", score), TC(white, 14));
CLAY_TEXT(DS("$%d", salary), TC(green, 13));
```

---

## 18. Common Patterns Cheatsheet

### Full-screen root container

```c
CLAY(CLAY_ID("Root"), {
    .layout = {
        .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
    },
    .backgroundColor = { 10, 10, 20, 255 },
}) {
    // all your UI goes here
}
```

### Fixed sidebar + growing main area

```c
CLAY(CLAY_ID("Root"), {
    .layout = {
        .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
        .layoutDirection = CLAY_LEFT_TO_RIGHT,
    },
}) {
    CLAY(CLAY_ID("Sidebar"), {
        .layout = { .sizing = { CLAY_SIZING_FIXED(200), CLAY_SIZING_GROW(0) } },
    }) { }

    CLAY(CLAY_ID("Content"), {
        .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) } },
    }) { }
}
```

### Centered content inside a box

```c
CLAY(CLAY_ID("Card"), {
    .layout = {
        .sizing         = { CLAY_SIZING_FIXED(200), CLAY_SIZING_FIXED(100) },
        .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
    },
    .backgroundColor = { 30, 30, 60, 255 },
}) {
    CLAY_TEXT(CLAY_STRING("Centered"), TC(white, 16));
}
```

### Badge / pill

```c
CLAY(CLAY_ID("Badge"), {
    .layout = {
        .sizing         = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIXED(22) },
        .padding        = { 10, 10, 4, 4 },
        .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
    },
    .backgroundColor = { 16, 60, 36, 255 },
    .cornerRadius    = CLAY_CORNER_RADIUS(11),
}) {
    CLAY_TEXT(CLAY_STRING("Active"), TC(green, 11));
}
```

### Spacer (push remaining items to the right/bottom)

```c
CLAY(CLAY_ID("Spacer"), {
    .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(1) } },
}) {}
```

### Table header row

```c
CLAY(CLAY_ID("Header"), {
    .layout = {
        .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(40) },
        .layoutDirection = CLAY_LEFT_TO_RIGHT,
    },
    .backgroundColor = { 25, 25, 46, 255 },
    .border          = { .color = borderColor, .width = { .bottom = 1 } },
}) {
    // column cells
}
```

---

## 19. Gotchas & Lessons Learned

### ❌ `CLAY_SIDI("literal", i)` — wrong for string literals
`CLAY_SIDI` expects a `Clay_String`, not a `const char*`.  
✅ Use `CLAY_IDI("literal", i)` for string literals with an index.

### ❌ Duplicate function name with Raylib's `MeasureText`
Raylib has its own `MeasureText()`. If you call your callback that too, it conflicts.  
✅ Name it something else: `ClayMeasureText`, `MeasureClayText`, etc.

### ❌ `CLAY_STRING()` at runtime / with variables
`CLAY_STRING(x)` requires `x` to be a **string literal** (checked at compile time).  
✅ For runtime strings use the ring-buffer `Clay_String` approach (see §17).

### ❌ Text config on the stack without storing
Passing `&(Clay_TextElementConfig){...}` from a local variable can break if Clay caches the pointer.  
✅ Always use `Clay__StoreTextElementConfig(cfg)` — copies it into Clay's arena.

### ❌ Forgetting to reset scroll when not calling `Clay_UpdateScrollContainers`  
Clay internally manages scroll state — always call it every frame even with `{0,0}` delta.

### ✅ `backgroundColor.a == 0` = no rectangle command  
If you set alpha to 0, Clay skips generating a `RECTANGLE` command entirely — useful for transparent layout containers.

### ✅ `Clay_Hovered()` must be called during layout (inside `CLAY()` block)  
It reads the hovered state of the element currently being configured. Call it in the config struct, not after.

### ✅ Render command array is already z-sorted  
Draw commands in the order they appear in the array — Clay guarantees correct layering.

### ✅ `Clay_StringSlice` is NOT null-terminated  
In your renderer, always copy to a buffer and add `\0` before passing to any C string function.

---

## Quick Reference Card

```
Clay_Initialize(arena, dimensions, errorHandler)
Clay_SetMeasureTextFunction(fn, userData)

─── Each frame ───────────────────────────────────
Clay_SetLayoutDimensions(dimensions)
Clay_SetPointerState(mousePos, mouseDown)
Clay_UpdateScrollContainers(drag, scrollDelta, dt)

Clay_BeginLayout()
  CLAY(id, { .layout={...}, .backgroundColor={r,g,b,a}, ... }) {
      CLAY_TEXT(string, textConfigPtr)
      CLAY(childId, { ... }) { }
  }
Clay_RenderCommandArray cmds = Clay_EndLayout()

─── Render ───────────────────────────────────────
for each cmd in cmds:
    switch cmd.commandType:
        RECTANGLE  → draw filled rect
        TEXT       → draw text (null-terminate first!)
        BORDER     → draw border lines
        SCISSOR_START → begin clip
        SCISSOR_END   → end clip

─── ID macros ────────────────────────────────────
CLAY_ID("Name")          static unique
CLAY_IDI("Name", i)      static + loop index
CLAY_SID(clayString)     runtime string
CLAY_SIDI(clayStr, i)    runtime + index
CLAY_ID_LOCAL("Name")    scoped to parent

─── Sizing ───────────────────────────────────────
CLAY_SIZING_GROW(min)    fill remaining space
CLAY_SIZING_FIXED(px)    exact size
CLAY_SIZING_FIT(min)     wrap children
CLAY_SIZING_PERCENT(f)   fraction of parent (0.0–1.0)

─── Layout ───────────────────────────────────────
CLAY_LEFT_TO_RIGHT  / CLAY_TOP_TO_BOTTOM
CLAY_ALIGN_X_LEFT/CENTER/RIGHT
CLAY_ALIGN_Y_TOP/CENTER/BOTTOM
CLAY_PADDING_ALL(n)
CLAY_CORNER_RADIUS(r)
CLAY_BORDER_ALL(w)
```
