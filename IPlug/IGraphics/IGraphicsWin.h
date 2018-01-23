#pragma once

#include <windows.h>
#include <windowsx.h>
#include <winuser.h>

//TODO: would be nice not to put this here
#ifndef NO_IGRAPHICS
  #ifdef IGRAPHICS_AGG
    #define IGRAPHICS_DRAW_CLASS IGraphicsAGG
    #include "IGraphicsAGG.h"
  #elif defined IGRAPHICS_CAIRO
    #define IGRAPHICS_DRAW_CLASS IGraphicsCairo
    #include "IGraphicsCairo.h"
  #elif defined IGRAPHICS_NANOVG
    #define IGRAPHICS_DRAW_CLASS IGraphicsNanoVG
    #include "IGraphicsNanoVG.h"
  #else
    #define IGRAPHICS_DRAW_CLASS IGraphicsLice
    #include "IGraphicsLice.h"
  #endif
#endif

/** IGraphics platform class for Windows  
*   @ingroup PlatformClasses
*/
class IGraphicsWin : public IGRAPHICS_DRAW_CLASS
{
public:
  IGraphicsWin(IPlugBaseGraphics& plug, int w, int h, int fps);
  ~IGraphicsWin();

  void SetPlatformInstance(void* instance) override { mHInstance = (HINSTANCE) instance; }
  void* GetPlatformInstance() override { return mHInstance; }
  
  void ForceEndUserEdit() override;

  void Resize(int w, int h, float scale) override;

  void HideMouseCursor() override;
  void ShowMouseCursor() override;
  int ShowMessageBox(const char* str, const char* caption, int type) override;

  void* OpenWindow(void* pParentWnd) override;
  void CloseWindow() override;
  bool WindowIsOpen() override { return (mPlugWnd); }
  
  void UpdateTooltips() {}

  void HostPath(WDL_String& path) override;
  void PluginPath(WDL_String& path) override;
  void DesktopPath(WDL_String& path) override;
  void AppSupportPath(WDL_String& path, bool isSystem) override;
  void SandboxSafeAppSupportPath(WDL_String& path) override { AppSupportPath(path, false); }
  void VST3PresetsPath(WDL_String& path, bool isSystem) override;
  bool RevealPathInExplorerOrFinder(WDL_String& path, bool select) override;

  void PromptForFile(WDL_String& filename, WDL_String& path, EFileAction action, const char* ext) override;
  bool PromptForColor(IColor& color, const char* str) override;

  IPopupMenu* GetItemMenu(long idx, long& idxInMenu, long& offsetIdx, IPopupMenu& baseMenu);
  HMENU CreateMenu(IPopupMenu& menu, long* offsetIdx);

  IPopupMenu* CreateIPopupMenu(IPopupMenu& menu, IRECT& areaRect) override;
  void CreateTextEntry(IControl* pControl, const IText& text, const IRECT& textRect, const char* str, IParam* pParam) override;

  bool OpenURL(const char* url, const char* msgWindowTitle, const char* confirmMsg, const char* errMsgOnFailure);

  void* GetWindow() { return mPlugWnd; }
  HWND GetParentWindow() { return mParentWnd; }
  HWND GetMainWnd();
  void SetMainWndClassName(const char* name) { mMainWndClassName.Set(name); }
//  void GetMainWndClassName(char* name) { strcpy(name, mMainWndClassName.Get()); }
  IRECT GetWindowRECT();
  void SetWindowTitle(const char* str);

  const char* GetGUIAPI() override { return "win32"; };
  
  bool GetTextFromClipboard(WDL_String& str);
  
  bool OSFindResource(const char* name, const char* type, WDL_String& result) override;
protected:
  void SetTooltip(const char* tooltip);
  void ShowTooltip();
  void HideTooltip();

private:
  enum EParamEditMsg
  {
    kNone,
    kEditing,
    kUpdate,
    kCancel,
    kCommit
  };

  inline IMouseInfo IGraphicsWin::GetMouseInfo(LPARAM lParam, WPARAM wParam);
  inline IMouseInfo IGraphicsWin::GetMouseInfoDeltas(float&dX, float& dY, LPARAM lParam, WPARAM wParam);

  HINSTANCE mHInstance = nullptr;
  HWND mPlugWnd = nullptr;
  HWND mParamEditWnd = nullptr;
  HWND mTooltipWnd = nullptr;
  HWND mParentWnd = nullptr;
  HWND mMainWnd = nullptr;
  COLORREF* mCustomColorStorage = nullptr;
  WNDPROC mDefEditProc = nullptr;
  DWORD mPID = 0;

  IControl* mEdControl = nullptr;
  IParam* mEdParam = nullptr;
  EParamEditMsg mParamEditMsg = kNone;
  bool mShowingTooltip = false;
  int mTooltipIdx = -1;

  WDL_String mMainWndClassName;
  
  float mMouseX;
  float mMouseY;
  float mHiddenMousePointX = -1;
  float mHiddenMousePointY = -1;
    
public:
  static BOOL EnumResNameProc(HANDLE module, LPCTSTR type, LPTSTR name, LONG param);
  static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
  static LRESULT CALLBACK ParamEditProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
  static BOOL CALLBACK FindMainWindow(HWND hWnd, LPARAM lParam);
};
