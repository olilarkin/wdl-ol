
/* Cockos SWELL (Simple/Small Win32 Emulation Layer for Linux)
   Copyright (C) 2006-2007, Cockos, Inc.

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
       claim that you wrote the original software. If you use this software
       in a product, an acknowledgment in the product documentation would be
       appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be
       misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
  

    This file provides basic win32 GDI-->lice? translation. 

*/

#ifndef SWELL_LICE_GDI
#ifndef SWELL_PROVIDED_BY_APP

#include "swell.h"
#include "swell-internal.h"

#include "../lice/lice.h"
#include "../mutex.h"
#include "../ptrlist.h"

#include "swell-gdi-internalpool.h"


HDC SWELL_CreateGfxContext(void *c)
{
  HDC__ *ctx=SWELL_GDP_CTX_NEW();

  
  return ctx;
}

HDC SWELL_CreateMemContext(HDC hdc, int w, int h)
{
  // we could use CGLayer here, but it's 10.4+ and seems to be slower than this
//  if (w&1) w++;
  void *buf=calloc(w*4,h);
  if (!buf) return 0;

  HDC__ *ctx=SWELL_GDP_CTX_NEW();
  ctx->ownedData=buf;
  
  SetTextColor(ctx,0);
  return ctx;
}


void SWELL_DeleteGfxContext(HDC ctx)
{
  HDC__ *ct=(HDC__ *)ctx;
  if (HDC_VALID(ct))
  {   
    if (ct->ownedData)
    {
      free(ct->ownedData);
    }
    SWELL_GDP_CTX_DELETE(ct);
  }
}
HPEN CreatePen(int attr, int wid, int col)
{
  return CreatePenAlpha(attr,wid,col,1.0f);
}

HBRUSH CreateSolidBrush(int col)
{
  return CreateSolidBrushAlpha(col,1.0f);
}

HPEN CreatePenAlpha(int attr, int wid, int col, float alpha)
{
  HGDIOBJ__ *pen=GDP_OBJECT_NEW();
  pen->type=TYPE_PEN;
  pen->wid=wid<0?0:wid;
//  pen->color=CreateColor(col,alpha);
  return pen;
}
HBRUSH  CreateSolidBrushAlpha(int col, float alpha)
{
  HGDIOBJ__ *brush=GDP_OBJECT_NEW();
  brush->type=TYPE_BRUSH;
//  brush->color=CreateColor(col,alpha);
  brush->wid=0; 
  return brush;
}

#define FONTSCALE 0.9
HFONT CreateFont(int lfHeight, int lfWidth, int lfEscapement, int lfOrientation, int lfWeight, char lfItalic, 
  char lfUnderline, char lfStrikeOut, char lfCharSet, char lfOutPrecision, char lfClipPrecision, 
         char lfQuality, char lfPitchAndFamily, const char *lfFaceName)
{
  HGDIOBJ__ *font=GDP_OBJECT_NEW();
  font->type=TYPE_FONT;
  float fontwid=lfHeight;
  
  
  if (!fontwid) fontwid=lfWidth;
  if (fontwid<0)fontwid=-fontwid;
  
  if (fontwid < 2 || fontwid > 8192) fontwid=10;
 
  return font;
}


HFONT CreateFontIndirect(LOGFONT *lf)
{
  return CreateFont(lf->lfHeight, lf->lfWidth,lf->lfEscapement, lf->lfOrientation, lf->lfWeight, lf->lfItalic, 
                    lf->lfUnderline, lf->lfStrikeOut, lf->lfCharSet, lf->lfOutPrecision,lf->lfClipPrecision, 
                    lf->lfQuality, lf->lfPitchAndFamily, lf->lfFaceName);
}

int GetTextFace(HDC ctx, int nCount, LPTSTR lpFaceName)
{
  if (lpFaceName) lpFaceName[0]=0;
  return 0;
}

void DeleteObject(HGDIOBJ pen)
{
  if (HGDIOBJ_VALID(pen))
  {
    HGDIOBJ__ *p=(HGDIOBJ__ *)pen;
    if (--p->additional_refcnt < 0)
    {
      if (p->type == TYPE_PEN || p->type == TYPE_BRUSH || p->type == TYPE_FONT || p->type == TYPE_BITMAP)
      {
        if (p->type == TYPE_PEN || p->type == TYPE_BRUSH)
          if (p->wid<0) return;

        GDP_OBJECT_DELETE(p);
      }
      // JF> don't free unknown objects, this should never happen anyway: else free(p);
    }
  }
}


