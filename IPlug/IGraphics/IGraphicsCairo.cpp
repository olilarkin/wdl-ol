#include <cmath>

#include "png.h"

#include "CairoNanoSVG.h"

#include "IGraphicsCairo.h"
#include "IControl.h"
#include "Log.h"

#ifdef OS_OSX
cairo_surface_t* LoadPNGResource(void *hInst, const WDL_String &path)
{
  return cairo_image_surface_create_from_png(path.Get());
}
#else //OS_WIN
class PNGStreamReader
{
public:
  PNGStreamReader(HMODULE hInst, const WDL_String &path) : mData(nullptr), mSize(0), mCount(0)
  {
    HRSRC resInfo = FindResource(hInst, path.Get(), "PNG");
    if (resInfo)
    {
      HGLOBAL res = LoadResource(hInst, resInfo);
      if (res)
      {
      mData = (unsigned char *) LockResource(res);
      mSize = SizeofResource(hInst, resInfo);
      }
    }
  }

  cairo_status_t Read(unsigned char *data, unsigned int length)
  {
    mCount += length;
    if (mCount <= mSize)
    {
      memcpy(data, mData + mCount - length, length);
        return CAIRO_STATUS_SUCCESS;
    }

    return CAIRO_STATUS_READ_ERROR;
    }

    static cairo_status_t StaticRead(void *reader, unsigned char *data, unsigned int length) 
    { 
      return ((PNGStreamReader *)reader)->Read(data, length);
    }
  
private:
  const unsigned char *mData;
  size_t mCount;
  size_t mSize;
};

cairo_surface_t* LoadPNGResource(void *hInst, const WDL_String &path)
{
  PNGStreamReader reader((HMODULE)hInst, path);
  return cairo_image_surface_create_from_png_stream(&PNGStreamReader::StaticRead, &reader);
}
#endif //OS_WIN

struct CairoBitmap {
  cairo_surface_t* surface = nullptr;
  int width = 0;
  int height = 0;
  
  CairoBitmap(cairo_surface_t* s, double scale)
  {
    surface = s;
    cairo_surface_set_device_scale(s, scale, scale);
    width = cairo_image_surface_get_width(s);
    height = cairo_image_surface_get_height(s);
  }
  
  ~CairoBitmap()
  {
    cairo_surface_destroy(surface);
  }
};

static StaticStorage<CairoBitmap> s_bitmapCache;

#pragma mark -

IGraphicsCairo::IGraphicsCairo(IPlugBaseGraphics& plug, int w, int h, int fps)
: IGraphics(plug, w, h, fps)
, mSurface(nullptr)
, mContext(nullptr)
{
}

IGraphicsCairo::~IGraphicsCairo() 
{
  if (mContext)
    cairo_destroy(mContext);
  
  if (mSurface)
    cairo_surface_destroy(mSurface);
}

IBitmap IGraphicsCairo::LoadIBitmap(const char* name, int nStates, bool framesAreHoriztonal, double sourceScale)
{
  const double targetScale = GetDisplayScale(); // targetScale = what this screen is

  CairoBitmap* pCB = s_bitmapCache.Find(name, targetScale);

  if (!pCB) // if bitmap not in cache already at targetScale
  {
    WDL_String fullPath;
    OSFindResource(name, "png", fullPath);
    cairo_surface_t* pSurface = LoadPNGResource(GetPlatformInstance(), fullPath);
#ifndef NDEBUG
    bool imgResourceFound = cairo_surface_status(pSurface) == CAIRO_STATUS_SUCCESS;
#endif
    assert(imgResourceFound); // Protect against typos in resource.h and .rc files.
    
    pCB = new CairoBitmap(pSurface, sourceScale);

    const IBitmap bitmap(pCB->surface, pCB->width / sourceScale, pCB->height / sourceScale, nStates, framesAreHoriztonal, sourceScale, name);

    if (sourceScale != targetScale)
      return ScaleIBitmap(bitmap, name, targetScale); // will add to cache
    else
      s_bitmapCache.Add(pCB, name, sourceScale);
  }

  return IBitmap(pCB->surface, pCB->width / targetScale, pCB->height / targetScale, nStates, framesAreHoriztonal, sourceScale, name);
}

void IGraphicsCairo::ReleaseIBitmap(IBitmap& bitmap)
{
}

void IGraphicsCairo::RetainIBitmap(IBitmap& bitmap, const char * cacheName)
{
}

