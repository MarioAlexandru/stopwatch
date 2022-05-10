#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>
#include <WindowsX.h>
#include <d2d1.h>
#include <atlbase.h>
#include <dwrite.h>
#include <wincodec.h>
#include <string>

#pragma comment(lib, "d2d1")
#pragma comment(lib, "Dwrite")

#include "BaseWin.h"
#include "Scene.h"

class Button
{
public:

    D2D_RECT_F rect{};
    bool IsHovered;
    bool Down;
    bool Up;

    Button(D2D_RECT_F rectangle = { 0,0,0,0 }) : rect(rectangle), IsHovered(false), Down(false), Up(false)
    {
    }
};

void LoadImageFromFile(LPCWSTR fileName, ID2D1Bitmap** pD2DBitmap, ID2D1HwndRenderTarget* pRenderTarget)
{
    IWICImagingFactory* pIWICFactory{};
    HRESULT hr = CoCreateInstance(
        CLSID_WICImagingFactory,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&pIWICFactory)
    );

    IWICBitmapDecoder* pDecoder{};
    if (SUCCEEDED(hr))
    {
        hr = pIWICFactory->CreateDecoderFromFilename(
            fileName,                      
            NULL,                          
            GENERIC_READ,                  
            WICDecodeMetadataCacheOnDemand,
            &pDecoder                      
        );
    }

    IWICBitmapFrameDecode* pFrame{};
    if (SUCCEEDED(hr))
    {
        hr = pDecoder->GetFrame(0, &pFrame);
    }

    IWICFormatConverter* pConvertedSourceBitmap{};
    pIWICFactory->CreateFormatConverter(&pConvertedSourceBitmap);
    hr = pConvertedSourceBitmap->Initialize(
        pFrame,                       
        GUID_WICPixelFormat32bppPBGRA,
        WICBitmapDitherTypeNone,      
        NULL,                         
        0.f,                          
        WICBitmapPaletteTypeCustom    
    );

    pRenderTarget->CreateBitmapFromWicBitmap(pConvertedSourceBitmap, NULL, pD2DBitmap);

    pIWICFactory->Release();
    pDecoder->Release();
    pFrame->Release();
    pConvertedSourceBitmap->Release();
}

void LoadImageFromResource(int IMAGE_ID, ID2D1Bitmap** pD2DBitmap, ID2D1HwndRenderTarget* pRenderTarget)
{
    IWICImagingFactory* pIWICFactory{};
    HRESULT hr = CoCreateInstance(
        CLSID_WICImagingFactory,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&pIWICFactory)
    );

    IWICStream* pIWICStream = NULL;
    IWICBitmapDecoder* pIDecoder = NULL;
    IWICBitmapFrameDecode* pIDecoderFrame = NULL;

    HRSRC imageResHandle = NULL;
    HGLOBAL imageResDataHandle = NULL;
    void* pImageFile = NULL;
    DWORD imageFileSize = 0;

    imageResHandle = FindResource(
        NULL,
        MAKEINTRESOURCE(IMAGE_ID),
        _T("PNG"));

    hr = (imageResHandle ? S_OK : E_FAIL);

    if (SUCCEEDED(hr)) {
        imageResDataHandle = LoadResource(NULL, imageResHandle);
        hr = (imageResDataHandle ? S_OK : E_FAIL);
    }

    if (SUCCEEDED(hr)) {
        pImageFile = LockResource(imageResDataHandle);
        hr = (pImageFile ? S_OK : E_FAIL);
    }

    if (SUCCEEDED(hr)) {
        imageFileSize = SizeofResource(NULL, imageResHandle);
        hr = (imageFileSize ? S_OK : E_FAIL);
    }

    if (SUCCEEDED(hr)) {
        hr = pIWICFactory->CreateStream(&pIWICStream);
    }

    if (SUCCEEDED(hr)) {
        hr = pIWICStream->InitializeFromMemory(
            reinterpret_cast<BYTE*>(pImageFile),
            imageFileSize);
    }

    if (SUCCEEDED(hr)) {
        hr = pIWICFactory->CreateDecoderFromStream(
            pIWICStream,                  
            NULL,                         
            WICDecodeMetadataCacheOnLoad, 
            &pIDecoder);                  
    }

    if (SUCCEEDED(hr)) {
        hr = pIDecoder->GetFrame(0, &pIDecoderFrame);
    }

    IWICFormatConverter* pConvertedSourceBitmap{};
    pIWICFactory->CreateFormatConverter(&pConvertedSourceBitmap);
    hr = pConvertedSourceBitmap->Initialize(
        pIDecoderFrame,               
        GUID_WICPixelFormat32bppPBGRA,
        WICBitmapDitherTypeNone,      
        NULL,                          
        0.f,                          
        WICBitmapPaletteTypeCustom    
    );

    pRenderTarget->CreateBitmapFromWicBitmap(pConvertedSourceBitmap, NULL, pD2DBitmap);

    pIWICFactory->Release();
    pIWICStream->Release();
    pIDecoder->Release();
    pIDecoderFrame->Release();
    pConvertedSourceBitmap->Release();
}