HGDIOBJ SelectObject(HDC ctx, HGDIOBJ pen)
{
  HDC__ *c=(HDC__ *)ctx;
  HGDIOBJ__ *p=(HGDIOBJ__ *) pen;
  HGDIOBJ__ **mod=0;
  if (!HDC_VALID(c)||!p) return 0;
  
  if (p == (HGDIOBJ__ *)TYPE_PEN) mod=&c->curpen;
  else if (p == (HGDIOBJ__ *)TYPE_BRUSH) mod=&c->curbrush;
  else if (p == (HGDIOBJ__ *)TYPE_FONT) mod=&c->curfont;

  if (mod)
  {
    HGDIOBJ__ *np=*mod;
    *mod=0;
    return np?np:p;
  }

  if (!HGDIOBJ_VALID(p)) return 0;
  
  if (p->type == TYPE_PEN) mod=&c->curpen;
  else if (p->type == TYPE_BRUSH) mod=&c->curbrush;
  else if (p->type == TYPE_FONT) mod=&c->curfont;
  else return 0;
  
  HGDIOBJ__ *op=*mod;
  if (!op) op=(HGDIOBJ__ *)p->type;
  if (op != p)
  {
    *mod=p;
  
    if (p->type == TYPE_FONT)
    {
//      CGContextSelectFont(c->ctx,p->fontface,(float)p->wid,kCGEncodingMacRoman);
    }
  }
  return op;
}



void SWELL_FillRect(HDC ctx, const RECT *r, HBRUSH br)
{
  HDC__ *c=(HDC__ *)ctx;
  HGDIOBJ__ *b=(HGDIOBJ__ *) br;
  if (!HDC_VALID(c) || !HGDIOBJ_VALID(b,TYPE_BRUSH)) return;

  if (b->wid<0) return;
  

}

void RoundRect(HDC ctx, int x, int y, int x2, int y2, int xrnd, int yrnd)
{
	xrnd/=3;
	yrnd/=3;
	POINT pts[10]={ // todo: curves between edges
		{x,y+yrnd},
		{x+xrnd,y},
		{x2-xrnd,y},
		{x2,y+yrnd},
		{x2,y2-yrnd},
		{x2-xrnd,y2},
		{x+xrnd,y2},
		{x,y2-yrnd},		
    {x,y+yrnd},
		{x+xrnd,y},
};
	
	WDL_GDP_Polygon(ctx,pts,sizeof(pts)/sizeof(pts[0]));
}

void Ellipse(HDC ctx, int l, int t, int r, int b)
{
  HDC__ *c=(HDC__ *)ctx;
  if (!HDC_VALID(c)) return;
  
  //CGRect rect=CGRectMake(l,t,r-l,b-t);
  
  if (HGDIOBJ_VALID(c->curbrush,TYPE_BRUSH) && c->curbrush->wid >=0)
  {
  }
  if (HGDIOBJ_VALID(c->curpen,TYPE_PEN) && c->curpen->wid >= 0)
  {
  }
}

void Rectangle(HDC ctx, int l, int t, int r, int b)
{
  HDC__ *c=(HDC__ *)ctx;
  if (!HDC_VALID(c)) return;
  
  //CGRect rect=CGRectMake(l,t,r-l,b-t);
  
  if (HGDIOBJ_VALID(c->curbrush,TYPE_BRUSH) && c->curbrush->wid >= 0)
  {
  }
  if (HGDIOBJ_VALID(c->curpen,TYPE_PEN) && c->curpen->wid >= 0)
  {
  }
}

HGDIOBJ GetStockObject(int wh)
{
  switch (wh)
  {
    case NULL_BRUSH:
    {
      static HGDIOBJ__ br={0,};
      br.type=TYPE_BRUSH;
      br.wid=-1;
      return &br;
    }
    case NULL_PEN:
    {
      static HGDIOBJ__ pen={0,};
      pen.type=TYPE_PEN;
      pen.wid=-1;
      return &pen;
    }
  }
  return 0;
}

