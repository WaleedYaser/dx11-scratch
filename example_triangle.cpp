#pragma comment(lib, "user32.lib")
#pragma comment(lib, "D3D11.lib")

#define WIN32_LEAN_AND_MEAN
#define UNICODE
#include <Windows.h>
#include <d3d11.h>

#if defined(min)
#undef min
#endif

#if defined(max)
#undef max
#endif

LRESULT CALLBACK
window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
        case WM_CLOSE:
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProc(hwnd, msg, wparam, lparam);
            break;
    }
}

int
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pCmdLine, int nCmdShow)
{
    // register window class
    {
        WNDCLASSEX wnd_class = {};
        wnd_class.cbSize = sizeof(wnd_class);
        wnd_class.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wnd_class.lpfnWndProc = window_proc;
        wnd_class.hInstance = hInstance;
        wnd_class.lpszClassName = L"dx11_wnd_class";

        if (RegisterClassEx(&wnd_class) == 0)
        {
            OutputDebugString(L"Failed to register window class");
            return GetLastError();
        }
    }

    // create window
    HWND hwnd = CreateWindowEx(
        0,
        L"dx11_wnd_class",
        L"example clear",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        nullptr,
        nullptr,
        nullptr,
        nullptr
    );
    if (hwnd == nullptr)
    {
        OutputDebugString(L"Failed to create window");
        return GetLastError();
    }

    // create dx11 swapchain, device, and immediate context
    IDXGISwapChain *swapchain = nullptr;
    ID3D11Device *device = nullptr;
    ID3D11DeviceContext *context = nullptr;
    {
        D3D_FEATURE_LEVEL feature_levels_requested[] = {
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0,
        };
        DXGI_SWAP_CHAIN_DESC swapchain_desc = {};
        swapchain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapchain_desc.SampleDesc.Count = 1;
        swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapchain_desc.BufferCount = 2;
        swapchain_desc.OutputWindow = hwnd;
        swapchain_desc.Windowed = TRUE;
        swapchain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        HRESULT result = D3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            D3D11_CREATE_DEVICE_DEBUG,
            feature_levels_requested,
            ARRAYSIZE(feature_levels_requested),
            D3D11_SDK_VERSION,
            &swapchain_desc,
            &swapchain,
            &device,
            nullptr,
            &context
        );
        if (FAILED(result))
        {
            OutputDebugString(L"Failed to create device and swapchain");
            return GetLastError();
        }
    }

    // create render target view
    ID3D11RenderTargetView *render_target_view = nullptr;
    {
        ID3D11Texture2D *back_buffer;
        swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void **)&back_buffer);

        device->CreateRenderTargetView(back_buffer, nullptr, &render_target_view);

        back_buffer->Release();
    }

    // msg loop
    bool running = true;
    while (running)
    {
        MSG msg = {};
        PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE);
        TranslateMessage(&msg);
        DispatchMessage(&msg);

        switch (msg.message)
        {
            case WM_QUIT:
                running = false;
                break;
        }

        // clear frame using red color
        float clear_color[4] = {1.0f, 0.0f, 0.0f, 1.0f};
        context->ClearRenderTargetView(render_target_view, clear_color);
        swapchain->Present(1, 0);
    }

    // release resources
    render_target_view->Release();
    context->Release();
    device->Release();
    swapchain->Release();
    DestroyWindow(hwnd);

    return 0;
}