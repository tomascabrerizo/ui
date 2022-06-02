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

/* Global Functions pointers */
typedef BOOL (WINAPI * PFNWGLSWAPINTERVALEXTPROC) (int interval);
static PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT;

/* Global Appication state */
static HGLRC global_gl_context;
static unsigned int global_running;

inline UI_i32 ui_i32_max(UI_i32 a, UI_i32 b) {
    UI_i32 result = a > b ? a : b;
    return result;
};

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

UI_RectMesh ui_rect_get_mesh(UI_V2i pos, UI_V2i dim) {
    UI_RectMesh result;
    /* Setup first triangle */
    result.t[0].v[0] = pos;
    result.t[0].v[1].x = pos.x;
    result.t[0].v[1].y = pos.y + dim.y;
    result.t[0].v[2].x = pos.x + dim.x;
    result.t[0].v[2].y = pos.y;
    /* Setup seconds triangle */
    result.t[1].v[0].x = pos.x + dim.x;
    result.t[1].v[0].y = pos.y;
    result.t[1].v[1].x = pos.x;
    result.t[1].v[1].y = pos.y + dim.y;
    result.t[1].v[2] = v2i_add(pos, dim);

    return result;
}

typedef struct UI_DrawCmmd {
    UI_V2i pos;
    UI_V2i dim;
    UI_V4f color;
} UI_DrawCmmd;

typedef enum UI_WidgetType {
    UI_WIDGET_BUTTON,
    UI_WIDGET_CHECKBOX,
    UI_WIDGET_SLIDER
} UI_WidgetType;

typedef struct UI_Widget {
    void *id;
    UI_WidgetType type;
    struct UI_Widget *next;
} UI_Widget;

typedef struct UI_Window {
    void *id;
    UI_V2i pos;
    UI_V2i dim;
    UI_V2i widget_offset; 
    struct UI_Window *next;
    struct UI_Widget *widget_first;
} UI_Window;

typedef struct UI_State {
    /* Widget */
    void *active;
    void *hot;
    void *hover;
    void *next_hover;

    UI_Window *window_first;
    UI_Window *window_current;
    
    /* Input */
    UI_V2i mouse;
    UI_b32 mouse_is_down;
    UI_b32 mouse_is_up;
    UI_b32 mouse_went_down;
    UI_b32 mouse_went_up;
} UI_State;

/* Global UI library state */

#define UI_DRAW_CMMD_BUFFER_MAX 1024
static struct UI_DrawCmmd draw_cmmd_buffer[UI_DRAW_CMMD_BUFFER_MAX];
static UI_u64 draw_cmmd_buffer_count;

static UI_State ui_state;
static UI_V2i ui_default_button_dim = {100, 50};
static UI_V4f ui_default_button_color = {0.4f, 0.4f, 0.4f, 1.0f};
static UI_V2i ui_default_checkbox_dim = {25, 25};
static UI_V2i ui_default_slider_dim = {200, 20};
static UI_V2i ui_default_window_dim = {300, 300};
static UI_V2i ui_default_window_margin = {10, 10};
static UI_V4f ui_default_window_color = {0.9f, 0.9f, 0.9f, 1.0f};

/* ------------------------------------------------------------------------ */

LRESULT ui_win32_get_input(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    UI_b32 last_mouse_is_down = ui_state.mouse_is_down;
    UI_b32 last_mouse_is_up = ui_state.mouse_is_up;
    
    LRESULT result = 0;
    switch (message) {
        case WM_MOUSEMOVE: {
            ui_state.mouse.x = LOWORD(lparam);
            ui_state.mouse.y = HIWORD(lparam);
        } break;
        case WM_LBUTTONUP: {
            ui_state.mouse_is_up = TRUE;
            ui_state.mouse_is_down = FALSE;

            ui_state.mouse_went_up = ui_state.mouse_is_up && !last_mouse_is_up;
        } break;
        case WM_LBUTTONDOWN: {
            ui_state.mouse_is_down = TRUE;
            ui_state.mouse_is_up = FALSE;

            ui_state.mouse_went_down = ui_state.mouse_is_down && !last_mouse_is_down;
        } break;
        default: {
        } break;
    }
    return result;
}