class Time
{
    using large_int = long long;


    large_int time;

public:

    Time(large_int time) : time(time)
    {
    }

    large_int asMicroseconds()
    {
        return time;
    }
    large_int asMilliseconds()
    {
        return time / 1000;
    }
    large_int asSeconds()
    {
        return time / 1000000;
    }
};

class Clock
{
    LARGE_INTEGER m_StartingTime{};
    LARGE_INTEGER m_EndingTime{};
    LARGE_INTEGER m_Frequency{};
    long long m_ElapsedTime{};
    bool m_ClockPaused;

public:

    Clock(bool paused) : m_ClockPaused(!paused)
    {
        QueryPerformanceFrequency(&m_Frequency);
        QueryPerformanceCounter(&m_StartingTime);
    }

    Time GetElapsedTime()
    {
        if (!m_ClockPaused)
        {
            QueryPerformanceCounter(&m_EndingTime);

            m_ElapsedTime += ((m_EndingTime.QuadPart - m_StartingTime.QuadPart) * 1000000) / m_Frequency.QuadPart;

            QueryPerformanceCounter(&m_StartingTime);
        }

        return Time(m_ElapsedTime);
    }

    void Reset()
    {
        m_ElapsedTime = 0;
    }

    void Pause()
    {
        m_ClockPaused = true;
    }

    void Restart()
    {
        if (m_ClockPaused)
        {
            QueryPerformanceCounter(&m_StartingTime);
            m_ClockPaused = false;
        }
    }
};

class Scene : public GraphicsScene
{
    CComPtr<ID2D1SolidColorBrush> m_pFill;
    CComPtr<ID2D1SolidColorBrush> m_pBlackBrush;
    CComPtr<ID2D1SolidColorBrush> m_pDarkBlueBrush;
    CComPtr<ID2D1SolidColorBrush> m_pDarkerBlueBrush;
    CComPtr<IDWriteFactory>       m_pDWriteFactory;
    CComPtr<IDWriteTextFormat>    m_pTextFormat;
    CComPtr<IDWriteTextLayout>    m_pDWriteTextLayout;
    CComPtr<ID2D1Bitmap>          m_PlayImg;
    CComPtr<ID2D1Bitmap>          m_PauseImg;
    CComPtr<ID2D1Bitmap>          m_ResetImg;
    CComPtr<ID2D1Bitmap>          m_FullscreenImg;

    D2D1_ELLIPSE          m_ellipse{};
    D2D_POINT_2F          m_Ticks[24]{};
    D2D_POINT_2F          m_origin{};
    D2D_RECT_F            m_PlayImgRect{};
    D2D_RECT_F            m_ResetImgRect{};
    D2D_RECT_F            m_FullscreenImgRect{};
    WINDOWPLACEMENT       m_wpPrev{ sizeof(m_wpPrev) };

    bool m_TimerPaused{};
    bool m_ChangedScreenMode{};
    float m_multiplier{};

    HRESULT CreateDeviceIndependentResources() { return S_OK; }
    void    DiscardDeviceIndependentResources() { }
    HRESULT CreateDeviceDependentResources();
    void    DiscardDeviceDependentResources();
    void    CalculateLayout();
    void    RenderScene();

