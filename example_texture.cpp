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

#define STB_IMAGE_IMPLEMENTATION
#include "external/stb_image.h"

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
    // set current directory to the executable directory
    {
        char module_path[512];
        GetModuleFileNameA(0, module_path, sizeof(module_path));

        char *last_slash = module_path;
        char *iter = module_path;
        while (*iter++)
        {
            if (*iter == '\\')
                last_slash = ++iter;
        }
        *last_slash = '\0';

        bool result = SetCurrentDirectoryA(module_path);
        if (result == false)
        {
            OutputDebugString(L"Failed to set current directory\n");
            return 1;
        }
    }

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
            OutputDebugString(L"Failed to register window class\n");
            return GetLastError();
        }
    }

    // create window
    HWND hwnd = CreateWindowEx(
        0,
        L"dx11_wnd_class",
        L"example texture",
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
            OutputDebugString(L"Failed to create device and swapchain\n");
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

    // create vertiex and index buffers
    ID3D11Buffer *vertex_buffer = nullptr;
    ID3D11Buffer *index_buffer = nullptr;
    {
        // vertex buffer
        {
            float vertices[] = {
                // position    uv
                -0.5f,  0.5f,  0.0f, 0.0f, // tl
                 0.5f,  0.5f,  1.0f, 0.0f, // tr
                 0.5f, -0.5f,  1.0f, 1.0f, // br
                -0.5f, -0.5f,  0.0f, 1.0f  // bl
            };

            D3D11_BUFFER_DESC buffer_desc = {};
            buffer_desc.ByteWidth = sizeof(vertices);
            buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            buffer_desc.StructureByteStride = 4 * sizeof(float);

            D3D11_SUBRESOURCE_DATA subresource_data = {};
            subresource_data.pSysMem = vertices;

            HRESULT result = device->CreateBuffer(&buffer_desc, &subresource_data, &vertex_buffer);
            if (FAILED(result))
            {
                OutputDebugString(L"Failed to create vertex buffer\n");
                return GetLastError();
            }
        }
        // index buffer
        {
            unsigned int indices[] = {
                // clockwise
                // first triangle: tl, tr, br
                0, 1, 2,
                // second triangle: tl, br, bl
                0, 2, 3
            };

            D3D11_BUFFER_DESC buffer_desc = {};
            buffer_desc.ByteWidth = sizeof(indices);
            buffer_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;

            D3D11_SUBRESOURCE_DATA subresource_data = {};
            subresource_data.pSysMem = indices;

            HRESULT result = device->CreateBuffer(&buffer_desc, &subresource_data, &index_buffer);
            if (FAILED(result))
            {
                OutputDebugString(L"Failed to create index buffer\n");
                return GetLastError();
            }
        }
    }

    // load image and create a 2d texture
    ID3D11ShaderResourceView *texture_view = nullptr;
    {
        // load image
        unsigned char * data = nullptr;
        int img_width, img_height, img_channels;
        {
            data = stbi_load("data/uv_grid.jpg", &img_width, &img_height, &img_channels, 4);
            if (data == nullptr)
            {
                OutputDebugString(L"Failed to load image\n");
                return 1;
            }
        }

        // craete texture
        ID3D11Texture2D *texture = nullptr;
        {
            D3D11_TEXTURE2D_DESC texture_desc = {};
            texture_desc.Width = img_width;
            texture_desc.Height = img_height;
            texture_desc.MipLevels = 1;
            texture_desc.ArraySize = 1;
            texture_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            texture_desc.SampleDesc.Count = 1;
            texture_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

            D3D11_SUBRESOURCE_DATA subresource_data = {};
            subresource_data.pSysMem = data;
            subresource_data.SysMemPitch = img_width * 4;

            HRESULT result = device->CreateTexture2D(&texture_desc, &subresource_data, &texture);
            if (FAILED(result))
            {
                OutputDebugString(L"Failed to create texture 2d\n");
                return GetLastError();
            }
        }
        stbi_image_free(data);

        // create texture view
        {
            D3D11_SHADER_RESOURCE_VIEW_DESC view_desc = {};
            view_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            view_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            view_desc.Texture2D.MipLevels = 1;
            HRESULT result = device->CreateShaderResourceView(texture, &view_desc, &texture_view);
            if (FAILED(result))
            {
                OutputDebugString(L"Failed to create render target view\n");
                return GetLastError();
            }
        }
        texture->Release();
    }

    // create sampler state
    ID3D11SamplerState *sampler_state = nullptr;
    {
        D3D11_SAMPLER_DESC sampler_desc = {};
        sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        HRESULT result = device->CreateSamplerState(&sampler_desc, &sampler_state);
        if (FAILED(result))
        {
            OutputDebugString(L"Failed to create sampler state\n");
            return GetLastError();
        }
    }

    // create vertex and pixel shaders
    ID3DBlob *vertex_shader_blob = nullptr;
    ID3D11VertexShader *vertex_shader = nullptr;
    ID3D11PixelShader *pixel_shader = nullptr;
    {
        const char shader_src[] = R"(
            struct VS_Out
            {
                float2 uv : TexCoord;
                float4 position : SV_Position;
            };

            VS_Out vs_main(float2 position : Position, float2 uv : TexCoord)
            {
                VS_Out output;
                output.position = float4(position, 0, 1);
                output.uv = uv;
                return output;
            }

            Texture2D tex;
            SamplerState tex_sampler;

            float4 ps_main(float2 uv : TexCoord) : SV_Target
            {
                return tex.Sample(tex_sampler, uv);
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
                OutputDebugString(L"Failed to compile vertex shader\n");
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
                OutputDebugString(L"Failed to compile pixel shader\n");
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
                OutputDebugString(L"Failed to create vertex shader\n");
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
                OutputDebugString(L"Failed to create pixel shader\n");
                return GetLastError();
            }
            pixel_shader_blob->Release();
        }
    }

    // create input layout
    ID3D11InputLayout *input_layout = nullptr;
    {
        D3D11_INPUT_ELEMENT_DESC input_element_desc[] = {
            {"Position", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"TexCoord", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0}
        };

        HRESULT result = device->CreateInputLayout(
            input_element_desc,
            ARRAYSIZE(input_element_desc),
            vertex_shader_blob->GetBufferPointer(),
            vertex_shader_blob->GetBufferSize(), &input_layout);
        if (FAILED(result))
        {
            OutputDebugString(L"Failed to create input layout\n");
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

        // set layout and primitive
        context->IASetInputLayout(input_layout);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // set vertex and index buffer
        UINT stride = 4 * sizeof(float);
        UINT offset = 0;
        context->IASetVertexBuffers(0, 1, &vertex_buffer, &stride, &offset);
        context->IASetIndexBuffer(index_buffer, DXGI_FORMAT_R32_UINT, 0);

        // set vertex and pixel shaders
        context->VSSetShader(vertex_shader, nullptr, 0);
        context->PSSetShader(pixel_shader, nullptr, 0);

        // set texture and sampler
        context->PSSetShaderResources(0, 1, &texture_view);
        context->PSSetSamplers(0, 1, &sampler_state);

        // set viewport
        context->RSSetViewports(1, &viewport);

        // set render target and viewport
        context->OMSetRenderTargets(1, &render_target_view, nullptr);

        // draw
        context->DrawIndexed(6, 0, 0);

        swapchain->Present(1, 0);
    }

    // release resources
    input_layout->Release();
    pixel_shader->Release();
    vertex_shader->Release();
    index_buffer->Release();
    vertex_buffer->Release();
    sampler_state->Release();
    texture_view->Release();
    render_target_view->Release();
    context->Release();
    device->Release();
    swapchain->Release();
    DestroyWindow(hwnd);

    return 0;
}