void ui_push_draw_cmmd(UI_DrawCmmd cmmd) {
    ASSERT(draw_cmmd_buffer_count < UI_DRAW_CMMD_BUFFER_MAX);
    draw_cmmd_buffer[draw_cmmd_buffer_count++] = cmmd;
}

void ui_push_rect(UI_V2i pos, UI_V2i dim, UI_V4f color) {
    UI_DrawCmmd cmmd;
    cmmd.pos = pos;
    cmmd.dim = dim;
    cmmd.color = color;
    ui_push_draw_cmmd(cmmd);
}

UI_Widget *ui_widget_get(UI_Window *window, void *id) {
    UI_Widget *widget = window->widget_first;
    while (widget) {
        if (widget->id == id) {
            return widget;
        }
    }
    return 0;
}

UI_Widget *ui_widget_register(UI_Window *window, void *id, UI_WidgetType type) {
    UI_Widget *widget = (UI_Widget *)malloc(sizeof(UI_Widget));
    memset(widget, 0, sizeof(UI_Widget));
    widget->id = id;
    widget->type = type;
    widget->next = window->widget_first;
    window->widget_first = widget;
    return widget;
}

UI_Window *ui_window_get(void *id) {
    UI_Window *window = ui_state.window_first;
    while (window) {
        if (window->id == id) {
            return window;
        }
        window = window->next;
    }
    return 0;
}

UI_Window *ui_window_register(void *id) {
    UI_Window *window = (UI_Window *)malloc(sizeof(UI_Window));
    memset(window, 0, sizeof(UI_Window));
    window->id = id;
    window->next = ui_state.window_first;
    ui_state.window_first = window;
    return window;
}

UI_b32 ui_mouse_inside_rect(UI_V2i pos, UI_V2i dim) {
    UI_b32 result = ui_state.mouse.x >= pos.x && ui_state.mouse.x < (pos.x + dim.x) &&
        ui_state.mouse.y >= pos.y && ui_state.mouse.y < (pos.y + dim.y);
    return result;
}

inline void ui_set_hot(void *id) {
    if (!ui_state.active) {
        ui_state.hot = id;
    }
}

inline void ui_set_active(void *id) {
    ui_state.active = id;
}

inline void ui_set_next_hover(void *id) {
    ui_state.next_hover = id;
}

inline UI_b32 ui_is_hot(void *id) {
    return ui_state.hot == id;
}

inline UI_b32 ui_is_active(void *id) {
    return ui_state.active == id;
}

inline UI_b32 ui_is_hover(void *id) {
    return ui_state.hover == id;
}

void ui_init(void) {
    /* TODO: initialize ui_state */
}

void ui_quit(void) {
    UI_Window *window = ui_state.window_first;
    while(window) {
        UI_Widget *widget = window->widget_first;
        while(widget) {
            void *to_free = widget;
            widget = widget->next;
            free(to_free);
        }
        void *to_free = window;
        window = window->next;
        free(to_free);
    }
}

