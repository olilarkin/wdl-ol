/* Cockos SWELL (Simple/Small Win32 Emulation Layer for Linux)
   Copyright (C) 2006-2008, Cockos, Inc.

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
  

  */


#ifndef SWELL_PROVIDED_BY_APP

#include "swell.h"

#define SWELL_INTERNAL_HTREEITEM_IMPL
#include "swell-internal.h"

#include <math.h>
#include "../mutex.h"
#include "../ptrlist.h"
#include "../queue.h"
#include "../wdlcstring.h"

#include "swell-dlggen.h"

int g_swell_want_nice_style = 1; //unused but here for compat

HWND__ *SWELL_topwindows;

static HWND s_captured_window;
HWND SWELL_g_focuswnd; // update from focus-in-event / focus-out-event signals, have to enable the GDK_FOCUS_CHANGE_MASK bits for the gdkwindow
static DWORD s_lastMessagePos;

#ifdef SWELL_TARGET_GDK

static GdkEvent *s_cur_evt;
static GdkWindow *SWELL_g_focus_oswindow;
static int SWELL_gdk_active;

HWND ChildWindowFromPoint(HWND h, POINT p);
bool IsWindowEnabled(HWND hwnd);

static void swell_gdkEventHandler(GdkEvent *event, gpointer data);
void SWELL_initargs(int *argc, char ***argv) 
{
  if (!SWELL_gdk_active) 
  {
   // maybe make the main app call this with real parms
    SWELL_gdk_active = gdk_init_check(argc,argv) ? 1 : -1;
    if (SWELL_gdk_active > 0)
      gdk_event_handler_set(swell_gdkEventHandler,NULL,NULL);
  }
}

static bool swell_initwindowsys()
{
  if (!SWELL_gdk_active) 
  {
   // maybe make the main app call this with real parms
    int argc=1;
    char buf[32];
    strcpy(buf,"blah");
    char *argv[2];
    argv[0] = buf;
    argv[1] = buf;
    char **pargv = argv;
    SWELL_initargs(&argc,&pargv);
  }
  
  return SWELL_gdk_active>0;
}

static void swell_destroyOSwindow(HWND hwnd)
{
  if (hwnd && hwnd->m_oswindow)
  {
    if (SWELL_g_focus_oswindow == hwnd->m_oswindow) SWELL_g_focus_oswindow=NULL;
    gdk_window_destroy(hwnd->m_oswindow);
    hwnd->m_oswindow=NULL;
#ifdef SWELL_LICE_GDI
    delete hwnd->m_backingstore;
    hwnd->m_backingstore=0;
#endif
  }
}
static void swell_setOSwindowtext(HWND hwnd)
{
  if (hwnd && hwnd->m_oswindow)
  {
    gdk_window_set_title(hwnd->m_oswindow, (char*)hwnd->m_title.Get());
  }
}
static void swell_manageOSwindow(HWND hwnd, bool wantfocus)
{
  if (!hwnd) return;

  bool isVis = !!hwnd->m_oswindow;
  bool wantVis = !hwnd->m_parent && hwnd->m_visible;

  if (isVis != wantVis)
  {
    if (!wantVis) swell_destroyOSwindow(hwnd);
    else 
    {
      if (swell_initwindowsys())
      {
        HWND owner = NULL; // hwnd->m_owner;
// parent windows dont seem to work the way we'd want, yet, in gdk...
/*        while (owner && !owner->m_oswindow)
        {
          if (owner->m_parent)  owner = owner->m_parent;
          else if (owner->m_owner) owner = owner->m_owner;
        }
*/
 
        RECT r = hwnd->m_position;
        GdkWindowAttr attr={0,};
        attr.title = "";
        attr.event_mask = GDK_ALL_EVENTS_MASK|GDK_EXPOSURE_MASK;
        attr.x = r.left;
        attr.y = r.top;
        attr.width = r.right-r.left;
        attr.height = r.bottom-r.top;
        attr.wclass = GDK_INPUT_OUTPUT;
        attr.window_type = GDK_WINDOW_TOPLEVEL;
        hwnd->m_oswindow = gdk_window_new(owner ? owner->m_oswindow : NULL,&attr,GDK_WA_X|GDK_WA_Y);
 
        if (hwnd->m_oswindow) 
        {
          gdk_window_set_user_data(hwnd->m_oswindow,hwnd);
          gdk_window_move_resize(hwnd->m_oswindow,r.left,r.top,r.right-r.left,r.bottom-r.top);
          if (!wantfocus) gdk_window_set_focus_on_map(hwnd->m_oswindow,false);
          HWND DialogBoxIsActive();
          if (!(hwnd->m_style & WS_CAPTION)) 
          {
            gdk_window_set_override_redirect(hwnd->m_oswindow,true);
          }
          else if (/*hwnd == DialogBoxIsActive() || */ !(hwnd->m_style&WS_THICKFRAME))
          {
            gdk_window_set_type_hint(hwnd->m_oswindow,GDK_WINDOW_TYPE_HINT_DIALOG);
            gdk_window_set_decorations(hwnd->m_oswindow,(GdkWMDecoration) (GDK_DECOR_BORDER|GDK_DECOR_TITLE|GDK_DECOR_MINIMIZE));
          }
          else
          {
            gdk_window_set_type_hint(hwnd->m_oswindow,GDK_WINDOW_TYPE_HINT_NORMAL);
            gdk_window_set_decorations(hwnd->m_oswindow,(GdkWMDecoration) (GDK_DECOR_ALL & ~(GDK_DECOR_MENU)));
          }

          gdk_window_show(hwnd->m_oswindow);
        }
      }
    }
  }
  if (wantVis) swell_setOSwindowtext(hwnd);

//  if (wantVis && isVis && wantfocus && hwnd && hwnd->m_oswindow) gdk_window_raise(hwnd->m_oswindow);
}

#ifdef SWELL_LICE_GDI
class LICE_CairoBitmap : public LICE_IBitmap
{
  public:
    LICE_CairoBitmap() 
    {
      m_fb = NULL; 
      m_allocsize = m_width = m_height = m_span = 0;
      m_surf = NULL;
    }
    virtual ~LICE_CairoBitmap() 
    { 
      if (m_surf) cairo_surface_destroy(m_surf);
      free(m_fb);
    }

    // LICE_IBitmap interface
    virtual LICE_pixel *getBits() 
    { 
      const UINT_PTR extra=LICE_MEMBITMAP_ALIGNAMT;
      return (LICE_pixel *) (((UINT_PTR)m_fb + extra)&~extra);
    }

    virtual int getWidth() { return m_width; }
    virtual int getHeight() { return m_height; }
    virtual int getRowSpan() { return m_span; }
    virtual bool resize(int w, int h)
    {
      if (w<0) w=0; 
      if (h<0) h=0;
      if (w == m_width && h == m_height) return false;

      if (m_surf) cairo_surface_destroy(m_surf);
      m_surf = NULL;

      m_span = w ? cairo_format_stride_for_width(CAIRO_FORMAT_RGB24,w)/4 : 0;
      const int sz = h * m_span * 4 + LICE_MEMBITMAP_ALIGNAMT;
      if (!m_fb || m_allocsize < sz || sz < m_allocsize/4)
      {
        const int newalloc = m_allocsize<sz ? (sz*3)/2 : sz;
        void *p = realloc(m_fb,newalloc);
        if (!p) return false;

        m_fb = (LICE_pixel *)p;
        m_allocsize = newalloc;
      }

      m_width=w && h ? w :0;
      m_height=w && h ? h : 0;
      return true;
    }
    virtual INT_PTR Extended(int id, void* data) 
    { 
      if (id == 0xca140) 
      {
        if (data) 
        {
          // in case we want to release surface
          return 0;
        }

        if (!m_surf) 
          m_surf = cairo_image_surface_create_for_data((guchar*)getBits(), CAIRO_FORMAT_RGB24, 
                                                       getWidth(),getHeight(), getRowSpan()*4);
        return (INT_PTR)m_surf;
      }
      return 0; 
    }

private:
  LICE_pixel *m_fb;
  int m_width, m_height, m_span;
  int m_allocsize;
  cairo_surface_t *m_surf;
};
#endif

void swell_OSupdateWindowToScreen(HWND hwnd, RECT *rect)
{
#ifdef SWELL_LICE_GDI
  if (hwnd && hwnd->m_backingstore && hwnd->m_oswindow)
  {
    LICE_IBitmap *bm = hwnd->m_backingstore;
    LICE_SubBitmap tmpbm(bm,rect->left,rect->top,rect->right-rect->left,rect->bottom-rect->top);

    GdkRectangle rrr={rect->left,rect->top,rect->right-rect->left,rect->bottom-rect->top};
    gdk_window_begin_paint_rect(hwnd->m_oswindow, &rrr);

    cairo_t * crc = gdk_cairo_create (hwnd->m_oswindow);
    cairo_surface_t *temp_surface = (cairo_surface_t*)bm->Extended(0xca140,NULL);
    cairo_reset_clip(crc);
    cairo_rectangle(crc, rect->left, rect->top, rect->right-rect->left, rect->bottom-rect->top);
    cairo_clip(crc);
    if (temp_surface) cairo_set_source_surface(crc, temp_surface, 0,0);
    cairo_paint(crc);
    cairo_destroy(crc);

    gdk_window_end_paint(hwnd->m_oswindow);

    if (temp_surface) bm->Extended(0xca140,temp_surface); // release

  }
#endif
}

static int swell_gdkConvertKey(int key)
{
  //gdk key to VK_ conversion
  switch(key)
  {
#if SWELL_TARGET_GDK == 2
  case GDK_Home: return VK_HOME;
  case GDK_End: return VK_END;
  case GDK_Up: return VK_UP;
  case GDK_Down: return VK_DOWN;
  case GDK_Left: return VK_LEFT;
  case GDK_Right: return VK_RIGHT;
  case GDK_Page_Up: return VK_PRIOR;
  case GDK_Page_Down: return VK_NEXT;
  case GDK_Insert: return VK_INSERT;
  case GDK_Delete: return VK_DELETE;
  case GDK_Escape: return VK_ESCAPE;
  case GDK_BackSpace: return VK_BACK;
  case GDK_Return: return VK_RETURN;
  case GDK_ISO_Left_Tab:
  case GDK_Tab: return VK_TAB;
  case GDK_F1: return VK_F1;
  case GDK_F2: return VK_F2;
  case GDK_F3: return VK_F3;
  case GDK_F4: return VK_F4;
  case GDK_F5: return VK_F5;
  case GDK_F6: return VK_F6;
  case GDK_F7: return VK_F7;
  case GDK_F8: return VK_F8;
  case GDK_F9: return VK_F9;
  case GDK_F10: return VK_F10;
  case GDK_F11: return VK_F11;
  case GDK_F12: return VK_F12;
#else
  case GDK_KEY_Home: return VK_HOME;
  case GDK_KEY_End: return VK_END;
  case GDK_KEY_Up: return VK_UP;
  case GDK_KEY_Down: return VK_DOWN;
  case GDK_KEY_Left: return VK_LEFT;
  case GDK_KEY_Right: return VK_RIGHT;
  case GDK_KEY_Page_Up: return VK_PRIOR;
  case GDK_KEY_Page_Down: return VK_NEXT;
  case GDK_KEY_Insert: return VK_INSERT;
  case GDK_KEY_Delete: return VK_DELETE;
  case GDK_KEY_Escape: return VK_ESCAPE;
  case GDK_KEY_BackSpace: return VK_BACK;
  case GDK_KEY_Return: return VK_RETURN;
  case GDK_KEY_ISO_Left_Tab:
  case GDK_KEY_Tab: return VK_TAB;
  case GDK_KEY_F1: return VK_F1;
  case GDK_KEY_F2: return VK_F2;
  case GDK_KEY_F3: return VK_F3;
  case GDK_KEY_F4: return VK_F4;
  case GDK_KEY_F5: return VK_F5;
  case GDK_KEY_F6: return VK_F6;
  case GDK_KEY_F7: return VK_F7;
  case GDK_KEY_F8: return VK_F8;
  case GDK_KEY_F9: return VK_F9;
  case GDK_KEY_F10: return VK_F10;
  case GDK_KEY_F11: return VK_F11;
  case GDK_KEY_F12: return VK_F12;
#endif
  }
  return 0;
}

static LRESULT SendMouseMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  if (!hwnd || !hwnd->m_wndproc) return -1;
  if (!IsWindowEnabled(hwnd)) 
  {
    HWND DialogBoxIsActive();
    HWND h = DialogBoxIsActive();
    if (h) SetForegroundWindow(h);
    return -1;
  }

  LRESULT htc=0;
  if (msg != WM_MOUSEWHEEL && !GetCapture())
  {
    DWORD p=GetMessagePos(); 

    htc=hwnd->m_wndproc(hwnd,WM_NCHITTEST,0,p); 
    if (hwnd->m_hashaddestroy||!hwnd->m_wndproc) 
    {
      return -1; // if somehow WM_NCHITTEST destroyed us, bail
    }
     
    if (htc!=HTCLIENT) 
    { 
      if (msg==WM_MOUSEMOVE) return hwnd->m_wndproc(hwnd,WM_NCMOUSEMOVE,htc,p); 
//      if (msg==WM_MOUSEWHEEL) return hwnd->m_wndproc(hwnd,WM_NCMOUSEWHEEL,htc,p); 
//      if (msg==WM_MOUSEHWHEEL) return hwnd->m_wndproc(hwnd,WM_NCMOUSEHWHEEL,htc,p); 
      if (msg==WM_LBUTTONUP) return hwnd->m_wndproc(hwnd,WM_NCLBUTTONUP,htc,p); 
      if (msg==WM_LBUTTONDOWN) return hwnd->m_wndproc(hwnd,WM_NCLBUTTONDOWN,htc,p); 
      if (msg==WM_LBUTTONDBLCLK) return hwnd->m_wndproc(hwnd,WM_NCLBUTTONDBLCLK,htc,p); 
      if (msg==WM_RBUTTONUP) return hwnd->m_wndproc(hwnd,WM_NCRBUTTONUP,htc,p); 
      if (msg==WM_RBUTTONDOWN) return hwnd->m_wndproc(hwnd,WM_NCRBUTTONDOWN,htc,p); 
      if (msg==WM_RBUTTONDBLCLK) return hwnd->m_wndproc(hwnd,WM_NCRBUTTONDBLCLK,htc,p); 
      if (msg==WM_MBUTTONUP) return hwnd->m_wndproc(hwnd,WM_NCMBUTTONUP,htc,p); 
      if (msg==WM_MBUTTONDOWN) return hwnd->m_wndproc(hwnd,WM_NCMBUTTONDOWN,htc,p); 
      if (msg==WM_MBUTTONDBLCLK) return hwnd->m_wndproc(hwnd,WM_NCMBUTTONDBLCLK,htc,p); 
    } 
  }


  LRESULT ret=hwnd->m_wndproc(hwnd,msg,wParam,lParam);

  if (msg==WM_LBUTTONUP || msg==WM_RBUTTONUP || msg==WM_MOUSEMOVE || msg==WM_MBUTTONUP) 
  {
    if (!GetCapture() && (hwnd->m_hashaddestroy || !hwnd->m_wndproc || !hwnd->m_wndproc(hwnd,WM_SETCURSOR,(WPARAM)hwnd,htc | (msg<<16))))    
    {
       // todo: set default cursor
    }
  }

  return ret;
}

static void swell_gdkEventHandler(GdkEvent *evt, gpointer data)
{
  GdkEvent *oldEvt = s_cur_evt;
  s_cur_evt = evt;
    {
      HWND hwnd = NULL;
      if (((GdkEventAny*)evt)->window) gdk_window_get_user_data(((GdkEventAny*)evt)->window,(gpointer*)&hwnd);

      if (hwnd) // validate window (todo later have a window class that we check)
      {
        HWND a = SWELL_topwindows;
        while (a && a != hwnd) a=a->m_next;
        if (!a) hwnd=NULL;
      }

      if (evt->type == GDK_FOCUS_CHANGE)
      {
        GdkEventFocus *fc = (GdkEventFocus *)evt;
        if (fc->in) SWELL_g_focus_oswindow = hwnd ? fc->window : NULL;
      }

      if (hwnd) switch (evt->type)
      {
        case GDK_DELETE:
          if (IsWindowEnabled(hwnd) && !SendMessage(hwnd,WM_CLOSE,0,0))
            SendMessage(hwnd,WM_COMMAND,IDCANCEL,0);
        break;
        case GDK_EXPOSE: // paint! GdkEventExpose...
          {
            GdkEventExpose *exp = (GdkEventExpose*)evt;
#ifdef SWELL_LICE_GDI
            // super slow
            RECT r,cr;

            // don't use GetClientRect(),since we're getting it pre-NCCALCSIZE etc

#if SWELL_TARGET_GDK==2
            { 
              gint w=0,h=0; 
              gdk_drawable_get_size(hwnd->m_oswindow,&w,&h);
              cr.right = w; cr.bottom = h;
            }
#else
            cr.right = gdk_window_get_width(hwnd->m_oswindow);
            cr.bottom = gdk_window_get_height(hwnd->m_oswindow);
#endif
            cr.left=cr.top=0;

            r.left = exp->area.x; 
            r.top=exp->area.y; 
            r.bottom=r.top+exp->area.height; 
            r.right=r.left+exp->area.width;

            if (!hwnd->m_backingstore) hwnd->m_backingstore = new LICE_CairoBitmap;
            // if (!hwnd->m_backingstore) hwnd->m_backingstore = new LICE_MemBitmap;

            bool forceref = hwnd->m_backingstore->resize(cr.right-cr.left,cr.bottom-cr.top);
            if (forceref) r = cr;

            LICE_SubBitmap tmpbm(hwnd->m_backingstore,r.left,r.top,r.right-r.left,r.bottom-r.top);

            if (tmpbm.getWidth()>0 && tmpbm.getHeight()>0) 
            {
              void SWELL_internalLICEpaint(HWND hwnd, LICE_IBitmap *bmout, int bmout_xpos, int bmout_ypos, bool forceref);
              SWELL_internalLICEpaint(hwnd, &tmpbm, r.left, r.top, forceref);

              GdkRectangle rrr={r.left,r.top,r.right-r.left,r.bottom-r.top};
              gdk_window_begin_paint_rect(exp->window, &rrr);

              cairo_t *crc = gdk_cairo_create (exp->window);
              LICE_IBitmap *bm = hwnd->m_backingstore;
              cairo_surface_t *temp_surface = (cairo_surface_t*)bm->Extended(0xca140,NULL);
              cairo_reset_clip(crc);
              cairo_rectangle(crc, r.left, r.top, r.right-r.left, r.bottom-r.top);
              cairo_clip(crc);
              if (temp_surface) cairo_set_source_surface(crc, temp_surface, 0,0);
              cairo_paint(crc);
              cairo_destroy(crc);
              if (temp_surface) bm->Extended(0xca140,temp_surface); // release

              gdk_window_end_paint(exp->window);
            }
#endif
          }
        break;
        case GDK_CONFIGURE: // size/move, GdkEventConfigure
          {
            GdkEventConfigure *cfg = (GdkEventConfigure*)evt;
            int flag=0;
            if (cfg->x != hwnd->m_position.left || cfg->y != hwnd->m_position.top)  flag|=1;
            if (cfg->width != hwnd->m_position.right-hwnd->m_position.left || cfg->height != hwnd->m_position.bottom - hwnd->m_position.top) flag|=2;
            hwnd->m_position.left = cfg->x;
            hwnd->m_position.top = cfg->y;
            hwnd->m_position.right = cfg->x + cfg->width;
            hwnd->m_position.bottom = cfg->y + cfg->height;
            if (flag&1) SendMessage(hwnd,WM_MOVE,0,0);
            if (flag&2) SendMessage(hwnd,WM_SIZE,0,0);
          }
        break;
        case GDK_WINDOW_STATE: /// GdkEventWindowState for min/max
          //printf("minmax\n");
        break;
        case GDK_GRAB_BROKEN:
          {
            GdkEventGrabBroken *bk = (GdkEventGrabBroken*)evt;
            if (s_captured_window)
            {
              SendMessage(s_captured_window,WM_CAPTURECHANGED,0,0);
              s_captured_window=0;
            }
          }
        break;
        case GDK_KEY_PRESS:
        case GDK_KEY_RELEASE:
          { // todo: pass through app-specific default processing before sending to child window
            GdkEventKey *k = (GdkEventKey *)evt;
            //printf("key%s: %d %s\n", evt->type == GDK_KEY_PRESS ? "down" : "up", k->keyval, k->string);
            int modifiers = 0;
            if (k->state&GDK_CONTROL_MASK) modifiers|=FCONTROL;
            if (k->state&GDK_MOD1_MASK) modifiers|=FALT;
            if (k->state&GDK_SHIFT_MASK) modifiers|=FSHIFT;

            int kv = swell_gdkConvertKey(k->keyval);
            if (kv) 
            {
              modifiers |= FVIRTKEY;
            }
            else 
            {
              kv = k->keyval; // ASCII!
            }

            HWND foc = GetFocus();
            if (foc && IsChild(hwnd,foc)) hwnd=foc;
            MSG msg = { hwnd, evt->type == GDK_KEY_PRESS ? WM_KEYDOWN : WM_KEYUP, 
                              kv, modifiers, };
            if (SWELLAppMain(SWELLAPP_PROCESSMESSAGE,(INT_PTR)&msg,0)<=0)
              SendMessage(msg.hwnd, msg.message, msg.wParam, msg.lParam);
          }
        break;
        case GDK_MOTION_NOTIFY:
          {
            GdkEventMotion *m = (GdkEventMotion *)evt;
            s_lastMessagePos = MAKELONG(((int)m->x_root&0xffff),((int)m->y_root&0xffff));
            POINT p={m->x, m->y};
            HWND hwnd2 = GetCapture();
            if (!hwnd2) hwnd2=ChildWindowFromPoint(hwnd, p);
            POINT p2={m->x_root, m->y_root};
            ScreenToClient(hwnd2, &p2);
            if (hwnd2) hwnd2->Retain();
            SendMouseMessage(hwnd2, WM_MOUSEMOVE, 0, MAKELPARAM(p2.x, p2.y));
            if (hwnd2) hwnd2->Release();
            gdk_event_request_motions(m);
          }
        break;
        case GDK_SCROLL:
          {
            GdkEventScroll *b = (GdkEventScroll *)evt;
            s_lastMessagePos = MAKELONG(((int)b->x_root&0xffff),((int)b->y_root&0xffff));
            POINT p={b->x, b->y};
            HWND hwnd2 = GetCapture();
            if (!hwnd2) hwnd2=ChildWindowFromPoint(hwnd, p);
            POINT p2={b->x_root, b->y_root};
            // p2 is screen coordinates for WM_MOUSEWHEEL

            int msg=(b->direction == GDK_SCROLL_UP || b->direction == GDK_SCROLL_DOWN) ? WM_MOUSEWHEEL :
                    (b->direction == GDK_SCROLL_LEFT || b->direction == GDK_SCROLL_RIGHT) ? WM_MOUSEHWHEEL : 0;
            
            if (msg) 
            {
              int v = (b->direction == GDK_SCROLL_UP || b->direction == GDK_SCROLL_LEFT) ? 120 : -120;
 
              if (hwnd2) hwnd2->Retain();
              SendMouseMessage(hwnd2, msg, (v<<16), MAKELPARAM(p2.x, p2.y));
              if (hwnd2) hwnd2->Release();
            }
          }
        break;
        case GDK_BUTTON_PRESS:
        case GDK_2BUTTON_PRESS:
        case GDK_BUTTON_RELEASE:
          {
            GdkEventButton *b = (GdkEventButton *)evt;
            s_lastMessagePos = MAKELONG(((int)b->x_root&0xffff),((int)b->y_root&0xffff));
            POINT p={b->x, b->y};
            HWND hwnd2 = GetCapture();
            if (!hwnd2) hwnd2=ChildWindowFromPoint(hwnd, p);
            POINT p2={b->x_root, b->y_root};
            ScreenToClient(hwnd2, &p2);

            int msg=WM_LBUTTONDOWN;
            if (b->button==2) msg=WM_MBUTTONDOWN;
            else if (b->button==3) msg=WM_RBUTTONDOWN;
            
            if (hwnd && hwnd->m_oswindow && 
                SWELL_g_focus_oswindow != hwnd->m_oswindow)
                     SWELL_g_focus_oswindow = hwnd->m_oswindow;

            if(evt->type == GDK_BUTTON_RELEASE) msg++; // move from down to up
            else if(evt->type == GDK_2BUTTON_PRESS) msg+=2; // move from down to up

            if (hwnd2) hwnd2->Retain();
            SendMouseMessage(hwnd2, msg, 0, MAKELPARAM(p2.x, p2.y));
            if (hwnd2) hwnd2->Release();
          }
        break;
        default:
          //printf("msg: %d\n",evt->type);
        break;
      }

    }
  s_cur_evt = oldEvt;
}

