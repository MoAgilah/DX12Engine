#pragma once

#include <Engine/Interface/Renderer/IWindow.h>
#include <Utilities/Vector2.h>
#include <memory>
#include <string>

struct HWND__;
struct HINSTANCE__;

using HWND = HWND__*;
using HINSTANCE = HINSTANCE__*;

class Win32Window : public INativeWindow
{
public:
    bool Create(const Vector2f& screenDims, const std::string& title) override;
    void PollEvents() override;
    bool ShouldClose() const override;
    void Close() override;
    void* GetNativeHandle() override;

    long HandleMessage(unsigned int msg, unsigned long long wParam, long long lParam);
    void SetWindowHandle(HWND hwnd);

private:

    HWND m_windowHandle = nullptr;
    HINSTANCE m_instanceHandle = nullptr;
    Vector2f m_screenDims;
    bool m_shouldClose = false;
};