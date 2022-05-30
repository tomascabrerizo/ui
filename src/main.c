#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <Windows.h>
#include <GL\gl.h>

typedef BOOL (WINAPI * PFNWGLSWAPINTERVALEXTPROC) (int interval);
static PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT;
static unsigned int global_running;

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
    HGLRC gl_context = wglCreateContext(device_context);
    wglMakeCurrent(device_context, gl_context);
    ReleaseDC(hwnd, device_context);

    wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
    if(wglSwapIntervalEXT == 0) {
        printf("Error: Cannot load SwapIntervalEXT\n");
        exit(-1);
    }
    wglSwapIntervalEXT(1);
}

LRESULT CALLBACK ui_win32_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    LRESULT result = 0;
    switch(message) {
        case WM_CREATE: {
            create_opengl_context(hwnd);
        } break;
        case WM_SIZE: {
            unsigned int width = LOWORD(lparam);
            unsigned int height = HIWORD(lparam);
            glViewport(0, 0, width, height);
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            glOrtho(0, width, height, 0, 0, 1);
        } break;
        case WM_DESTROY: {
            global_running = 0;
            PostQuitMessage(0);
        }break;
        case WM_QUIT: {
            /* TODO: check why we are never geting WM_QUIT message*/
        } break;
        default: {
            result = DefWindowProcA(hwnd, message, wparam, lparam);
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
    HDC device_context = GetDC(window);
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

        glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBegin(GL_TRIANGLES);
        glColor3f(1.0f, 0.0f, 0.0f);
        glVertex3i(  0,   0, 0);
        glVertex3i(  0, 200, 0);
        glVertex3i(200, 200, 0);
        glEnd();
        
        SwapBuffers(device_context);
    }

    ReleaseDC(window, device_context);
    return 0;
}