void swell_runOSevents()
{
  if (SWELL_gdk_active>0) 
  {
//    static GMainLoop *loop;
//    if (!loop) loop = g_main_loop_new(NULL,TRUE);
    gdk_window_process_all_updates();

    GMainContext *ctx=g_main_context_default();
    while (g_main_context_iteration(ctx,FALSE))
    {
      GdkEvent *evt;
      while (gdk_events_pending() && (evt = gdk_event_get()))
      {
        swell_gdkEventHandler(evt,(gpointer)1);
        gdk_event_free(evt);
      }
    }
  }
}
void SWELL_RunEvents()
{
  swell_runOSevents();
}

#else
void SWELL_initargs(int *argc, char ***argv)
{
}
void swell_OSupdateWindowToScreen(HWND hwnd, RECT *rect)
{
}
#define swell_initwindowsys() (0)
#define swell_destroyOSwindow(x)
#define swell_manageOSwindow(x,y)
#define swell_runOSevents()
#define swell_setOSwindowtext(x) { if (x) printf("SWELL: swt '%s'\n",(x)->m_title.Get()); }
#endif

HWND__::HWND__(HWND par, int wID, RECT *wndr, const char *label, bool visible, WNDPROC wndproc, DLGPROC dlgproc)
{
  m_refcnt=1;
  m_private_data=0;

     m_classname = "unknown";
     m_wndproc=wndproc?wndproc:dlgproc?(WNDPROC)SwellDialogDefaultWindowProc:(WNDPROC)DefWindowProc;
     m_dlgproc=dlgproc;
     m_userdata=0;
     m_style=0;
     m_exstyle=0;
     m_id=wID;
     m_owned=m_owner=0;
     m_children=m_parent=m_next=m_prev=0; 
     if (wndr) m_position = *wndr;
     else memset(&m_position,0,sizeof(m_position));
     memset(&m_extra,0,sizeof(m_extra));
     m_visible=visible;
     m_hashaddestroy=false;
     m_enabled=true;
     m_wantfocus=true;
     m_menu=NULL;
#ifdef SWELL_TARGET_GDK
     m_oswindow = 0;
#endif

#ifdef SWELL_LICE_GDI
     m_paintctx=0;
     m_invalidated=true;
     m_child_invalidated=true;
     m_backingstore=0;
#endif

     if (label) m_title.Set(label);
     SetParent(this, par);

}

HWND__::~HWND__()
{
}



HWND GetParent(HWND hwnd)
{  
  return hwnd ? hwnd->m_parent : NULL;
}

HWND GetDlgItem(HWND hwnd, int idx)
{
  if (!idx) return hwnd;
  if (hwnd) hwnd=hwnd->m_children;
  while (hwnd && hwnd->m_id != idx) hwnd=hwnd->m_next;
  return hwnd;
}


LONG_PTR SetWindowLong(HWND hwnd, int idx, LONG_PTR val)
{
  if (!hwnd) return 0;
  if (idx==GWL_STYLE)
  {
     // todo: special case for buttons
    LONG ret = hwnd->m_style;
    hwnd->m_style=val;
    return ret;
  }
  if (idx==GWL_EXSTYLE)
  {
    LONG ret = hwnd->m_exstyle;
    hwnd->m_exstyle=val;
    return ret;
  }
  if (idx==GWL_USERDATA)
  {
    LONG_PTR ret = hwnd->m_userdata;
    hwnd->m_userdata=val;
    return ret;
  }
  if (idx==GWL_ID)
  {
    LONG ret = hwnd->m_id;
    hwnd->m_id=val;
    return ret;
  }
  
  if (idx==GWL_WNDPROC)
  {
    LONG_PTR ret = (LONG_PTR)hwnd->m_wndproc;
    hwnd->m_wndproc=(WNDPROC)val;
    return ret;
  }
  if (idx==DWL_DLGPROC)
  {
    LONG_PTR ret = (LONG_PTR)hwnd->m_dlgproc;
    hwnd->m_dlgproc=(DLGPROC)val;
    return ret;
  }
  
  if (idx>=0 && idx < 64*(int)sizeof(INT_PTR))
  {
    INT_PTR ret = hwnd->m_extra[idx/sizeof(INT_PTR)];
    hwnd->m_extra[idx/sizeof(INT_PTR)]=val;
    return (LONG_PTR)ret;
  }
  return 0;
}

LONG_PTR GetWindowLong(HWND hwnd, int idx)
{
  if (!hwnd) return 0;
  if (idx==GWL_STYLE)
  {
     // todo: special case for buttons
    return hwnd->m_style;
  }
  if (idx==GWL_EXSTYLE)
  {
    return hwnd->m_exstyle;
  }
  if (idx==GWL_USERDATA)
  {
    return hwnd->m_userdata;
  }
  if (idx==GWL_ID)
  {
    return hwnd->m_id;
  }
  
  if (idx==GWL_WNDPROC)
  {
    return (LONG_PTR)hwnd->m_wndproc;
  }
  if (idx==DWL_DLGPROC)
  {
    return (LONG_PTR)hwnd->m_dlgproc;
  }
  
  if (idx>=0 && idx < 64*(int)sizeof(INT_PTR))
  {
    return (LONG_PTR)hwnd->m_extra[idx/sizeof(INT_PTR)];
  }
  return 0;
}


bool IsWindow(HWND hwnd)
{
  // todo: verify window is valid (somehow)
  return !!hwnd;
}

bool IsWindowVisible(HWND hwnd)
{
  if (!hwnd) return false;
  while (hwnd->m_visible)
  {
    hwnd = hwnd->m_parent;
    if (!hwnd) return true;
  }
  return false;
}

LRESULT SendMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  if (!hwnd) return 0;
  WNDPROC wp = hwnd->m_wndproc;

  if (msg == WM_DESTROY)
  {
    if (hwnd->m_hashaddestroy) return 0;// todo: allow certain messages to pass?
    hwnd->m_hashaddestroy=true;
    if (GetCapture()==hwnd) ReleaseCapture(); 
    SWELL_MessageQueue_Clear(hwnd);
  }
  else if (msg==WM_CAPTURECHANGED && hwnd->m_hashaddestroy) return 0;
    
  int ret = wp ? wp(hwnd,msg,wParam,lParam) : 0;
 
  if (msg == WM_DESTROY)
  {
    if (GetCapture()==hwnd) ReleaseCapture(); 

    SWELL_MessageQueue_Clear(hwnd);
    // send WM_DESTROY to all children
    HWND tmp=hwnd->m_children;
    while (tmp)
    {
      SendMessage(tmp,WM_DESTROY,0,0);
      tmp=tmp->m_next;
    }
    tmp=hwnd->m_owned;
    while (tmp)
    {
      SendMessage(tmp,WM_DESTROY,0,0);
      tmp=tmp->m_next;
    }
    KillTimer(hwnd,-1);
    if (SWELL_g_focuswnd == hwnd) SWELL_g_focuswnd=0;
  }
  return ret;
}

static void swell_removeWindowFromNonChildren(HWND__ *hwnd)
{
  if (hwnd->m_next) hwnd->m_next->m_prev = hwnd->m_prev;
  if (hwnd->m_prev) hwnd->m_prev->m_next = hwnd->m_next;
  else
  {
    if (hwnd->m_parent && hwnd->m_parent->m_children == hwnd) hwnd->m_parent->m_children = hwnd->m_next;
    if (hwnd->m_owner && hwnd->m_owner->m_owned == hwnd) hwnd->m_owner->m_owned = hwnd->m_next;
    if (hwnd == SWELL_topwindows) SWELL_topwindows = hwnd->m_next;
    if (hwnd->m_parent && !hwnd->m_parent->m_hashaddestroy) InvalidateRect(hwnd->m_parent,NULL,FALSE);
  }
}

static void RecurseDestroyWindow(HWND hwnd)
{
  HWND tmp=hwnd->m_children;
  while (tmp)
  {
    HWND old = tmp;
    tmp=tmp->m_next;
    RecurseDestroyWindow(old);
  }
  tmp=hwnd->m_owned;
  while (tmp)
  {
    HWND old = tmp;
    tmp=tmp->m_next;
    RecurseDestroyWindow(old);
  }

  if (s_captured_window == hwnd) s_captured_window=NULL;
  if (SWELL_g_focuswnd == hwnd) SWELL_g_focuswnd=NULL;

  swell_destroyOSwindow(hwnd);

  if (hwnd->m_menu) DestroyMenu(hwnd->m_menu);
  hwnd->m_menu=0;

#ifdef SWELL_LICE_GDI
  delete hwnd->m_backingstore;
  hwnd->m_backingstore=0;
#endif

  hwnd->m_wndproc=NULL;
  hwnd->Release();
}


void DestroyWindow(HWND hwnd)
{
  if (!hwnd) return;
  if (hwnd->m_hashaddestroy) return; 
 
  // broadcast WM_DESTROY
  SendMessage(hwnd,WM_DESTROY,0,0);

  // remove from parent/global lists
  swell_removeWindowFromNonChildren(hwnd);

  // safe to delete this window and all children directly
  RecurseDestroyWindow(hwnd);
}


bool IsWindowEnabled(HWND hwnd)
{
  if (!hwnd) return false;
  while (hwnd && hwnd->m_enabled) 
  {
    hwnd=hwnd->m_parent;
  }
  return !hwnd;
}

void EnableWindow(HWND hwnd, int enable)
{
  if (!hwnd) return;
  if (!!hwnd->m_enabled == !!enable) return;

  hwnd->m_enabled=!!enable;
#ifdef SWELL_TARGET_GDK
  if (hwnd->m_oswindow) gdk_window_set_accept_focus(hwnd->m_oswindow,!!enable);
#endif

  if (!enable && SWELL_g_focuswnd == hwnd) SWELL_g_focuswnd = 0;
  InvalidateRect(hwnd,NULL,FALSE);
}


void SetFocus(HWND hwnd)
{
  if (!hwnd) return;

  SWELL_g_focuswnd = hwnd;
#ifdef SWELL_TARGET_GDK
  while (hwnd && !hwnd->m_oswindow) hwnd=hwnd->m_parent;
  if (hwnd) gdk_window_raise(hwnd->m_oswindow);
  if (hwnd && hwnd->m_oswindow != SWELL_g_focus_oswindow)
  {
    gdk_window_focus(SWELL_g_focus_oswindow = hwnd->m_oswindow,GDK_CURRENT_TIME);
  }
#endif
}
void SetForegroundWindow(HWND hwnd)
{
  SetFocus(hwnd); 
}


int IsChild(HWND hwndParent, HWND hwndChild)
{
  if (!hwndParent || !hwndChild || hwndParent == hwndChild) return 0;

  while (hwndChild && hwndChild != hwndParent) hwndChild = hwndChild->m_parent;

  return hwndChild == hwndParent;
}


HWND GetForegroundWindowIncludeMenus()
{
#ifdef SWELL_TARGET_GDK
  if (!SWELL_g_focus_oswindow) return 0;
  HWND a = SWELL_topwindows;
  while (a && a->m_oswindow != SWELL_g_focus_oswindow) a=a->m_next;
  return a;
#else
  HWND h = SWELL_g_focuswnd;
  while (h && h->m_parent) h=h->m_parent;
  return h;
#endif
}

HWND GetFocusIncludeMenus()
{
#ifdef SWELL_TARGET_GDK
  if (!SWELL_g_focus_oswindow) return 0;
  HWND a = SWELL_topwindows;
  while (a && a->m_oswindow != SWELL_g_focus_oswindow) a=a->m_next;
  return a && IsChild(a,SWELL_g_focuswnd) ? SWELL_g_focuswnd : a;
#else
  return SWELL_g_focuswnd;
#endif
}

HWND GetForegroundWindow()
{
  HWND h =GetForegroundWindowIncludeMenus();
  HWND ho;
  while (h && (ho=(HWND)GetProp(h,"SWELL_MenuOwner"))) h=ho; 
  return h;
}

HWND GetFocus()
{
  HWND h =GetFocusIncludeMenus();
  HWND ho;
  while (h && (ho=(HWND)GetProp(h,"SWELL_MenuOwner"))) h=ho; 
  return h;
}


void SWELL_GetViewPort(RECT *r, const RECT *sourcerect, bool wantWork)
{
#ifdef SWELL_TARGET_GDK
  if (swell_initwindowsys())
  {
    GdkScreen *defscr = gdk_screen_get_default();
    if (!defscr) { r->left=r->top=0; r->right=r->bottom=1024; return; }
    gint idx = sourcerect ? gdk_screen_get_monitor_at_point(defscr,
           (sourcerect->left+sourcerect->right)/2,
           (sourcerect->top+sourcerect->bottom)/2) : 0;
    GdkRectangle rc={0,0,1024,1024};
    gdk_screen_get_monitor_geometry(defscr,idx,&rc);
    r->left=rc.x; r->top = rc.y;
    r->right=rc.x+rc.width;
    r->bottom=rc.y+rc.height;
    return;
  }
#endif
  r->left=r->top=0;
  r->right=1024;
  r->bottom=768;
}


void ScreenToClient(HWND hwnd, POINT *p)
{
  if (!hwnd) return;
  
  int x=p->x,y=p->y;

  HWND tmp=hwnd;
  while (tmp 
#ifdef SWELL_TARGET_GDK
            && !tmp->m_oswindow
#endif
         ) // top level window's m_position left/top should always be 0 anyway
  {
    NCCALCSIZE_PARAMS p = {{ tmp->m_position, }, };
    if (tmp->m_wndproc) tmp->m_wndproc(tmp,WM_NCCALCSIZE,0,(LPARAM)&p);

    x -= p.rgrc[0].left;
    y -= p.rgrc[0].top;
    tmp = tmp->m_parent;
  }

  if (tmp)
  {
    NCCALCSIZE_PARAMS p = {{ tmp->m_position, }, };
    if (tmp->m_wndproc) tmp->m_wndproc(tmp,WM_NCCALCSIZE,0,(LPARAM)&p);
    x -= p.rgrc[0].left - tmp->m_position.left;
    y -= p.rgrc[0].top - tmp->m_position.top;
  }

#ifdef SWELL_TARGET_GDK
  if (tmp && tmp->m_oswindow)
  {
    GdkWindow *wnd = tmp->m_oswindow;
    gint px=0,py=0;
    gdk_window_get_origin(wnd,&px,&py); // this is probably unreliable but ugh (use get_geometry?)
    x-=px;
    y-=py;
  }
#endif

  p->x=x;
  p->y=y;
}

void ClientToScreen(HWND hwnd, POINT *p)
{
  if (!hwnd) return;
  
  int x=p->x,y=p->y;

  HWND tmp=hwnd;
  while (tmp 
#ifdef SWELL_TARGET_GDK
         && !tmp->m_oswindow
#endif
         ) // top level window's m_position left/top should always be 0 anyway
  {
    NCCALCSIZE_PARAMS p={{tmp->m_position, }, };
    if (tmp->m_wndproc) tmp->m_wndproc(tmp,WM_NCCALCSIZE,0,(LPARAM)&p);
    x += p.rgrc[0].left;
    y += p.rgrc[0].top;
    tmp = tmp->m_parent;
  }
  if (tmp) 
  {
    NCCALCSIZE_PARAMS p={{tmp->m_position, }, };
    if (tmp->m_wndproc) tmp->m_wndproc(tmp,WM_NCCALCSIZE,0,(LPARAM)&p);
    x += p.rgrc[0].left - tmp->m_position.left;
    y += p.rgrc[0].top - tmp->m_position.top;
  }

#ifdef SWELL_TARGET_GDK
  if (tmp && tmp->m_oswindow)
  {
    GdkWindow *wnd = tmp->m_oswindow;
    gint px=0,py=0;
    gdk_window_get_origin(wnd,&px,&py); // this is probably unreliable but ugh (use get_geometry?)
    x+=px;
    y+=py;
  }
#endif

  p->x=x;
  p->y=y;
}

bool GetWindowRect(HWND hwnd, RECT *r)
{
  if (!hwnd) return false;
#ifdef SWELL_TARGET_GDK
  if (hwnd->m_oswindow)
  {
    GdkRectangle rc;
    gdk_window_get_frame_extents(hwnd->m_oswindow,&rc);
    r->left=rc.x;
    r->top=rc.y;
    r->right=rc.x+rc.width;
    r->bottom = rc.y+rc.height;
    return true;
  }
#endif

  r->left=r->top=0; 
  ClientToScreen(hwnd,(LPPOINT)r);
  r->right = r->left + hwnd->m_position.right - hwnd->m_position.left;
  r->bottom = r->top + hwnd->m_position.bottom - hwnd->m_position.top;
  return true;
}

void GetWindowContentViewRect(HWND hwnd, RECT *r)
{
#ifdef SWELL_TARGET_GDK
  if (hwnd && hwnd->m_oswindow) 
  {
    gint w=0,h=0,px=0,py=0;
    gdk_window_get_position(hwnd->m_oswindow,&px,&py);
#if SWELL_TARGET_GDK==2
    gdk_drawable_get_size(hwnd->m_oswindow,&w,&h);
#else
    w = gdk_window_get_width(hwnd->m_oswindow);
    h = gdk_window_get_height(hwnd->m_oswindow);
#endif
    r->left=px;
    r->top=py;
    r->right = px+w;
    r->bottom = py+h;
    return;
  }
#endif
  GetWindowRect(hwnd,r);
}