void Polygon(HDC ctx, POINT *pts, int npts)
{
  HDC__ *c=(HDC__ *)ctx;
  if (!HDC_VALID(c)) return;
  if (((!HGDIOBJ_VALID(c->curbrush,TYPE_BRUSH)||c->curbrush->wid<0) && 
       (!HGDIOBJ_VALID(c->curpen,TYPE_PEN)||c->curpen->wid<0)) || npts<2) return;

//  CGContextBeginPath(c->ctx);
 // CGContextMoveToPoint(c->ctx,(float)pts[0].x,(float)pts[0].y);
  int x;
  for (x = 1; x < npts; x ++)
  {
  //  CGContextAddLineToPoint(c->ctx,(float)pts[x].x,(float)pts[x].y);
  }
  if (HGDIOBJ_VALID(c->curbrush,TYPE_BRUSH) && c->curbrush->wid >= 0)
  {
   // CGContextSetFillColorWithColor(c->ctx,c->curbrush->color);
  }
  if (HGDIOBJ_VALID(c->curpen,TYPE_PEN) && c->curpen->wid>=0)
  {
//    CGContextSetLineWidth(c->ctx,(float)wdl_max(c->curpen->wid,1));
 //   CGContextSetStrokeColorWithColor(c->ctx,c->curpen->color);	
  }
//  CGContextDrawPath(c->ctx,c->curpen && c->curpen->wid>=0 && c->curbrush && c->curbrush->wid>=0 ?  kCGPathFillStroke : c->curpen && c->curpen->wid>=0 ? kCGPathStroke : kCGPathFill);
}

void MoveToEx(HDC ctx, int x, int y, POINT *op)
{
  HDC__ *c=(HDC__ *)ctx;
  if (!HDC_VALID(c)) return;
  if (op) 
  { 
    op->x = (int) (c->lastpos_x);
    op->y = (int) (c->lastpos_y);
  }
  c->lastpos_x=(float)x;
  c->lastpos_y=(float)y;
}

void PolyBezierTo(HDC ctx, POINT *pts, int np)
{
  HDC__ *c=(HDC__ *)ctx;
  if (!HDC_VALID(c)||!HGDIOBJ_VALID(c->curpen,TYPE_PEN)||c->curpen->wid<0||np<3) return;
  
//  CGContextSetLineWidth(c->ctx,(float)wdl_max(c->curpen->wid,1));
//  CGContextSetStrokeColorWithColor(c->ctx,c->curpen->color);
	
//  CGContextBeginPath(c->ctx);
//  CGContextMoveToPoint(c->ctx,c->lastpos_x,c->lastpos_y);
  int x; 
  float xp,yp;
  for (x = 0; x < np-2; x += 3)
  {
/*    CGContextAddCurveToPoint(c->ctx,
      (float)pts[x].x,(float)pts[x].y,
      (float)pts[x+1].x,(float)pts[x+1].y,
*/
      xp=(float)pts[x+2].x;
      yp=(float)pts[x+2].y;    
  }
  c->lastpos_x=(float)xp;
  c->lastpos_y=(float)yp;
//  CGContextStrokePath(c->ctx);
}


void SWELL_LineTo(HDC ctx, int x, int y)
{
  HDC__ *c=(HDC__ *)ctx;
  if (!HDC_VALID(c)||!HGDIOBJ_VALID(c->curpen,TYPE_PEN)||c->curpen->wid<0) return;

//  CGContextSetLineWidth(c->ctx,(float)wdl_max(c->curpen->wid,1));
//  CGContextSetStrokeColorWithColor(c->ctx,c->curpen->color);
	
//  CGContextBeginPath(c->ctx);
//  CGContextMoveToPoint(c->ctx,c->lastpos_x,c->lastpos_y);
  float fx=(float)x,fy=(float)y;
  
//  CGContextAddLineToPoint(c->ctx,fx,fy);
  c->lastpos_x=fx;
  c->lastpos_y=fy;
//  CGContextStrokePath(c->ctx);
}

void PolyPolyline(HDC ctx, POINT *pts, DWORD *cnts, int nseg)
{
  HDC__ *c=(HDC__ *)ctx;
  if (!HDC_VALID(c)||!HGDIOBJ_VALID(c->curpen,TYPE_PEN)||c->curpen->wid<0||nseg<1) return;

//  CGContextSetLineWidth(c->ctx,(float)wdl_max(c->curpen->wid,1));
//  CGContextSetStrokeColorWithColor(c->ctx,c->curpen->color);
	
//  CGContextBeginPath(c->ctx);
  
  while (nseg-->0)
  {
    DWORD cnt=*cnts++;
    if (!cnt) continue;
    if (!--cnt) { pts++; continue; }
    
 //   CGContextMoveToPoint(c->ctx,(float)pts->x,(float)pts->y);
    pts++;
    
    while (cnt--)
    {
//      CGContextAddLineToPoint(c->ctx,(float)pts->x,(float)pts->y);
      pts++;
    }
  }
//  CGContextStrokePath(c->ctx);
}
void *SWELL_GetCtxGC(HDC ctx)
{
  HDC__ *ct=(HDC__ *)ctx;
  if (!HDC_VALID(ct)) return 0;
  return NULL;
}


