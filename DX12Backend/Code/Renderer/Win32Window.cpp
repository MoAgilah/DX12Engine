#include "Win32Window.h"

#include <Utilities/Guards.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

static LRESULT CALLBACK StaticWndProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
{
    Win32Window* window = nullptr;

    if (msg == WM_NCCREATE)
    {
        const auto* createStruct =
            reinterpret_cast<CREATESTRUCTW*>(lParam);

        if (!createStruct)
            return FALSE;

        window = reinterpret_cast<Win32Window*>(
            createStruct->lpCreateParams);

        if (!window)
            return FALSE;

        SetLastError(0);

        const LONG_PTR previousValue = SetWindowLongPtr(
            hwnd,
            GWLP_USERDATA,
            reinterpret_cast<LONG_PTR>(window));

        if (previousValue == 0 && GetLastError() != ERROR_SUCCESS)
            return FALSE;

        window->SetWindowHandle(hwnd);
    }
    else
    {
        window = reinterpret_cast<Win32Window*>(
            GetWindowLongPtr(
                hwnd,
                GWLP_USERDATA));
    }

    if (!window)
    {
        return DefWindowProc(
            hwnd,
            msg,
            wParam,
            lParam);
    }

    return static_cast<LRESULT>(
        window->HandleMessage(
            static_cast<unsigned int>(msg),
            static_cast<unsigned long long>(wParam),
            static_cast<long long>(lParam)));
}

bool Win32Window::Create(const Vector2f& screenDims, const std::string& title)
{
    m_screenDims = screenDims;
    m_shouldClose = false;

    m_instanceHandle = GetModuleHandle(nullptr);
    if (!CheckNotNull(m_instanceHandle, "Invalid Handle 'm_instanceHandle'"))
        return false;

    std::wstring windowTitle(title.begin(), title.end());
    const wchar_t* className = L"MyEngineWindowClass";

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = StaticWndProc;
    wc.hInstance = m_instanceHandle;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = className;
    wc.style = CS_HREDRAW | CS_VREDRAW;

    if (!RegisterClassExW(&wc))
    {
        if (GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
        {
            ThrowIfFalse(
                GetLastError() == ERROR_CLASS_ALREADY_EXISTS,
                "Failed to register window class");
        }
    }

    RECT r =
    {
        0,
        0,
        static_cast<LONG>(m_screenDims.x),
        static_cast<LONG>(m_screenDims.y)
    };

    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);

    m_windowHandle = CreateWindowExW(
        0,
        className,
        windowTitle.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        r.right - r.left,
        r.bottom - r.top,
        nullptr,
        nullptr,
        m_instanceHandle,
        this);

    if (!CheckNotNull(m_windowHandle, "Invalid Handle 'm_windowHandle'"))
        return false;

    ShowWindow(m_windowHandle, SW_SHOW);
    UpdateWindow(m_windowHandle);

    return true;
}

void Win32Window::PollEvents()
{
    MSG msg = {};

    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);

        if (msg.message == WM_QUIT)
        {
            m_shouldClose = true;
        }
    }
}

bool Win32Window::ShouldClose() const
{
    return m_shouldClose;
}

void Win32Window::Close()
{
    if (!m_windowHandle)
        return;

    m_shouldClose = true;
    DestroyWindow(m_windowHandle);
    m_windowHandle = nullptr;
}

void* Win32Window::GetNativeHandle()
{
    return m_windowHandle;
}

long Win32Window::HandleMessage(unsigned int msg, unsigned long long wParam, long long lParam)
{
    switch (msg)
    {
    case WM_CLOSE:
    {
        Close();
        return 0;
    }

    case WM_DESTROY:
    {
        m_shouldClose = true;
        PostQuitMessage(0);
        return 0;
    }

    case WM_KEYDOWN:
    {
        if (wParam == VK_ESCAPE)
        {
            Close();
            return 0;
        }
        return 0;
    }

    case WM_SIZE:
    {
        m_screenDims.x = static_cast<float>(LOWORD(lParam));
        m_screenDims.y = static_cast<float>(HIWORD(lParam));
        return 0;
    }
    }

    return static_cast<long>(
        DefWindowProc(
            m_windowHandle,
            static_cast<UINT>(msg),
            static_cast<WPARAM>(wParam),
            static_cast<LPARAM>(lParam)));
}

void Win32Window::SetWindowHandle(HWND hwnd)
{
    m_windowHandle = hwnd;
}