void GetClientRect(HWND hwnd, RECT *r)
{
  r->left=r->top=r->right=r->bottom=0;
  if (!hwnd) return;
  
#ifdef SWELL_TARGET_GDK
  if (hwnd->m_oswindow)
  {
#if SWELL_TARGET_GDK==2
    gint w=0, h=0;
    gdk_drawable_get_size(hwnd->m_oswindow,&w,&h);
    r->right = w;
    r->bottom = h;
#else
    r->right = gdk_window_get_width(hwnd->m_oswindow);
    r->bottom = gdk_window_get_height(hwnd->m_oswindow);
#endif
    
  }
  else
#endif
  {
    r->right = hwnd->m_position.right - hwnd->m_position.left;
    r->bottom = hwnd->m_position.bottom - hwnd->m_position.top;
  }

  NCCALCSIZE_PARAMS tr={{*r, },};
  SendMessage(hwnd,WM_NCCALCSIZE,FALSE,(LPARAM)&tr);
  r->right = r->left + (tr.rgrc[0].right-tr.rgrc[0].left);
  r->bottom=r->top + (tr.rgrc[0].bottom-tr.rgrc[0].top);
}



void SetWindowPos(HWND hwnd, HWND zorder, int x, int y, int cx, int cy, int flags)
{
  if (!hwnd) return;
 // todo: handle SWP_SHOWWINDOW
  RECT f = hwnd->m_position;
  int reposflag = 0;
  if (!(flags&SWP_NOZORDER))
  {
    if (hwnd->m_parent && zorder != hwnd)
    {
      HWND par = hwnd->m_parent;
      HWND tmp = par->m_children;
      while (tmp && tmp != hwnd) tmp=tmp->m_next;
      if (tmp) // we are in the list, so we can do a reorder
      {
        // take hwnd out of list
        if (hwnd->m_prev) hwnd->m_prev->m_next = hwnd->m_next;
        else par->m_children = hwnd->m_next;
        if (hwnd->m_next) hwnd->m_next->m_prev = hwnd->m_prev;
        hwnd->m_next=hwnd->m_prev=NULL;// leave hwnd->m_parent valid since it wont change

        // add back in
        tmp = par->m_children;
        if (zorder == HWND_TOP || !tmp || tmp == zorder) // no children, topmost, or zorder is at top already
        {
          if (tmp) tmp->m_prev=hwnd;
          hwnd->m_next = tmp;
          par->m_children = hwnd;
        }
        else if (zorder == HWND_BOTTOM) 
        {
addToBottom:
          while (tmp && tmp->m_next) tmp=tmp->m_next;
          tmp->m_next=hwnd; 
          hwnd->m_prev=tmp;
        }
        else
        {
          while (tmp && tmp != zorder) tmp=tmp->m_next;
          if (!tmp) goto addToBottom;

          HWND next = zorder->m_next;
          hwnd->m_next = next;
          if (next) next->m_prev = hwnd;

          zorder->m_next = hwnd;
          hwnd->m_prev = zorder;
        }
        reposflag|=4;
      }
    }
  }
  if (!(flags&SWP_NOMOVE))
  {
    int oldw = f.right-f.left;
    int oldh = f.bottom-f.top; 
    f.left=x; 
    f.right=x+oldw;
    f.top=y; 
    f.bottom=y+oldh;
    reposflag|=1;
  }
  if (!(flags&SWP_NOSIZE))
  {
    f.right = f.left + cx;
    f.bottom = f.top + cy;
    reposflag|=2;
  }
  if (reposflag)
  {
#ifdef SWELL_TARGET_GDK
    if (hwnd->m_oswindow) 
    {
      //printf("repos %d,%d,%d,%d, %d\n",f.left,f.top,f.right,f.bottom,reposflag);
      if ((reposflag&3)==3) gdk_window_move_resize(hwnd->m_oswindow,f.left,f.top,f.right-f.left,f.bottom-f.top);
      else if (reposflag&2) gdk_window_resize(hwnd->m_oswindow,f.right-f.left,f.bottom-f.top);
      else if (reposflag&1) gdk_window_move(hwnd->m_oswindow,f.left,f.top);
    }
    else // top level windows above get their position from gdk and cache it in m_position
#endif
    {
      if (reposflag&3) 
      {
        hwnd->m_position = f;
        SendMessage(hwnd,WM_SIZE,0,0);
      }
      InvalidateRect(hwnd->m_parent ? hwnd->m_parent : hwnd,NULL,FALSE);
    }
  }  
  
}


BOOL EnumWindows(BOOL (*proc)(HWND, LPARAM), LPARAM lp)
{
    return FALSE;
}

HWND GetWindow(HWND hwnd, int what)
{
  if (!hwnd) return 0;
  
  if (what == GW_CHILD) return hwnd->m_children;
  if (what == GW_OWNER) return hwnd->m_owner;
  if (what == GW_HWNDNEXT) return hwnd->m_next;
  if (what == GW_HWNDPREV) return hwnd->m_prev;
  if (what == GW_HWNDFIRST) 
  { 
    while (hwnd->m_prev) hwnd = hwnd->m_prev;
    return hwnd;
  }
  if (what == GW_HWNDLAST) 
  { 
    while (hwnd->m_next) hwnd = hwnd->m_next;
    return hwnd;
  }
  return 0;
}

HWND SetParent(HWND hwnd, HWND newPar)
{
  if (!hwnd) return NULL;

  swell_removeWindowFromNonChildren(hwnd);

  HWND oldPar = hwnd->m_parent;
  hwnd->m_prev=0;
  hwnd->m_next=0;
  hwnd->m_parent = NULL;
  hwnd->m_owner = NULL; // todo

  if (newPar)
  {
    hwnd->m_parent = newPar;
    hwnd->m_next=newPar->m_children;
    if (hwnd->m_next) hwnd->m_next->m_prev = hwnd;
    newPar->m_children=hwnd;
  }
  else // add to top level windows
  {
    hwnd->m_next=SWELL_topwindows;
    if (hwnd->m_next) hwnd->m_next->m_prev = hwnd;
    SWELL_topwindows = hwnd;
  }

  swell_manageOSwindow(hwnd,false);
  return oldPar;
}




// timer stuff
typedef struct TimerInfoRec
{
  UINT_PTR timerid;
  HWND hwnd;
  UINT interval;
  DWORD nextFire;
  TIMERPROC tProc;
  struct TimerInfoRec *_next;
} TimerInfoRec;

static TimerInfoRec *m_timer_list;
static WDL_Mutex m_timermutex;
static pthread_t m_pmq_mainthread;

void SWELL_RunMessageLoop()
{
  SWELL_MessageQueue_Flush();
  swell_runOSevents();

  DWORD now = GetTickCount();
  WDL_MutexLock lock(&m_timermutex);
  TimerInfoRec *rec = m_timer_list;
  while (rec)
  {
    if (now > rec->nextFire || now < rec->nextFire - rec->interval*4)
    {
      rec->nextFire = now + rec->interval;

      HWND h = rec->hwnd;
      TIMERPROC tProc = rec->tProc;
      UINT_PTR tid = rec->timerid;
      m_timermutex.Leave();

      if (tProc) tProc(h,WM_TIMER,tid,now);
      else if (h) SendMessage(h,WM_TIMER,tid,0);

      m_timermutex.Enter();
      TimerInfoRec *tr = m_timer_list;
      while (tr && tr != rec) tr=tr->_next;
      if (!tr) 
      {
        rec = m_timer_list;  // if no longer in the list, then abort
        continue;
      }
    }
    rec=rec->_next;
  } 
}


UINT_PTR SetTimer(HWND hwnd, UINT_PTR timerid, UINT rate, TIMERPROC tProc)
{
  if (!hwnd && !tProc) return 0; // must have either callback or hwnd
  
  if (hwnd && !timerid) return 0;

  WDL_MutexLock lock(&m_timermutex);
  TimerInfoRec *rec=NULL;
  if (hwnd||timerid)
  {
    rec = m_timer_list;
    while (rec)
    {
      if (rec->timerid == timerid && rec->hwnd == hwnd) // works for both kinds
        break;
      rec=rec->_next;
    }
  }
  
  bool recAdd=false;
  if (!rec) 
  {
    rec=(TimerInfoRec*)malloc(sizeof(TimerInfoRec));
    recAdd=true;
  }
   
  rec->tProc = tProc;
  rec->timerid=timerid;
  rec->hwnd=hwnd;
  rec->interval = rate<1?1: rate;
  rec->nextFire = GetTickCount() + rate;
  
  if (!hwnd) timerid = rec->timerid = (UINT_PTR)rec;

  if (recAdd)
  {
    rec->_next=m_timer_list;
    m_timer_list=rec;
  }
  
  return timerid;
}

BOOL KillTimer(HWND hwnd, UINT_PTR timerid)
{
  if (!hwnd && !timerid) return FALSE;

  WDL_MutexLock lock(&m_timermutex);
  BOOL rv=FALSE;

  // don't allow removing all global timers
  if (timerid!=(UINT_PTR)-1 || hwnd) 
  {
    TimerInfoRec *rec = m_timer_list, *lrec=NULL;
    while (rec)
    {
      if (rec->hwnd == hwnd && (timerid==(UINT_PTR)-1 || rec->timerid == timerid))
      {
        TimerInfoRec *nrec = rec->_next;
        
        // remove self from list
        if (lrec) lrec->_next = nrec;
        else m_timer_list = nrec;
        
        free(rec);

        rv=TRUE;
        if (timerid!=(UINT_PTR)-1) break;
        
        rec=nrec;
      }
      else 
      {
        lrec=rec;
        rec=rec->_next;
      }
    }
  }
  return rv;
}

BOOL SetDlgItemText(HWND hwnd, int idx, const char *text)
{
  hwnd =(idx ? GetDlgItem(hwnd,idx) : hwnd);
  if (!hwnd) return false;

  if (!text) text="";
 

  if (strcmp(hwnd->m_title.Get(), text))
  {
    hwnd->m_title.Set(text);
    SendMessage(hwnd,WM_SETTEXT,0,(LPARAM)text);
    swell_setOSwindowtext(hwnd);
  } 
  return true;
}

BOOL GetDlgItemText(HWND hwnd, int idx, char *text, int textlen)
{
  *text=0;
  hwnd = idx?GetDlgItem(hwnd,idx) : hwnd;
  if (!hwnd) return false;
  
  // todo: sendmessage WM_GETTEXT etc? special casing for combo boxes etc
  lstrcpyn_safe(text,hwnd->m_title.Get(), textlen);
  return true;
}

void CheckDlgButton(HWND hwnd, int idx, int check)
{
  hwnd = GetDlgItem(hwnd,idx);
  if (!hwnd) return;
  SendMessage(hwnd,BM_SETCHECK,check,0);
}


int IsDlgButtonChecked(HWND hwnd, int idx)
{
  hwnd = GetDlgItem(hwnd,idx);
  if (!hwnd) return 0;
  return SendMessage(hwnd,BM_GETCHECK,0,0);
}


BOOL SetDlgItemInt(HWND hwnd, int idx, int val, int issigned)
{
  char buf[128];
  sprintf(buf,issigned?"%d":"%u",val);
  return SetDlgItemText(hwnd,idx,buf);
}

int GetDlgItemInt(HWND hwnd, int idx, BOOL *translated, int issigned)
{
  char buf[128];
  if (!GetDlgItemText(hwnd,idx,buf,sizeof(buf)))
  {
    if (translated) *translated=0;
    return 0;
  }
  char *p=buf;
  while (*p == ' ' || *p == '\t') p++;
  int a=atoi(p);
  if ((a<0 && !issigned) || (!a && p[0] != '0')) { if (translated) *translated=0; return 0; }
  if (translated) *translated=1;
  return a;
}

void ShowWindow(HWND hwnd, int cmd)
{
  if (!hwnd) return;
 
  if (cmd==SW_SHOW||cmd==SW_SHOWNA) 
  {
    hwnd->m_visible=true;
  }
  else if (cmd==SW_HIDE) hwnd->m_visible=false;

  swell_manageOSwindow(hwnd,cmd==SW_SHOW);
  if (cmd == SW_SHOW) SWELL_g_focuswnd = hwnd;

  InvalidateRect(hwnd,NULL,FALSE);

}

void *SWELL_ModalWindowStart(HWND hwnd)
{
  return 0;
}

bool SWELL_ModalWindowRun(void *ctx, int *ret) // returns false and puts retval in *ret when done
{
  return false;
}

void SWELL_ModalWindowEnd(void *ctx)
{
  if (ctx) 
  {
  }
}

void SWELL_CloseWindow(HWND hwnd)
{
  DestroyWindow(hwnd);
}


#include "swell-dlggen.h"

static HWND m_make_owner;
static RECT m_transform;
static bool m_doautoright;
static RECT m_lastdoauto;
static bool m_sizetofits;

#define ACTIONTARGET (m_make_owner)

void SWELL_MakeSetCurParms(float xscale, float yscale, float xtrans, float ytrans, HWND parent, bool doauto, bool dosizetofit)
{
  m_sizetofits=dosizetofit;
  m_lastdoauto.left = 0;
  m_lastdoauto.top = -100<<16;
  m_lastdoauto.right = 0;
  m_doautoright=doauto;
  m_transform.left=(int)(xtrans*65536.0);
  m_transform.top=(int)(ytrans*65536.0);
  m_transform.right=(int)(xscale*65536.0);
  m_transform.bottom=(int)(yscale*65536.0);
  m_make_owner=parent;
}

static void UpdateAutoCoords(RECT r)
{
  m_lastdoauto.right=r.left + r.right - m_lastdoauto.left;
}


static RECT MakeCoords(int x, int y, int w, int h, bool wantauto)
{
  if (w<0&&h<0)
  {
    RECT r = { -x, -y, -x-w, -y-h};
    return r;
  }

  float ysc=m_transform.bottom/65536.0;
  int newx=(int)((x+m_transform.left/65536.0)*m_transform.right/65536.0 + 0.5);
  int newy=(int)(((((double)y+(double)m_transform.top/65536.0) )*ysc) + 0.5);
                         
  RECT  ret= { newx,  
                         newy,                  
                        (int) (newx + w*(double)m_transform.right/65536.0+0.5),
                        (int) (newy + h*fabs(ysc)+0.5)
             };
                        

  RECT oret=ret;
  if (wantauto && m_doautoright)
  {
    float dx = ret.left - m_lastdoauto.left;
    if (fabs(dx)<32 && m_lastdoauto.top >= ret.top && m_lastdoauto.top < ret.bottom)
    {
      ret.left += (int) m_lastdoauto.right;
    }
    
    m_lastdoauto.left = oret.right;
    m_lastdoauto.top = (ret.top + ret.bottom)*0.5;
    m_lastdoauto.right=0;
  }
  return ret;
}

static const double minwidfontadjust=1.81;
#define TRANSFORMFONTSIZE ((m_transform.right/65536.0+1.0)*3.7)


#ifdef SWELL_LICE_GDI
//#define SWELL_ENABLE_VIRTWND_CONTROLS
#include "../wingui/virtwnd-controls.h"
#endif

static LRESULT WINAPI virtwndWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
#ifdef SWELL_ENABLE_VIRTWND_CONTROLS
  WDL_VWnd *vwnd = (WDL_VWnd *) ( msg == WM_CREATE ? (void*)lParam : GetProp(hwnd,"WDL_control_vwnd") );
  if (vwnd) switch (msg)
  {
    case WM_CREATE:
      {
        SetProp(hwnd,"WDL_control_vwnd",vwnd);
        RECT r;
        GetClientRect(hwnd,&r);
        vwnd->SetRealParent(hwnd);
        vwnd->SetPosition(&r);
        vwnd->SetID(0xf);
      }
    return 0;
    case WM_SIZE:
      {
        RECT r;
        GetClientRect(hwnd,&r);
        vwnd->SetPosition(&r);
        InvalidateRect(hwnd,NULL,FALSE);
      }
    break;
    case WM_COMMAND:
      if (LOWORD(wParam)==0xf) SendMessage(GetParent(hwnd),WM_COMMAND,(wParam&0xffff0000) | GetWindowLong(hwnd,GWL_ID),NULL);
    break;
    case WM_DESTROY:
      RemoveProp(hwnd,"WDL_control_vwnd");
      delete vwnd;
      vwnd=0;
    return 0;
    case WM_LBUTTONDOWN:
      SetCapture(hwnd);
      vwnd->OnMouseDown(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));
    return 0;
    case WM_MOUSEMOVE:
      vwnd->OnMouseMove(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));
    return 0;
    case WM_LBUTTONUP:
      ReleaseCapture(); 
      vwnd->OnMouseUp(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));
    return 0;
    case WM_PAINT:
      { 
        PAINTSTRUCT ps;
        if (BeginPaint(hwnd,&ps))
        {
          RECT r; 
          GetClientRect(hwnd,&r); 

          HDC hdc = ps.hdc;
          if (hdc)
          {
            RECT tr = ps.rcPaint; // todo: offset by surface_offs.x/y
            vwnd->OnPaint(hdc->surface,hdc->surface_offs.x,hdc->surface_offs.y,&tr);
            vwnd->OnPaintOver(hdc->surface,hdc->surface_offs.x,hdc->surface_offs.y,&tr);
          }

          EndPaint(hwnd,&ps);
        }
      }
    return 0;
    case WM_SETTEXT:
      if (lParam)
      {
        if (!strcmp(vwnd->GetType(),"vwnd_iconbutton")) 
        {
          WDL_VirtualIconButton *b = (WDL_VirtualIconButton *) vwnd;
          b->SetTextLabel((const char *)lParam);
        }
      }
    break;
    case BM_SETCHECK:
    case BM_GETCHECK:
      if (!strcmp(vwnd->GetType(),"vwnd_iconbutton")) 
      {
        WDL_VirtualIconButton *b = (WDL_VirtualIconButton *) vwnd;
        if (msg == BM_GETCHECK) return b->GetCheckState();

        b->SetCheckState(wParam);
      }
    return 0;
  }
#endif
  return DefWindowProc(hwnd,msg,wParam,lParam);
}

#ifdef SWELL_ENABLE_VIRTWND_CONTROLS
static HWND swell_makeButton(HWND owner, int idx, RECT *tr, const char *label, bool vis, int style)
{
  WDL_VirtualIconButton *vwnd = new WDL_VirtualIconButton;
  if (label) vwnd->SetTextLabel(label);
  vwnd->SetForceBorder(true);
  if (style & BS_AUTOCHECKBOX) vwnd->SetCheckState(0);
  HWND hwnd = new HWND__(owner,idx,tr,label,vis,virtwndWindowProc);
  hwnd->m_classname = "Button";
  hwnd->m_style = style|WS_CHILD;
  hwnd->m_wndproc(hwnd,WM_CREATE,0,(LPARAM)vwnd);
  return hwnd;
}

#endif


