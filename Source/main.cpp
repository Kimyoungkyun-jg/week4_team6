#include "Editor/Application/FEditorApplication.h"
#include "Runtime/Core/Log.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/Engine/FRenderView.h"
#include "Runtime/Engine/FTimeManager.h"
#include "Runtime/Engine/UScene.h"
#include "Runtime/Engine/USceneManager.h"
#include "Runtime/Input/FInputManager.h"
#include "Runtime/Math/FVector2.h"
#include "Runtime/Rendering/FRenderResourceLibrary.h"
#include "Runtime/Rendering/FRenderer.h"
#include "ThirdParty/Imgui/imgui.h"
#include "ThirdParty/Imgui/imgui_internal.h"
#include <Windows.h>
#include <windowsx.h>

#include "ThirdParty/stb/stb_image.h"
#include "Runtime/Rendering/FObjDecoder.h"


extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg,
                                              WPARAM wParam, LPARAM lParam);

static bool bRequestNewScene = false;
static bool bRequestSaveScene = false;
static bool bRequestLoadScene = false;
static bool bRequestResize = false;
static UINT ResizeWidth = 0u;
static UINT ResizeHeight = 0u;

namespace {
constexpr LPCWSTR WindowName = L"My Engine";

HWND CreateWindowHandle(HINSTANCE Instance, HWND& OutSplashWnd);
bool ProcessWindowMessage();

LRESULT CALLBACK WindowCallback(HWND Window, UINT Message, WPARAM WParam,
                                LPARAM LParam);
} // namespace

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,                    _In_ LPWSTR lpCmdLine, _In_ int nShowCmd) {
  HWND SplashWindow = nullptr;
  HWND Window = CreateWindowHandle(hInstance, SplashWindow);
  if (!Window) {
    return -1;
  }

  

  ShowWindow(Window, nShowCmd);

  FInputManager::Get();

  FRenderer Renderer;
  if (!Renderer.Initialize(Window)) {
    return -1;
  }
  FRenderView RenderView(Renderer);

  FRenderResourceLibrary &RenderResources = FRenderResourceLibrary::Get();
  if (!RenderResources.Initialize(Renderer)) {
    return -1;
  }

  // RTTI를 위한 UClass 초기화
  UClass::ResolveTypeBitsets();
  // 새씬 생성
  USceneManager SceneManager;


  SceneManager.SetScene(NewObject<UScene>());

  FEditorApplication &EditorApp = FEditorApplication::Get();
  {
    ID3D11Device *Device = nullptr;
    ID3D11DeviceContext *Context = nullptr;
    Renderer.GetDeviceAndContext_ImplDX11(Device, Context);
    EditorApp.Initialize_ImguiWin32DX11(Window, Device, Context);
  }
  EditorApp.Initialize_Runtime(&SceneManager, &RenderView);

  // 초기화가 끝났으니 로딩 화면을 닫고 메인 창을 띄운다
  if (SplashWindow) {
      DestroyWindow(SplashWindow);
      SplashWindow = nullptr;
  }
  ShowWindow(Window, nShowCmd);
  SetForegroundWindow(Window);


  bool bQuit = false;
  while (!bQuit) {
    FTimeManager::Get().Update();
    FTimeManager::Get().Resume();

    if (!ProcessWindowMessage()) {
      bQuit = true;
      break;
    }

    if (bRequestResize) {
      Renderer.OnWindowSize(ResizeWidth, ResizeHeight);
      EditorApp.OnWindowSize(ResizeWidth, ResizeHeight);
      bRequestResize = false;
    }

    FInputManager::Get().BeginFrame();
    EditorApp.Update(FTimeManager::Get().GetDeltaTime());

    Renderer.BeginFrame();
    EditorApp.Render();
    Renderer.SwapBuffer();

    // EditorApp.CollectGarbage();
  }

  EditorApp.Shutdown();
  SceneManager.Release();
  // EditorApp.CollectGarbage();
  Renderer.Shutdown();

  return 0;
}

namespace {


HWND ShowLoadingWindow(HINSTANCE& hInstance)
{
    int ImageW = 0, ImageH = 0, Channels = 0;
    stbi_uc* Pixels = stbi_load("./Resources/Textures/LoadingImage.png",
        &ImageW, &ImageH, &Channels, 4);
    if (!Pixels)
    {
        return nullptr;
    }

    // stb 는 RGBA 순서, Windows DIB 는 BGRA 순서라 R/B 를 맞바꾼다.
    // 알파가 있는 PNG 는 흰 배경 위에 미리 합성해 둔다.
    for (int i = 0; i < ImageW * ImageH; ++i)
    {
        stbi_uc* P = Pixels + i * 4;
        const int A = P[3];
        const stbi_uc R = static_cast<stbi_uc>((P[0] * A + 255 * (255 - A)) / 255);
        const stbi_uc G = static_cast<stbi_uc>((P[1] * A + 255 * (255 - A)) / 255);
        const stbi_uc B = static_cast<stbi_uc>((P[2] * A + 255 * (255 - A)) / 255);
        P[0] = B; P[1] = G; P[2] = R; P[3] = 255;
    }

    const int w = GetSystemMetrics(SM_CXSCREEN);   // 화면 전체
    const int h = GetSystemMetrics(SM_CYSCREEN);

    WNDCLASSW splashClass = { 0, DefWindowProcW, 0, 0, 0, 0, 0, 0, 0, L"JungleSplash" };
    RegisterClassW(&splashClass);
    HWND splashWnd = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW, L"JungleSplash", L"",
        WS_POPUP | WS_VISIBLE, 0, 0, w, h, nullptr, nullptr, hInstance, nullptr);