    void    DrawClockHand(float fHandLength, float fAngle, float fStrokeWidth);
    void    DrawTimer();
    void    DrawButtons();

public:
    void SwitchScreenMode();

    Button      m_PlayButton;
    Button      m_ResetButton;
    Button      m_FullscreenButton;

    DWRITE_TEXT_METRICS   m_tm{};
};

HRESULT Scene::CreateDeviceDependentResources()
{
    m_multiplier = 1;

    HRESULT hr = m_pRenderTarget->CreateSolidColorBrush(
        D2D1::ColorF(0.6f, 0.6f, 0.6f),
        D2D1::BrushProperties(),
        &m_pFill
    );

    if (SUCCEEDED(hr))
    {
        hr = m_pRenderTarget->CreateSolidColorBrush(
            D2D1::ColorF(0, 0, 0),
            D2D1::BrushProperties(),
            &m_pBlackBrush
        );
    }

    if (SUCCEEDED(hr))
    {
        D2D1_COLOR_F color;
        color.r = 0.42f;
        color.g = 0.70f;
        color.b = 0.90f;
        color.a = 1;

        hr = m_pRenderTarget->CreateSolidColorBrush(
            color,
            D2D1::BrushProperties(),
            &m_pDarkerBlueBrush
        );
    }

    if (SUCCEEDED(hr))
    {
        D2D1_COLOR_F color;
        color.r = 0.47f;
        color.g = 0.75f;
        color.b = 0.92f;
        color.a = 1;

        m_pRenderTarget->CreateSolidColorBrush(
            color,
            D2D1::BrushProperties(),
            &m_pDarkBlueBrush);
    }

    if (SUCCEEDED(hr))
    {
        hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
            __uuidof(IDWriteFactory),
            reinterpret_cast<IUnknown**>(&m_pDWriteFactory));
    }

    if (SUCCEEDED(hr))
    {
        m_pDWriteFactory->CreateTextFormat(
            L"Segoe UI Black",
            NULL,
            DWRITE_FONT_WEIGHT_BOLD,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            100.0f * m_multiplier * 96.0f / 72.0f,
            L"en-US",
            &m_pTextFormat
        );
    }

    if (SUCCEEDED(hr))
    {
        D2D1_SIZE_F fSize = m_pRenderTarget->GetSize();

        m_pDWriteFactory->CreateTextLayout(
            L"00:00:00.00",
            wcslen(L"00:00:00.00"),
            m_pTextFormat,
            fSize.width,
            fSize.height,
            &m_pDWriteTextLayout);

        DWRITE_TEXT_RANGE textRange{ 8,3 };
        m_pDWriteTextLayout->SetFontSize(60.0f, textRange);

        m_pDWriteTextLayout->GetMetrics(&m_tm);
    }

    if (SUCCEEDED(hr))
    {
        LoadImageFromResource(103, &m_PauseImg, m_pRenderTarget);
        LoadImageFromResource(104, &m_PlayImg, m_pRenderTarget);
        LoadImageFromResource(105, &m_FullscreenImg, m_pRenderTarget);
        LoadImageFromResource(106, &m_ResetImg, m_pRenderTarget);
    }

    m_TimerPaused = true;

    return hr;
}

