#include <cmath>

#include "IGraphicsNanoVG.h"
#ifdef OS_WIN
#define NANOVG_GL3_IMPLEMENTATION
#include <glad/glad.h>
#include "nanovg_gl.h"
#endif

#pragma mark -

inline int GetBitmapIdx(APIBitmap* pBitmap) { return (int) ((long long) pBitmap->GetBitmap()); }

NanoVGBitmap::NanoVGBitmap(NVGcontext* pContext, const char* path, double sourceScale)
{
  mVG = pContext;
  int w = 0, h = 0;
  long long idx = nvgCreateImage(mVG, path, 0);
  nvgImageSize(mVG, idx, &w, &h);
      
  SetBitmap((void*) idx, w, h, sourceScale);
}

NanoVGBitmap::~NanoVGBitmap()
{
  int idx = GetBitmapIdx(this);
  nvgDeleteImage(mVG, idx);
}

#pragma mark -

// Utility conversions

inline NVGcolor NanoVGColor(const IColor& color, const IBlend* pBlend = 0)
{
  NVGcolor c;
  c.r = (float) color.R / 255.0f;
  c.g = (float) color.G / 255.0f;
  c.b = (float) color.B / 255.0f;
  c.a = (BlendWeight(pBlend) * color.A) / 255.0f;
  return c;
}

inline NVGcompositeOperation NanoVGBlendMode(const IBlend* pBlend)
{
  if (!pBlend)
  {
    return NVG_COPY;
  }
  
  switch (pBlend->mMethod)
  {
    case kBlendClobber:
    {
      return NVG_SOURCE_OVER;
    }
    case kBlendAdd:
    case kBlendColorDodge:
    case kBlendNone:
    default:
    {
      return NVG_COPY;
    }
  }
}

NVGpaint NanoVGPaint(NVGcontext* context, const IPattern& pattern, const IBlend* pBlend)
{
  NVGcolor icol = NanoVGColor(pattern.GetStop(0).mColor, pBlend);
  NVGcolor ocol = NanoVGColor(pattern.GetStop(pattern.NStops() - 1).mColor, pBlend);
  
  // Invert transform
  
  float inverse[6];
  nvgTransformInverse(inverse, pattern.mTransform);
  float s[2];
  
  nvgTransformPoint(&s[0], &s[1], inverse, 0, 0);
  
  if (pattern.mType == kRadialPattern)
  {
    return nvgRadialGradient(context, s[0], s[1], 0.0, inverse[0], icol, ocol);
  }
  else
  {
    float e[2];
    nvgTransformPoint(&e[0], &e[1], inverse, 1, 0);
    
    return nvgLinearGradient(context, s[0], s[1], e[0], e[1], icol, ocol);
  }
}

#pragma mark -

IGraphicsNanoVG::IGraphicsNanoVG(IDelegate& dlg, int w, int h, int fps)
: IGraphicsPathBase(dlg, w, h, fps)
{
}

IGraphicsNanoVG::~IGraphicsNanoVG() 
{
  mBitmaps.Empty(true);
  
#ifdef OS_MAC
  if(mVG)
    nvgDeleteMTL(mVG);
#endif
#ifdef OS_WIN
  if (mVG)
    nvgDeleteGL3(mVG);
#endif
}

IBitmap IGraphicsNanoVG::LoadBitmap(const char* name, int nStates, bool framesAreHorizontal)
{
  WDL_String fullPath;
  const int targetScale = round(GetDisplayScale());
  int sourceScale = 0;
  bool resourceFound = SearchImageResource(name, "png", fullPath, targetScale, sourceScale);
  assert(resourceFound);
    
  NanoVGBitmap* bitmap = (NanoVGBitmap*) LoadAPIBitmap(fullPath, sourceScale);
  assert(bitmap);
  mBitmaps.Add(bitmap);
  
  return IBitmap(bitmap, nStates, framesAreHorizontal, name);
}

APIBitmap* IGraphicsNanoVG::LoadAPIBitmap(const WDL_String& resourcePath, int scale)
{
  return new NanoVGBitmap(mVG, resourcePath.Get(), scale);
}

APIBitmap* IGraphicsNanoVG::ScaleAPIBitmap(const APIBitmap* pBitmap, int scale)
{
  return nullptr;
}

void IGraphicsNanoVG::RetainBitmap(const IBitmap& bitmap, const char* cacheName)
{
}