    HDC dc = GetDC(splashWnd);

    // 1) 흰 배경
    RECT full = { 0, 0, w, h };
    FillRect(dc, &full, (HBRUSH)GetStockObject(WHITE_BRUSH));

    // 2) 로딩이미지를 비율 유지해서 가운데. 화면 높이의 60% 로 맞춘다
    const int drawH = static_cast<int>(h * 0.6f);
    const int drawW = drawH * ImageW / ImageH;
    const int drawX = (w - drawW) / 2;
    const int drawY = (h - drawH) / 2;

    BITMAPINFO Info{};
    Info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    Info.bmiHeader.biWidth = ImageW;
    Info.bmiHeader.biHeight = -ImageH;   // 음수 = 위에서 아래로 저장된 이미지
    Info.bmiHeader.biPlanes = 1;
    Info.bmiHeader.biBitCount = 32;
    Info.bmiHeader.biCompression = BI_RGB;

    SetStretchBltMode(dc, HALFTONE);
    SetBrushOrgEx(dc, 0, 0, nullptr);
    StretchDIBits(dc,
        drawX, drawY, drawW, drawH,
        0, 0, ImageW, ImageH,
        Pixels, &Info, DIB_RGB_COLORS, SRCCOPY);

    ReleaseDC(splashWnd, dc);
    stbi_image_free(Pixels);

    return splashWnd;
}

// TODO: Resizing 처리
HWND CreateWindowHandle(HINSTANCE Instance, HWND& OutSplashWnd) {
  WNDCLASS WindowClass{};
  WindowClass.lpfnWndProc = WindowCallback;

  WindowClass.hInstance = Instance;
  WindowClass.lpszClassName = L"MyEngine";

  if (!RegisterClass(&WindowClass) &&
      GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
    return nullptr;
  }

  OutSplashWnd = ShowLoadingWindow(Instance);
  HWND Window = CreateWindowExW(0, WindowClass.lpszClassName, WindowName,
                                WS_POPUP | WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
                                CW_USEDEFAULT, 1200, 800, nullptr, nullptr,
                                Instance, nullptr);

  return Window;
}

// 닫아야되면 false 반환
bool ProcessWindowMessage() {
  MSG Message;
  while (PeekMessageW(&Message, nullptr, 0u, 0u, PM_REMOVE)) {
    if (Message.message == WM_QUIT) {
      return false;
    }

    TranslateMessage(&Message);
    DispatchMessageW(&Message);
  }

  return true;
}

LRESULT CALLBACK WindowCallback(HWND Window, UINT Message, WPARAM WParam,
                                LPARAM LParam) {
  if (LRESULT ImGuiResult = ImGui_ImplWin32_WndProcHandler(
          Window, Message, WParam, LParam)) // imgui의 프레임 스냅샷 상태를 갱신
    return ImGuiResult; // ImGuiResult != 0인 경우: 상태가 DefWindowProcW() 함수
                        // 동작을 오버라이드해야 하는 경우

  const FVector2 MousePos{static_cast<float>(GET_X_LPARAM(LParam)),
                          static_cast<float>(GET_Y_LPARAM(LParam))};

  switch (Message) {
  case WM_DESTROY:
    PostQuitMessage(0);
    break;

  case WM_SIZE: {
    if (WParam != SIZE_MINIMIZED) {
      bRequestResize = true;
      ResizeWidth = LOWORD(LParam);
      ResizeHeight = HIWORD(LParam);
    }

    break;
  }

  case WM_LBUTTONDOWN:
    FInputManager::Get().OnMouseButtonDown(EMouseButton::Left, MousePos);
    SetCapture(Window);
    break;

  case WM_RBUTTONDOWN:
    FInputManager::Get().OnMouseButtonDown(EMouseButton::Right, MousePos);
    SetCapture(Window);
    break;

  case WM_MBUTTONDOWN:
    FInputManager::Get().OnMouseButtonDown(EMouseButton::Middle, MousePos);
    SetCapture(Window);
    break;

  case WM_LBUTTONUP:
    FInputManager::Get().OnMouseButtonUp(EMouseButton::Left, MousePos);
    ReleaseCapture();
    break;

  case WM_RBUTTONUP:
    FInputManager::Get().OnMouseButtonUp(EMouseButton::Right, MousePos);
    ReleaseCapture();
    break;

  case WM_MBUTTONUP:
    FInputManager::Get().OnMouseButtonUp(EMouseButton::Middle, MousePos);
    ReleaseCapture();
    break;

  case WM_MOUSEMOVE:
    FInputManager::Get().OnMouseMove(MousePos);
    break;

  // case WM_CAPTURECHANGED:
  case WM_CANCELMODE:
  case WM_KILLFOCUS: {
    const FVector2 Last = FInputManager::Get().GetMousePosition();
    FInputManager::Get().OnMouseButtonUp(EMouseButton::Left, Last);
    FInputManager::Get().OnMouseButtonUp(EMouseButton::Right, Last);
    FInputManager::Get().OnMouseButtonUp(EMouseButton::Middle, Last);
    break;
  }
  // WM_CA
  break;
  case WM_KEYDOWN:
    switch (WParam) {
    case VK_F5:
      bRequestSaveScene = true;
      break;
    case VK_F6:
      bRequestLoadScene = true;
      break;
    case VK_F7:
      bRequestNewScene = true;
      break;
    }
    break;
  default:
    return DefWindowProc(Window, Message, WParam, LParam);
  }

  return 0;
}
} // namespace