void ui_update(void) {
    /* UI update pass */
    UI_Window *window = ui_state.window_first;
    while (window) {
        UI_Widget *widget = window->widget_first;
        while (widget) {
            switch (widget->type) {
                case UI_WIDGET_BUTTON: {
                    window->dim.x = ui_i32_max(window->dim.x, ui_default_button_dim.x + (ui_default_window_margin.x * 2));
                    window->dim.y += ui_default_button_dim.y + (ui_default_window_margin.y * 2);
                } break;
                case UI_WIDGET_CHECKBOX: {
                } break;
                case UI_WIDGET_SLIDER: {
                } break;
                default: {
                    INVALID_CODE_PATH();
                } break;
            }
            widget = widget->next;
        }
        window = window->next;
    }

    /* UI render pass */
    window = ui_state.window_first;
    while (window) {
        ui_push_rect(window->pos, window->dim, ui_default_window_color);
        UI_Widget *widget = window->widget_first;
        while(widget) {
            switch (widget->type) {
                case UI_WIDGET_BUTTON: {
                    UI_V2i pos = v2i_add(window->widget_offset, ui_default_window_margin);
                    ui_push_rect(v2i_add(window->pos, pos), ui_default_button_dim, ui_default_button_color);
                    window->widget_offset = v2i_add(window->widget_offset, v2i_add(pos, ui_default_button_dim));
                } break;
                case UI_WIDGET_CHECKBOX: {
                } break;
                case UI_WIDGET_SLIDER: {
                } break;
                default: {
                    INVALID_CODE_PATH();
                } break;
            }
            widget = widget->next;
        }
        window = window->next;
    }

    /* ------------------------------------------------ */

    /* TODO: See how to update frame information */
    ui_state.mouse_went_down = FALSE;
    ui_state.mouse_went_up = FALSE;
    /* TODO: Check if next_hover = 0 is necessary */
    ui_state.hover = ui_state.next_hover;
    ui_state.next_hover = 0;
}

void ui_begin_window(void *id, int x, int y) {
    UI_Window *window = ui_window_get(id);
    if (!window) {
        /* Initialize window */
        window = ui_window_register(id);
        window->pos.x = x;
        window->pos.y = y;
    }
    ui_state.window_current = window;
    /* TODO: Implemets window logic */
}
void ui_end_window(void) {
    ui_state.window_current = 0;
}

UI_b32 ui_button(void *id, char *name, int x, int y) {

    /* Widget dimensions:
    If the widget is not iside the window x and y are abs coordinates.
    Uf the widget is inside a window x and y are coordiantes relative to that window. */
    UI_V2i pos = v2i(x, y);
    UI_V2i dim = ui_default_button_dim;
    UI_V4f color = v4f(0.4f, 0.4f, 0.4f, 1.0f);
    UI_b32 result = FALSE;
    if (ui_state.window_current) {
        UI_Window *window = ui_state.window_current;
        UI_Widget *widget = ui_widget_get(window, id);
        if (!widget) {
            widget = ui_widget_register(window, id, UI_WIDGET_BUTTON);
        }
        pos = v2i_add(window->pos, pos);
    }
    /* Widget logic */
    if (ui_is_hover(id)) {
        if (ui_is_active(id) && ui_state.mouse_went_up) {
            if (ui_is_hover(id) && ui_is_hot(id) && ui_mouse_inside_rect(pos, dim)) {
                result = TRUE;
            }
            ui_set_active(0);
        } else if (ui_is_hot(id)) {
            if (ui_state.mouse_went_down) {
                ui_set_active(id);
            }
        }
        if (ui_mouse_inside_rect(pos, dim)) {
            ui_set_hot(id);
        }
    }
    if (ui_mouse_inside_rect(pos, dim)) {
        ui_set_next_hover(id);
    }
    /* Widget rendering */
    if (ui_is_hot(id) && ui_mouse_inside_rect(pos, dim)) {
        color = v4f(0.6f, 0.7f, 0.6f, 1.0f);
    }
    if (result) {
        color = v4f(0.4f, 0.4f, 0.8f, 1.0f);
    }

    if(!ui_state.window_current) {
        ui_push_rect(pos, dim, color);
    }
    return result;
}