#ifndef SWELL_ENABLE_VIRTWND_CONTROLS
static LRESULT WINAPI buttonWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {
    case WM_LBUTTONDOWN:
      SetCapture(hwnd);
      SendMessage(hwnd,WM_USER+100,0,0); // invalidate
    return 0;
    case WM_MOUSEMOVE:
    return 0;
    case WM_LBUTTONUP:
      if (GetCapture()==hwnd)
      {
        ReleaseCapture(); // WM_CAPTURECHANGED will take care of the invalidate
        RECT r;
        GetClientRect(hwnd,&r);
        POINT p={GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam)};
        if (PtInRect(&r,p) && hwnd->m_id && hwnd->m_parent) 
        {
          int sf = (hwnd->m_style & 0xf);
          if (sf == BS_AUTO3STATE)
          {
            int a = hwnd->m_private_data&3;
            if (a==0) a=1;
            else if (a==1) a=2;
            else a=0;
            hwnd->m_private_data = (a) | (hwnd->m_private_data&~3);
          }    
          else if (sf == BS_AUTOCHECKBOX)
          {
            hwnd->m_private_data = (!(hwnd->m_private_data&3)) | (hwnd->m_private_data&~3);
          }
          else if (sf == BS_AUTORADIOBUTTON)
          {
            // todo: uncheck other nearby radios 
          }
          SendMessage(hwnd->m_parent,WM_COMMAND,MAKEWPARAM(hwnd->m_id,BN_CLICKED),(LPARAM)hwnd);
        }
      }
    return 0;
    case WM_PAINT:
      { 
        PAINTSTRUCT ps;
        if (BeginPaint(hwnd,&ps))
        {
          RECT r; 
          GetClientRect(hwnd,&r); 
          bool pressed = GetCapture()==hwnd;

          SetTextColor(ps.hdc,hwnd->m_enabled ? GetSysColor(COLOR_BTNTEXT): RGB(128,128,128));
          SetBkMode(ps.hdc,TRANSPARENT);

          int f=DT_VCENTER;
          int sf = (hwnd->m_style & 0xf);
          if (sf == BS_AUTO3STATE || sf == BS_AUTOCHECKBOX || sf == BS_AUTORADIOBUTTON)
          {
            const int chksz = 12;
            RECT tr={r.left,(r.top+r.bottom)/2-chksz/2,r.left+chksz};
            tr.bottom = tr.top+chksz;

            HPEN pen=CreatePen(PS_SOLID,0,RGB(0,0,0));
            HGDIOBJ oldPen = SelectObject(ps.hdc,pen);
            if (sf == BS_AUTOCHECKBOX || sf == BS_AUTO3STATE)
            {
              int st = (int)(hwnd->m_private_data&3);
              if (st==3||(st==2 && (hwnd->m_style & 0xf) == BS_AUTOCHECKBOX)) st=1;
              
              HBRUSH br = CreateSolidBrush(st==2?RGB(192,192,192):RGB(255,255,255));
              FillRect(ps.hdc,&tr,br);
              DeleteObject(br);

              if (st == 1||pressed)
              {
                RECT ar=tr;
                ar.left+=2;
                ar.right-=3;
                ar.top+=2;
                ar.bottom-=3;
                if (pressed) 
                { 
                  const int rsz=chksz/4;
                  ar.left+=rsz;
                  ar.top+=rsz;
                  ar.right-=rsz;
                  ar.bottom-=rsz;
                }
                MoveToEx(ps.hdc,ar.left,ar.top,NULL);
                LineTo(ps.hdc,ar.right,ar.bottom);
                MoveToEx(ps.hdc,ar.right,ar.top,NULL);
                LineTo(ps.hdc,ar.left,ar.bottom);
              }
            }
            else if (sf == BS_AUTORADIOBUTTON)
            {
              // todo radio circle
            }
            SelectObject(ps.hdc,oldPen);
            DeleteObject(pen);
            r.left += chksz + 4;
          }
          else
          {

            HBRUSH br = CreateSolidBrush(GetSysColor(COLOR_3DFACE));
            FillRect(ps.hdc,&r,br);
            DeleteObject(br);

            HPEN pen2 = CreatePen(PS_SOLID,0,GetSysColor(pressed?COLOR_3DHILIGHT : COLOR_3DSHADOW));
            HPEN pen = CreatePen(PS_SOLID,0,GetSysColor((!pressed)?COLOR_3DHILIGHT : COLOR_3DSHADOW));
            HGDIOBJ oldpen = SelectObject(ps.hdc,pen);
            MoveToEx(ps.hdc,r.left,r.bottom-1,NULL);
            LineTo(ps.hdc,r.left,r.top);
            LineTo(ps.hdc,r.right-1,r.top);
            SelectObject(ps.hdc,pen2);
            LineTo(ps.hdc,r.right-1,r.bottom-1);
            LineTo(ps.hdc,r.left,r.bottom-1);
            SelectObject(ps.hdc,oldpen);
            DeleteObject(pen);
            DeleteObject(pen2);
            f|=DT_CENTER;
            if (pressed) 
            {
              r.left+=2;
              r.top+=2;
            }
          }


          char buf[512];
          buf[0]=0;
          GetWindowText(hwnd,buf,sizeof(buf));
          if (buf[0]) DrawText(ps.hdc,buf,-1,&r,f);


          EndPaint(hwnd,&ps);
        }
      }
    return 0;
    case BM_GETCHECK:
      if (hwnd)
      {
        return (hwnd->m_private_data&3)==2 ? 1 : (hwnd->m_private_data&3); 
      }
    return 0;
    case BM_SETCHECK:
      if (hwnd)
      {
        int check = (int)wParam;
        INT_PTR op = hwnd->m_private_data;
        hwnd->m_private_data=(check > 2 || check<0 ? 1 : (check&3)) | (hwnd->m_private_data&~3);
        if (hwnd->m_private_data == op) break; 
      }
      else
      {
        break;
      }
      // fall through (invalidating)
    case WM_USER+100:
    case WM_CAPTURECHANGED:
    case WM_SETTEXT:
      {
        int sf = (hwnd->m_style & 0xf);
        if (sf == BS_AUTO3STATE || sf == BS_AUTOCHECKBOX || sf == BS_AUTORADIOBUTTON)
        {
          InvalidateRect(hwnd,NULL,TRUE);
        }
        else InvalidateRect(hwnd,NULL,FALSE);
      }
    break;
  }
  return DefWindowProc(hwnd,msg,wParam,lParam);
}

static HWND swell_makeButton(HWND owner, int idx, RECT *tr, const char *label, bool vis, int style)
{
  HWND hwnd = new HWND__(owner,idx,tr,label,vis,buttonWindowProc);
  hwnd->m_classname = "Button";
  hwnd->m_style = style|WS_CHILD;
  hwnd->m_wndproc(hwnd,WM_CREATE,0,0);
  return hwnd;
}
#endif

static LRESULT WINAPI groupWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {
    case WM_PAINT:
      { 
        PAINTSTRUCT ps;
        if (BeginPaint(hwnd,&ps))
        {
          RECT r; 
          GetClientRect(hwnd,&r); 
          int col = GetSysColor(COLOR_BTNTEXT);

          const char *buf = hwnd->m_title.Get();
          int th=20;
          int tw=0;
          int xp=0;
          if (buf && buf[0]) 
          {
            RECT tr={0,};
            DrawText(ps.hdc,buf,-1,&tr,DT_CALCRECT);
            th=tr.bottom-tr.top;
            tw=tr.right-tr.left;
          }
          if (hwnd->m_style & SS_CENTER)
          {
            xp = r.right/2 - tw/2;
          }
          else if (hwnd->m_style & SS_RIGHT)
          {
            xp = r.right - tw;
          }
          if (xp<8)xp=8;
          if (xp+tw > r.right-8) tw=r.right-8-xp;

          HPEN pen = CreatePen(PS_SOLID,0,GetSysColor(COLOR_3DHILIGHT));
          HPEN pen2 = CreatePen(PS_SOLID,0,GetSysColor(COLOR_3DSHADOW));
          HGDIOBJ oldPen=SelectObject(ps.hdc,pen);

          MoveToEx(ps.hdc,xp - (tw?4:0) + 1,th/2+1,NULL);
          LineTo(ps.hdc,1,th/2+1);
          LineTo(ps.hdc,1,r.bottom-1);
          LineTo(ps.hdc,r.right-1,r.bottom-1);
          LineTo(ps.hdc,r.right-1,th/2+1);
          LineTo(ps.hdc,xp+tw + (tw?4:0),th/2+1);

          SelectObject(ps.hdc,pen2);

          MoveToEx(ps.hdc,xp - (tw?4:0),th/2,NULL);
          LineTo(ps.hdc,0,th/2);
          LineTo(ps.hdc,0,r.bottom-2);
          LineTo(ps.hdc,r.right-2,r.bottom-2);
          LineTo(ps.hdc,r.right-2,th/2);
          LineTo(ps.hdc,xp+tw + (tw?4:0),th/2);


          SelectObject(ps.hdc,oldPen);
          DeleteObject(pen);
          DeleteObject(pen2);

          SetTextColor(ps.hdc,col);
          SetBkMode(ps.hdc,TRANSPARENT);
          r.left = xp;
          r.right = xp+tw;
          r.bottom = th;
          if (buf && buf[0]) DrawText(ps.hdc,buf,-1,&r,DT_LEFT|DT_TOP);
          EndPaint(hwnd,&ps);
        }
      }
    return 0;
    case WM_SETTEXT:
      InvalidateRect(hwnd,NULL,TRUE);
    break;
  }
  return DefWindowProc(hwnd,msg,wParam,lParam);
}

static LRESULT OnEditKeyDown(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, bool allowEnter)
{
  if (lParam & (FCONTROL|FALT)) return 0;

  if (wParam < 32)
  {
    if (wParam == VK_BACK)
    { 
      if (hwnd->m_title.GetLength())
        hwnd->m_title.SetLen(hwnd->m_title.GetLength()-1); // todo: UTF-8
      else
        return 0;
    }
    else if (wParam == VK_RETURN)
    {
      if (allowEnter) 
      { 
        hwnd->m_title.Append("\n");
        return 1;
      }

      return 0;
    }
  }
  else if (wParam >= 128)
  {
    // todo: UTF-8, other filtering
    return 0;
  }
  else if (!(lParam & FVIRTKEY))
  {
    char b[3]={(char)wParam,};
     
    hwnd->m_title.Append(b);
  }
  return 1;
}

static LRESULT WINAPI editWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {
    case WM_RBUTTONDOWN:
    case WM_LBUTTONDOWN:
      SetFocus(hwnd);
      InvalidateRect(hwnd,NULL,FALSE);
    return 0;
    case WM_KEYDOWN:
      if (OnEditKeyDown(hwnd,msg,wParam,lParam, !!(hwnd->m_style&ES_WANTRETURN)))
      {
        SendMessage(GetParent(hwnd),WM_COMMAND,(EN_CHANGE<<16) | (hwnd->m_id&0xffff),(LPARAM)hwnd);
        InvalidateRect(hwnd,NULL,FALSE);
      }
    return 0;
    case WM_KEYUP:
    return 0;
    case WM_PAINT:
      { 
        PAINTSTRUCT ps;
        if (BeginPaint(hwnd,&ps))
        {
          RECT r; 
          GetClientRect(hwnd,&r); 
          HBRUSH br = CreateSolidBrush(RGB(255,255,255)); // todo edit colors
          FillRect(ps.hdc,&r,br);
          DeleteObject(br);
          SetTextColor(ps.hdc,RGB(0,0,0)); // todo edit colors
          SetBkMode(ps.hdc,TRANSPARENT);
          const char *buf = hwnd->m_title.Get();
          r.left+=2; r.right-=2;
          if (buf && buf[0]) DrawText(ps.hdc,buf,-1,&r,(hwnd->m_style & ES_MULTILINE) ? (DT_TOP) : (DT_VCENTER));
          // todo: cursor drawing
          EndPaint(hwnd,&ps);
        }
      }
    return 0;
    case WM_SETTEXT:
      InvalidateRect(hwnd,NULL,FALSE);
    break;
  }
  return DefWindowProc(hwnd,msg,wParam,lParam);
}

static LRESULT WINAPI labelWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {
    case WM_PAINT:
      { 
        PAINTSTRUCT ps;
        if (BeginPaint(hwnd,&ps))
        {
          RECT r; 
          GetClientRect(hwnd,&r); 

          SetBkColor(ps.hdc,GetSysColor(COLOR_WINDOW));
          HBRUSH hbrush = (HBRUSH) SendMessage(GetParent(hwnd),WM_CTLCOLORSTATIC,(WPARAM)ps.hdc,(LPARAM)hwnd);
          if (hbrush == (HBRUSH)(INT_PTR)1) hbrush = NULL;
          SetTextColor(ps.hdc,GetSysColor(COLOR_BTNTEXT));
          SetBkMode(ps.hdc,hbrush ? TRANSPARENT : OPAQUE);
          if (hbrush) FillRect(ps.hdc,&r,hbrush);
          const char *buf = hwnd->m_title.Get();
          if (buf && buf[0]) DrawText(ps.hdc,buf,-1,&r,((hwnd->m_style & SS_CENTER) ? DT_CENTER:0)|DT_VCENTER);
          EndPaint(hwnd,&ps);
        }
      }
    return 0;
    case WM_SETTEXT:
       InvalidateRect(hwnd,NULL,TRUE);
    break;
    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
       if (hwnd->m_style & SS_NOTIFY)
         SendMessage(GetParent(hwnd),WM_COMMAND,
              ((msg==WM_LBUTTONDOWN?STN_CLICKED:STN_DBLCLK)<<16)|(hwnd->m_id&0xffff),0);
    return 1;
  }
  return DefWindowProc(hwnd,msg,wParam,lParam);
}

struct __SWELL_ComboBoxInternalState_rec 
{ 
  __SWELL_ComboBoxInternalState_rec(const char *_desc=NULL, LPARAM _parm=0) { desc=_desc?strdup(_desc):NULL; parm=_parm; } 
  ~__SWELL_ComboBoxInternalState_rec() { free(desc); } 
  char *desc; 
  LPARAM parm; 
  static int cmp(const __SWELL_ComboBoxInternalState_rec **a, const __SWELL_ComboBoxInternalState_rec **b) { return strcmp((*a)->desc, (*b)->desc); }
};

class __SWELL_ComboBoxInternalState
{
  public:
    __SWELL_ComboBoxInternalState() { selidx=-1; }
    ~__SWELL_ComboBoxInternalState() { }

    int selidx;
    WDL_PtrList_DeleteOnDestroy<__SWELL_ComboBoxInternalState_rec> items;
};

static LRESULT WINAPI comboWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  static const int buttonwid = 16; // used in edit combobox
  __SWELL_ComboBoxInternalState *s = (__SWELL_ComboBoxInternalState*)hwnd->m_private_data;
  if (msg >= CB_ADDSTRING && msg <= CB_INITSTORAGE)
  {
    if (s)
    {
      switch (msg)
      {
        case CB_ADDSTRING:
          
          if (!(hwnd->m_style & CBS_SORT))
          {
            s->items.Add(new __SWELL_ComboBoxInternalState_rec((const char *)lParam));
            return s->items.GetSize() - 1;
          }
          else
          {
            __SWELL_ComboBoxInternalState_rec *r=new __SWELL_ComboBoxInternalState_rec((const char *)lParam);
            // find position of insert for wParam
            bool m;
            int idx = s->items.LowerBound(r,&m,__SWELL_ComboBoxInternalState_rec::cmp);
            s->items.Insert(idx,r);
            return idx;
          }

        case CB_INSERTSTRING:
          if ((int)wParam == -1)
          {
            s->items.Add(new __SWELL_ComboBoxInternalState_rec((const char *)lParam));
            return s->items.GetSize() - 1;
          }
          else
          {
            if (wParam > (WPARAM)s->items.GetSize()) wParam=(WPARAM)s->items.GetSize();
            s->items.Insert(wParam,new __SWELL_ComboBoxInternalState_rec((const char *)lParam));
            return wParam;
          }
        return 0;

        case CB_DELETESTRING:
          if (wParam >= (WPARAM)s->items.GetSize()) return CB_ERR;
          s->items.Delete(wParam,true);
        return s->items.GetSize();

        case CB_GETCOUNT: return s->items.GetSize();
        case CB_GETCURSEL: return s->selidx >=0 && s->selidx < s->items.GetSize() ? s->selidx : -1;

        case CB_GETLBTEXTLEN: 
        case CB_GETLBTEXT: 
          if (wParam < (WPARAM)s->items.GetSize()) 
          {
            __SWELL_ComboBoxInternalState_rec *rec = s->items.Get(wParam);
            if (!rec) return CB_ERR;
            const char *ptr=rec->desc;
            int l = ptr ? strlen(ptr) : 0;
            if (msg == CB_GETLBTEXT && lParam) memcpy((char *)lParam,ptr?ptr:"",l+1);
            return l;
          }
        return CB_ERR;
        case CB_RESETCONTENT:
          s->selidx=-1;
          s->items.Empty(true);
        return 0;
        case CB_SETCURSEL:
          if (wParam >= (WPARAM)s->items.GetSize())
          {
            if (s->selidx!=-1)
            {
              s->selidx = -1;
              SetWindowText(hwnd,"");
              InvalidateRect(hwnd,NULL,FALSE);
            }
          }
          else
          {
            if (s->selidx != (int)wParam)
            {
              s->selidx=(int)wParam;
              char *ptr=s->items.Get(wParam)->desc;
              SetWindowText(hwnd,ptr);
              InvalidateRect(hwnd,NULL,FALSE);
            }
          }
        case CB_GETITEMDATA:
          if (wParam < (WPARAM)s->items.GetSize()) 
          {
            return s->items.Get(wParam)->parm;
          }
        return CB_ERR;
        case CB_SETITEMDATA:
          if (wParam < (WPARAM)s->items.GetSize()) 
          {
            s->items.Get(wParam)->parm=lParam;
            return 0;
          }
        return CB_ERR;
        case CB_INITSTORAGE:
        return 0;

        case CB_FINDSTRINGEXACT:
        case CB_FINDSTRING:
          {
            int x;
            int a = (int)wParam;
            a++;
            for (x=a;x<s->items.GetSize();x++) 
              if (msg == CB_FINDSTRING ? 
                  !stricmp(s->items.Get(x)->desc,(char *)lParam)  :
                  !strcmp(s->items.Get(x)->desc,(char *)lParam)) return x;

            for (x=0;x<a;x++)
              if (msg == CB_FINDSTRING ? 
                  !stricmp(s->items.Get(x)->desc,(char *)lParam)  :
                  !strcmp(s->items.Get(x)->desc,(char *)lParam)) return x;
          }
        return CB_ERR;
      }
    }
  }

  switch (msg)
  {
    case WM_DESTROY:
      {
       hwnd->m_private_data=0;
       delete s;
      }
    break;

    case WM_LBUTTONDOWN:
      {
        RECT r;
        GetClientRect(hwnd,&r);
        if ((hwnd->m_style & CBS_DROPDOWNLIST) == CBS_DROPDOWNLIST || GET_X_LPARAM(lParam) >= r.right-buttonwid)
          SetCapture(hwnd);
        else
          SetFocus(hwnd);
      }
      InvalidateRect(hwnd,NULL,FALSE);
    return 0;
    case WM_MOUSEMOVE:
    return 0;
    case WM_LBUTTONUP:
      if (GetCapture()==hwnd)
      {
        ReleaseCapture(); 
        if (s && s->items.GetSize())
        {
          int x;
          HMENU menu = CreatePopupMenu();
          for (x=0;x<s->items.GetSize();x++)
          {
            MENUITEMINFO mi={sizeof(mi),MIIM_ID|MIIM_STATE|MIIM_TYPE,MFT_STRING,
              x == s->selidx?MFS_CHECKED:0,100+x,NULL,NULL,NULL,0,s->items.Get(x)->desc};
            InsertMenuItem(menu,x,TRUE,&mi);
          }
          RECT r;
          GetWindowRect(hwnd,&r);
          int a = TrackPopupMenu(menu,TPM_NONOTIFY|TPM_RETURNCMD|TPM_LEFTALIGN,r.left,r.bottom,0,hwnd,0);
          DestroyMenu(menu);
          if (a>=100 && a < s->items.GetSize()+100)
          {
            s->selidx = a-100;
            char *ptr=s->items.Get(s->selidx)->desc;
            SetWindowText(hwnd,ptr);
            InvalidateRect(hwnd,NULL,FALSE);
            SendMessage(GetParent(hwnd),WM_COMMAND,(GetWindowLong(hwnd,GWL_ID)&0xffff) | (CBN_SELCHANGE<<16),(LPARAM)hwnd);
          }
        }
      }
    return 0;
    case WM_KEYDOWN:
      if ((hwnd->m_style & CBS_DROPDOWNLIST) != CBS_DROPDOWNLIST && OnEditKeyDown(hwnd,msg,wParam,lParam,false))
      {
        if (s) s->selidx=-1; // lookup from text?
        SendMessage(GetParent(hwnd),WM_COMMAND,(CBN_EDITCHANGE<<16) | (hwnd->m_id&0xffff),(LPARAM)hwnd);
        InvalidateRect(hwnd,NULL,FALSE);
      }
    return 0;
    case WM_KEYUP:
    return 0;
    case WM_PAINT:
      { 
        PAINTSTRUCT ps;
        if (BeginPaint(hwnd,&ps))
        {
          RECT r; 
          GetClientRect(hwnd,&r); 
          bool pressed = GetCapture()==hwnd;

          SetTextColor(ps.hdc,hwnd->m_enabled ? GetSysColor(COLOR_BTNTEXT): RGB(128,128,128));
          SetBkMode(ps.hdc,TRANSPARENT);

          int f=DT_VCENTER;
          {
            HBRUSH br = CreateSolidBrush(GetSysColor(COLOR_3DFACE));
            FillRect(ps.hdc,&r,br);
            DeleteObject(br);

            HPEN pen2 = CreatePen(PS_SOLID,0,GetSysColor(pressed?COLOR_3DHILIGHT : COLOR_3DSHADOW));
            HPEN pen = CreatePen(PS_SOLID,0,GetSysColor((!pressed)?COLOR_3DHILIGHT : COLOR_3DSHADOW));
            HGDIOBJ oldpen = SelectObject(ps.hdc,pen);
            MoveToEx(ps.hdc,r.left,r.bottom-1,NULL);
            LineTo(ps.hdc,r.left,r.top);
            LineTo(ps.hdc,r.right-1,r.top);
            SelectObject(ps.hdc,pen2);
            LineTo(ps.hdc,r.right-1,r.bottom-1);
            LineTo(ps.hdc,r.left,r.bottom-1);

            if ((hwnd->m_style & CBS_DROPDOWNLIST) != CBS_DROPDOWNLIST)
            { 
              HBRUSH br = CreateSolidBrush(RGB(255,255,255));
              RECT tr=r; 
              tr.left+=2; tr.top+=2; tr.bottom-=2; tr.right -= buttonwid+2;
              FillRect(ps.hdc,&tr,br);
              DeleteObject(br);
            }

            const int dw = 8;
            const int dh = 4;
            const int cx = r.right-dw/2-4;
            const int cy = (r.bottom+r.top)/2;
            MoveToEx(ps.hdc,cx-dw/2,cy-dh/2,NULL);
            LineTo(ps.hdc,cx,cy+dh/2);
            LineTo(ps.hdc,cx+dw/2,cy-dh/2);


            SelectObject(ps.hdc,oldpen);
            DeleteObject(pen);
            DeleteObject(pen2);

           
            if (pressed) 
            {
              r.left+=2;
              r.top+=2;
            }
          }

          char buf[512];
          buf[0]=0;
          GetWindowText(hwnd,buf,sizeof(buf));
          r.left+=3;
          r.right-=3;
          if (buf[0]) DrawText(ps.hdc,buf,-1,&r,f);

          EndPaint(hwnd,&ps);
        }
      }
    return 0;

    case WM_CAPTURECHANGED:
    case WM_SETTEXT:
      InvalidateRect(hwnd,NULL,FALSE);
    break;
  }
  return DefWindowProc(hwnd,msg,wParam,lParam);
}