void Scene::SwitchScreenMode()
{
    HWND hwnd = m_pRenderTarget->GetHwnd();

    DWORD dwStyle = GetWindowLong(hwnd, GWL_STYLE);

    if (dwStyle & WS_OVERLAPPEDWINDOW)
    {
        MONITORINFO mi = { sizeof(mi) };
        if (GetWindowPlacement(hwnd, &m_wpPrev) && GetMonitorInfo(MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY), &mi))
        {
            SetWindowLong(hwnd, GWL_STYLE, dwStyle & ~WS_OVERLAPPEDWINDOW);
            SetWindowPos(hwnd, HWND_TOP,
                mi.rcMonitor.left,
                mi.rcMonitor.top,
                mi.rcMonitor.right - mi.rcMonitor.left,
                mi.rcMonitor.bottom - mi.rcMonitor.top,
                SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }

        m_multiplier = 1.5f;
    }
    else
    {
        SetWindowLong(hwnd, GWL_STYLE, dwStyle | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(hwnd, &m_wpPrev);
        SetWindowPos(hwnd, NULL, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
            SWP_NOOWNERZORDER | SWP_FRAMECHANGED);

        m_multiplier = 1.f;
    }

    m_pTextFormat.Release();
    m_pDWriteFactory->CreateTextFormat(
        L"Segoe UI Black",
        NULL,
        DWRITE_FONT_WEIGHT_BOLD,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        100.0f * m_multiplier * 96.0f / 72.0f,
        L"en-US",
        &m_pTextFormat
    );
    m_ChangedScreenMode = true;
}

void Scene::DrawClockHand(float fHandLength, float fAngle, float fStrokeWidth)
{
    m_pRenderTarget->SetTransform(
        D2D1::Matrix3x2F::Rotation(fAngle, m_ellipse.point)
    );

    D2D_POINT_2F endPoint = D2D1::Point2F(
        m_ellipse.point.x,
        m_ellipse.point.y - (m_ellipse.radiusY * fHandLength)
    );

    m_pRenderTarget->DrawLine(m_ellipse.point, endPoint, m_pBlackBrush, fStrokeWidth);
}


void Scene::RenderScene()
{
    m_pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::SkyBlue));
    DrawTimer();
    DrawButtons();
}

void Scene::DrawTimer()
{
    static Clock clock(false);

    if (m_PlayButton.Up)
    {
        m_PlayButton.Up = false;
        if (m_TimerPaused)
        {
            m_TimerPaused = false;
            clock.Restart();
        }
        else
        {
            m_TimerPaused = true;
            clock.Pause();
        }
    }

    if (m_ResetButton.Up)
    {
        m_ResetButton.Up = false;
        clock.Reset();
        m_TimerPaused = true;
        clock.Pause();
    }

    long long hours{ (clock.GetElapsedTime().asSeconds() / 3600) % 24 };
    long long minutes{ (clock.GetElapsedTime().asSeconds() / 60) % 60 };
    long long seconds{ clock.GetElapsedTime().asSeconds() % 60 };
    long long milliseconds{ (clock.GetElapsedTime().asMilliseconds() % 1000) / 10 };

    std::string string{};

    if (hours < 10)
    {
        string += ("0" + std::to_string(hours) + ":");
    }
    else
    {
        string += (std::to_string(hours) + ":");
    }

    if (minutes < 10)
    {
        string += ("0" + std::to_string(minutes) + ":");
    }
    else
    {
        string += (std::to_string(minutes) + ":");
    }

    if (seconds < 10)
    {
        string += ("0" + std::to_string(seconds) + ".");
    }
    else
    {
        string += (std::to_string(seconds) + ".");
    }

    if (milliseconds < 10)
    {
        string += ("0" + std::to_string(milliseconds));
    }
    else
    {
        string += std::to_string(milliseconds);
    }

    CA2W w_string(string.c_str());

    D2D1_SIZE_F fSize = m_pRenderTarget->GetSize();
    if (m_pDWriteTextLayout)
    {
        m_pDWriteTextLayout.Release();
    }

    m_pDWriteFactory->CreateTextLayout(
        w_string,
        wcslen(w_string),
        m_pTextFormat,
        fSize.width,
        fSize.height,
        &m_pDWriteTextLayout
    );

    DWRITE_TEXT_RANGE textRange{ 8,3 };
    m_pDWriteTextLayout->SetFontSize(m_pDWriteTextLayout->GetFontSize() * 0.5f, textRange);

    if (m_ChangedScreenMode)
    {
        m_pDWriteTextLayout->GetMetrics(&m_tm);

        m_FullscreenImg.Release();
        if (m_multiplier > 1.1f)
        {
            LoadImageFromResource(107, &m_FullscreenImg, m_pRenderTarget);
        }
        else
        {
            LoadImageFromResource(105, &m_FullscreenImg, m_pRenderTarget);
        }

        CalculateLayout();
        m_ChangedScreenMode = false;
    }
    m_pRenderTarget->DrawTextLayout(m_origin, m_pDWriteTextLayout, m_pBlackBrush);
}