IBitmap IGraphicsCairo::ScaleIBitmap(const IBitmap& inBitmap, const char* name, double targetScale)
{
  int newW = (int)(inBitmap.W * targetScale);
  int newH = (int)(inBitmap.H * targetScale);
  
  // Create resources to redraw
    
  cairo_surface_t* pOutSurface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, newW, newH);
  cairo_t* pOutContext = cairo_create(pOutSurface);

  // Scale and paint (destroying the context / the surface is retained)
    
  cairo_scale(pOutContext, targetScale, targetScale);
  cairo_set_source_surface(pOutContext, (cairo_surface_t*)inBitmap.mData, 0, 0);
  cairo_paint(pOutContext);
  cairo_destroy(pOutContext);

 // Cache and retun as an IBitmap
    
  CairoBitmap* pCB = new CairoBitmap(pOutSurface, targetScale);
  s_bitmapCache.Add(pCB, name, targetScale);

  return IBitmap(pCB->surface, inBitmap.W, inBitmap.H, inBitmap.N, inBitmap.mFramesAreHorizontal, inBitmap.mSourceScale, name);
}

IBitmap IGraphicsCairo::CropIBitmap(const IBitmap& inBitmap, const IRECT& rect, const char* name, double targetScale)
{
  int newW = (int)(inBitmap.W * targetScale);
  int newH = (int)(inBitmap.H * targetScale);
    
  unsigned char* pOutBuffer = new unsigned char[newW * newH * 4];
  
  // Convert output to cairo
  cairo_surface_t* pOutSurface = cairo_image_surface_create_for_data(pOutBuffer, CAIRO_FORMAT_ARGB32, newW, newH, 0);
  cairo_t* pOutContext = cairo_create(pOutSurface);
  
  // Paint from one surface to another
  cairo_set_source_surface(pOutContext, (cairo_surface_t*) inBitmap.mData, 0, 0);
  cairo_rectangle(pOutContext, rect.L, rect.T, rect.W(), rect.H());
  cairo_scale(pOutContext, targetScale, targetScale);
  cairo_paint(pOutContext);
  cairo_surface_set_device_scale(pOutSurface, targetScale, targetScale);
    
  // Destroy cairo stuff
  cairo_destroy(pOutContext);
  
  return IBitmap(pOutSurface, newW, newH); //TODO: surface will not be destroyed, unless this is retained
}

void IGraphicsCairo::PrepDraw()
{
// not sure if needed yet may change api
}

void IGraphicsCairo::ReScale()
{
  IGraphics::ReScale(); // will cause all the controls to update their bitmaps
}

void IGraphicsCairo::DrawSVG(ISVG& svg, const IRECT& dest, const IBlend* pBlend)
{
  cairo_save(mContext);
  cairo_translate(mContext, dest.L, dest.T);
  cairo_rectangle(mContext, 0, 0, dest.W(), dest.H());
  cairo_clip(mContext);

  double xScale = dest.W() / svg.W();
  double yScale = dest.H() / svg.H();
  double scale = xScale < yScale ? xScale : yScale;

  cairo_scale(mContext, scale, scale);

  CairoNanoSVGRender::RenderNanoSVG(mContext, svg.mImage);

  cairo_restore(mContext);
}

void IGraphicsCairo::DrawBitmap(IBitmap& bitmap, const IRECT& dest, int srcX, int srcY, const IBlend* pBlend)
{
  cairo_save(mContext);
  ClipRegion(dest);
  cairo_surface_t* surface = (cairo_surface_t*) bitmap.mData;
  cairo_set_source_surface(mContext, surface, dest.L - srcX, dest.T - srcY);
  cairo_set_operator(mContext, CairoBlendMode(pBlend));
  cairo_paint_with_alpha(mContext, CairoWeight(pBlend));
  cairo_restore(mContext);
}

void IGraphicsCairo::DrawRotatedBitmap(IBitmap& bitmap, int destCtrX, int destCtrY, double angle, int yOffsetZeroDeg, const IBlend* pBlend)
{
  //TODO:

}

void IGraphicsCairo::DrawRotatedMask(IBitmap& base, IBitmap& mask, IBitmap& top, int x, int y, double angle, const IBlend* pBlend)
{
  //TODO:

}

void IGraphicsCairo::DrawPoint(const IColor& color, float x, float y, const IBlend* pBlend, bool aa)
{
  SetCairoSourceRGBA(color, pBlend);
  
  if (!aa)
  {
    x = std::floor(x);
    y = std::floor(y);
  }
  
  cairo_move_to(mContext, x + 0.5, y + 0.5);
  cairo_line_to(mContext, x + 1.5, y + 1.5);
  cairo_set_line_width(mContext, 1);
  cairo_stroke(mContext);
}