void ui_checkbox(void *id, UI_b32 *value, int x, int y) {
    /* Widget dimensions */
    UI_V2i pos = v2i(x, y);
    UI_V2i dim = ui_default_checkbox_dim;
    UI_V4f color = v4f(0.4f, 0.4f, 0.4f, 1.0f);
    UI_V2i inner_pos = v2i_add(pos, v2i(4, 4));
    UI_V2i inner_dim = v2i_sub(dim, v2i(8, 8));
    UI_V4f inner_color = v4f(0.7f, 0.7f, 0.7f, 1.0f);
    /* Widget logic */
    if (ui_is_hover(id)) {
        if (ui_is_active(id) && ui_state.mouse_went_up) {
            if (ui_is_hot(id) && ui_mouse_inside_rect(inner_pos, inner_dim)) {
                *value = !(*value);
            }
            ui_set_active(0);
        } else if (ui_is_hot(id)) {
            if (ui_state.mouse_went_down) {
                ui_set_active(id);
            }
        }
        if (ui_mouse_inside_rect(pos, dim)) {
            ui_set_hot(id);
        }
    }
    if (ui_mouse_inside_rect(pos, dim)) {
        ui_set_next_hover(id);
    }
    /* Widget rendering */
    if (ui_is_hot(id) && ui_mouse_inside_rect(pos, dim)) {
        color = v4f(0.6f, 0.7f, 0.6f, 1.0f);
    }
    if (*value) {
        inner_color = v4f(0.7f, 1.0f, 0.7f, 1.0f);
    }
    ui_push_rect(pos, dim, color);
    ui_push_rect(inner_pos, inner_dim, inner_color);
}

void ui_slider(void *id, float *value, int x, int y) {
    /* Widget dimensions */
    UI_V2i pos = v2i(x, y);
    UI_V2i dim = ui_default_slider_dim;
    UI_V4f color = v4f(0.4f, 0.4f, 0.4f, 1.0f);
    UI_f32 inner_offset_x = (((*value + 1.0f) / 2.0f) * dim.x);
    UI_V2i inner_dim = v2i(20, dim.y);
    UI_V2i inner_pos = v2i(pos.x + (UI_i32)inner_offset_x - (inner_dim.x/2), pos.y);
    UI_V4f inner_color = v4f(0.7f, 0.7f, 0.7f, 1.0f);
    /* Widget logic */
    if (ui_is_hover(id)) {
        if (ui_is_active(id)) {
            if (ui_state.mouse_is_down) {
                if (ui_is_hover(id) && ui_is_hot(id) && ui_mouse_inside_rect(pos, dim)) {
                    *value = ((((UI_f32)(ui_state.mouse.x - pos.x) / dim.x) * 2) - 1);
                }
            }
            if (ui_state.mouse_went_up) {
                ui_set_active(0);
            }
        } else if (ui_is_hot(id)) {
            if (ui_state.mouse_went_down) {
                ui_set_active(id);
            }
        }
        if (ui_mouse_inside_rect(pos, dim)) {
            ui_set_hot(id);
        }
    }
    if (ui_mouse_inside_rect(pos, dim)) {
        ui_set_next_hover(id);
    }
    /* Widget rendering*/
    if (ui_is_hot(id) && ui_mouse_inside_rect(inner_pos, inner_dim)) {
        inner_color = v4f(0.6f, 0.7f, 0.6f, 1.0f);
    }
    ui_push_rect(pos, dim, color);
    ui_push_rect(inner_pos, inner_dim, inner_color);
}

/* ------------------------------------------------------------------------ */

void ui_draw_draw_cmmd_buffer(void) {
    for (UI_u64 i = 0; i < draw_cmmd_buffer_count; ++i) {
        UI_DrawCmmd cmmd = draw_cmmd_buffer[i];
        UI_RectMesh r = ui_rect_get_mesh(cmmd.pos, cmmd.dim);

        glBegin(GL_TRIANGLES);
        /* Set rect color */
        glColor3f(cmmd.color.x, cmmd.color.y, cmmd.color.z);
        /* TODO: we can render trinagle for the ui in a for loop */

        /* First triangle */
        glVertex3i(r.t[0].v[0].x, r.t[0].v[0].y, 0);
        glVertex3i(r.t[0].v[1].x, r.t[0].v[1].y, 0);
        glVertex3i(r.t[0].v[2].x, r.t[0].v[2].y, 0);
        /* Second triangle */
        glVertex3i(r.t[1].v[0].x, r.t[1].v[0].y, 0);
        glVertex3i(r.t[1].v[1].x, r.t[1].v[1].y, 0);
        glVertex3i(r.t[1].v[2].x, r.t[1].v[2].y, 0);

        glEnd();
    }
    draw_cmmd_buffer_count = 0;
}

