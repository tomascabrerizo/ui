#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <Windows.h>
#include <GL\gl.h>

#define ASSERT(v) assert((v))

#define TRUE 1
#define FALSE 0

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

typedef struct UI_State {
    void *active;
    void *hot;

    UI_V2i mouse_pos;
} UI_State;


/* Global UI library state */

#define UI_DRAW_CMMD_BUFFER_MAX 1024
static struct UI_DrawCmmd draw_cmmd_buffer[UI_DRAW_CMMD_BUFFER_MAX];
static UI_u64 draw_cmmd_buffer_count;

static UI_State ui_state;
static UI_V2i ui_default_button_dim = {200, 100};

/* ------------------------------------------------------------------------ */

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

UI_b32 ui_button(char *name, void *id, int x, int y) {
    ui_push_rect(v2i(x, y), ui_default_button_dim, v4f(0.0f, 1.0f, 0.0f, 1.0f));
    return FALSE;
}

void ui_draw_draw_cmmd_buffer(HWND window) {
    (void)window;
    for(UI_u64 i = 0; i < draw_cmmd_buffer_count; ++i) {
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

    /* TODO: check if this pointers have to be static */
    static char *button_name0 = "button";
    if(ui_button(button_name0, &button_name0, 100, 100)) {
    }
    static char *button_name1 = "button";
    if(ui_button(button_name1, &button_name1, 100, 300)) {
    }

    /* printf("b0 addrs: %llx. b1 addrs: %llx\n", (UI_u64)&button_name0, (UI_u64)&button_name1); */

    glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ui_draw_draw_cmmd_buffer(window);

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

LRESULT CALLBACK ui_win32_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    LRESULT result = 0;
    switch(message) {
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
    window_class.lpfnWndProc = ui_win32_proc;
    window_class.hInstance = hinstance;
    window_class.lpszClassName = "ui_window_class";

    if(RegisterClassA(&window_class) == 0) {
        printf("Error: Cannot register Window class\n");
        exit(-1);
    }

    HWND window = CreateWindowA(window_class.lpszClassName, "ui test",
                                WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                                100, 100, 800, 600, 0, 0, hinstance, 0);
    if(window == 0) {
        printf("Error: Cannot create Window\n");
        exit(-1);
    }

    global_running = 1;
    while(global_running) {
        MSG message;
        while(PeekMessage(&message, window, 0, 0, PM_REMOVE)) {
            TranslateMessage(&message);
            DispatchMessageA(&message);
        }

        main_loop(window);
    }

    wglDeleteContext(global_gl_context);
    return 0;
}
