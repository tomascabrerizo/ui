#ifndef UI_H
#define UI_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <Windows.h>
#include <GL\gl.h>

#define TRUE 1
#define FALSE 0

#define ASSERT(v) assert((v))
#define INVALID_CODE_PATH() ASSERT(TRUE)
#define ARRAY_COUNT(a) (sizeof(a)/sizeof(a[0]))

typedef uint64_t UI_u64;
typedef uint32_t UI_u32;
typedef uint16_t UI_u16;
typedef uint8_t  UI_u8;

typedef int64_t UI_i64;
typedef int32_t UI_i32;
typedef int16_t UI_i16;
typedef int8_t  UI_i8;

typedef uint32_t UI_b32;
typedef float    UI_f32;
typedef double   UI_f64;

inline UI_i32 ui_i32_max(UI_i32 a, UI_i32 b) {
    UI_i32 result = (a > b) ? a : b;
    return result;
}

typedef struct UI_V4f {
    UI_f32 x;
    UI_f32 y;
    UI_f32 z;
    UI_f32 w;
} UI_V4f;

UI_V4f v4f(UI_f32 x, UI_f32 y, UI_f32 z, UI_f32 w) {
    UI_V4f result = (UI_V4f){x, y, z, w};
    return result;
}

typedef struct UI_V2i {
    UI_i32 x;
    UI_i32 y;
} UI_V2i;

inline UI_V2i v2i(UI_i32 x, UI_i32 y) {
    UI_V2i result = (UI_V2i){x, y};
    return result;
}

inline UI_V2i v2i_add(UI_V2i a, UI_V2i b) {
    UI_V2i result = (UI_V2i){a.x + b.x, a.y + b.y};
    return result;
}

inline UI_V2i v2i_sub(UI_V2i a, UI_V2i b) {
    UI_V2i result = (UI_V2i){a.x - b.x, a.y - b.y};
    return result;
}

typedef struct UI_Triangle {
    UI_V2i v[3];
} UI_Triangle;

typedef struct UI_RectMesh {
    UI_Triangle t[2];
} UI_RectMesh;


typedef struct UI_DrawCmmd {
    UI_V2i pos;
    UI_V2i dim;
    UI_V4f color;
} UI_DrawCmmd;

/* Global Functions pointers */
typedef BOOL (WINAPI * PFNWGLSWAPINTERVALEXTPROC) (int interval);
static PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT;

typedef enum UI_Layout {
    WIDGET_LAYOUT_NONE,
    WIDGET_LAYOUT_COLUMN,
    WIDGET_LAYOUT_ROW,
    WIDGET_LAYOUT_GRID,
} UI_Layout;

typedef enum UI_Flags {
    UI_CLICKABLE        = (1 << 0),
    UI_DRAW_BACKGROUND  = (1 << 1),
    UI_CONTAINER        = (1 << 2),
    UI_CLIPPING         = (1 << 3),
    UI_HOT_ANIMATION    = (1 << 4),
    UI_ACTIVE_ANIMATION = (1 << 5),
} UI_Flags;

typedef struct UI_Widget {
    /* Widget state */
    void *id;
    UI_Flags flags;
    UI_Layout layout;
    UI_V2i dim;
    /* Widget hierarchy */
    struct UI_Widget *parent;
    struct UI_Widget *first;
    struct UI_Widget *last;
    struct UI_Widget *next;
    struct UI_Widget *prev;
    /* register list */
    struct UI_Widget *reglist_next;
} UI_Widget;

typedef struct UI_Ctrl {
    void *temp;
} UI_Ctrl;

typedef struct UI_State {
    void *hot;
    void *active;

    UI_Widget *root;
    UI_Widget *current;
    UI_Widget *reglist; /* TODO: this can be a hash table */
} UI_State;

#endif /* UI_H */