/// these are for swell-dlggen.h
HWND SWELL_MakeButton(int def, const char *label, int idx, int x, int y, int w, int h, int flags)
{  
  UINT_PTR a=(UINT_PTR)label;
  if (a < 65536) label = "ICONTEMP";
  
  RECT tr=MakeCoords(x,y,w,h,true);
  HWND hwnd = swell_makeButton(m_make_owner,idx,&tr,label,!(flags&SWELL_NOT_WS_VISIBLE),0);

  if (m_doautoright) UpdateAutoCoords(tr);
  if (def) { }
  return hwnd;
}


HWND SWELL_MakeLabel( int align, const char *label, int idx, int x, int y, int w, int h, int flags)
{
  RECT tr=MakeCoords(x,y,w,h,true);
  HWND hwnd = new HWND__(m_make_owner,idx,&tr,label, !(flags&SWELL_NOT_WS_VISIBLE),labelWindowProc);
  hwnd->m_classname = "static";
  hwnd->m_style = (flags & ~SWELL_NOT_WS_VISIBLE)|WS_CHILD;
  hwnd->m_wndproc(hwnd,WM_CREATE,0,0);
  if (m_doautoright) UpdateAutoCoords(tr);
  return hwnd;
}
HWND SWELL_MakeEditField(int idx, int x, int y, int w, int h, int flags)
{  
  RECT tr=MakeCoords(x,y,w,h,true);
  HWND hwnd = new HWND__(m_make_owner,idx,&tr,NULL, !(flags&SWELL_NOT_WS_VISIBLE),editWindowProc);
  hwnd->m_style = WS_CHILD | (flags & ~SWELL_NOT_WS_VISIBLE);
  hwnd->m_classname = "Edit";
  hwnd->m_wndproc(hwnd,WM_CREATE,0,0);
  if (m_doautoright) UpdateAutoCoords(tr);
  return hwnd;
}


HWND SWELL_MakeCheckBox(const char *name, int idx, int x, int y, int w, int h, int flags=0)
{
  return SWELL_MakeControl(name,idx,"Button",BS_AUTOCHECKBOX|flags,x,y,w,h,0);
}

struct SWELL_ListView_Col
{
  char *name;
  int xwid;
};

struct listViewState
{
  listViewState(bool ownerData, bool isMultiSel, bool isListBox)
  {
    m_selitem=-1;
    m_is_multisel = isMultiSel;
    m_is_listbox = isListBox;
    m_owner_data_size = ownerData ? 0 : -1;
    m_last_row_height = 0;
  } 
  ~listViewState()
  { 
    m_data.Empty(true);
    const int n=m_cols.GetSize();
    for (int x=0;x<n;x++) free(m_cols.Get()[x].name);
  }
  WDL_PtrList<SWELL_ListView_Row> m_data;
  WDL_TypedBuf<SWELL_ListView_Col> m_cols;
  
  int GetNumItems() const { return m_owner_data_size>=0 ? m_owner_data_size : m_data.GetSize(); }
  bool IsOwnerData() const { return m_owner_data_size>=0; }
  bool HasColumnHeaders(HWND hwnd) const
  { 
     if (m_is_listbox || !m_cols.GetSize()) return false;
     return !(hwnd->m_style & LVS_NOCOLUMNHEADER) && (hwnd->m_style & LVS_REPORT);
  }

  int m_owner_data_size; // -1 if m_data valid, otherwise size
  int m_last_row_height;
  int m_selitem; // for single sel, or used for focus for multisel

  WDL_TypedBuf<unsigned int> m_owner_multisel_state;

  bool get_sel(int idx)
  {
    if (!m_is_multisel) return idx>=0 && idx == m_selitem;
    if (m_owner_data_size<0)
    {
      SWELL_ListView_Row *p = m_data.Get(idx);
      return p && (p->m_tmp&1);
    }
    const unsigned int mask = 1<<(idx&31);
    const int szn = idx/32;
    const unsigned int *p=m_owner_multisel_state.Get();
    return p && idx>=0 && szn < m_owner_multisel_state.GetSize() && (p[szn]&mask);
  }
  void set_sel(int idx, bool v)
  {
    if (!m_is_multisel) 
    { 
      if (v) m_selitem = idx;
      else if (m_selitem == idx) m_selitem = -1;
    }
    else if (m_owner_data_size<0)
    {
      SWELL_ListView_Row *p = m_data.Get(idx);
      if (p) p->m_tmp = (v ? (p->m_tmp|1) : (p->m_tmp&~1));
    }
    else 
    {
      if (idx>=0 && idx < m_owner_data_size)
      {
        const int szn = idx/32;
        const int oldsz=m_owner_multisel_state.GetSize();
        unsigned int *p = m_owner_multisel_state.Get();
        if (oldsz<szn+1) 
        {
          p = m_owner_multisel_state.ResizeOK(szn+1,false);
          if (p) memset(p+oldsz,0,(szn+1-oldsz)*sizeof(*p));
        }
        const unsigned int mask = 1<<(idx&31);
        if (p) p[szn] = v ? (p[szn]|mask) : (p[szn]&~mask);
      }
    }
  }
  void clear_sel()
  {
    if (!m_is_multisel) m_selitem = -1;
    else if (m_owner_data_size<0)
    {
      int x;
      const int n=m_data.GetSize();
      for (x=0;x<n;x++) m_data.Get(x)->m_tmp&=~1;
    }
    else m_owner_multisel_state.Resize(0,false);
  }
  

  bool m_is_multisel, m_is_listbox;
};

static LRESULT listViewWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  listViewState *lvs = (listViewState *)hwnd->m_private_data;
  switch (msg)
  {
    case WM_LBUTTONDBLCLK:
    case WM_LBUTTONDOWN:
      SetCapture(hwnd);
      if (lvs && lvs->m_last_row_height>0)
      {
        const int ypos = GET_Y_LPARAM(lParam) - (lvs->HasColumnHeaders(hwnd) ? lvs->m_last_row_height + 2 : 0);
        const int hit = ypos >= 0 ? (ypos / lvs->m_last_row_height) : -1;
        if (hit < 0)
        {
          // column click handling
        }
        else if (!lvs->m_is_multisel)
        {
          if (hit >= 0 && hit < lvs->GetNumItems()) lvs->m_selitem = hit;
          else lvs->m_selitem = -1;

          if (lvs->m_is_listbox)
          {
            SendMessage(GetParent(hwnd),WM_COMMAND,(LBN_SELCHANGE<<16) | (hwnd->m_id&0xffff),(LPARAM)hwnd);
          }
          else
          {
            NMLISTVIEW nm={{hwnd,hwnd->m_id,msg == WM_LBUTTONDBLCLK ? NM_DBLCLK : LVN_ITEMCHANGED},lvs->m_selitem,0,LVIS_SELECTED,};
            SendMessage(GetParent(hwnd),WM_NOTIFY,hwnd->m_id,(LPARAM)&nm);
          }
          InvalidateRect(hwnd,NULL,FALSE);
        }
        else 
        {
          if (!(GetAsyncKeyState(VK_CONTROL)&0x8000)) lvs->clear_sel();
          lvs->set_sel(hit,true);
          lvs->m_selitem = hit;

          if (hit >=0 && hit < lvs->GetNumItems()) 
          {
            if (lvs->m_is_listbox)
            {
              SendMessage(GetParent(hwnd),WM_COMMAND,(LBN_SELCHANGE<<16) | (hwnd->m_id&0xffff),(LPARAM)hwnd);
            }
            else
            {
              NMLISTVIEW nm={{hwnd,hwnd->m_id,msg == WM_LBUTTONDBLCLK ? NM_DBLCLK : LVN_ITEMCHANGED},hit,0,LVIS_SELECTED,};
              SendMessage(GetParent(hwnd),WM_NOTIFY,hwnd->m_id,(LPARAM)&nm);
            }
          }

          InvalidateRect(hwnd,NULL,FALSE);
        }
      }
    return 0;
    case WM_MOUSEMOVE:
    return 0;
    case WM_LBUTTONUP:
      if (GetCapture()==hwnd)
      {
        ReleaseCapture(); // WM_CAPTURECHANGED will take care of the invalidate
      }
    return 0;
    case WM_PAINT:
      { 
        PAINTSTRUCT ps;
        if (BeginPaint(hwnd,&ps))
        {
          RECT r; 
          GetClientRect(hwnd,&r); 
          HBRUSH br = CreateSolidBrush(RGB(255,255,255));
          FillRect(ps.hdc,&r,br);
          DeleteObject(br);
          br=CreateSolidBrush(RGB(128,128,255));
          if (lvs) 
          {
            const bool owner_data = lvs->IsOwnerData();
            const int n = owner_data ? lvs->m_owner_data_size : lvs->m_data.GetSize();
            TEXTMETRIC tm; 
            GetTextMetrics(ps.hdc,&tm);
            const int row_height = tm.tmHeight;
            int ypos = r.top;

            lvs->m_last_row_height = row_height;
            SetBkMode(ps.hdc,TRANSPARENT);
            const int ncols = lvs->m_cols.GetSize();
            const int nc = wdl_max(ncols,1);
            SWELL_ListView_Col *cols = lvs->m_cols.Get();
            if (lvs->HasColumnHeaders(hwnd))
            {
              HBRUSH br = CreateSolidBrush(RGB(192,192,192));
              int x,xpos=0;
              SetTextColor(ps.hdc,RGB(0,0,0));
              for (x=0; x < ncols; x ++)
              {
                RECT tr={xpos,ypos,0,ypos + row_height};
                xpos += cols[x].xwid;
                tr.right = xpos - 2;
               
                if (tr.right > tr.left) 
                {
                  FillRect(ps.hdc,&tr,br);
                  if (cols[x].name) 
                    DrawText(ps.hdc,cols[x].name,-1,&tr,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);
                }
              }
              ypos += row_height + 2;
              DeleteObject(br);
            }

            SetTextColor(ps.hdc,RGB(0,0,0));
            int x;
            for (x = 0; x < n && ypos < r.bottom; x ++)
            {
              const char *str = NULL;
              char buf[4096];
              int sel=0;

              if (!lvs->m_is_multisel) sel = x == lvs->m_selitem;
              else sel = lvs->get_sel(x);

              RECT tr={r.left,ypos,r.right,ypos + row_height};
              if (sel)
              {
                FillRect(ps.hdc,&tr,br);
              }

              SWELL_ListView_Row *row = lvs->m_data.Get(x);
              int col,xpos=0;
              for (col = 0; col < nc; col ++)
              {
                // todo: multiple columns too
                if (owner_data)
                {
                  NMLVDISPINFO nm={{hwnd,hwnd->m_id,LVN_GETDISPINFO},{LVIF_TEXT, x,col, 0,0, buf, sizeof(buf) }};
                  buf[0]=0;
                  SendMessage(GetParent(hwnd),WM_NOTIFY,hwnd->m_id,(LPARAM)&nm);
                  str=buf;
                }
                else
                {
                  if (row) str = row->m_vals.Get(col);
                }
  
                if (str) 
                {
                  RECT ar=tr;
                  if (ncols > 0)
                  {
                    ar.left += xpos;
                    ar.right = ar.left + cols[col].xwid;
                    xpos += cols[col].xwid;
                  }
                  DrawText(ps.hdc,str,-1,&ar,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);
                }
              }        
              ypos += row_height;
            }
          }
          DeleteObject(br);

          EndPaint(hwnd,&ps);
        }
      }
    return 0;
    case WM_DESTROY:
      hwnd->m_private_data = 0;
      delete lvs;
    return 0;
    case LB_ADDSTRING:
      if (lvs && !lvs->IsOwnerData())
      {
         // todo: optional sort
        int rv=lvs->m_data.GetSize();
        SWELL_ListView_Row *row=new SWELL_ListView_Row;
        row->m_vals.Add(strdup((const char *)lParam));
        lvs->m_data.Add(row); 
        InvalidateRect(hwnd,NULL,FALSE);
        return rv;
      }
    return LB_ERR;
     
    case LB_INSERTSTRING:
      if (lvs && !lvs->IsOwnerData())
      {
        int idx =  (int) wParam;
        if (idx<0 || idx>lvs->m_data.GetSize()) idx=lvs->m_data.GetSize();
        SWELL_ListView_Row *row=new SWELL_ListView_Row;
        row->m_vals.Add(strdup((const char *)lParam));
        lvs->m_data.Insert(idx,row); 
        InvalidateRect(hwnd,NULL,FALSE);
        return idx;
      }
    return LB_ERR;
    case LB_DELETESTRING:
      if (lvs && !lvs->IsOwnerData())
      {
        int idx =  (int) wParam;
        if (idx<0 || idx>=lvs->m_data.GetSize()) return LB_ERR;
        lvs->m_data.Delete(idx,true);
        InvalidateRect(hwnd,NULL,FALSE);
        return lvs->m_data.GetSize();
      }
    return LB_ERR;
    case LB_GETTEXT:
      if (!lParam) return LB_ERR;
      *(char *)lParam = 0;
      if (lvs && !lvs->IsOwnerData())
      {
        SWELL_ListView_Row *row = lvs->m_data.Get(wParam);
        if (row && row->m_vals.Get(0))
        {
          strcpy((char *)lParam, row->m_vals.Get(0));
          return (LRESULT)strlen(row->m_vals.Get(0));
        }
      }
    return LB_ERR;
    case LB_GETTEXTLEN:
        {
          SWELL_ListView_Row *row=lvs->m_data.Get(wParam);
          if (row) 
          {
            const char *p=row->m_vals.Get(0);
            return p?strlen(p):0;
          }
        }
    return LB_ERR;
    case LB_RESETCONTENT:
      if (lvs && !lvs->IsOwnerData())
      {
        lvs->m_data.Empty(true,free);
      }
      InvalidateRect(hwnd,NULL,FALSE);
    return 0;
    case LB_SETSEL:
      if (lvs && lvs->m_is_multisel)
      {
        if (lvs->IsOwnerData())
        {
        }
        else
        {
          if ((int)lParam == -1)
          {
            int x;
            const int n=lvs->m_data.GetSize();
            for(x=0;x<n;x++) 
            {
              SWELL_ListView_Row *row=lvs->m_data.Get(x);
              if (row) row->m_tmp = (row->m_tmp&~1) | (wParam?1:0);
            }
          }
          else
          {
            SWELL_ListView_Row *row=lvs->m_data.Get((int)lParam);
            if (!row) return LB_ERR;
            row->m_tmp = (row->m_tmp&~1) | (wParam?1:0);
            return 0;
          }
        }
      }
    return LB_ERR;
    case LB_SETCURSEL:
      if (lvs && !lvs->IsOwnerData() && !lvs->m_is_multisel)
      {
        lvs->m_selitem = (int)wParam;
        InvalidateRect(hwnd,NULL,FALSE);
      }
    return LB_ERR;
    case LB_GETSEL:
      if (lvs && lvs->m_is_multisel)
      {
        if (lvs->IsOwnerData())
        {
        }
        else
        {
          SWELL_ListView_Row *row=lvs->m_data.Get((int)wParam);
          if (!row) return LB_ERR;
          return row->m_tmp&1;
        }
      }
    return LB_ERR;
    case LB_GETCURSEL:
      if (lvs)
      {
        return (LRESULT)lvs->m_selitem;
      }
    return LB_ERR;
    case LB_GETCOUNT:
      if (lvs) return lvs->GetNumItems();
    return LB_ERR;
    case LB_GETSELCOUNT:
      if (lvs && lvs->m_is_multisel)
      {
        int cnt=0;
        if (lvs->IsOwnerData())
        {
        }
        else
        {
          int x;
          const int n=lvs->m_data.GetSize();
          for(x=0;x<n;x++) 
          {
            SWELL_ListView_Row *row=lvs->m_data.Get(x);
            if (row && (row->m_tmp&1)) cnt++;
          }
        }
        return cnt;
      }
    return LB_ERR;
    case LB_GETITEMDATA:
      if (lvs && !lvs->IsOwnerData())
      {
        SWELL_ListView_Row *row = lvs->m_data.Get(wParam);
        return row ? row->m_param : LB_ERR;
      }
    return LB_ERR;
    case LB_SETITEMDATA:
      if (lvs && !lvs->IsOwnerData())
      {
        SWELL_ListView_Row *row = lvs->m_data.Get(wParam);
        if (row) row->m_param = lParam;
        return row ? 0 : LB_ERR;
      }
    return LB_ERR;
  }
  return DefWindowProc(hwnd,msg,wParam,lParam);
}

struct treeViewState 
{
  treeViewState() 
  { 
    m_sel=NULL;
    m_last_row_height=0;
  }
  ~treeViewState() 
  {
    m_items.Empty(true);
  }
  bool findItem(HTREEITEM item, HTREEITEM *parOut, int *idxOut)
  {
    int x;
    const int n=m_items.GetSize();
    for (x=0; x < n; x ++)
    {
      HTREEITEM a = m_items.Get(x);
      if (a == item) 
      {
        if (parOut) *parOut = NULL;
        if (idxOut) *idxOut = x;
        return true;
      }
      if (a && a->FindItem(item,parOut,idxOut)) return true;
    }

    return false;
  }

  void doDrawItem(HTREEITEM item, HDC hdc, RECT *rect) // draws any subitems too, updates rect->top
  {
#ifdef SWELL_LICE_GDI
    if (!item) return;

    const int ob = rect->bottom;
    rect->bottom = rect->top + m_last_row_height;
    if (rect->right > rect->left)
    {
      if (item == m_sel) 
      {
        HBRUSH br=CreateSolidBrush(RGB(128,128,255));
        FillRect(hdc,rect,br);
        DeleteObject(br);
      }

      DrawText(hdc,item->m_value ? item->m_value : "",-1,rect,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);
    }
    rect->top = rect->bottom;
    rect->bottom = ob;

    if ((item->m_state & TVIS_EXPANDED) && item->m_haschildren && item->m_children.GetSize())
    {
      int x;
      const int n = item->m_children.GetSize();
      for (x=0;x<n && rect->top < rect->bottom;x++)
      {
        rect->left += m_last_row_height;
        doDrawItem(item->m_children.Get(x),hdc,rect);
        rect->left -= m_last_row_height;
      }
 
    } 
#endif
  }
  HTREEITEM hitTestItem(HTREEITEM item, int *y) 
  {
    *y -= m_last_row_height;
    if (*y < 0) return item;
    if ((item->m_state & TVIS_EXPANDED) && item->m_haschildren && item->m_children.GetSize())
    {
      int x;
      const int n = item->m_children.GetSize();
      for (x=0;x<n;x++)
      {
        HTREEITEM t=hitTestItem(item->m_children.Get(x),y);
        if (t) return t;
      }
    } 
    return NULL;
  }

  WDL_PtrList<HTREEITEM__> m_items;
  HTREEITEM m_sel;
  int m_last_row_height;
};

static LRESULT treeViewWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  treeViewState *tvs = (treeViewState *)hwnd->m_private_data;
  switch (msg)
  {
    case WM_LBUTTONDOWN:
      if (tvs && tvs->m_last_row_height) 
      {
        int x;
        const int n = tvs->m_items.GetSize();
        int y = GET_Y_LPARAM(lParam);
        for (x = 0; x < n; x ++)
        {
          HTREEITEM hit = tvs->hitTestItem(tvs->m_items.Get(x),&y);
          if (hit) 
          {
            if (tvs->m_sel != hit)
            {
              tvs->m_sel = hit;
              InvalidateRect(hwnd,NULL,FALSE);
              NMTREEVIEW nm={{(HWND)hwnd,(UINT_PTR)hwnd->m_id,TVN_SELCHANGED},};
              SendMessage(GetParent(hwnd),WM_NOTIFY,nm.hdr.idFrom,(LPARAM)&nm);
            }
            break;
          }
        }
      }
    return 0;
    case WM_PAINT:
      { 
        PAINTSTRUCT ps;
        if (BeginPaint(hwnd,&ps))
        {
          RECT r; 
          GetClientRect(hwnd,&r); 
          HBRUSH br = CreateSolidBrush(RGB(255,255,255));
          FillRect(ps.hdc,&r,br);
          DeleteObject(br);
          if (tvs)
          {
            SetTextColor(ps.hdc,RGB(0,0,0));

            TEXTMETRIC tm; 
            GetTextMetrics(ps.hdc,&tm);
            const int row_height = tm.tmHeight;
            tvs->m_last_row_height = row_height;

            SetBkMode(ps.hdc,TRANSPARENT);

            int x;
            const int n = tvs->m_items.GetSize();
            for (x = 0; x < n && r.top < r.bottom; x ++)
            {
              tvs->doDrawItem(tvs->m_items.Get(x),ps.hdc,&r);
            }
          }

          EndPaint(hwnd,&ps);
        }
      }
    return 0;
    case WM_DESTROY:
      hwnd->m_private_data = 0;
      delete tvs;
    return 0;
  }
  return DefWindowProc(hwnd,msg,wParam,lParam);
}