void SWELL_SetPixel(HDC ctx, int x, int y, int c)
{
  HDC__ *ct=(HDC__ *)ctx;
  if (!HDC_VALID(ct)) return;
 /* CGContextBeginPath(ct->ctx);
  CGContextMoveToPoint(ct->ctx,(float)x,(float)y);
  CGContextAddLineToPoint(ct->ctx,(float)x+0.5,(float)y+0.5);
  CGContextSetLineWidth(ct->ctx,(float)1.5);
  CGContextSetRGBStrokeColor(ct->ctx,GetRValue(c)/255.0,GetGValue(c)/255.0,GetBValue(c)/255.0,1.0);
  CGContextStrokePath(ct->ctx);	
*/
}


BOOL GetTextMetrics(HDC ctx, TEXTMETRIC *tm)
{
  HDC__ *ct=(HDC__ *)ctx;
  if (tm) // give some sane defaults
  {
    tm->tmInternalLeading=3;
    tm->tmAscent=12;
    tm->tmDescent=4;
    tm->tmHeight=16;
    tm->tmAveCharWidth = 10;
  }
  if (!HDC_VALID(ct)||!tm) return 0;
  
  return 1;
}


int DrawText(HDC ctx, const char *buf, int buflen, RECT *r, int align)
{
  HDC__ *ct=(HDC__ *)ctx;
  if (!HDC_VALID(ct)) return 0;
  if (r && (align&DT_CALCRECT)) 
  {
    r->top=r->left=0;
    r->bottom=10;
    r->right = ( buflen < 0 ? strlen(buf) : buflen ) *8;
  }
  else printf("DrawText: %s\n",buf);
  return 10;
}

void SetBkColor(HDC ctx, int col)
{
  HDC__ *ct=(HDC__ *)ctx;
  if (!HDC_VALID(ct)) return;
  ct->curbkcol=col;
}

void SetBkMode(HDC ctx, int col)
{
  HDC__ *ct=(HDC__ *)ctx;
  if (!HDC_VALID(ct)) return;
  ct->curbkmode=col;
}
int GetTextColor(HDC ctx)
{
  HDC__ *ct=(HDC__ *)ctx;
  if (!HDC_VALID(ct)) return -1;
  return ct->cur_text_color_int;
}

void SetTextColor(HDC ctx, int col)
{
  HDC__ *ct=(HDC__ *)ctx;
  if (!HDC_VALID(ct)) return;
  ct->cur_text_color_int = col;
  
}

HICON LoadNamedImage(const char *name, bool alphaFromMask)
{
  return 0; // todo
}

void DrawImageInRect(HDC ctx, HICON img, const RECT *r)
{
  // todo
}


BOOL GetObject(HICON icon, int bmsz, void *_bm)
{
  memset(_bm,0,bmsz);
  if (bmsz != sizeof(BITMAP)) return false;
  BITMAP *bm=(BITMAP *)_bm;
  HGDIOBJ__ *i = (HGDIOBJ__ *)icon;
  if (!HGDIOBJ_VALID(i,TYPE_BITMAP)) return false;

  return false;
/*
  NSImage *img = i->bitmapptr;
  if (!img) return false;
  bm->bmWidth = (int) ([img size].width+0.5);
  bm->bmHeight = (int) ([img size].height+0.5);
  return true;
*/
}


#define ColorFromNSColor(a,b) (b)
int GetSysColor(int idx)
{
 // NSColors that seem to be valid: textBackgroundColor, selectedTextBackgroundColor, textColor, selectedTextColor
  switch (idx)
  {
    case COLOR_WINDOW: return ColorFromNSColor([NSColor controlColor],RGB(192,192,192));
    case COLOR_3DFACE: 
    case COLOR_BTNFACE: return ColorFromNSColor([NSColor controlColor],RGB(192,192,192));
    case COLOR_SCROLLBAR: return ColorFromNSColor([NSColor controlColor],RGB(32,32,32));
    case COLOR_3DSHADOW: return ColorFromNSColor([NSColor selectedTextBackgroundColor],RGB(96,96,96));
    case COLOR_3DHILIGHT: return ColorFromNSColor([NSColor selectedTextBackgroundColor],RGB(224,224,224));
    case COLOR_BTNTEXT: return ColorFromNSColor([NSColor selectedTextBackgroundColor],RGB(0,0,0));
    case COLOR_3DDKSHADOW: return (ColorFromNSColor([NSColor selectedTextBackgroundColor],RGB(96,96,96))>>1)&0x7f7f7f;
    case COLOR_INFOBK: return RGB(255,240,200);
    case COLOR_INFOTEXT: return RGB(0,0,0);
      
  }
  return 0;
}

void BitBltAlphaFromMem(HDC hdcOut, int x, int y, int w, int h, void *inbufptr, int inbuf_span, int inbuf_h, int xin, int yin, int mode, bool useAlphaChannel, float opacity)
{
}