void Scene::DrawButtons()
{
    int size = (int)(22 * m_multiplier);
    int offset = 0;
    if (m_TimerPaused)
    {
        offset = (int)(2 * m_multiplier);
    }

    if (m_PlayButton.Down)
    {
        size = (int)(18 * m_multiplier);

        m_pRenderTarget->DrawEllipse(m_ellipse, m_pBlackBrush, 10 * m_multiplier);
        m_pRenderTarget->FillEllipse(m_ellipse, m_pDarkerBlueBrush);
    }
    else if (m_PlayButton.IsHovered)
    {

        m_pRenderTarget->DrawEllipse(m_ellipse, m_pBlackBrush, 6 * m_multiplier);
        m_pRenderTarget->FillEllipse(m_ellipse, m_pDarkBlueBrush);
    }
    else
    {
        m_pRenderTarget->DrawEllipse(m_ellipse, m_pBlackBrush, 6 * m_multiplier);
    }
    m_PlayImgRect = D2D_RECT_F{ m_ellipse.point.x - size / 2 + offset, m_ellipse.point.y - size / 2, m_ellipse.point.x + size / 2 + offset, m_ellipse.point.y + size / 2 };
    if (m_TimerPaused)
    {
        m_pRenderTarget->DrawBitmap(m_PlayImg, m_PlayImgRect);
    }
    else m_pRenderTarget->DrawBitmap(m_PauseImg, m_PlayImgRect);



    size = (int)(30 * m_multiplier);
    if (m_ResetButton.Down)
    {
        size = (int)(26 * m_multiplier);
        m_pRenderTarget->FillRectangle(m_ResetButton.rect, m_pDarkerBlueBrush);
    }
    else if (m_ResetButton.IsHovered)
    {
        m_pRenderTarget->FillRectangle(m_ResetButton.rect, m_pDarkBlueBrush);
    }
    m_ResetImgRect = D2D_RECT_F{ m_ellipse.point.x - 150 * m_multiplier - size / 2, m_ellipse.point.y - size / 2, m_ellipse.point.x - 150 * m_multiplier + size / 2, m_ellipse.point.y + size / 2 };
    m_pRenderTarget->DrawBitmap(m_ResetImg, m_ResetImgRect);


    size = (int)(30 * m_multiplier);
    if (m_FullscreenButton.Down)
    {
        size = (int)(26 * m_multiplier);
        m_pRenderTarget->FillRectangle(m_FullscreenButton.rect, m_pDarkerBlueBrush);
    }
    else if (m_FullscreenButton.IsHovered)
    {
        m_pRenderTarget->FillRectangle(m_FullscreenButton.rect, m_pDarkBlueBrush);
    }
    m_FullscreenImgRect = D2D_RECT_F{ m_ellipse.point.x + 150 * m_multiplier - size / 2, m_ellipse.point.y - size / 2, m_ellipse.point.x + 150 * m_multiplier + size / 2, m_ellipse.point.y + size / 2 };
    m_pRenderTarget->DrawBitmap(m_FullscreenImg, m_FullscreenImgRect);
}

void Scene::CalculateLayout()
{
    RECT rc;
    GetClientRect(m_pRenderTarget->GetHwnd(), &rc);
    m_origin = D2D1::Point2F((rc.right - m_tm.width) / 2, ((rc.bottom - (m_tm.height + 45 * m_multiplier) - 130 * m_multiplier) / 2));

    m_ellipse = D2D1::Ellipse(D2D1::Point2F(m_origin.x + m_tm.width / 2, m_origin.y + m_tm.height + 80 * m_multiplier), 44 * m_multiplier, 44 * m_multiplier);

    m_PlayButton.rect = D2D1_RECT_F{ m_ellipse.point.x - 50 * m_multiplier, m_ellipse.point.y - 50 * m_multiplier, m_ellipse.point.x + 50 * m_multiplier, m_ellipse.point.y + 50 * m_multiplier };
    m_ResetButton.rect = D2D1_RECT_F{ m_ellipse.point.x - 200 * m_multiplier, m_ellipse.point.y - 50 * m_multiplier, m_ellipse.point.x - 100 * m_multiplier, m_ellipse.point.y + 50 * m_multiplier };
    m_FullscreenButton.rect = D2D1_RECT_F{ m_ellipse.point.x + 100 * m_multiplier, m_ellipse.point.y - 50 * m_multiplier, m_ellipse.point.x + 200 * m_multiplier, m_ellipse.point.y + 50 * m_multiplier };
}


