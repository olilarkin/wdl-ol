#pragma once

#include "IPlugPlatform.h"

#include "cairo/cairo.h"
#ifdef OS_MAC
#include "cairo/cairo-quartz.h"
#else
#include "cairo/cairo-win32.h"
#endif

#ifdef IGRAPHICS_FREETYPE
#include "cairo/cairo-ft.h"
#endif

#include "IGraphicsPathBase.h"

class CairoBitmap : public APIBitmap
{
public:
  CairoBitmap(cairo_surface_t* s, int scale);
  virtual ~CairoBitmap();
};

/** IGraphics draw class using Cairo
*   @ingroup DrawClasses
*/
class IGraphicsCairo : public IGraphicsPathBase
{
public:
  const char* GetDrawingAPIStr() override { return "CAIRO"; }

  IGraphicsCairo(IDelegate& dlg, int w, int h, int fps);
  ~IGraphicsCairo();

  void DrawBitmap(IBitmap& bitmap, const IRECT& dest, int srcX, int srcY, const IBlend* pBlend) override;
  void DrawRotatedBitmap(IBitmap& bitmap, int destCtrX, int destCtrY, double angle, int yOffsetZeroDeg, const IBlend* pBlend) override;
  void DrawRotatedMask(IBitmap& base, IBitmap& mask, IBitmap& top, int x, int y, double angle, const IBlend* pBlend) override;
      
  void PathClear() override { cairo_new_path(mContext); }
  void PathStart() override { cairo_new_sub_path(mContext); }
  void PathClose() override { cairo_close_path(mContext); }

  void PathArc(float cx, float cy, float r, float aMin, float aMax) override { cairo_arc(mContext, cx, cy, r, DegToRad(aMin), DegToRad(aMax)); }
    
  void PathMoveTo(float x, float y) override { cairo_move_to(mContext, x, y); }
  void PathLineTo(float x, float y) override { cairo_line_to(mContext, x, y);}
  void PathCurveTo(float x1, float y1, float x2, float y2, float x3, float y3) override { cairo_curve_to(mContext, x1, y1, x2, y2, x3, y3); }

  void PathStroke(const IPattern& pattern, float thickness, const IStrokeOptions& options, const IBlend* pBlend) override;
  void PathFill(const IPattern& pattern, const IFillOptions& options, const IBlend* pBlend) override;
  
  void PathStateSave() override { cairo_save(mContext); }
  void PathStateRestore() override { cairo_restore(mContext); }
  
  void PathTransformTranslate(float x, float y) override { cairo_translate(mContext, x, y); }
  void PathTransformScale(float scale) override { cairo_scale(mContext, scale, scale); }
  void PathTransformRotate(float angle) override { cairo_rotate(mContext, DegToRad(angle)); }
    
  IColor GetPoint(int x, int y) override;
  void* GetData() override { return (void*) mContext; }

  bool DrawText(const IText& text, const char* str, IRECT& rect, bool measure) override;
  bool MeasureText(const IText& text, const char* str, IRECT& destRect) override;

  //IBitmap CropBitmap(const IBitmap& bitmap, const IRECT& rect, const char* name, int targetScale) override;

  void RenderDrawBitmap() override;

  void SetPlatformContext(void* pContext) override;

  inline void ClipRegion(const IRECT& r) override
  {
    PathClear();
    PathRect(r);
    cairo_clip(mContext);
  }

  inline void ResetClipRegion() override
  {
    cairo_reset_clip(mContext);
  }

protected:

  APIBitmap* LoadAPIBitmap(const WDL_String& resourcePath, int scale) override;
  APIBitmap* ScaleAPIBitmap(const APIBitmap* pBitmap, int scale) override;

  inline float CairoWeight(const IBlend* pBlend)
  {
    return (pBlend ? pBlend->mWeight : 1.0f);
  }

  inline cairo_operator_t CairoBlendMode(const IBlend* pBlend)
  {
    if (!pBlend)
    {
      return CAIRO_OPERATOR_OVER;
    }
    switch (pBlend->mMethod)
    {
      case kBlendClobber: return CAIRO_OPERATOR_OVER;
      case kBlendAdd: return CAIRO_OPERATOR_ADD;
      case kBlendColorDodge: return CAIRO_OPERATOR_COLOR_DODGE;
      case kBlendNone:
      default:
        return CAIRO_OPERATOR_OVER; // TODO: is this correct - same as clobber?
    }
  }
  
  void SetCairoSourcePattern(const IPattern& pattern, const IBlend* pBlend);
    
  void Stroke(const IPattern& pattern, const IBlend* pBlend)
  {
    SetCairoSourcePattern(pattern, pBlend);
    cairo_set_line_width(mContext, 1);
    cairo_stroke(mContext);
  }

  void CairoSetStrokeOptions(const IStrokeOptions& options = IStrokeOptions());
  
private:
  cairo_t* mContext;
  cairo_surface_t* mSurface;
};
