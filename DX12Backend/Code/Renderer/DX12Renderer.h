#pragma once


#include <Engine/Interface/Renderer/IRenderer.h>
#include <Utilities/Vector2.h>
#include <string>
#include <memory>

class IRenderable;
struct DX12RendererImpl;

class DX12Renderer : public IRenderer
{
public:
    DX12Renderer();
    ~DX12Renderer() override;

    bool Initialise(const Vector2f& screenDims, const std::string& title) override;
    void PollWindowEvents() override;
    void Clear() override;
    void Draw(IRenderable* object) override;
    void Draw(IRenderable* object, IShader* shader) override;
    void Present() override;

private:

    bool IntialiseDX12(const Vector2f& screenDims);

    bool CreateDX12Device();
    bool CreateCommandQueueAndList();
    bool CreateSwapChain(const Vector2f& screenDims);
    bool CreateDescriptorHeaps();
    bool CreateRenderTargetView();
    bool CreateDepthStencilBuffer(const Vector2f& screenDims);
    void UpdateViewportAndScissor(const Vector2f& screenDims);

    void FlushCommandQueue();

    std::unique_ptr<DX12RendererImpl> m_impl;
};