void Scene::DiscardDeviceDependentResources()
{
    m_pFill.Release();
    m_pBlackBrush.Release();
    m_pDarkBlueBrush.Release();
    m_pDarkerBlueBrush.Release();
    m_pDWriteFactory.Release();
    m_pTextFormat.Release();
    m_pDWriteTextLayout.Release();
    m_PlayImg.Release();
    m_PauseImg.Release();
    m_ResetImg.Release();
    m_FullscreenImg.Release();
}


class MainWindow : public BaseWindow<MainWindow>
{
    HANDLE      m_hTimer{};
    UINT_PTR    IDT_TIMER1{};
    HANDLE      hThread{};
    DWORD       dwThreadId{};
    Scene       m_scene;

    int SZ_x{};
    int SZ_y{};
    bool sizing;
    bool resize;
    bool EndPainting;
    bool StartPainting;

    BOOL    InitializeTimer();

public:

    MainWindow() : sizing(false), EndPainting(false), StartPainting(false), resize(false)
    {
    }

    void    Draw();
    void    WaitTimer();

    PCWSTR  ClassName() const { return L"Clock Window Class"; }
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
};

const WCHAR WINDOW_NAME[] = L"Stopwatch";

void MainWindow::Draw()
{
    while (!EndPainting)
    {
        if (resize)
        {
            m_scene.Resize(SZ_x, SZ_y);
            resize = false;
        }
        if (!sizing)
        {
            PAINTSTRUCT ps;
            BeginPaint(m_hwnd, &ps);
            m_scene.Render(m_hwnd);
            EndPaint(m_hwnd, &ps);
        }
        Sleep(15);
    }
}

DWORD WINAPI Paint(LPVOID lpParam)
{
    if (FAILED(CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE)))
    {
        return 0;
    }

    ((MainWindow*)(lpParam))->Draw();

    CoUninitialize();
    return 0;
}


INT WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ PWSTR, _In_ INT nCmdShow)
{
    
    if (FAILED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE)))
    {
        return 0;
    }
    
    MainWindow win;

    if (!win.Create(WINDOW_NAME, WS_OVERLAPPEDWINDOW))
    {
        return 0;
    }

    ShowWindow(win.Window(), nCmdShow);

    MSG msg = { };
    while (msg.message != WM_QUIT)
    {

        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        win.WaitTimer();
    }

    CoUninitialize();
    
    return 0;
}