struct tabControlState
{
  tabControlState() { m_curtab=0; }
  ~tabControlState() { m_tabs.Empty(true,free); }
  int m_curtab;
  WDL_PtrList<char> m_tabs;
};
static const int TABCONTROL_HEIGHT = 20;

static LRESULT tabControlWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  const int xdiv = 6,xpad=4;

  tabControlState *s = (tabControlState *)hwnd->m_private_data;
  switch (msg)
  {
    case WM_DESTROY:
      hwnd->m_private_data = 0;
      delete s;
    return 0;  
    case WM_LBUTTONUP:
      if (GET_Y_LPARAM(lParam) < TABCONTROL_HEIGHT)
      {
        return 1;
      }
    break;
    case WM_LBUTTONDOWN:
      if (GET_Y_LPARAM(lParam) < TABCONTROL_HEIGHT)
      {
        int xp=GET_X_LPARAM(lParam),tab;
        HDC dc = GetDC(hwnd);
        for (tab = 0; tab < s->m_tabs.GetSize(); tab ++)
        {
          const char *buf = s->m_tabs.Get(tab);
          RECT tr={0,};
          DrawText(dc,buf,-1,&tr,DT_CALCRECT|DT_NOPREFIX|DT_SINGLELINE);
          xp -= tr.right - tr.left + 2*xpad + xdiv;
          if (xp < 0)
          {
            if (s->m_curtab != tab)
            {
              s->m_curtab = tab;
              InvalidateRect(hwnd,NULL,FALSE);
              NMHDR nm={hwnd,(UINT_PTR)hwnd->m_id,TCN_SELCHANGE};
              SendMessage(GetParent(hwnd),WM_NOTIFY,nm.idFrom,(LPARAM)&nm);
            }
            break;
          }
        }
 
        ReleaseDC(hwnd,dc);
        return 1;
      }
    break;
    case WM_PAINT:
      { 
        PAINTSTRUCT ps;
        if (BeginPaint(hwnd,&ps))
        {
          RECT r; 
          GetClientRect(hwnd,&r); 

          int tab;
          int xp=0;
          HPEN pen = CreatePen(PS_SOLID,0,GetSysColor(COLOR_3DHILIGHT));
          HPEN pen2 = CreatePen(PS_SOLID,0,GetSysColor(COLOR_3DSHADOW));
          int col = GetSysColor(COLOR_BTNTEXT);

          SetBkMode(ps.hdc,TRANSPARENT);
          SetTextColor(ps.hdc,col);
          HGDIOBJ oldPen=SelectObject(ps.hdc,pen);
          const int th = TABCONTROL_HEIGHT;
          int lx=0;
          for (tab = 0; tab < s->m_tabs.GetSize() && xp < r.right; tab ++)
          {
            const char *buf = s->m_tabs.Get(tab);
            RECT tr={0,};
            DrawText(ps.hdc,buf,-1,&tr,DT_CALCRECT|DT_NOPREFIX|DT_SINGLELINE);
            int tw=tr.right-tr.left + 2*xpad;

            const int olx=lx;
            lx=xp + tw+xdiv;
 
            MoveToEx(ps.hdc,xp,th,NULL);
            LineTo(ps.hdc,xp,0);
            LineTo(ps.hdc,xp+tw,0);
            SelectObject(ps.hdc,pen2);
            LineTo(ps.hdc,xp+tw,th);

            MoveToEx(ps.hdc, tab == s->m_curtab ? lx-xdiv : olx,th-1,NULL);
            LineTo(ps.hdc,lx,th-1);

            SelectObject(ps.hdc,pen);

            tr.left = xp+xpad;
            tr.top=0;
            tr.right = xp+tw-xpad;
            tr.bottom = th;

            DrawText(ps.hdc,buf,-1,&tr,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);
            xp = lx;
          }
          SelectObject(ps.hdc,pen2);
          MoveToEx(ps.hdc,lx,th-1,NULL);
          LineTo(ps.hdc,r.right,th-1);

          SelectObject(ps.hdc,oldPen);

          EndPaint(hwnd,&ps);
          DeleteObject(pen);
          DeleteObject(pen2);
        }
      }
      return 0;
  }
  return DefWindowProc(hwnd,msg,wParam,lParam);
}



HWND SWELL_MakeListBox(int idx, int x, int y, int w, int h, int styles)
{
  RECT tr=MakeCoords(x,y,w,h,true);
  HWND hwnd = new HWND__(m_make_owner,idx,&tr,NULL, !(styles&SWELL_NOT_WS_VISIBLE), listViewWindowProc);
  hwnd->m_style = WS_CHILD;
  hwnd->m_classname = "ListBox";
  hwnd->m_private_data = (INT_PTR) new listViewState(false, !!(styles & LBS_EXTENDEDSEL), true);
  hwnd->m_wndproc(hwnd,WM_CREATE,0,0);
  if (m_doautoright) UpdateAutoCoords(tr);
  return hwnd;
}

typedef struct ccprocrec
{
  SWELL_ControlCreatorProc proc;
  int cnt;
  struct ccprocrec *next;
} ccprocrec;

static ccprocrec *m_ccprocs;

void SWELL_RegisterCustomControlCreator(SWELL_ControlCreatorProc proc)
{
  if (!proc) return;
  
  ccprocrec *p=m_ccprocs;
  while (p && p->next)
  {
    if (p->proc == proc)
    {
      p->cnt++;
      return;
    }
    p=p->next;
  }
  ccprocrec *ent = (ccprocrec*)malloc(sizeof(ccprocrec));
  ent->proc=proc;
  ent->cnt=1;
  ent->next=0;
  
  if (p) p->next=ent;
  else m_ccprocs=ent;
}

void SWELL_UnregisterCustomControlCreator(SWELL_ControlCreatorProc proc)
{
  if (!proc) return;
  
  ccprocrec *lp=NULL;
  ccprocrec *p=m_ccprocs;
  while (p)
  {
    if (p->proc == proc)
    {
      if (--p->cnt <= 0)
      {
        if (lp) lp->next=p->next;
        else m_ccprocs=p->next;
        free(p);
      }
      return;
    }
    lp=p;
    p=p->next;
  }
}



HWND SWELL_MakeControl(const char *cname, int idx, const char *classname, int style, int x, int y, int w, int h, int exstyle)
{
  if (m_ccprocs)
  {
    RECT poo=MakeCoords(x,y,w,h,false);
    ccprocrec *p=m_ccprocs;
    while (p)
    {
      HWND hhh=p->proc((HWND)m_make_owner,cname,idx,classname,style,poo.left,poo.top,poo.right-poo.left,poo.bottom-poo.top);
      if (hhh) 
      {
        if (exstyle) SetWindowLong(hhh,GWL_EXSTYLE,exstyle);
        return hhh;
      }
      p=p->next;
    }
  }
  if (!stricmp(classname,"SysTabControl32"))
  {
    RECT tr=MakeCoords(x,y,w,h,false);
    HWND hwnd = new HWND__(m_make_owner,idx,&tr,NULL, !(style&SWELL_NOT_WS_VISIBLE), tabControlWindowProc);
    hwnd->m_style = WS_CHILD | (style & ~SWELL_NOT_WS_VISIBLE);
    hwnd->m_classname = "SysTabControl32";
    hwnd->m_private_data = (INT_PTR) new tabControlState;
    hwnd->m_wndproc(hwnd,WM_CREATE,0,0);
    SetWindowPos(hwnd,HWND_BOTTOM,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE); 
    return hwnd;
  }
  else if (!stricmp(classname, "SysListView32")||!stricmp(classname, "SysListView32_LB"))
  {
    RECT tr=MakeCoords(x,y,w,h,false);
    HWND hwnd = new HWND__(m_make_owner,idx,&tr,NULL, !(style&SWELL_NOT_WS_VISIBLE), listViewWindowProc);
    hwnd->m_style = WS_CHILD | (style & ~SWELL_NOT_WS_VISIBLE);
    hwnd->m_classname = "SysListView32";
    if (!stricmp(classname, "SysListView32"))
      hwnd->m_private_data = (INT_PTR) new listViewState(!!(style & LVS_OWNERDATA), !(style & LVS_SINGLESEL), false);
    else
      hwnd->m_private_data = (INT_PTR) new listViewState(false,false, true);

    hwnd->m_wndproc(hwnd,WM_CREATE,0,0);
    return hwnd;
  }
  else if (!stricmp(classname, "SysTreeView32"))
  {
    RECT tr=MakeCoords(x,y,w,h,false);
    HWND hwnd = new HWND__(m_make_owner,idx,&tr,NULL, !(style&SWELL_NOT_WS_VISIBLE), treeViewWindowProc);
    hwnd->m_style = WS_CHILD | (style & ~SWELL_NOT_WS_VISIBLE);
    hwnd->m_classname = "SysTreeView32";
    hwnd->m_private_data = (INT_PTR) new treeViewState;
    hwnd->m_wndproc(hwnd,WM_CREATE,0,0);
    return hwnd;
  }
  else if (!stricmp(classname, "msctls_progress32"))
  {
    RECT tr=MakeCoords(x,y,w,h,false);
    HWND hwnd = new HWND__(m_make_owner,idx,&tr,NULL, !(style&SWELL_NOT_WS_VISIBLE));
    hwnd->m_style = WS_CHILD | (style & ~SWELL_NOT_WS_VISIBLE);
    hwnd->m_classname = "msctls_progress32";
    hwnd->m_wndproc(hwnd,WM_CREATE,0,0);
    return hwnd;
  }
  else if (!stricmp(classname,"Edit"))
  {
    return SWELL_MakeEditField(idx,x,y,w,h,style);
  }
  else if (!stricmp(classname, "static"))
  {
    RECT tr=MakeCoords(x,y,w,h,false);
    HWND hwnd = new HWND__(m_make_owner,idx,&tr,cname, !(style&SWELL_NOT_WS_VISIBLE),labelWindowProc);
    hwnd->m_style = WS_CHILD | (style & ~SWELL_NOT_WS_VISIBLE);
    hwnd->m_classname = "static";
    hwnd->m_wndproc(hwnd,WM_CREATE,0,0);
    if (m_doautoright) UpdateAutoCoords(tr);
    return hwnd;
  }
  else if (!stricmp(classname,"Button"))
  {
    RECT tr=MakeCoords(x,y,w,h,true);
    HWND hwnd = swell_makeButton(m_make_owner,idx,&tr,cname,!(style&SWELL_NOT_WS_VISIBLE),(style&~SWELL_NOT_WS_VISIBLE)|WS_CHILD);
    if (m_doautoright) UpdateAutoCoords(tr);
    return hwnd;
  }
  else if (!stricmp(classname,"REAPERhfader")||!stricmp(classname,"msctls_trackbar32"))
  {
    RECT tr=MakeCoords(x,y,w,h,true);
    HWND hwnd = new HWND__(m_make_owner,idx,&tr,cname, !(style&SWELL_NOT_WS_VISIBLE));
    hwnd->m_style = WS_CHILD | (style & ~SWELL_NOT_WS_VISIBLE);
    hwnd->m_classname = !stricmp(classname,"REAPERhfader") ? "REAPERhfader" : "msctls_trackbar32";
    hwnd->m_wndproc(hwnd,WM_CREATE,0,0);
    return hwnd;
  }
  return 0;
}

HWND SWELL_MakeCombo(int idx, int x, int y, int w, int h, int flags)
{
  if (h>13)h=13;
  RECT tr=MakeCoords(x,y,w,h,true);
  HWND hwnd = new HWND__(m_make_owner,idx,&tr,NULL, !(flags&SWELL_NOT_WS_VISIBLE),comboWindowProc);
  hwnd->m_private_data = (INT_PTR) new __SWELL_ComboBoxInternalState;
  hwnd->m_style = (flags & ~SWELL_NOT_WS_VISIBLE)|WS_CHILD;
  hwnd->m_classname = "combobox";
  hwnd->m_wndproc(hwnd,WM_CREATE,0,0);
  if (m_doautoright) UpdateAutoCoords(tr);
  return hwnd;
}

HWND SWELL_MakeGroupBox(const char *name, int idx, int x, int y, int w, int h, int style)
{
  RECT tr=MakeCoords(x,y,w,h,false);
  HWND hwnd = new HWND__(m_make_owner,idx,&tr,name, !(style&SWELL_NOT_WS_VISIBLE),groupWindowProc);
  hwnd->m_style = WS_CHILD | (style & ~SWELL_NOT_WS_VISIBLE);
  hwnd->m_classname = "groupbox";
  hwnd->m_wndproc(hwnd,WM_CREATE,0,0);
  SetWindowPos(hwnd,HWND_BOTTOM,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE); 
  return hwnd;
}


int TabCtrl_GetItemCount(HWND hwnd)
{
   tabControlState *s = hwnd ? (tabControlState*) hwnd->m_private_data : NULL;
   return s ? s->m_tabs.GetSize() : 0;
}

BOOL TabCtrl_AdjustRect(HWND hwnd, BOOL fLarger, RECT *r)
{
  if (!r || !hwnd) return FALSE;
 
  r->top += TABCONTROL_HEIGHT;
  
  return TRUE;
}


BOOL TabCtrl_DeleteItem(HWND hwnd, int idx)
{
  tabControlState *s = hwnd ? (tabControlState*) hwnd->m_private_data : NULL;
  if (!s || !s->m_tabs.Get(idx)) return FALSE;
  
  s->m_tabs.Delete(idx,true);
  if (s->m_curtab>0) s->m_curtab--;
  InvalidateRect(hwnd,NULL,FALSE);
  // todo: send notification?

  return TRUE;
}

int TabCtrl_InsertItem(HWND hwnd, int idx, TCITEM *item)
{
  tabControlState *s = hwnd ? (tabControlState*) hwnd->m_private_data : NULL;
  if (!item || !s) return -1;
  if (!(item->mask & TCIF_TEXT) || !item->pszText) return -1;

  s->m_tabs.Insert(idx, strdup(item->pszText));

  InvalidateRect(hwnd,NULL,FALSE);
  // todo: send notification if s->m_tabs.GetSize()==1 ?

  return TRUE;
}

int TabCtrl_SetCurSel(HWND hwnd, int idx)
{
  tabControlState *s = hwnd ? (tabControlState*) hwnd->m_private_data : NULL;
  if (!s || !s->m_tabs.Get(idx)) return -1;
  const int lt =s->m_curtab;
  s->m_curtab = idx;
  InvalidateRect(hwnd,NULL,FALSE);
  
  return lt; 
}

int TabCtrl_GetCurSel(HWND hwnd)
{
  tabControlState *s = hwnd ? (tabControlState*) hwnd->m_private_data : NULL;
  return s ? s->m_curtab : -1;
}

void ListView_SetExtendedListViewStyleEx(HWND h, int flag, int mask)
{
}

void SWELL_SetListViewFastClickMask(HWND hList, int mask)
{
}

void ListView_SetImageList(HWND h, HIMAGELIST imagelist, int which)
{
  if (!h || !imagelist || which != LVSIL_STATE) return;
  
}

int ListView_GetColumnWidth(HWND h, int pos)
{
  listViewState *lvs = h ? (listViewState *)h->m_private_data : NULL;
  if (!lvs) return 0;
  if (pos < 0 || pos >= lvs->m_cols.GetSize()) return 0;

  return lvs->m_cols.Get()[pos].xwid;
}

void ListView_InsertColumn(HWND h, int pos, const LVCOLUMN *lvc)
{
  listViewState *lvs = h ? (listViewState *)h->m_private_data : NULL;
  if (!lvs || !lvc) return;
  SWELL_ListView_Col col = { 0, 100 };
  if (lvc->mask & LVCF_WIDTH) col.xwid = lvc->cx;
  if (lvc->mask & LVCF_TEXT) col.name = lvc->pszText ? strdup(lvc->pszText) : NULL;
  if (pos<0)pos=0;
  else if (pos>lvs->m_cols.GetSize()) pos=lvs->m_cols.GetSize();
  lvs->m_cols.Insert(col,pos);
}

void ListView_SetColumn(HWND h, int pos, const LVCOLUMN *lvc)
{
  listViewState *lvs = h ? (listViewState *)h->m_private_data : NULL;
  if (!lvs || !lvc) return;
  SWELL_ListView_Col *col = pos>=0&&pos < lvs->m_cols.GetSize() ? lvs->m_cols.Get()+pos : NULL;
  if (!col) return;
  if (lvc->mask & LVCF_WIDTH) col->xwid = lvc->cx;
  if (lvc->mask & LVCF_TEXT) 
  {
    free(col->name);
    col->name = lvc->pszText ? strdup(lvc->pszText) : NULL;
  }
}

void ListView_GetItemText(HWND hwnd, int item, int subitem, char *text, int textmax)
{
  LVITEM it={LVIF_TEXT,item,subitem,0,0,text,textmax,};
  ListView_GetItem(hwnd,&it);
}

int ListView_InsertItem(HWND h, const LVITEM *item)
{
  listViewState *lvs = h ? (listViewState *)h->m_private_data : NULL;
  if (!lvs || lvs->IsOwnerData() || !item || item->iSubItem) return 0;

  int idx =  (int) item->iItem;
  if (idx<0 || idx>lvs->m_data.GetSize()) idx=lvs->m_data.GetSize();
  SWELL_ListView_Row *row=new SWELL_ListView_Row;
  row->m_vals.Add((item->mask&LVIF_TEXT) && item->pszText ? strdup(item->pszText) : NULL);
  row->m_param = (item->mask&LVIF_PARAM) ? item->lParam : 0;
  row->m_tmp = ((item->mask & LVIF_STATE) && (item->state & LVIS_SELECTED)) ? 1:0;
  lvs->m_data.Insert(idx,row); 
  InvalidateRect(h,NULL,FALSE);
  return idx;
}

void ListView_SetItemText(HWND h, int ipos, int cpos, const char *txt)
{
  listViewState *lvs = h ? (listViewState *)h->m_private_data : NULL;
  if (!lvs || lvs->IsOwnerData() || cpos < 0 || cpos >= 32) return;
  SWELL_ListView_Row *row=lvs->m_data.Get(ipos);
  if (!row) return;
  while (row->m_vals.GetSize()<=cpos) row->m_vals.Add(NULL);
  free(row->m_vals.Get(cpos));
  row->m_vals.Set(cpos,txt?strdup(txt):NULL);
  InvalidateRect(h,NULL,FALSE);
}

int ListView_GetNextItem(HWND h, int istart, int flags)
{
  listViewState *lvs = h ? (listViewState *)h->m_private_data : NULL;
  if (!lvs) return -1;
  const int n = lvs->GetNumItems();
  int x;
  if (istart < 0) istart=-1;
  for (x=istart+1; x < n; x ++) 
  {
    if (flags&LVNI_SELECTED) if (lvs->get_sel(x)) return x;
    if (flags&LVNI_FOCUSED) if (lvs->m_selitem==x) return x;
  }
  for (x=0;x<=istart; x++) 
  {
    if (flags&LVNI_SELECTED) if (lvs->get_sel(x)) return x;
    if (flags&LVNI_FOCUSED) if (lvs->m_selitem==x) return x;
  }
  return -1;
}

bool ListView_SetItem(HWND h, LVITEM *item)
{
  listViewState *lvs = h ? (listViewState *)h->m_private_data : NULL;
  if (!lvs || !item) return false;

  const bool ownerData = lvs->IsOwnerData();
  if (!ownerData)
  {
    SWELL_ListView_Row *row=lvs->m_data.Get(item->iItem);
    if (!row) return false;
    while (row->m_vals.GetSize()<=item->iSubItem) row->m_vals.Add(NULL);
    if (item->mask&LVIF_TEXT) 
    {
      free(row->m_vals.Get(item->iSubItem));
      row->m_vals.Set(item->iSubItem,item->pszText?strdup(item->pszText):NULL);
    }
    if (item->mask & LVIF_PARAM) 
    {
      row->m_param = item->lParam;
    }
  }
  else 
  {
    if (item->iItem < 0 || item->iItem >= lvs->GetNumItems()) return false;
  }
  if (item->mask & LVIF_STATE)
  {
    ListView_SetItemState(h,item->iItem,item->state,item->stateMask);
  }

  InvalidateRect(h,NULL,FALSE);

  return true;
}

bool ListView_GetItem(HWND h, LVITEM *item)
{
  listViewState *lvs = h ? (listViewState *)h->m_private_data : NULL;
  if (!lvs || !item) return false;
  if (!lvs->IsOwnerData())
  {
    SWELL_ListView_Row *row=lvs->m_data.Get(item->iItem);
    if (!row) return false;
    if ((item->mask&LVIF_TEXT)&&item->pszText && item->cchTextMax > 0) 
    {
      const char *v=row->m_vals.Get(item->iSubItem);
      lstrcpyn_safe(item->pszText, v?v:"",item->cchTextMax);
    }
    if (item->mask & LVIF_PARAM) item->lParam = row->m_param;
  }
  else 
  {
    if (item->iItem < 0 || item->iItem >= lvs->GetNumItems()) return false;
  }

  if (item->mask & LVIF_STATE) 
  {
    item->state = lvs->get_sel(item->iItem) ? LVIS_SELECTED : 0;
    if (lvs->m_selitem == item->iItem) item->state |= LVIS_FOCUSED;
  }

  return true;
}
int ListView_GetItemState(HWND h, int ipos, UINT mask)
{
  listViewState *lvs = h ? (listViewState *)h->m_private_data : NULL;
  if (!lvs) return 0;
  int ret  = 0;
  if (mask & LVIS_SELECTED) ret |= (lvs->get_sel(ipos) ? LVIS_SELECTED : 0 );
  if ((mask & LVIS_FOCUSED) && lvs->m_selitem == ipos) ret |= LVIS_FOCUSED;
  return ret;
}