void IGraphicsNanoVG::SetPlatformContext(void* pContext) {
  mPlatformContext = pContext;
#ifdef OS_WIN
  PIXELFORMATDESCRIPTOR pfd =
  {
    sizeof(PIXELFORMATDESCRIPTOR),
    1,
    PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,    //Flags
    PFD_TYPE_RGBA,        // The kind of framebuffer. RGBA or palette.
    32,                   // Colordepth of the framebuffer.
    0, 0, 0, 0, 0, 0,
    0,
    0,
    0,
    0, 0, 0, 0,
    24,                   // Number of bits for the depthbuffer
    8,                    // Number of bits for the stencilbuffer
    0,                    // Number of Aux buffers in the framebuffer.
    PFD_MAIN_PLANE,
    0,
    0, 0, 0
  };

  HDC dc = (HDC)pContext;

  int fmt = ChoosePixelFormat(dc, &pfd);
  SetPixelFormat(dc, fmt, &pfd);

  HGLRC hglrc = wglCreateContext(dc);
  wglMakeCurrent(dc, hglrc);
  if (!gladLoadGL())
    throw std::runtime_error{"Error initializing glad"};
  glGetError();
#endif

  mVG = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
}

IBitmap IGraphicsNanoVG::ScaleBitmap(const IBitmap& bitmap, const char* name, int targetScale)
{
  return bitmap;
}

void IGraphicsNanoVG::ViewInitialized(void* layer)
{
#ifdef OS_MAC
  mVG = nvgCreateMTL(layer, NVG_ANTIALIAS | NVG_STENCIL_STROKES);
#endif
}

void IGraphicsNanoVG::BeginFrame()
{
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  glViewport(0, 0, Width()*GetDisplayScale(), Height()*GetDisplayScale());
  nvgBeginFrame(mVG, Width(), Height(), GetDisplayScale());
}

void IGraphicsNanoVG::EndFrame()
{
  nvgEndFrame(mVG);
}

void IGraphicsNanoVG::DrawBitmap(IBitmap& bitmap, const IRECT& dest, int srcX, int srcY, const IBlend* pBlend)
{
  int idx = GetBitmapIdx(bitmap.GetAPIBitmap());
  NVGpaint imgPaint = nvgImagePattern(mVG, std::round(dest.L) - srcX, std::round(dest.T) - srcY, bitmap.W(), bitmap.H(), 0.f, idx, BlendWeight(pBlend));
  PathClear();
  nvgRect(mVG, dest.L, dest.T, dest.W(), dest.H());
  nvgFillPaint(mVG, imgPaint);
  nvgFill(mVG);
  PathClear();
}

IColor IGraphicsNanoVG::GetPoint(int x, int y)
{
  return COLOR_BLACK; //TODO:
}

bool IGraphicsNanoVG::DrawText(const IText& text, const char* str, IRECT& bounds, bool measure)
{
  return true;
}

bool IGraphicsNanoVG::MeasureText(const IText& text, const char* str, IRECT& bounds)
{
  return DrawText(text, str, bounds, true);
}

void IGraphicsNanoVG::PathStroke(const IPattern& pattern, float thickness, const IStrokeOptions& options, const IBlend* pBlend)
{
  // First set options
  
  switch (options.mCapOption)
  {
    case kCapButt:   nvgLineCap(mVG, NSVG_CAP_BUTT);     break;
    case kCapRound:  nvgLineCap(mVG, NSVG_CAP_ROUND);    break;
    case kCapSquare: nvgLineCap(mVG, NSVG_CAP_SQUARE);   break;
  }
  
  switch (options.mJoinOption)
  {
    case kJoinMiter:   nvgLineJoin(mVG, NVG_MITER);   break;
    case kJoinRound:   nvgLineJoin(mVG, NVG_ROUND);   break;
    case kJoinBevel:   nvgLineJoin(mVG, NVG_BEVEL);   break;
  }
  
  nvgMiterLimit(mVG, options.mMiterLimit);
  nvgStrokeWidth(mVG, thickness);
 
  // TODO Dash

  if (pattern.mType == kSolidPattern)
    nvgStrokeColor(mVG, NanoVGColor(pattern.GetStop(0).mColor, pBlend));
  else
    nvgStrokePaint(mVG, NanoVGPaint(mVG, pattern, pBlend));
  
  nvgPathWinding(mVG, NVG_CCW);
  nvgStroke(mVG);
  
  if (!options.mPreserve)
    PathClear();
}

void IGraphicsNanoVG::PathFill(const IPattern& pattern, const IFillOptions& options, const IBlend* pBlend)
{
  nvgPathWinding(mVG, options.mFillRule == kFillWinding ? NVG_CCW : NVG_CW);
  
  if (pattern.mType == kSolidPattern)
    nvgFillColor(mVG, NanoVGColor(pattern.GetStop(0).mColor, pBlend));
  else
    nvgFillPaint(mVG, NanoVGPaint(mVG, pattern, pBlend));
  
  nvgFill(mVG);
  
  if (!options.mPreserve)
    PathClear();
}