LRESULT MainWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HWND hwnd = m_hwnd;

    switch (uMsg)
    {
        
    case WM_CREATE:
    {
        if (FAILED(m_scene.Initialize()) || !InitializeTimer())
        {
            PostQuitMessage(0);
            return -1;
        }
    }
    return 0;


    case WM_DESTROY:
        EndPainting = true;
        if (WaitForSingleObject(hThread, INFINITE) == WAIT_OBJECT_0)
        {
            m_scene.CleanUp();
            PostQuitMessage(0);
        }
        return 0;

        
    case WM_PAINT:
    case WM_DISPLAYCHANGE:
    case WM_TIMER:
    {
        if (!StartPainting)
        {
            StartPainting = true;
            hThread = CreateThread(NULL, 0, Paint, this, 0, &dwThreadId);
        }
        if (sizing)
        {
            PAINTSTRUCT ps;
            BeginPaint(m_hwnd, &ps);
            m_scene.Render(m_hwnd);
            EndPaint(m_hwnd, &ps);
        }
    }
    return 0;

    case WM_SIZE:
    {
        SZ_x = (int)(short)LOWORD(lParam);
        SZ_y = (int)(short)HIWORD(lParam);

        resize = true;
        InvalidateRect(m_hwnd, NULL, FALSE);
    }
    return 0;

    case WM_ERASEBKGND:
        return 1;

    case WM_NCLBUTTONDOWN:
    {
        switch (wParam)
        {

        case HTLEFT:
        case HTTOPLEFT:
        case HTTOP:
        case HTTOPRIGHT:
        case HTRIGHT:
        case HTBOTTOMRIGHT:
        case HTBOTTOM:
        case HTBOTTOMLEFT:
        {
            sizing = true;
            SetTimer(hwnd, IDT_TIMER1, 10, (TIMERPROC)NULL);
        }

        }

        DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;


    case WM_EXITSIZEMOVE:
    {
        sizing = false;
        KillTimer(hwnd, IDT_TIMER1);
    }
    return 0;


    case WM_SETCURSOR:
    {
        if (LOWORD(lParam) == HTCLIENT)
        {
            HCURSOR hCursor = LoadCursor(NULL, IDC_ARROW);
            SetCursor(hCursor);
            return TRUE;
        }
        else DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;

    case WM_MOUSEMOVE:
    {
        int xPos = GET_X_LPARAM(lParam);
        int yPos = GET_Y_LPARAM(lParam);

        if (xPos > m_scene.m_PlayButton.rect.left && xPos < m_scene.m_PlayButton.rect.right && yPos > m_scene.m_PlayButton.rect.top && yPos < m_scene.m_PlayButton.rect.bottom)
        {
            m_scene.m_PlayButton.IsHovered = true;
        }
        else m_scene.m_PlayButton.IsHovered = false;

        if (xPos > m_scene.m_ResetButton.rect.left && xPos < m_scene.m_ResetButton.rect.right && yPos > m_scene.m_ResetButton.rect.top && yPos < m_scene.m_ResetButton.rect.bottom)
        {
            m_scene.m_ResetButton.IsHovered = true;
        }
        else m_scene.m_ResetButton.IsHovered = false;

        if (xPos > m_scene.m_FullscreenButton.rect.left && xPos < m_scene.m_FullscreenButton.rect.right && yPos > m_scene.m_FullscreenButton.rect.top && yPos < m_scene.m_FullscreenButton.rect.bottom)
        {
            m_scene.m_FullscreenButton.IsHovered = true;
        }
        else m_scene.m_FullscreenButton.IsHovered = false;
    }
    return 0;


    case WM_LBUTTONDOWN:
    {
        if (m_scene.m_PlayButton.IsHovered)
        {
            m_scene.m_PlayButton.Down = true;
        }
        if (m_scene.m_ResetButton.IsHovered)
        {
            m_scene.m_ResetButton.Down = true;
        }
        if (m_scene.m_FullscreenButton.IsHovered)
        {
            m_scene.m_FullscreenButton.Down = true;
        }
    }
    return 0;

    case WM_LBUTTONUP:
    {
        if (m_scene.m_PlayButton.IsHovered && m_scene.m_PlayButton.Down)
        {
            m_scene.m_PlayButton.Up = true;
        }
        m_scene.m_PlayButton.Down = false;

        if (m_scene.m_ResetButton.IsHovered && m_scene.m_ResetButton.Down)
        {
            m_scene.m_ResetButton.Up = true;
        }
        m_scene.m_ResetButton.Down = false;

        if (m_scene.m_FullscreenButton.IsHovered && m_scene.m_FullscreenButton.Down)
        {
            m_scene.SwitchScreenMode();
        }
        m_scene.m_FullscreenButton.Down = false;
    }
    return 0;

    case WM_GETMINMAXINFO:
    {
        ((MINMAXINFO*)(lParam))->ptMinTrackSize.x = (long)m_scene.m_tm.width + 50;
        ((MINMAXINFO*)(lParam))->ptMinTrackSize.y = (long)m_scene.m_tm.height + 45 + 130;
    }
    return 0;
    
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    
}

BOOL MainWindow::InitializeTimer()
{
    m_hTimer = CreateWaitableTimer(NULL, FALSE, NULL);
    if (m_hTimer == NULL)
    {
        return FALSE;
    }

    LARGE_INTEGER li = { 0 };

    if (!SetWaitableTimer(m_hTimer, &li, (1000 / 60), NULL, NULL, FALSE))
    {
        CloseHandle(m_hTimer);
        m_hTimer = NULL;
        return FALSE;
    }

    return TRUE;
}

void MainWindow::WaitTimer()
{
    if (MsgWaitForMultipleObjects(1, &m_hTimer, FALSE, INFINITE, QS_ALLINPUT)
        == WAIT_OBJECT_0)
    {
        InvalidateRect(m_hwnd, NULL, FALSE);
    }
}
