#pragma comment(lib, "user32.lib")
#pragma comment(lib, "D3D11.lib")
#pragma comment(lib, "d3dcompiler.lib")

#define WIN32_LEAN_AND_MEAN
#define UNICODE
#include <Windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

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
        L"example cubes",
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

    // get window width and height
    RECT rect;
    GetClientRect(hwnd, &rect);
    int window_width = rect.right - rect.left;
    int window_height = rect.bottom - rect.top;

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

    // create depth target view
    ID3D11DepthStencilView *depth_stencil_view = nullptr;
    {
        // create depth stencil texture
        ID3D11Texture2D *depth_stencil;
        {
            D3D11_TEXTURE2D_DESC texture_desc = {};
            texture_desc.Width = window_width;
            texture_desc.Height = window_height;
            texture_desc.MipLevels = 1;
            texture_desc.ArraySize = 1;
            texture_desc.Format = DXGI_FORMAT_D32_FLOAT;
            texture_desc.SampleDesc.Count = 1;
            texture_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
            HRESULT result = device->CreateTexture2D(&texture_desc, nullptr, &depth_stencil);
            if (FAILED(result))
            {
                OutputDebugString(L"Failed to create device and swapchain");
                return GetLastError();
            }
        }

        // create depth stencil view
        {
            D3D11_DEPTH_STENCIL_VIEW_DESC view_desc = {};
            view_desc.Format = DXGI_FORMAT_D32_FLOAT;
            view_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
            device->CreateDepthStencilView(depth_stencil, &view_desc, &depth_stencil_view);
        }

        depth_stencil->Release();
    }

    // create vertiex and index buffers
    ID3D11Buffer *vertex_buffer = nullptr;
    ID3D11Buffer *index_buffer = nullptr;
    {
        // vertex buffer
        {
            float vertices[] = {
                // position
                -1.0f, -1.0f, -1.0f,
                 1.0f, -1.0f, -1.0f,
                -1.0f,  1.0f, -1.0f,
                 1.0f,  1.0f, -1.0f,
                -1.0f, -1.0f,  1.0f,
                 1.0f, -1.0f,  1.0f,
                -1.0f,  1.0f,  1.0f,
                 1.0f,  1.0f,  1.0f
            };

            D3D11_BUFFER_DESC buffer_desc = {};
            buffer_desc.ByteWidth = sizeof(vertices);
            buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            buffer_desc.StructureByteStride = 3 * sizeof(float);

            D3D11_SUBRESOURCE_DATA subresource_data = {};
            subresource_data.pSysMem = vertices;

            HRESULT result = device->CreateBuffer(&buffer_desc, &subresource_data, &vertex_buffer);
            if (FAILED(result))
            {
                OutputDebugString(L"Failed to create vertex buffer");
                return GetLastError();
            }
        }
        // index buffer
        {
            unsigned int indices[] = {
                // clockwise
                0, 2, 3,  0, 3, 1,
                1, 3, 7,  1, 7, 5,
                5, 7, 6,  5, 6, 4,
                4, 6, 2,  4, 2, 0,
                2, 6, 7,  2, 7, 3,
                0, 1, 5,  0, 5, 4
            };

            D3D11_BUFFER_DESC buffer_desc = {};
            buffer_desc.ByteWidth = sizeof(indices);
            buffer_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;

            D3D11_SUBRESOURCE_DATA subresource_data = {};
            subresource_data.pSysMem = indices;

            HRESULT result = device->CreateBuffer(&buffer_desc, &subresource_data, &index_buffer);
            if (FAILED(result))
            {
                OutputDebugString(L"Failed to create index buffer");
                return GetLastError();
            }
        }
    }

    // create vertex and pixel shaders
    ID3DBlob *vertex_shader_blob = nullptr;
    ID3D11VertexShader *vertex_shader = nullptr;
    ID3D11PixelShader *pixel_shader = nullptr;
    {
        const char shader_src[] = R"(
            cbuffer Transform
            {
                float4x4 mvp;
            };

            float4 vs_main(float3 position : Position) : SV_Position
            {
                return mul(float4(position, 1.0), mvp);
            }

            cbuffer Colors
            {
                float4 colors[6];
            };

            float4 ps_main(uint id: SV_PrimitiveID) : SV_Target
            {
                return colors[id / 2];
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
            {"Position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}
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
        viewport.Width = (float)window_width;
        viewport.Height = (float)window_height;
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
    }

    // create dynamic transform and static colors constant buffers
    ID3D11Buffer *transform_cbuffer = nullptr;
    ID3D11Buffer *colors_cbuffer = nullptr;
    {
        // create transform buffer, dynamic as we will update it every frame
        {
            D3D11_BUFFER_DESC buffer_desc = {};
            buffer_desc.ByteWidth = sizeof(DirectX::XMMATRIX);
            buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

            HRESULT result = device->CreateBuffer(&buffer_desc, nullptr, &transform_cbuffer);
            if (FAILED(result))
            {
                OutputDebugString(L"Failed to create transform constant buffer");
                return GetLastError();
            }
        }
        // create colors buffer, static we won't update it every frame
        {
            float colors[] = {
                1.0f, 0.0f, 0.0f, 1.0f,
                0.0f, 1.0f, 0.0f, 1.0f,
                0.0f, 0.0f, 1.0f, 1.0f,
                1.0f, 1.0f, 0.0f, 1.0f,
                0.0f, 1.0f, 1.0f, 1.0f,
                1.0f, 0.0f, 1.0f, 1.0f
            };

            D3D11_BUFFER_DESC buffer_desc = {};
            buffer_desc.ByteWidth = sizeof(colors);
            buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

            D3D11_SUBRESOURCE_DATA subresource_data = {};
            subresource_data.pSysMem = colors;

            HRESULT result = device->CreateBuffer(&buffer_desc, &subresource_data, &colors_cbuffer);
            if (FAILED(result))
            {
                OutputDebugString(L"Failed to create colors constant buffer");
                return GetLastError();
            }
        }
    }

    // create depth stencil state
    ID3D11DepthStencilState *depth_stencil_state = nullptr;
    {
        D3D11_DEPTH_STENCIL_DESC depth_stencil_desc = {};
        depth_stencil_desc.DepthEnable = TRUE;
        depth_stencil_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        depth_stencil_desc.DepthFunc = D3D11_COMPARISON_LESS;

        HRESULT result = device->CreateDepthStencilState(&depth_stencil_desc, &depth_stencil_state);
        if (FAILED(result))
        {
            OutputDebugString(L"Failed to create constant buffer");
            return GetLastError();
        }
    }

    // create projection matrix
    DirectX::XMMATRIX proj = DirectX::XMMatrixPerspectiveFovLH(
        DirectX::XMConvertToRadians(60.0f),
        viewport.Width / viewport.Height,
        0.1f,
        100.0f);

    // msg loop
    float angle = 0.0f;
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
        context->ClearDepthStencilView(depth_stencil_view, D3D11_CLEAR_DEPTH, 1.0f, 1);

        // set layout and primitive
        context->IASetInputLayout(input_layout);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // set vertex and index buffer
        UINT stride = 3 * sizeof(float);
        UINT offset = 0;
        context->IASetVertexBuffers(0, 1, &vertex_buffer, &stride, &offset);
        context->IASetIndexBuffer(index_buffer, DXGI_FORMAT_R32_UINT, 0);

        // set vertex and pixel shaders
        context->VSSetShader(vertex_shader, nullptr, 0);
        context->PSSetShader(pixel_shader, nullptr, 0);

        // set constant buffers
        context->VSSetConstantBuffers(0, 1, &transform_cbuffer);
        context->PSSetConstantBuffers(0, 1, &colors_cbuffer);

        // set viewport
        context->RSSetViewports(1, &viewport);

        // set render target and viewport
        context->OMSetRenderTargets(1, &render_target_view, depth_stencil_view);

        // set depth stencil state
        context->OMSetDepthStencilState(depth_stencil_state, 1);

        // update first cube transform constant buffer
        {
            angle += (1.0f / 60.0f);
            DirectX::XMMATRIX mvp = DirectX::XMMatrixTranspose(
                DirectX::XMMatrixRotationX(angle) *
                DirectX::XMMatrixRotationY(angle) *
                DirectX::XMMatrixRotationZ(angle) *
                DirectX::XMMatrixTranslation(0.0f, 0.0f, 5.0f) *
                proj
            );
            context->UpdateSubresource(transform_cbuffer, 0, nullptr, &mvp, 0, 0);
        }

        // draw first cube
        context->DrawIndexed(36, 0, 0);

        // update second cube transform constant buffer
        {
            DirectX::XMMATRIX mvp = DirectX::XMMatrixTranspose(
                DirectX::XMMatrixRotationX(angle / 2.0f) *
                DirectX::XMMatrixRotationY(angle / 2.0f) *
                DirectX::XMMatrixRotationZ(angle / 2.0f) *
                DirectX::XMMatrixTranslation(0.0f, 0.0f, 5.0f) *
                proj
            );
            context->UpdateSubresource(transform_cbuffer, 0, nullptr, &mvp, 0, 0);
        }

        // draw second cube
        context->DrawIndexed(36, 0, 0);

        swapchain->Present(1, 0);
    }

    // release resources
    depth_stencil_state->Release();
    colors_cbuffer->Release();
    transform_cbuffer->Release();
    input_layout->Release();
    pixel_shader->Release();
    vertex_shader->Release();
    index_buffer->Release();
    vertex_buffer->Release();
    depth_stencil_view->Release();
    render_target_view->Release();
    context->Release();
    device->Release();
    swapchain->Release();
    DestroyWindow(hwnd);

    return 0;
}