void IGraphicsCairo::ForcePixel(const IColor& color, int x, int y)
{
  SetCairoSourceRGBA(color);
  
  cairo_move_to(mContext, x + 0.5, y + 0.5);
  cairo_line_to(mContext, x + 1.5, y + 1.5);
  cairo_set_line_width(mContext, 1);
  cairo_stroke(mContext);
}

void IGraphicsCairo::DrawLine(const IColor& color, float x1, float y1, float x2, float y2, const IBlend* pBlend, bool aa)
{
  SetCairoSourceRGBA(color, pBlend);
  cairo_set_line_width(mContext, 1);
  cairo_move_to(mContext, x1, y1);
  cairo_line_to(mContext, x2, y2);
  cairo_stroke(mContext);
}

void IGraphicsCairo::DrawRect(const IColor& color, const IRECT& rect, const IBlend* pBlend)
{
  cairo_set_line_width(mContext, 1);
  SetCairoSourceRGBA(color, pBlend);
  CairoDrawRect(rect);
  cairo_stroke(mContext);
}

void IGraphicsCairo::DrawTriangle(const IColor& color, int x1, int y1, int x2, int y2, int x3, int y3, const IBlend* pBlend)
{
  SetCairoSourceRGBA(color, pBlend);
  cairo_set_line_width(mContext, 1);
  cairo_move_to(mContext, x1, y1);
  cairo_line_to(mContext, x2, y2);
  cairo_line_to(mContext, x3, y3);
  cairo_close_path(mContext);
  cairo_stroke(mContext);
}

void IGraphicsCairo::DrawArc(const IColor& color, float cx, float cy, float r, float minAngle, float maxAngle, const IBlend* pBlend, bool aa)
{
  SetCairoSourceRGBA(color, pBlend);
  cairo_set_line_width(mContext, 1);
  cairo_arc(mContext, cx, cy, r, minAngle, maxAngle);
  cairo_stroke(mContext);
}

void IGraphicsCairo::DrawCircle(const IColor& color, float cx, float cy, float r, const IBlend* pBlend, bool aa)
{
  SetCairoSourceRGBA(color, pBlend);
  cairo_set_line_width(mContext, 1);
  cairo_arc(mContext, cx, cy, r, 0, PI * 2.);
  cairo_stroke(mContext);
}

void IGraphicsCairo::DrawRoundRect(const IColor& color, const IRECT& rect, const IBlend* pBlend, int corner, bool aa)
{
  const double y = rect.B - rect.H();
  SetCairoSourceRGBA(color, pBlend);
  cairo_new_sub_path(mContext);
  cairo_arc(mContext, rect.L + rect.W() - corner, y + corner, corner, PI * -0.5, 0);
  cairo_arc(mContext, rect.L + rect.W() - corner, y + rect.H() - corner, corner, 0, PI * 0.5);
  cairo_arc(mContext, rect.L + corner, y + rect.H() - corner, corner, PI * 0.5, PI);
  cairo_arc(mContext, rect.L + corner, y + corner, corner, PI, PI * 1.25);
  cairo_stroke(mContext);
}

void IGraphicsCairo::DrawDottedRect(const IColor& color, const IRECT& rect, const IBlend* pBlend)
{
  //TODO: stipple?
  DrawRect(color, rect, pBlend);
}

void IGraphicsCairo::FillRoundRect(const IColor& color, const IRECT& rect, const IBlend* pBlend, int corner, bool aa)
{
  const double y = rect.B - rect.H();
  SetCairoSourceRGBA(color, pBlend);
  cairo_new_sub_path(mContext);
  cairo_arc(mContext, rect.L + rect.W() - corner, y + corner, corner, PI * -0.5, 0);
  cairo_arc(mContext, rect.L + rect.W() - corner, y + rect.H() - corner, corner, 0, PI * 0.5);
  cairo_arc(mContext, rect.L + corner, y + rect.H() - corner, corner, PI * 0.5, PI);
  cairo_arc(mContext, rect.L + corner, y + corner, corner, PI, PI * 1.25);
  cairo_fill(mContext);
}

void IGraphicsCairo::FillIRect(const IColor& color, const IRECT& rect, const IBlend* pBlend)
{
  SetCairoSourceRGBA(color, pBlend);
  CairoDrawRect(rect);
  cairo_fill(mContext);
}

void IGraphicsCairo::FillCircle(const IColor& color, int cx, int cy, float r, const IBlend* pBlend, bool aa)
{
  SetCairoSourceRGBA(color, pBlend);
  cairo_arc(mContext, cx, cy, r, 0, 2 * PI);
  cairo_fill(mContext);
}

