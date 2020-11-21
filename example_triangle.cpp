#pragma comment(lib, "user32.lib")
#pragma comment(lib, "D3D11.lib")
#pragma comment(lib, "d3dcompiler.lib")

#define WIN32_LEAN_AND_MEAN
#define UNICODE
#include <Windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>

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

    // create vertices buffer
    ID3D11Buffer *vertex_buffer = nullptr;
    {
        float vertices[] = {
             0.0f,  0.5f,
             0.5f, -0.5f,
            -0.5f, -0.5f
        };

        D3D11_BUFFER_DESC buffer_desc = {};
        buffer_desc.ByteWidth = sizeof(vertices);
        buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        buffer_desc.StructureByteStride = 2 * sizeof(float);

        D3D11_SUBRESOURCE_DATA subresource_data = {};
        subresource_data.pSysMem = vertices;

        HRESULT result = device->CreateBuffer(&buffer_desc, &subresource_data, &vertex_buffer);
        if (FAILED(result))
        {
            OutputDebugString(L"Failed to create vertex buffer");
            return GetLastError();
        }
    }

    // create vertex and pixel shaders
    ID3DBlob *vertex_shader_blob = nullptr;
    ID3D11VertexShader *vertex_shader = nullptr;
    ID3D11PixelShader *pixel_shader = nullptr;
    {
        const char shader_src[] = R"(
            float4 vs_main(float2 position : Position) : SV_Position
            {
                return float4(position, 0, 1);
            }

            float4 ps_main() : SV_Target
            {
                return float4(1, 1, 1, 1);
            }
        )";

        // compile vertex shader
        {
            ID3DBlob *error_blob = nullptr;
            HRESULT result = D3DCompile(
                shader_src,
                sizeof(shader_src),
                nullptr,
                nullptr,
                nullptr,
                "vs_main",
                "vs_5_0",
                0,
                0,
                &vertex_shader_blob,
                &error_blob);
            if (FAILED(result))
            {
                OutputDebugString(L"Failed to compile vertex shader");
                OutputDebugStringA((char *)error_blob->GetBufferPointer());
                return GetLastError();
            }
        }

        // compile pixel shader
        ID3DBlob *pixel_shader_blob = nullptr;
        {
            ID3DBlob *error_blob = nullptr;
            HRESULT result = D3DCompile(
                shader_src,
                sizeof(shader_src),
                nullptr,
                nullptr,
                nullptr,
                "ps_main",
                "ps_5_0",
                0,
                0,
                &pixel_shader_blob,
                &error_blob);
            if (FAILED(result))
            {
                OutputDebugString(L"Failed to compile pixel shader");
                OutputDebugStringA((char *)error_blob->GetBufferPointer());
                return GetLastError();
            }
        }

        // create vertex shader
        {
            HRESULT result = device->CreateVertexShader(
                vertex_shader_blob->GetBufferPointer(),
                vertex_shader_blob->GetBufferSize(),
                nullptr,
                &vertex_shader);
            if (FAILED(result))
            {
                OutputDebugString(L"Failed to create vertex shader");
                return GetLastError();
            }
        }

        // create pixel shader
        {
            HRESULT result = device->CreatePixelShader(
                pixel_shader_blob->GetBufferPointer(),
                pixel_shader_blob->GetBufferSize(),
                nullptr,
                &pixel_shader);
            if (FAILED(result))
            {
                OutputDebugString(L"Failed to create pixel shader");
                return GetLastError();
            }
            pixel_shader_blob->Release();
        }
    }

    // create input layout
    ID3D11InputLayout *input_layout = nullptr;
    {
        D3D11_INPUT_ELEMENT_DESC input_element_desc[] = {
            {"Position", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}
        };

        HRESULT result = device->CreateInputLayout(
            input_element_desc,
            ARRAYSIZE(input_element_desc),
            vertex_shader_blob->GetBufferPointer(),
            vertex_shader_blob->GetBufferSize(), &input_layout);
        if (FAILED(result))
        {
            OutputDebugString(L"Failed to create input layout");
            return GetLastError();
        }
        vertex_shader_blob->Release();
    }

    // create viewport
    D3D11_VIEWPORT viewport = {};
    {
        // get window width and height
        RECT rect;
        GetClientRect(hwnd, &rect);
        int window_width = rect.right - rect.left;
        int window_height = rect.bottom - rect.top;

        viewport.Width = (float)window_width;
        viewport.Height = (float)window_height;
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
        float clear_color[4] = {0.0f, 0.0f, 0.0f, 1.0f};
        context->ClearRenderTargetView(render_target_view, clear_color);

        // set vertex buffer in input assempler stage
        UINT stride = 2 * sizeof(float);
        UINT offset = 0;
        context->IASetVertexBuffers(0, 1, &vertex_buffer, &stride, &offset);
        context->IASetInputLayout(input_layout);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // set vertex and pixel shaders
        context->VSSetShader(vertex_shader, nullptr, 0);
        context->PSSetShader(pixel_shader, nullptr, 0);

        // set viewport
        context->RSSetViewports(1, &viewport);

        // set render target and viewport
        context->OMSetRenderTargets(1, &render_target_view, nullptr);

        // draw
        context->Draw(3, 0);

        swapchain->Present(1, 0);
    }

    // release resources
    input_layout->Release();
    pixel_shader->Release();
    vertex_shader->Release();
    vertex_buffer->Release();
    render_target_view->Release();
    context->Release();
    device->Release();
    swapchain->Release();
    DestroyWindow(hwnd);

    return 0;
}