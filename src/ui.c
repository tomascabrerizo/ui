#include "ui.h"

/* Global Appication state */
static HGLRC global_gl_context;
static unsigned int global_running;

/* UI renderer agnostic buffer */
#define UI_DRAW_CMMD_BUFFER_MAX 1024
static struct UI_DrawCmmd draw_cmmd_buffer[UI_DRAW_CMMD_BUFFER_MAX];
static UI_u64 draw_cmmd_buffer_count;

/* UI state globals */
static UI_State ui;

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

inline void ui_clear_tree_nodes(UI_Widget *widget) {
    widget->parent = 0;
    widget->first  = 0;
    widget->last   = 0;
    widget->next   = 0;
    widget->prev   = 0;
}

UI_Widget *ui_get_widget(void *id) {
    UI_Widget *widget = ui.reglist;
    while (widget) {
        if (widget->id == id) {
            ui_clear_tree_nodes(widget);
            return widget;
        }
        widget = widget->reglist_next;
    }
    widget = (UI_Widget *)malloc(sizeof(UI_Widget));
    memset(widget, 0, sizeof(UI_Widget));
    widget->id = id;
    widget->reglist_next = ui.reglist;
    ui.reglist = widget;
    printf("malloc widget %llx\n", (UI_u64)widget);
    return widget;
}

void ui_init(void) {
}

void ui_update(void) {
    ui.root = 0;
    ui.current = 0;
}

void ui_quit(void) {
    UI_Widget *widget = ui.reglist;
    while(widget) {
        UI_Widget *to_free = widget;
        widget = widget->reglist_next;
        printf("  free widget %llx\n", (UI_u64)to_free);
        free(to_free); /* TODO: Why after call free on widget the programs jumps to undefine behavior */
    }
    ui.reglist = 0;
}

void ui_add_widget_to_tree(UI_Widget *widget) {
    UI_Widget *parent = ui.current;
    if(parent) {
        if (!parent->first) {
            parent->first = widget;
        } else {
            parent->last->next = widget;
        }
        widget->prev = parent->last;
        parent->last = widget;
        widget->parent = parent;
    } else {
        ui.root = widget;
    }
}

UI_Widget *ui_begin_widget(void *id) {
    UI_Widget *widget = ui_get_widget(id);
    ui_add_widget_to_tree(widget);
    ui.current = widget;
    return widget;
}

void ui_end_widget(void) {
    ui.current = ui.current->parent;
}

void main_loop(float dt) {
    ui_push_rect(v2i(100, 100), v2i(100, 100), v4f(0.6f, 0.2f, 0.8f, 1.0f));

    static char *ids = "0123456789";
    ui_begin_widget((void *)(ids + 0));

    ui_begin_widget((void *)(ids + 1));
    ui_end_widget();

    ui_begin_widget((void *)(ids + 2));
    ui_end_widget();

    ui_end_widget();

    ui_update();
}

void ui_draw_draw_cmmd_buffer(HDC device_context) {
    glClearColor(0.12f, 0.1f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    for (UI_u64 i = 0; i < draw_cmmd_buffer_count; ++i) {
        UI_DrawCmmd cmmd = draw_cmmd_buffer[i];
        UI_RectMesh r = ui_rect_get_mesh(cmmd.pos, cmmd.dim);
        glBegin(GL_TRIANGLES);
        glColor3f(cmmd.color.x, cmmd.color.y, cmmd.color.z);
        for (int index = 0; index < ARRAY_COUNT(r.t); ++index) {
            glVertex3i(r.t[index].v[0].x, r.t[index].v[0].y, 0);
            glVertex3i(r.t[index].v[1].x, r.t[index].v[1].y, 0);
            glVertex3i(r.t[index].v[2].x, r.t[index].v[2].y, 0);
        }
        glEnd();
    }
    draw_cmmd_buffer_count = 0;
    SwapBuffers(device_context);
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
    switch (message) {
        case WM_CREATE: {
            create_opengl_context(window);
        } break;
        case WM_DESTROY: {
            wglDeleteContext(global_gl_context);
            global_running = 0;
            PostQuitMessage(0);
        }break;
        case WM_SIZE: {
            unsigned int width = LOWORD(lparam);
            unsigned int height = HIWORD(lparam);
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            glOrtho(0, width, height, 0, 0, 1);
            glViewport(0, 0, width, height);
        } break;
        case WM_PAINT: {
            float dt = 1.0f/60.0f;
            main_loop(dt);
            PAINTSTRUCT ps;
            HDC device_context = BeginPaint(window, &ps);
            ui_draw_draw_cmmd_buffer(device_context);
            EndPaint(window, &ps);
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

    if (RegisterClassA(&window_class) == 0) {
        printf("Error: Cannot register Window class\n");
        exit(1);
    }

    HWND window = CreateWindowA(window_class.lpszClassName, "ui test",
                                WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                                100, 100, 800, 600, 0, 0, hinstance, 0);
    if (window == 0) {
        printf("Error: Cannot create Window\n");
        exit(-1);
    }

    HDC device_context = GetDC(window);
    float dt = 1.0f/60.0f;
    global_running = 1;
    ui_init();
    while (global_running) {
        MSG message;
        while (PeekMessageA(&message, window, 0, 0, PM_REMOVE)) {
            TranslateMessage(&message);
            DispatchMessageA(&message);
        }
        main_loop(dt);
        ui_draw_draw_cmmd_buffer(device_context);
    }
    ui_quit();
    ReleaseDC(window, device_context);
    return 0;
}