void IGraphicsCairo::FillTriangle(const IColor& color, int x1, int y1, int x2, int y2, int x3, int y3, const IBlend* pBlend)
{
  SetCairoSourceRGBA(color, pBlend);
  cairo_move_to(mContext, x1, y1);
  cairo_line_to(mContext, x2, y2);
  cairo_line_to(mContext, x3, y3);
  cairo_close_path(mContext);
  cairo_fill(mContext);
}

void IGraphicsCairo::FillIConvexPolygon(const IColor& color, int* x, int* y, int npoints, const IBlend* pBlend)
{
  SetCairoSourceRGBA(color, pBlend);

  cairo_move_to(mContext, x[0], y[0]);
  
  for(int i = 1; i < npoints; i++)
    cairo_line_to(mContext, x[i], y[i]);
  
  cairo_fill(mContext);
}

IColor IGraphicsCairo::GetPoint(int x, int y)
{
  // Convert suface to cairo image surface
  cairo_surface_t* pOutSurface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, Width(), Height());
  cairo_t* pOutContext = cairo_create(pOutSurface);
  cairo_set_source_surface(pOutContext, mSurface, 0, 0);
  cairo_paint(pOutContext);
  
  unsigned char* pData = cairo_image_surface_get_data(pOutSurface);
  int stride = cairo_image_surface_get_stride(pOutSurface);
  
  unsigned int* pPixel = (unsigned int*)(pData + y * stride);
  pPixel += x;
  
  int A = ((*pPixel) >> 0) & 0xff;
  int R = ((*pPixel) >> 8) & 0xff;
  int G = ((*pPixel) >> 16) & 0xff;
  int B = ((*pPixel) >> 24) & 0xff;
  
  cairo_surface_destroy(pOutSurface);
  cairo_destroy(pOutContext);
    
  return IColor(A, R, G, B);
}

bool IGraphicsCairo::DrawIText(const IText& text, const char* str, IRECT& rect, bool measure)
{
	// TODO: Add way to handle the fonts.
	icairo_create_font_from_path("C:/Windows/Fonts/Verdana.ttf");

	icairo_initialize_font_face(mContext);
	icairo_set_text(mContext, str);

	// TODO: Add vertical alignment
	icairo_text_w_aligement w_aligement;

	if (text.mAlign == IText::EAlign::kAlignNear) w_aligement = icairo_text_w_aligement::W_ALIGN_LEFT;
	if (text.mAlign == IText::EAlign::kAlignFar) w_aligement = icairo_text_w_aligement::W_ALIGN_RIGHT;
	if (text.mAlign == IText::EAlign::kAlignCenter) w_aligement = icairo_text_w_aligement::W_ALIGN_CENTER;

	icairo_set_text_position(mContext, rect, w_aligement);

	cairo_set_source_rgba(mContext, text.mColor.R / 255.0, text.mColor.G / 255.0, text.mColor.B / 255.0, text.mColor.A / 255.0);
	icairo_show_text(mContext);

	icairo_destroy_font_face();

	return true;
}

bool IGraphicsCairo::MeasureIText(const IText& text, const char* str, IRECT& destRect)
{
	// TODO: Add way to handle the fonts.
	icairo_create_font_from_path("C:/Windows/Fonts/Verdana.ttf");

	icairo_initialize_font_face(mContext);
	icairo_set_text(mContext, str);

	icairo_calculate_extents(mContext);

	cairo_text_extents_t* text_extents = icairo_get_text_extents(mContext);
	cairo_font_extents_t* font_extents = icairo_get_font_extents(mContext);

	destRect = IRECT(0, 0, text_extents->width, font_extents->height);

	icairo_destroy_font_face();

  return true;
}

void IGraphicsCairo::SetPlatformContext(void* pContext)
{
  if (!pContext)
  {
    if (mContext)
      cairo_destroy(mContext);
    if (mSurface)
      cairo_surface_destroy(mSurface);
      
    mContext = nullptr;
    mSurface = nullptr;
  }
  else if(!mSurface)
  {
#ifdef OS_OSX
    mSurface = cairo_quartz_surface_create_for_cg_context(CGContextRef(pContext), Width(), Height());
    mContext = cairo_create(mSurface);
    cairo_surface_set_device_scale(mSurface, 1, -1);
    cairo_surface_set_device_offset(mSurface, 0, Height());
#else
    HDC dc = (HDC) pContext;
    mSurface = cairo_win32_surface_create(HDC(pContext));
    mContext = cairo_create(mSurface);
    cairo_surface_set_device_scale(mSurface, Scale(), Scale());
#endif
  }
  
  IGraphics::SetPlatformContext(pContext);
}