bool ListView_SetItemState(HWND h, int ipos, UINT state, UINT statemask)
{
  listViewState *lvs = h ? (listViewState *)h->m_private_data : NULL;
  if (!lvs) return false;

  int doref=0;
  static int _is_doing_all;
  
  if (ipos == -1)
  {
    int x;
    int n=ListView_GetItemCount(h);
    _is_doing_all++;
    for (x = 0; x < n; x ++)
      ListView_SetItemState(h,x,state,statemask);
    _is_doing_all--;
    ListView_RedrawItems(h,0,n-1);
    return true;
  }

  if (statemask & LVIS_SELECTED) lvs->set_sel(ipos,!!(state&LVIS_SELECTED));
  if (statemask & LVIS_FOCUSED)
  {
    if (state&LVIS_FOCUSED) lvs->m_selitem = ipos;
  }

  if (!_is_doing_all)
  {
    static int __rent;
    if (!__rent)
    {
      __rent++;
      NMLISTVIEW nm={{(HWND)h,(unsigned short)h->m_id,LVN_ITEMCHANGED},ipos,0,state,};
      SendMessage(GetParent(h),WM_NOTIFY,h->m_id,(LPARAM)&nm);      
      __rent--;
    }
    if (doref) ListView_RedrawItems(h,ipos,ipos);
  }
  return true;
}
void ListView_RedrawItems(HWND h, int startitem, int enditem)
{
  if (!h) return;
  InvalidateRect(h,NULL,FALSE);
}

void ListView_DeleteItem(HWND h, int ipos)
{
  listViewState *lvs = h ? (listViewState *)h->m_private_data : NULL;
  if (!lvs || lvs->IsOwnerData()) return;
  lvs->m_data.Delete(ipos,true);
  InvalidateRect(h,NULL,FALSE);
}

void ListView_DeleteAllItems(HWND h)
{
  listViewState *lvs = h ? (listViewState *)h->m_private_data : NULL;
  if (!lvs || lvs->IsOwnerData()) return;
  lvs->m_data.Empty(true);
  InvalidateRect(h,NULL,FALSE);
}

int ListView_GetSelectedCount(HWND h)
{
  listViewState *lvs = h ? (listViewState *)h->m_private_data : NULL;
  if (!lvs) return 0;
  const int n = lvs->GetNumItems();
  int sum=0,x;
  for (x=0;x<n;x++) if (lvs->get_sel(x)) sum++;
  return sum;
}

int ListView_GetItemCount(HWND h)
{
  listViewState *lvs = h ? (listViewState *)h->m_private_data : NULL;
  if (!lvs) return 0;
  return lvs->GetNumItems();
}

int ListView_GetSelectionMark(HWND h)
{
  listViewState *lvs = h ? (listViewState *)h->m_private_data : NULL;
  if (!lvs) return 0;
  const int n = lvs->GetNumItems();
  int x;
  for (x=0;x<n;x++) if (lvs->get_sel(x)) return x;
  return -1;
}
int SWELL_GetListViewHeaderHeight(HWND h)
{
  listViewState *lvs = h ? (listViewState *)h->m_private_data : NULL;
  if (!lvs || !lvs->HasColumnHeaders(h)) return 0;
  return lvs->m_last_row_height;
}

void ListView_SetColumnWidth(HWND h, int pos, int wid)
{
  listViewState *lvs = h ? (listViewState *)h->m_private_data : NULL;
  if (!lvs) return;
  SWELL_ListView_Col *col = pos>=0&&pos < lvs->m_cols.GetSize() ? lvs->m_cols.Get()+pos : NULL;
  if (col) 
  {
    col->xwid = wid;
    InvalidateRect(h,NULL,FALSE);
  }
}

int ListView_HitTest(HWND h, LVHITTESTINFO *pinf)
{
  listViewState *lvs = h ? (listViewState *)h->m_private_data : NULL;
  if (!lvs || !pinf) return -1;

  pinf->flags=0;
  pinf->iItem=-1;

  int x=pinf->pt.x;
  int y=pinf->pt.y;

  RECT r;
  GetClientRect(h,&r);

  if (x < 0) pinf->flags |= LVHT_TOLEFT;
  if (x >= r.right) pinf->flags |= LVHT_TORIGHT;
  if (y < 0) pinf->flags |= LVHT_ABOVE;
  if (y >= r.bottom) pinf->flags |= LVHT_BELOW;

  if (!pinf->flags && lvs->m_last_row_height)
  {
    const int ypos = y - (lvs->HasColumnHeaders(h) ? lvs->m_last_row_height + 2 : 0);
    const int hit = ypos >= 0 ? (ypos / lvs->m_last_row_height) : -1;
    if (hit < 0) pinf->flags |= LVHT_ABOVE;
    pinf->iItem=hit;
    if (pinf->iItem >= 0)
    {
      if (0) //tv->m_status_imagelist && pt.x <= [tv rowHeight])
      {
        pinf->flags=LVHT_ONITEMSTATEICON;
      }
      else 
      {
        pinf->flags=LVHT_ONITEMLABEL;
      }
    }
    else 
    {
      pinf->flags=LVHT_NOWHERE;
    }
  }

  return pinf->iItem;
}
int ListView_SubItemHitTest(HWND h, LVHITTESTINFO *pinf)
{
  listViewState *lvs = h ? (listViewState *)h->m_private_data : NULL;
  if (!lvs || !pinf) return -1;

  const int row = ListView_HitTest(h, pinf);
  int x,xpos=0,idx=0;
  const int n=lvs->m_cols.GetSize();
  for (x=0;x<n;x++)
  {
    const int xwid = lvs->m_cols.Get()[x].xwid;
    if (pinf->pt.x >= xpos && pinf->pt.x < xpos+xwid) { idx = x; break; }
    xpos += xwid;
  }
  pinf->iSubItem = idx;
  return row;
}

void ListView_SetItemCount(HWND h, int cnt)
{
  listViewState *lvs = h ? (listViewState *)h->m_private_data : NULL;
  if (!lvs || !lvs->IsOwnerData()) return;
  lvs->m_owner_data_size = cnt > 0 ? cnt : 0;
}

void ListView_EnsureVisible(HWND h, int i, BOOL pok)
{
  listViewState *lvs = h ? (listViewState *)h->m_private_data : NULL;
  if (!lvs) return;
}
bool ListView_GetSubItemRect(HWND h, int item, int subitem, int code, RECT *r)
{
  if (!h) return false;
  return false;
}
bool ListView_GetItemRect(HWND h, int item, RECT *r, int code)
{
  return false;
}

bool ListView_Scroll(HWND h, int xscroll, int yscroll)
{
  return false;
}
void ListView_SortItems(HWND hwnd, PFNLVCOMPARE compf, LPARAM parm)
{
  if (!hwnd) return;
}
bool ListView_DeleteColumn(HWND h, int pos)
{
  return false;
}
int ListView_GetCountPerPage(HWND h)
{
  return 1;
}

HWND ChildWindowFromPoint(HWND h, POINT p)
{
  if (!h) return 0;

  RECT r={0,};

  for(;;)
  {
    HWND h2=h->m_children;
    RECT sr;

    NCCALCSIZE_PARAMS tr={{h->m_position,},};
    if (h->m_wndproc) h->m_wndproc(h,WM_NCCALCSIZE,0,(LPARAM)&tr);
    r.left += tr.rgrc[0].left - h->m_position.left;
    r.top += tr.rgrc[0].top - h->m_position.top;

    while (h2)
    {
      sr = h2->m_position;
      sr.left += r.left;
      sr.right += r.left;
      sr.top += r.top;
      sr.bottom += r.top;

      if (h2->m_visible && PtInRect(&sr,p)) break;

      h2 = h2->m_next;
    }
    if (!h2) break; // h is the window we care about

    h=h2; // descend to h2
    r=sr;
  }

  return h;
}

HWND WindowFromPoint(POINT p)
{
  HWND h = SWELL_topwindows;
  while (h)
  {
    RECT r;
    GetWindowContentViewRect(h,&r);
    if (PtInRect(&r,p))
    {
      p.x -= r.left;
      p.y -= r.top;
      return ChildWindowFromPoint(h,p);
    }
    h = h->m_next;
  }
  return NULL;
}

void UpdateWindow(HWND hwnd)
{
  if (hwnd)
  {
#ifdef SWELL_TARGET_GDK
    while (hwnd && !hwnd->m_oswindow) hwnd=hwnd->m_parent;
    if (hwnd && hwnd->m_oswindow) gdk_window_process_updates(hwnd->m_oswindow,true);
#endif
  }
}

BOOL InvalidateRect(HWND hwnd, const RECT *r, int eraseBk)
{ 
  if (!hwnd) return FALSE;
  HWND hwndCall=hwnd;
#ifdef SWELL_LICE_GDI
  {
    hwnd->m_invalidated=true;
    HWND t=hwnd->m_parent;
    while (t && !t->m_child_invalidated) 
    { 
      if (eraseBk)
      {
        t->m_invalidated=true;
        eraseBk--;
      }
      t->m_child_invalidated=true;
      t=t->m_parent; 
    }
  }
#endif
#ifdef SWELL_TARGET_GDK
  GdkRectangle rect;
  if (r) { rect.x = r->left; rect.y = r->top; rect.width = r->right-r->left; rect.height = r->bottom - r->top; }
  else
  {
    rect.x=rect.y=0;
    rect.width = hwnd->m_position.right - hwnd->m_position.left;
    rect.height = hwnd->m_position.bottom - hwnd->m_position.top;
  }
  while (hwnd && !hwnd->m_oswindow) 
  {
    NCCALCSIZE_PARAMS tr={{ hwnd->m_position, },};
    if (hwnd->m_wndproc) hwnd->m_wndproc(hwnd,WM_NCCALCSIZE,0,(LPARAM)&tr);
    rect.x += tr.rgrc[0].left;
    rect.y += tr.rgrc[0].top;
    hwnd=hwnd->m_parent;
  }
  if (hwnd && hwnd->m_oswindow) 
  {
    RECT tr={0,0,hwnd->m_position.right-hwnd->m_position.left,hwnd->m_position.bottom-hwnd->m_position.top};
    NCCALCSIZE_PARAMS p={{tr,},};
    if (hwnd->m_wndproc) hwnd->m_wndproc(hwnd,WM_NCCALCSIZE,0,(LPARAM)&p);
    rect.x += p.rgrc[0].left;
    rect.y += p.rgrc[0].top;

    gdk_window_invalidate_rect(hwnd->m_oswindow,hwnd!=hwndCall || r ? &rect : NULL,true);
  }
#endif
  return TRUE;
}


HWND GetCapture()
{
  return s_captured_window;
}

HWND SetCapture(HWND hwnd)
{
  HWND oc = s_captured_window;
  if (oc != hwnd)
  {
    s_captured_window=hwnd;
    if (oc) SendMessage(oc,WM_CAPTURECHANGED,0,(LPARAM)hwnd);
#ifdef SWELL_TARGET_GDK
// this doesnt seem to be necessary
//    if (gdk_pointer_is_grabbed()) gdk_pointer_ungrab(GDK_CURRENT_TIME);
//    while (hwnd && !hwnd->m_oswindow) hwnd=hwnd->m_parent;
//    if (hwnd) gdk_pointer_grab(hwnd->m_oswindow,TRUE,GDK_ALL_EVENTS_MASK,hwnd->m_oswindow,NULL,GDK_CURRENT_TIME);
#endif
  } 
  return oc;
}

void ReleaseCapture()
{
  if (s_captured_window) 
  {
    SendMessage(s_captured_window,WM_CAPTURECHANGED,0,0);
    s_captured_window=0;
#ifdef SWELL_TARGET_GDK
//    if (gdk_pointer_is_grabbed()) gdk_pointer_ungrab(GDK_CURRENT_TIME);
#endif
  }
}

LRESULT SwellDialogDefaultWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  DLGPROC d=(DLGPROC)GetWindowLong(hwnd,DWL_DLGPROC);
  if (d) 
  {
    if (uMsg == WM_PAINT)
    {
        PAINTSTRUCT ps;
        if (BeginPaint(hwnd,&ps))
        {
          HBRUSH hbrush = (HBRUSH) d(hwnd,WM_CTLCOLORDLG,(WPARAM)ps.hdc,(LPARAM)hwnd);
          if (hbrush && hbrush != (HBRUSH)1)
          {
            FillRect(ps.hdc,&ps.rcPaint,hbrush);
          }
          else if (1) 
          {
            hbrush=CreateSolidBrush(GetSysColor(COLOR_WINDOW));
            FillRect(ps.hdc,&ps.rcPaint,hbrush);
            DeleteObject(hbrush);
          }
          else if (0) // todo only on top level windows?
          {
            SWELL_FillDialogBackground(ps.hdc,&ps.rcPaint,3);
          }
          
          EndPaint(hwnd,&ps);
        }
    }
    
    LRESULT r=(LRESULT) d(hwnd,uMsg,wParam,lParam);
    
   
    if (r) return r; 
  }
  return DefWindowProc(hwnd,uMsg,wParam,lParam);
}

BOOL EndPaint(HWND hwnd, PAINTSTRUCT *ps)
{
  return TRUE;
}

LRESULT DefWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  const int menubar_xspacing=5;
  switch (msg)
  {
    case WM_NCCALCSIZE:
      if (!hwnd->m_parent && hwnd->m_menu)
      {
        RECT *r = (RECT*)lParam;
        r->top += SWELL_INTERNAL_MENUBAR_SIZE;
      }
    break;
    case WM_NCPAINT:
      if (!hwnd->m_parent && hwnd->m_menu)
      {
        HDC dc = GetWindowDC(hwnd);
        if (dc)
        {
          RECT r;
          GetWindowContentViewRect(hwnd,&r);
          r.right -= r.left; r.left=0;
          r.bottom -= r.top; r.top=0;
          if (r.bottom>SWELL_INTERNAL_MENUBAR_SIZE) r.bottom=SWELL_INTERNAL_MENUBAR_SIZE;

          HBRUSH br=CreateSolidBrush(GetSysColor(COLOR_3DFACE));
          FillRect(dc,&r,br);
          DeleteObject(br);

          SetBkMode(dc,TRANSPARENT);
          int cols[2]={ GetSysColor(COLOR_BTNTEXT),GetSysColor(COLOR_3DHILIGHT)};

          int x,xpos=0;
          HMENU__ *menu = (HMENU__*)hwnd->m_menu;
          for(x=0;x<menu->items.GetSize();x++)
          {
            MENUITEMINFO *inf = menu->items.Get(x);
            if (inf->fType == MFT_STRING && inf->dwTypeData)
            {
              bool dis = !!(inf->fState & MF_GRAYED);
              SetTextColor(dc,cols[dis]);
              RECT cr=r; cr.left=cr.right=xpos;
              DrawText(dc,inf->dwTypeData,-1,&cr,DT_CALCRECT);
              DrawText(dc,inf->dwTypeData,-1,&cr,DT_VCENTER|DT_LEFT);
              xpos=cr.right+menubar_xspacing;
            }
          }

          ReleaseDC(hwnd,dc);
        }
      }
    break;
    case WM_RBUTTONUP:
    case WM_NCRBUTTONUP:
      {  
        POINT p={GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam)};
        HWND hwndDest=hwnd;
        if (msg==WM_RBUTTONUP)
        {
          ClientToScreen(hwnd,&p);
          HWND h=WindowFromPoint(p);
          if (h && IsChild(hwnd,h)) hwndDest=h;
        }
        SendMessage(hwnd,WM_CONTEXTMENU,(WPARAM)hwndDest,(p.x&0xffff)|(p.y<<16));
      }
    return 1;
    case WM_MOUSEWHEEL:
    case WM_MOUSEHWHEEL:
      {
        HWND par = GetParent(hwnd);
        if (par) return SendMessage(par,msg,wParam,lParam); // forward to parent
      }
    break;
//    case WM_NCLBUTTONDOWN:
    case WM_NCLBUTTONUP:
      if (!hwnd->m_parent && hwnd->m_menu)
      {
        RECT r;
        GetWindowContentViewRect(hwnd,&r);
        if (GET_Y_LPARAM(lParam)>=r.top && GET_Y_LPARAM(lParam) < r.top+SWELL_INTERNAL_MENUBAR_SIZE) 
        {
          HDC dc = GetWindowDC(hwnd);

          int x,xpos=r.left;
          HMENU__ *menu = (HMENU__*)hwnd->m_menu;
          for(x=0;x<menu->items.GetSize();x++)
          {
            MENUITEMINFO *inf = menu->items.Get(x);
            if (inf->fType == MFT_STRING && inf->dwTypeData)
            {
              bool dis = !!(inf->fState & MF_GRAYED);
              RECT cr=r; cr.left=cr.right=xpos;
              DrawText(dc,inf->dwTypeData,-1,&cr,DT_CALCRECT);

              if (GET_X_LPARAM(lParam) >=cr.left && GET_X_LPARAM(lParam)<cr.right + menubar_xspacing)
              {
                if (!dis)
                {
                  if (inf->hSubMenu) TrackPopupMenu(inf->hSubMenu,0,xpos,r.top+SWELL_INTERNAL_MENUBAR_SIZE,0,hwnd,NULL);
                  else if (inf->wID) SendMessage(hwnd,WM_COMMAND,inf->wID,0);
                }
                break;
              }

              xpos=cr.right+menubar_xspacing;
            }
          }
          
          if (dc) ReleaseDC(hwnd,dc);
        }
      }
    break;
    case WM_NCHITTEST: 
      if (!hwnd->m_parent && hwnd->m_menu)
      {
        RECT r;
        GetWindowContentViewRect(hwnd,&r);
        if (GET_Y_LPARAM(lParam)>=r.top && GET_Y_LPARAM(lParam) < r.top+SWELL_INTERNAL_MENUBAR_SIZE) return HTMENU;
      }
      // todo: WM_NCCALCSIZE etc
    return HTCLIENT;
    case WM_KEYDOWN:
    case WM_KEYUP: return 69;
    case WM_CONTEXTMENU:
        return hwnd->m_parent ? SendMessage(hwnd->m_parent,msg,wParam,lParam) : 0;
    case WM_GETFONT:
#ifdef SWELL_FREETYPE
        {
          HFONT SWELL_GetDefaultFont();
          return (LRESULT)SWELL_GetDefaultFont();
        }
#endif

        return 0;
  }
  return 0;
}


















///////////////// clipboard compatability (NOT THREAD SAFE CURRENTLY)


BOOL DragQueryPoint(HDROP hDrop,LPPOINT pt)
{
  if (!hDrop) return 0;
  DROPFILES *df=(DROPFILES*)GlobalLock(hDrop);
  BOOL rv=!df->fNC;
  *pt=df->pt;
  GlobalUnlock(hDrop);
  return rv;
}

void DragFinish(HDROP hDrop)
{
//do nothing for now (caller will free hdrops)
}

UINT DragQueryFile(HDROP hDrop, UINT wf, char *buf, UINT bufsz)
{
  if (!hDrop) return 0;
  DROPFILES *df=(DROPFILES*)GlobalLock(hDrop);

  UINT rv=0;
  char *p=(char*)df + df->pFiles;
  if (wf == 0xFFFFFFFF)
  {
    while (*p)
    {
      rv++;
      p+=strlen(p)+1;
    }
  }
  else
  {
    while (*p)
    {
      if (!wf--)
      {
        if (buf)
        {
          lstrcpyn_safe(buf,p,bufsz);
          rv=strlen(buf);
        }
        else rv=strlen(p);
          
        break;
      }
      p+=strlen(p)+1;
    }
  }
  GlobalUnlock(hDrop);
  return rv;
}



static WDL_PtrList<void> m_clip_recs;
//static WDL_PtrList<NSString> m_clip_fmts;
static WDL_PtrList<char> m_clip_curfmts;
bool OpenClipboard(HWND hwndDlg)
{
  m_clip_curfmts.Empty();
  return true;
}

void CloseClipboard() // frees any remaining items in clipboard
{
  m_clip_recs.Empty(true,GlobalFree);
}

UINT EnumClipboardFormats(UINT lastfmt)
{
  return 0;
}

HANDLE GetClipboardData(UINT type)
{
  return 0;
}


void EmptyClipboard()
{
}

void SetClipboardData(UINT type, HANDLE h)
{
}

UINT RegisterClipboardFormat(const char *desc)
{
  return 0;
}



///////// PostMessage emulation

BOOL PostMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  return SWELL_Internal_PostMessage(hwnd,message,wParam,lParam);
}

void SWELL_MessageQueue_Clear(HWND h)
{
  SWELL_Internal_PMQ_ClearAllMessages(h);
}



// implementation of postmessage stuff




typedef struct PMQ_rec
{
  HWND hwnd;
  UINT msg;
  WPARAM wParam;
  LPARAM lParam;

  struct PMQ_rec *next;
} PMQ_rec;