void main_loop(HWND window) {
    HDC device_context = GetDC(window);

    char *button_name = "button";
    if(ui_button((void *)(button_name + 0), button_name, 100, 50)) {
    }

    ui_begin_window((void *)(button_name + 1), 100, 200);
    if(ui_button((void *)(button_name + 2), button_name, 16, 16)) {
    }
    ui_end_window();

    static UI_b32 checked = 0;
    ui_checkbox((void *)&checked, &checked, 400, 50);
    static float value = 0.0f;
    ui_slider((void  *)&value, &value, 400, 100);
    
    ui_update();
    
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ui_draw_draw_cmmd_buffer();

    SwapBuffers(device_context);
    ReleaseDC(window, device_context);
}

void create_opengl_context(HWND hwnd) {
    PIXELFORMATDESCRIPTOR pfd = {0};
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;
    pfd.iLayerType = PFD_MAIN_PLANE;

    HDC device_context = GetDC(hwnd);
    int pixel_format = ChoosePixelFormat(device_context, &pfd);
    if(SetPixelFormat(device_context, pixel_format, &pfd) == 0) {
        printf("Error: Cannot set opengl pixel format\n");
        exit(-1);
    }
    global_gl_context = wglCreateContext(device_context);
    wglMakeCurrent(device_context, global_gl_context);
    ReleaseDC(hwnd, device_context);

    wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
    if(wglSwapIntervalEXT == 0) {
        printf("Error: Cannot load SwapIntervalEXT\n");
        exit(-1);
    }
    wglSwapIntervalEXT(1);
}

LRESULT CALLBACK win32_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    LRESULT result = ui_win32_get_input(window, message, wparam, lparam);
    
    switch (message) {
        case WM_CREATE: {
            create_opengl_context(window);
        } break;
        case WM_SIZE: {
            unsigned int width = LOWORD(lparam);
            unsigned int height = HIWORD(lparam);
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            glOrtho(0, width, height, 0, 0, 1);
            glViewport(0, 0, width, height);
        } break;
        case WM_PAINT: {
            main_loop(window);
        } break;
        case WM_DESTROY: {
            global_running = 0;
            PostQuitMessage(0);
        }break;
        case WM_QUIT: {
            /* TODO: check why we are never geting WM_QUIT message */
        } break;
        default: {
            result = DefWindowProcA(window, message, wparam, lparam);
        } break;
    }
    return result;
}

int main(void) {
    HINSTANCE hinstance = GetModuleHandle(0);

    WNDCLASSA window_class = {0};
    window_class.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
    window_class.lpfnWndProc = win32_proc;
    window_class.hInstance = hinstance;
    window_class.lpszClassName = "ui_window_class";

    if (RegisterClassA(&window_class) == 0) {
        printf("Error: Cannot register Window class\n");
        exit(-1);
    }

    HWND window = CreateWindowA(window_class.lpszClassName, "ui test",
                                WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                                100, 100, 800, 600, 0, 0, hinstance, 0);
    if (window == 0) {
        printf("Error: Cannot create Window\n");
        exit(-1);
    }

    global_running = 1;
    while (global_running) {
        MSG message;
        while (PeekMessage(&message, window, 0, 0, PM_REMOVE)) {
            TranslateMessage(&message);
            DispatchMessageA(&message);
        }

        main_loop(window);
    }

    wglDeleteContext(global_gl_context);
    return 0;
}