void BitBltAlpha(HDC hdcOut, int x, int y, int w, int h, HDC hdcIn, int xin, int yin, int mode, bool useAlphaChannel, float opacity)
{
}

void BitBlt(HDC hdcOut, int x, int y, int w, int h, HDC hdcIn, int xin, int yin, int mode)
{
}

void StretchBlt(HDC hdcOut, int x, int y, int w, int h, HDC hdcIn, int xin, int yin, int srcw, int srch, int mode)
{
}

void SWELL_PushClipRegion(HDC ctx)
{
  HDC__ *ct=(HDC__ *)ctx;
//  if (ct && ct->ctx) CGContextSaveGState(ct->ctx);
}

void SWELL_SetClipRegion(HDC ctx, const RECT *r)
{
  HDC__ *ct=(HDC__ *)ctx;
//  if (ct && ct->ctx) CGContextClipToRect(ct->ctx,CGRectMake(r->left,r->top,r->right-r->left,r->bottom-r->top));

}

void SWELL_PopClipRegion(HDC ctx)
{
  HDC__ *ct=(HDC__ *)ctx;
//  if (ct && ct->ctx) CGContextRestoreGState(ct->ctx);
}

void *SWELL_GetCtxFrameBuffer(HDC ctx)
{
  HDC__ *ct=(HDC__ *)ctx;
  if (HDC_VALID(ct)) return ct->ownedData;
  return 0;
}


HDC GetDC(HWND h)
{
  return NULL;
}

HDC GetWindowDC(HWND h)
{
  return NULL;
}

void ReleaseDC(HWND h, HDC hdc)
{
}

void SWELL_FillDialogBackground(HDC hdc, const RECT *r, int level)
{
}

HGDIOBJ SWELL_CloneGDIObject(HGDIOBJ a)
{
  if (HGDIOBJ_VALID(a))
  {
    a->additional_refcnt++;
    return a;
  }
  return NULL;
}

HDC BeginPaint(HWND hwnd, PAINTSTRUCT *ps)
{
  if (!ps) return 0;
  memset(ps,0,sizeof(PAINTSTRUCT));
  if (!hwnd) return 0;

  return NULL;
}


HBITMAP CreateBitmap(int width, int height, int numplanes, int bitsperpixel, unsigned char* bits)
{
  return NULL;
}

HICON CreateIconIndirect(ICONINFO* iconinfo)
{
  return NULL;
}
HIMAGELIST ImageList_CreateEx()
{
  return (HIMAGELIST)new WDL_PtrList<HGDIOBJ__>;
}
BOOL ImageList_Remove(HIMAGELIST list, int idx)
{
  WDL_PtrList<HGDIOBJ__>* imglist=(WDL_PtrList<HGDIOBJ__>*)list;
  if (imglist && idx < imglist->GetSize())
  {
    if (idx < 0) 
    {
      int x,n=imglist->GetSize();
      for (x=0;x<n;x++)
      {
        HGDIOBJ__ *a = imglist->Get(x);
        if (a) DeleteObject(a);
      }
      imglist->Empty();
    }
    else 
    {
      HGDIOBJ__ *a = imglist->Get(idx);
      imglist->Set(idx, NULL); 
      if (a) DeleteObject(a);
    }
    return TRUE;
  }
  
  return FALSE;
}

void ImageList_Destroy(HIMAGELIST list)
{
  if (!list) return;
  WDL_PtrList<HGDIOBJ__> *p=(WDL_PtrList<HGDIOBJ__>*)list;
  ImageList_Remove(list,-1);
  delete p;
}

int ImageList_ReplaceIcon(HIMAGELIST list, int offset, HICON image)
{
  if (!image || !list) return -1;
  WDL_PtrList<HGDIOBJ__> *l=(WDL_PtrList<HGDIOBJ__> *)list;

  HGDIOBJ__ *imgsrc = (HGDIOBJ__*)image;
  if (!HGDIOBJ_VALID(imgsrc,TYPE_BITMAP)) return -1;

  HGDIOBJ__* icon=GDP_OBJECT_NEW();
  icon->type=TYPE_BITMAP;
  icon->wid=1;
  // todo: copy underlying image

  image = (HICON) icon;

  if (offset<0||offset>=l->GetSize()) 
  {
    l->Add(image);
    offset=l->GetSize()-1;
  }
  else
  {
    HICON old=l->Get(offset);
    l->Set(offset,image);
    if (old) DeleteObject(old);
  }
  return offset;
}




#endif
#endif // !SWELL_LICE_GDI