static WDL_Mutex *m_pmq_mutex;
static PMQ_rec *m_pmq, *m_pmq_empty, *m_pmq_tail;
static int m_pmq_size;

#define MAX_POSTMESSAGE_SIZE 1024


void SWELL_Internal_PostMessage_Init()
{
  if (m_pmq_mutex) return;
  
  m_pmq_mainthread=pthread_self();
  m_pmq_mutex = new WDL_Mutex;
}

void SWELL_MessageQueue_Flush()
{
  if (!m_pmq_mutex) return;
  
  m_pmq_mutex->Enter();
  PMQ_rec *p=m_pmq, *startofchain=m_pmq;
  m_pmq=m_pmq_tail=0;
  m_pmq_mutex->Leave();
  
  int cnt=0;
  // process out queue
  while (p)
  {
    // process this message
    SendMessage(p->hwnd,p->msg,p->wParam,p->lParam); 

    cnt ++;
    if (!p->next) // add the chain back to empties
    {
      m_pmq_mutex->Enter();
      m_pmq_size-=cnt;
      p->next=m_pmq_empty;
      m_pmq_empty=startofchain;
      m_pmq_mutex->Leave();
      break;
    }
    p=p->next;
  }
}

void SWELL_Internal_PMQ_ClearAllMessages(HWND hwnd)
{
  if (!m_pmq_mutex) return;
  
  m_pmq_mutex->Enter();
  PMQ_rec *p=m_pmq;
  PMQ_rec *lastrec=NULL;
  while (p)
  {
    if (hwnd && p->hwnd != hwnd) { lastrec=p; p=p->next; }
    else
    {
      PMQ_rec *next=p->next; 
      
      p->next=m_pmq_empty; // add p to empty list
      m_pmq_empty=p;
      m_pmq_size--;
      
      
      if (p==m_pmq_tail) m_pmq_tail=lastrec; // update tail
      
      if (lastrec)  p = lastrec->next = next;
      else p = m_pmq = next;
    }
  }
  m_pmq_mutex->Leave();
}

BOOL SWELL_Internal_PostMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  if (!hwnd||!m_pmq_mutex) return FALSE;

  BOOL ret=FALSE;
  m_pmq_mutex->Enter();

  if (m_pmq_empty||m_pmq_size<MAX_POSTMESSAGE_SIZE)
  {
    PMQ_rec *rec=m_pmq_empty;
    if (rec) m_pmq_empty=rec->next;
    else rec=(PMQ_rec*)malloc(sizeof(PMQ_rec));
    rec->next=0;
    rec->hwnd=hwnd;
    rec->msg=msg;
    rec->wParam=wParam;
    rec->lParam=lParam;

    if (m_pmq_tail) m_pmq_tail->next=rec;
    else 
    {
      PMQ_rec *p=m_pmq;
      while (p && p->next) p=p->next; // shouldnt happen unless m_pmq is NULL As well but why not for safety
      if (p) p->next=rec;
      else m_pmq=rec;
    }
    m_pmq_tail=rec;
    m_pmq_size++;

    ret=TRUE;
  }

  m_pmq_mutex->Leave();

  return ret;
}


int EnumPropsEx(HWND hwnd, PROPENUMPROCEX proc, LPARAM lParam)
{
  if (!hwnd) return -1;
  int x;
  for (x =0 ; x < hwnd->m_props.GetSize(); x ++)
  {
    const char *k="";
    void *p = hwnd->m_props.Enumerate(x,&k);
    if (!proc(hwnd,k,p,lParam)) return 0;
  }
  return 1;
}

HANDLE GetProp(HWND hwnd, const char *name)
{
  if (!hwnd) return NULL;
  return hwnd->m_props.Get(name);
}

BOOL SetProp(HWND hwnd, const char *name, HANDLE val)
{
  if (!hwnd) return false;
  hwnd->m_props.Insert(name,(void *)val);
  return TRUE;
}

HANDLE RemoveProp(HWND hwnd, const char *name)
{
  HANDLE h =GetProp(hwnd,name);
  hwnd->m_props.Delete(name);
  return h;
}


int GetSystemMetrics(int p)
{
  switch (p)
  {
    case SM_CXSCREEN:
    case SM_CYSCREEN:
      {
         RECT r;
         SWELL_GetViewPort(&r, NULL, false);
         return p==SM_CXSCREEN ? r.right-r.left : r.bottom-r.top; 
      }
    case SM_CXHSCROLL: return 16;
    case SM_CYHSCROLL: return 16;
    case SM_CXVSCROLL: return 16;
    case SM_CYVSCROLL: return 16;
  }
  return 0;
}

BOOL ScrollWindow(HWND hwnd, int xamt, int yamt, const RECT *lpRect, const RECT *lpClipRect)
{
  if (!hwnd || (!xamt && !yamt)) return FALSE;
  
  // move child windows only
  hwnd = hwnd->m_children;
  while (hwnd)
  {
    hwnd->m_position.left += xamt;
    hwnd->m_position.right += xamt;
    hwnd->m_position.top += yamt;
    hwnd->m_position.bottom += yamt;

    hwnd=hwnd->m_next;
  }
  return TRUE;
}

HWND FindWindowEx(HWND par, HWND lastw, const char *classname, const char *title)
{
  if (!par&&!lastw) return NULL; // need to implement this modes
  HWND h=lastw?GetWindow(lastw,GW_HWNDNEXT):GetWindow(par,GW_CHILD);
  while (h)
  {
    bool isOk=true;
    if (title)
    {
      char buf[512];
      buf[0]=0;
      GetWindowText(h,buf,sizeof(buf));
      if (strcmp(title,buf)) isOk=false;
    }
    if (classname)
    {
      // todo: other classname translations
    }
    
    if (isOk) return h;
    h=GetWindow(h,GW_HWNDNEXT);
  }
  return h;
}


HTREEITEM TreeView_InsertItem(HWND hwnd, TV_INSERTSTRUCT *ins)
{
  treeViewState *tvs = hwnd ? (treeViewState *)hwnd->m_private_data : NULL;
  if (!tvs || !ins) return NULL;

  HTREEITEM__ *par=NULL;
  int inspos=0;
  
  if (ins->hParent && ins->hParent != TVI_ROOT && ins->hParent != TVI_FIRST && ins->hParent != TVI_LAST && ins->hParent != TVI_SORT)
  {
    if (tvs->findItem(ins->hParent,&par,&inspos))
    {
      par = ins->hParent; 
    }
    else return 0;
  }
  
  if (ins->hInsertAfter == TVI_FIRST) inspos=0;
  else if (ins->hInsertAfter == TVI_LAST || ins->hInsertAfter == TVI_SORT || !ins->hInsertAfter) inspos=par ? par->m_children.GetSize() : tvs->m_items.GetSize();
  else inspos = par ? par->m_children.Find(ins->hInsertAfter)+1 : tvs->m_items.Find(ins->hInsertAfter)+1;
  
  HTREEITEM__ *item=new HTREEITEM__;
  if (ins->item.mask & TVIF_CHILDREN) item->m_haschildren = !!ins->item.cChildren;
  if (ins->item.mask & TVIF_PARAM) item->m_param = ins->item.lParam;
  if (ins->item.mask & TVIF_TEXT) item->m_value = strdup(ins->item.pszText);
  if (!par)
  {
    tvs->m_items.Insert(inspos,item);
  }
  else par->m_children.Insert(inspos,item);
  
  InvalidateRect(hwnd,NULL,FALSE);
  return item;
}

BOOL TreeView_Expand(HWND hwnd, HTREEITEM item, UINT flag)
{
  treeViewState *tvs = hwnd ? (treeViewState *)hwnd->m_private_data : NULL;
  if (!tvs || !tvs->findItem(item,NULL,NULL)) return FALSE;
 
  const int os = item->m_state;
  if (flag == TVE_EXPAND) item->m_state |= TVIS_EXPANDED;
  else if (flag == TVE_COLLAPSE) item->m_state &= ~TVIS_EXPANDED;
  else if (flag == TVE_TOGGLE) item->m_state ^= TVIS_EXPANDED;
  
  if (item->m_state != os) InvalidateRect(hwnd,NULL,FALSE);
  return TRUE;
}

HTREEITEM TreeView_GetSelection(HWND hwnd)
{ 
  treeViewState *tvs = hwnd ? (treeViewState *)hwnd->m_private_data : NULL;
  if (!tvs || !tvs->m_sel || !tvs->findItem(tvs->m_sel,NULL,NULL)) return NULL;
  return tvs->m_sel;
}

void TreeView_DeleteItem(HWND hwnd, HTREEITEM item)
{
  treeViewState *tvs = hwnd ? (treeViewState *)hwnd->m_private_data : NULL;
  if (!tvs) return;
  HTREEITEM par=NULL;
  int idx=0;
  if (!tvs->findItem(item,&par,&idx)) return;

  if (tvs->m_sel && (item == tvs->m_sel || item->FindItem(tvs->m_sel,NULL,NULL))) tvs->m_sel=NULL;

  if (par) par->m_children.Delete(idx,true);
  else tvs->m_items.Delete(idx,true);
  InvalidateRect(hwnd,NULL,FALSE);
}

void TreeView_DeleteAllItems(HWND hwnd)
{
  treeViewState *tvs = hwnd ? (treeViewState *)hwnd->m_private_data : NULL;
  if (!tvs) return;
  tvs->m_items.Empty(true);
  tvs->m_sel=NULL;
  InvalidateRect(hwnd,NULL,FALSE);
}

void TreeView_SelectItem(HWND hwnd, HTREEITEM item)
{
  treeViewState *tvs = hwnd ? (treeViewState *)hwnd->m_private_data : NULL;
  if (!tvs) return;

  if (tvs->m_sel == item || (item && !tvs->findItem(item,NULL,NULL))) return;

  tvs->m_sel = item;

  static int __rent;
  if (!__rent)
  {
    __rent++;
    NMTREEVIEW nm={{(HWND)hwnd,(UINT_PTR)hwnd->m_id,TVN_SELCHANGED},};
    SendMessage(GetParent(hwnd),WM_NOTIFY,nm.hdr.idFrom,(LPARAM)&nm);
    __rent--;
  }
  InvalidateRect(hwnd,NULL,FALSE);
}

BOOL TreeView_GetItem(HWND hwnd, LPTVITEM pitem)
{
  treeViewState *tvs = hwnd ? (treeViewState *)hwnd->m_private_data : NULL;
  if (!tvs || !pitem || !(pitem->mask & TVIF_HANDLE) || !(pitem->hItem)) return FALSE;
  
  HTREEITEM ti = pitem->hItem;
  pitem->cChildren = ti->m_haschildren ? 1:0;
  pitem->lParam = ti->m_param;
  if ((pitem->mask&TVIF_TEXT)&&pitem->pszText&&pitem->cchTextMax>0)
  {
    lstrcpyn_safe(pitem->pszText,ti->m_value?ti->m_value:"",pitem->cchTextMax);
  }
  pitem->state=(ti == tvs->m_sel ? TVIS_SELECTED : 0) | (ti->m_state & TVIS_EXPANDED);
  
  return TRUE;
}

BOOL TreeView_SetItem(HWND hwnd, LPTVITEM pitem)
{
  treeViewState *tvs = hwnd ? (treeViewState *)hwnd->m_private_data : NULL;
  if (!tvs || !pitem || !(pitem->mask & TVIF_HANDLE) || !(pitem->hItem)) return FALSE;

  if (!tvs->findItem(pitem->hItem,NULL,NULL)) return FALSE;
  
  HTREEITEM__ *ti = (HTREEITEM__*)pitem->hItem;
  
  if (pitem->mask & TVIF_CHILDREN) ti->m_haschildren = pitem->cChildren?1:0;
  if (pitem->mask & TVIF_PARAM)  ti->m_param =  pitem->lParam;
  
  if ((pitem->mask&TVIF_TEXT)&&pitem->pszText)
  {
    free(ti->m_value);
    ti->m_value=strdup(pitem->pszText);
    InvalidateRect(hwnd, 0, FALSE);
  }
 
  ti->m_state = (ti->m_state & ~pitem->stateMask) | (pitem->state & pitem->stateMask &~ TVIS_SELECTED);

  if (pitem->stateMask & pitem->state & TVIS_SELECTED)
  {
    tvs->m_sel = ti;
    static int __rent;
    if (!__rent)
    {
      __rent++;
      NMTREEVIEW nm={{hwnd,(UINT_PTR)hwnd->m_id,TVN_SELCHANGED},};
      SendMessage(GetParent(hwnd),WM_NOTIFY,nm.hdr.idFrom,(LPARAM)&nm);
      __rent--;
    }
  }

  InvalidateRect(hwnd,NULL,FALSE);
    
  return TRUE;
}

HTREEITEM TreeView_HitTest(HWND hwnd, TVHITTESTINFO *hti)
{
  if (!hwnd || !hti) return NULL;
  
  return NULL; // todo implement
}

HTREEITEM TreeView_GetRoot(HWND hwnd)
{
  treeViewState *tvs = hwnd ? (treeViewState *)hwnd->m_private_data : NULL;
  if (!tvs) return NULL;
  return tvs->m_items.Get(0);
}

HTREEITEM TreeView_GetChild(HWND hwnd, HTREEITEM item)
{
  treeViewState *tvs = hwnd ? (treeViewState *)hwnd->m_private_data : NULL;
  if (!tvs) return NULL;
  return item && item != TVI_ROOT ? item->m_children.Get(0) : tvs->m_items.Get(0);
}

HTREEITEM TreeView_GetNextSibling(HWND hwnd, HTREEITEM item)
{
  treeViewState *tvs = hwnd ? (treeViewState *)hwnd->m_private_data : NULL;
  
  HTREEITEM par=NULL;
  int idx=0;
  if (!tvs || !tvs->findItem(item,&par,&idx)) return NULL;

  if (par) return par->m_children.Get(idx+1);
  return tvs->m_items.Get(idx+1);
}
BOOL TreeView_SetIndent(HWND hwnd, int indent)
{
  return FALSE;
}

void TreeView_SetBkColor(HWND hwnd, int color)
{
}
void TreeView_SetTextColor(HWND hwnd, int color)
{
}
void ListView_SetBkColor(HWND hwnd, int color)
{
}
void ListView_SetTextBkColor(HWND hwnd, int color)
{
}
void ListView_SetTextColor(HWND hwnd, int color)
{
}
void ListView_SetGridColor(HWND hwnd, int color)
{
}
void ListView_SetSelColors(HWND hwnd, int *colors, int ncolors)
{
}
int ListView_GetTopIndex(HWND h)
{
  return 0;
}
BOOL ListView_GetColumnOrderArray(HWND h, int cnt, int* arr)
{
  return FALSE;
}
BOOL ListView_SetColumnOrderArray(HWND h, int cnt, int* arr)
{
  return FALSE;
}
HWND ListView_GetHeader(HWND h)
{
  return 0;
}

int Header_GetItemCount(HWND h)
{
  return 0;
}

BOOL Header_GetItem(HWND h, int col, HDITEM* hi)
{
  return FALSE;
}

BOOL Header_SetItem(HWND h, int col, HDITEM* hi)
{
  return FALSE;
}


BOOL EnumChildWindows(HWND hwnd, BOOL (*cwEnumFunc)(HWND,LPARAM),LPARAM lParam)
{
  if (hwnd && hwnd->m_children)
  {
    HWND n=hwnd->m_children;
    while (n)
    {
      if (!cwEnumFunc(n,lParam) || !EnumChildWindows(n,cwEnumFunc,lParam)) return FALSE;
      n = n->m_next;
    }
  }
  return TRUE;
}
void SWELL_GetDesiredControlSize(HWND hwnd, RECT *r)
{
}

BOOL SWELL_IsGroupBox(HWND hwnd)
{
  //todo
  return FALSE;
}
BOOL SWELL_IsButton(HWND hwnd)
{
  //todo
  return FALSE;
}
BOOL SWELL_IsStaticText(HWND hwnd)
{
  //todo
  return FALSE;
}


BOOL ShellExecute(HWND hwndDlg, const char *action,  const char *content1, const char *content2, const char *content3, int blah)
{
  return FALSE;
}




// r=NULL to "free" handle
// otherwise r is in hwndPar coordinates
void SWELL_DrawFocusRect(HWND hwndPar, RECT *rct, void **handle)
{
  if (!handle) return;
}

void SWELL_BroadcastMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{

  HWND h = SWELL_topwindows;
  while (h) 
  { 
    SendMessage(h,uMsg,wParam,lParam);
    h = h->m_next;
  }
}

int SWELL_SetWindowLevel(HWND hwnd, int newlevel)
{
  return 0;
}
void SetOpaque(HWND h, bool opaque)
{
}
void SetAllowNoMiddleManRendering(HWND h, bool allow)
{
}
int SWELL_GetDefaultButtonID(HWND hwndDlg, bool onlyIfEnabled)
{
  return 0;
}

void GetCursorPos(POINT *pt)
{
  pt->x=0;
  pt->y=0;
#ifdef SWELL_TARGET_GDK
  if (SWELL_gdk_active>0)
  {
//#if SWELL_TARGET_GDK == 3
//    GdkDevice *dev=NULL;
//    if (s_cur_evt) dev = gdk_event_get_device(s_cur_evt);
//    if (!dev) dev = gdk_device_manager_get_client_pointer(gdk_display_get_device_manager(gdk_display_get_default()));
//    if (dev) gdk_device_get_position(dev,NULL,&pt->x,&pt->y);
//#else
    gdk_display_get_pointer(gdk_display_get_default(),NULL,&pt->x,&pt->y,NULL);
//#endif
  }
#endif
}

WORD GetAsyncKeyState(int key)
{
#ifdef SWELL_TARGET_GDK
  if (SWELL_gdk_active>0)
  {
    GdkModifierType mod=(GdkModifierType)0;
    HWND h = GetFocus();
    while (h && !h->m_oswindow) h = h->m_parent;
//#if SWELL_TARGET_GDK == 3
//    GdkDevice *dev=NULL;
//    if (s_cur_evt) dev = gdk_event_get_device(s_cur_evt);
//    if (!dev) dev = gdk_device_manager_get_client_pointer(gdk_display_get_device_manager(gdk_display_get_default()));
//    if (dev) gdk_window_get_device_position(h?  h->m_oswindow : gdk_get_default_root_window(),dev, NULL, NULL,&mod);
//#else
    gdk_window_get_pointer(h?  h->m_oswindow : gdk_get_default_root_window(),NULL,NULL,&mod);
//#endif
 
    if (key == VK_LBUTTON) return (mod&GDK_BUTTON1_MASK)?0x8000:0;
    if (key == VK_MBUTTON) return (mod&GDK_BUTTON2_MASK)?0x8000:0;
    if (key == VK_RBUTTON) return (mod&GDK_BUTTON3_MASK)?0x8000:0;

    if (key == VK_CONTROL) return (mod&GDK_CONTROL_MASK)?0x8000:0;
    if (key == VK_MENU) return (mod&GDK_MOD1_MASK)?0x8000:0;
    if (key == VK_SHIFT) return (mod&GDK_SHIFT_MASK)?0x8000:0;
  }
#endif
  return 0;
}


DWORD GetMessagePos()
{  
  return s_lastMessagePos;
}

void SWELL_HideApp()
{
}

BOOL SWELL_GetGestureInfo(LPARAM lParam, GESTUREINFO* gi)
{
  return FALSE;
}

void SWELL_SetWindowWantRaiseAmt(HWND h, int  amt)
{
}
int SWELL_GetWindowWantRaiseAmt(HWND h)
{
  return 0;
}

// copied from swell-wnd.mm, can maybe have a common impl instead
void SWELL_GenerateDialogFromList(const void *_list, int listsz)
{
#define SIXFROMLIST list->p1,list->p2,list->p3, list->p4, list->p5, list->p6
  SWELL_DlgResourceEntry *list = (SWELL_DlgResourceEntry*)_list;
  while (listsz>0)
  {
    if (!strcmp(list->str1,"__SWELL_BUTTON"))
    {
      SWELL_MakeButton(list->flag1,list->str2, SIXFROMLIST);
    } 
    else if (!strcmp(list->str1,"__SWELL_EDIT"))
    {
      SWELL_MakeEditField(SIXFROMLIST);
    }
    else if (!strcmp(list->str1,"__SWELL_COMBO"))
    {
      SWELL_MakeCombo(SIXFROMLIST);
    }
    else if (!strcmp(list->str1,"__SWELL_LISTBOX"))
    {
      SWELL_MakeListBox(SIXFROMLIST);
    }
    else if (!strcmp(list->str1,"__SWELL_GROUP"))
    {
      SWELL_MakeGroupBox(list->str2,SIXFROMLIST);
    }
    else if (!strcmp(list->str1,"__SWELL_CHECKBOX"))
    {
      SWELL_MakeCheckBox(list->str2,SIXFROMLIST);
    }
    else if (!strcmp(list->str1,"__SWELL_LABEL"))
    {
      SWELL_MakeLabel(list->flag1, list->str2, SIXFROMLIST);
    }
    else if (*list->str2)
    {
      SWELL_MakeControl(list->str1, list->flag1, list->str2, SIXFROMLIST);
    }
    listsz--;
    list++;
  }
}

#endif
