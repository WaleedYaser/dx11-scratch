#pragma comment(lib, "user32.lib")
#pragma comment(lib, "DXGI.lib")
#pragma comment(lib, "D3D11.lib")

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <d3d11.h>

int
main(int argc, char **argv)
{

    WNDCLASSEXA wnd_class = {};
    wnd_class.cbSize = sizeof(wnd_class);
    wnd_class.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wnd_class.lpfnWndProc = DefWindowProcA;
    wnd_class.lpszClassName = "dx11_scratch_window_class";
    RegisterClassExA(&wnd_class);

    HWND hwnd = CreateWindowExA(
        0,
        wnd_class.lpszClassName,
        "dx11 scratch",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 640, 480,
        nullptr,
        nullptr,
        nullptr,
        nullptr
    );

    HRESULT res;

    IDXGIFactory *factory = nullptr;
    res = CreateDXGIFactory(__uuidof(IDXGIFactory), (void **)&factory);

    DXGI_SWAP_CHAIN_DESC swapchain_desc = {};
    swapchain_desc.BufferDesc.Width = 640;
    swapchain_desc.BufferDesc.Height = 480;
    swapchain_desc.BufferDesc.RefreshRate.Numerator = 60;
    swapchain_desc.BufferDesc.RefreshRate.Denominator = 1;
    swapchain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapchain_desc.SampleDesc.Count = 1;
    swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchain_desc.BufferCount = 1;
    swapchain_desc.OutputWindow = hwnd;
    swapchain_desc.Windowed = TRUE;

    UINT device_flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(_DEBUG)
    device_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL feature_levels_requested[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_9_3,
        D3D_FEATURE_LEVEL_9_1,
    };

    IDXGISwapChain *swapchain;
    ID3D11Device *device;
    D3D_FEATURE_LEVEL feature_level_supported;
    // immediate context, if you have to use the context from another thread you have to create a deferred context
    ID3D11DeviceContext *context;
    res = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        device_flags,
        feature_levels_requested,
        ARRAYSIZE(feature_levels_requested),
        D3D11_SDK_VERSION,
        &swapchain_desc,
        &swapchain,
        &device,
        &feature_level_supported,
        &context
    );

    // get a pointer to back buffer
    ID3D11Texture2D *back_buffer;
    res = swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void **)&back_buffer);

    // create render target view
    ID3D11RenderTargetView *render_target_view;
    device->CreateRenderTargetView(back_buffer, nullptr, &render_target_view);

    // bind view
    context->OMSetRenderTargets(1, &render_target_view, nullptr);

    // setup viewport
    D3D11_VIEWPORT viewport;
    viewport.Width = 640;
    viewport.Height = 480;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    context->RSSetViewports(1, &viewport);

    return 0;
}