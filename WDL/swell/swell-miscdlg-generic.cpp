/* Cockos SWELL (Simple/Small Win32 Emulation Layer for Losers (who use OS X))
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
  

    This file provides basic APIs for browsing for files, directories, and messageboxes.

    These APIs don't all match the Windows equivelents, but are close enough to make it not too much trouble.

  */


#ifndef SWELL_PROVIDED_BY_APP

#include "swell.h"
#include "swell-internal.h"
#include "swell-dlggen.h"

#include "../wdlcstring.h"
#include <dirent.h>

static const char *BFSF_Templ_dlgid;
static DLGPROC BFSF_Templ_dlgproc;
static struct SWELL_DialogResourceIndex *BFSF_Templ_reshead;
void BrowseFile_SetTemplate(const char *dlgid, DLGPROC dlgProc, struct SWELL_DialogResourceIndex *reshead)
{
  BFSF_Templ_reshead=reshead;
  BFSF_Templ_dlgid=dlgid;
  BFSF_Templ_dlgproc=dlgProc;
}

struct BrowseFile_State
{
  const char *caption;
  const char *initialdir;
  const char *initialfile;
  const char *extlist;

  enum { SAVE=0,OPEN, OPENMULTI, OPENDIR } mode;
  char *fnout; // if NULL this will be malloced by the window
  int fnout_sz;
};

static LRESULT WINAPI swellFileSelectProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
    case WM_CREATE:
      if (lParam)  // swell-specific
      {
        SetWindowLong(hwnd,GWL_WNDPROC,(LPARAM)SwellDialogDefaultWindowProc);
        SetWindowLong(hwnd,DWL_DLGPROC,(LPARAM)swellFileSelectProc);
        SetWindowLongPtr(hwnd,GWLP_USERDATA,lParam);
        BrowseFile_State *parms = (BrowseFile_State *)lParam;
        if (parms->caption) SetWindowText(hwnd,parms->caption);

        SWELL_MakeSetCurParms(1,1,0,0,hwnd,false,false);

        SWELL_MakeButton(0,
              parms->mode == BrowseFile_State::OPENDIR ? "Choose directory" :
              parms->mode == BrowseFile_State::SAVE ? "Save" : "Open",
              IDOK,0,0,0,0, 0);

        SWELL_MakeButton(0, "Cancel", IDCANCEL,0,0,0,0, 0);
        HWND edit = SWELL_MakeEditField(0x100, 0,0,0,0,  0);
        if (edit)
        {
          if (parms->initialfile && *parms->initialfile) SetWindowText(edit,parms->initialfile);
          else if (parms->initialdir && *parms->initialdir) 
          {
            char buf[1024];
            lstrcpyn_safe(buf,parms->initialdir,sizeof(buf) - 1);
            if (parms->mode != BrowseFile_State::OPENDIR && buf[0] && buf[strlen(buf)-1]!='/') lstrcatn(buf,"/",sizeof(buf));
            SetWindowText(edit,buf);
          }
        }
        SWELL_MakeLabel(-1,parms->mode == BrowseFile_State::OPENDIR ? "Directory: " : "File:",0x101, 0,0,0,0, 0); 
        
        if (BFSF_Templ_dlgid && BFSF_Templ_dlgproc)
        {
          HWND dlg = SWELL_CreateDialog(BFSF_Templ_reshead, BFSF_Templ_dlgid, hwnd, BFSF_Templ_dlgproc, 0);
          if (dlg) SetWindowLong(dlg,GWL_ID,0x102);
          BFSF_Templ_dlgproc=0;
          BFSF_Templ_dlgid=0;
        }

        SWELL_MakeSetCurParms(1,1,0,0,NULL,false,false);
        SetWindowPos(hwnd,NULL,0,0,600, 400, SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOMOVE);
      }
    break;
    case WM_GETMINMAXINFO:
      {
        LPMINMAXINFO p=(LPMINMAXINFO)lParam;
        p->ptMinTrackSize.x = 300;
        p->ptMinTrackSize.y = 300;
      }
    break;
    case WM_SIZE:
      {
        BrowseFile_State *parms = (BrowseFile_State *)GetWindowLongPtr(hwnd,GWLP_USERDATA);
        // reposition controls
        RECT r;
        GetClientRect(hwnd,&r);
        const int buth = 24, cancelbutw = 50, okbutw = parms->mode == BrowseFile_State::OPENDIR ? 120 : 50;
        const int xborder = 4, yborder=8;
        const int fnh = 20, fnlblw = parms->mode == BrowseFile_State::OPENDIR ? 70 : 50;

        int ypos = r.bottom - 4 - buth;
        int xpos = r.right;
        SetWindowPos(GetDlgItem(hwnd,IDCANCEL), NULL, xpos -= cancelbutw + xborder, ypos, cancelbutw,buth, SWP_NOZORDER|SWP_NOACTIVATE);
        SetWindowPos(GetDlgItem(hwnd,IDOK), NULL, xpos -= okbutw + xborder, ypos, okbutw,buth, SWP_NOZORDER|SWP_NOACTIVATE);

        HWND emb = GetDlgItem(hwnd,0x102);
        if (emb)
        {
          RECT sr;
          GetClientRect(emb,&sr);
          if (ypos > r.bottom-4-sr.bottom) ypos = r.bottom-4-sr.bottom;
          SetWindowPos(emb,NULL, xborder,ypos, xpos - xborder*2, sr.bottom, SWP_NOZORDER|SWP_NOACTIVATE);
          ShowWindow(emb,SW_SHOWNA);
        }
 

        SetWindowPos(GetDlgItem(hwnd,0x100), NULL, xborder*2 + fnlblw, ypos -= fnh + yborder, r.right-fnlblw-xborder*3, fnh, SWP_NOZORDER|SWP_NOACTIVATE);
        SetWindowPos(GetDlgItem(hwnd,0x101), NULL, xborder, ypos, fnlblw, fnh, SWP_NOZORDER|SWP_NOACTIVATE);
  
      }
    break;
    case WM_COMMAND:
      switch (LOWORD(wParam))
      {
        case IDCANCEL: EndDialog(hwnd,0); return 0;
        case IDOK: 
          {
            char buf[1024],msg[2048];
            GetDlgItemText(hwnd,0x100,buf,sizeof(buf));
            BrowseFile_State *parms = (BrowseFile_State *)GetWindowLongPtr(hwnd,GWLP_USERDATA);
            switch (parms->mode)
            {
              case BrowseFile_State::SAVE:
                 if (!buf[0]) 
                 {
                   MessageBox(hwnd,"No file specified","Error",MB_OK);
                   return 0;
                 }
                 else  
                 {
                   struct stat st={0,};
                   DIR *dir = opendir(buf);
                   if (dir)
                   {
                     closedir(dir);
                     snprintf(msg,sizeof(msg),"Path is a directory:\r\n\r\n%s",buf);
                     MessageBox(hwnd,msg,"Invalid file",MB_OK);
                     return 0;
                   }
                   if (!stat(buf,&st))
                   {
                     snprintf(msg,sizeof(msg),"File exists:\r\n\r\n%s\r\n\r\nOverwrite?",buf);
                     if (MessageBox(hwnd,msg,"Overwrite file?",MB_OKCANCEL)==IDCANCEL) return 0;
                   }
                 }
              break;
              case BrowseFile_State::OPENDIR:
                 if (!buf[0]) 
                 { 
                   MessageBox(hwnd,"No directory specified","Error",MB_OK);
                   return 0;
                 } 
                 else
                 {
                   DIR *dir = opendir(buf);
                   if (!dir) 
                   {
                     snprintf(msg,sizeof(msg),"Error opening directory:\r\n\r\n%s\r\n\r\nCreate?",buf);
                     if (MessageBox(hwnd,msg,"Create directory?",MB_OKCANCEL)==IDCANCEL) return 0;
                     CreateDirectory(buf,NULL);
                     dir=opendir(buf);
                     if (!dir) { MessageBox(hwnd,"Error creating directory","Error",MB_OK); return 0; }
                   }
                   if (dir) closedir(dir);
                 }
              break;
              default:
                 if (!buf[0]) 
                 {
                   MessageBox(hwnd,"No file specified","Error",MB_OK);
                   return 0;
                 }
                 else  
                 {
                   struct stat st={0,};
                   DIR *dir = opendir(buf);
                   if (dir)
                   {
                     closedir(dir);
                     snprintf(msg,sizeof(msg),"Path is a directory:\r\n\r\n%s",buf);
                     MessageBox(hwnd,msg,"Invalid file",MB_OK);
                     return 0;
                   }
                   if (stat(buf,&st))
                   {
                     snprintf(msg,sizeof(msg),"File does not exist:\r\n\r\n%s",buf);
                     MessageBox(hwnd,msg,"File not found",MB_OK);
                     return 0;
                   }
                 }
              break;
            }
            if (parms->fnout) 
            {
              lstrcpyn_safe(parms->fnout,buf,parms->fnout_sz);
            }
            else
            {
              size_t l = strlen(buf);
              parms->fnout = (char*)calloc(l+2,1);
              memcpy(parms->fnout,buf,l);
            }
          }
          EndDialog(hwnd,1);
        return 0;
      }
    break;
  }
  return 0;
}

// return true
bool BrowseForSaveFile(const char *text, const char *initialdir, const char *initialfile, const char *extlist,
                       char *fn, int fnsize)
{
  BrowseFile_State state = { text, initialdir, initialfile, extlist, BrowseFile_State::SAVE, fn, fnsize };
  return !!DialogBoxParam(NULL,NULL,NULL,swellFileSelectProc,(LPARAM)&state);
}

bool BrowseForDirectory(const char *text, const char *initialdir, char *fn, int fnsize)
{
  BrowseFile_State state = { text, initialdir, initialdir, NULL, BrowseFile_State::OPENDIR, fn, fnsize };
  return !!DialogBoxParam(NULL,NULL,NULL,swellFileSelectProc,(LPARAM)&state);
}


char *BrowseForFiles(const char *text, const char *initialdir, 
                     const char *initialfile, bool allowmul, const char *extlist)
{
  BrowseFile_State state = { text, initialdir, initialfile, extlist, 
           allowmul ? BrowseFile_State::OPENMULTI : BrowseFile_State::OPEN, NULL, 0 };
  return DialogBoxParam(NULL,NULL,NULL,swellFileSelectProc,(LPARAM)&state) ? state.fnout : NULL;
}


static LRESULT WINAPI swellMessageBoxProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  const int button_spacing = 8;
  switch (uMsg)
  {
    case WM_CREATE:
      if (lParam)  // swell-specific
      {
        SetWindowLong(hwnd,GWL_WNDPROC,(LPARAM)SwellDialogDefaultWindowProc);
        SetWindowLong(hwnd,DWL_DLGPROC,(LPARAM)swellMessageBoxProc);
        void **parms = (void **)lParam;
        if (parms[1]) SetWindowText(hwnd,(const char*)parms[1]);


        int nbuttons=1;
        const char *buttons[3] = { "OK", "", "" };
        int button_ids[3] = {IDOK,0,0};
        int button_sizes[3];

        int mode =  ((int)(INT_PTR)parms[2]);
        if (mode == MB_RETRYCANCEL) { buttons[0]="Retry"; button_ids[0]=IDRETRY;  }
        if (mode == MB_YESNO || mode == MB_YESNOCANCEL) { buttons[0]="Yes"; button_ids[0] = IDYES;  buttons[nbuttons] = "No"; button_ids[nbuttons] = IDNO; nbuttons++; }
        if (mode == MB_OKCANCEL || mode == MB_YESNOCANCEL || mode == MB_RETRYCANCEL) { buttons[nbuttons] = "Cancel"; button_ids[nbuttons] = IDCANCEL; nbuttons++; }

        SWELL_MakeSetCurParms(1,1,0,0,hwnd,false,false);
        RECT labsize = {0,0,300,20};
        HWND lab = SWELL_MakeLabel(-1,parms[0] ? (const char *)parms[0] : "", 0x100, 0,0,10,10,SS_CENTER); //we'll resize this manually
        HDC dc=GetDC(lab); 
        if (lab && parms[0])
        {
          DrawText(dc,(const char *)parms[0],-1,&labsize,DT_CALCRECT|DT_NOPREFIX);// if dc isnt valid yet, try anyway
        }
        labsize.top += 10;
        labsize.bottom += 18;

        int x;
        int button_height=0, button_total_w=0;;
        for (x = 0; x < nbuttons; x ++)
        {
          RECT r={0,0,35,12};
          DrawText(dc,buttons[x],-1,&r,DT_CALCRECT|DT_NOPREFIX|DT_SINGLELINE);
          button_sizes[x] = r.right-r.left + 8;
          button_total_w += button_sizes[x] + (x ? button_spacing : 0);
          if (r.bottom-r.top+10 > button_height) button_height = r.bottom-r.top+10;
        }

        if (labsize.right < button_total_w+16) labsize.right = button_total_w+16;

        int xpos = labsize.right/2 - button_total_w/2;
        for (x = 0; x < nbuttons; x ++)
        {
          SWELL_MakeButton(0,buttons[x],button_ids[x],xpos,labsize.bottom,button_sizes[x],button_height,0);
          xpos += button_sizes[x] + button_spacing;
        }

        if (dc) ReleaseDC(lab,dc);
        SWELL_MakeSetCurParms(1,1,0,0,NULL,false,false);
        SetWindowPos(hwnd,NULL,0,0,labsize.right + 16,labsize.bottom + button_height + 8,SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOMOVE);
        if (lab) SetWindowPos(lab,NULL,8,0,labsize.right,labsize.bottom,SWP_NOACTIVATE|SWP_NOZORDER);
      }
    break;
    case WM_SIZE:
      {
        RECT r;
        GetClientRect(hwnd,&r);
        HWND h = GetWindow(hwnd,GW_CHILD);
        int n = 100;
        int w[8];
        HWND tab[8],lbl=NULL;
        int tabsz=0, bxwid=0, button_height=0;
        while (h && n--) {
          int idx = GetWindowLong(h,GWL_ID);
          if (idx == IDCANCEL || idx == IDOK || idx == IDNO || idx == IDYES) 
          { 
            RECT tr;
            GetClientRect(h,&tr);
            tab[tabsz] = h;
            w[tabsz++] = tr.right - tr.left;
            button_height = tr.bottom-tr.top;
            bxwid += tr.right-tr.left;
          } else if (idx==0x100) lbl=h;
          h = GetWindow(h,GW_HWNDNEXT);
        }
        if (lbl) SetWindowPos(h,NULL,8,0,r.right,r.bottom - 8 - button_height,  SWP_NOZORDER|SWP_NOACTIVATE);
        int xo = r.right/2 - (bxwid + (tabsz-1)*button_spacing)/2,x;
        for (x=0; x < tabsz; x++)
        {
          SetWindowPos(tab[x],NULL,xo,r.bottom - button_height - 8, 0,0, SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
          xo += w[x] + button_spacing;
        }
      }
    break;
    case WM_COMMAND:
      if (LOWORD(wParam) && HIWORD(wParam) == BN_CLICKED ) EndDialog(hwnd,LOWORD(wParam));
    break;
    case WM_CLOSE:
      if (GetDlgItem(hwnd,IDCANCEL)) EndDialog(hwnd,IDCANCEL);
      else if (GetDlgItem(hwnd,IDNO)) EndDialog(hwnd,IDNO);
      else if (GetDlgItem(hwnd,IDYES)) EndDialog(hwnd,IDYES);
      else EndDialog(hwnd,IDOK);
    break;
  }
  return 0;
}

int MessageBox(HWND hwndParent, const char *text, const char *caption, int type)
{
  printf("MessageBox: %s %s\n",text,caption);
  const void *parms[3]= {text,caption,(void*)(INT_PTR)type} ;
  return DialogBoxParam(NULL,NULL,NULL,swellMessageBoxProc,(LPARAM)parms);

#if 0
  int ret=0;
  
  if (type == MB_OK)
  {
    // todo
    ret=IDOK;
  }	
  else if (type == MB_OKCANCEL)
  {
    ret = 1; // todo
    if (ret) ret=IDOK;
    else ret=IDCANCEL;
  }
  else if (type == MB_YESNO)
  {
    ret = 1 ; // todo
    if (ret) ret=IDYES;
    else ret=IDNO;
  }
  else if (type == MB_RETRYCANCEL)
  {
    ret = 1; // todo

    if (ret) ret=IDRETRY;
    else ret=IDCANCEL;
  }
  else if (type == MB_YESNOCANCEL)
  {
    ret = 1; // todo

    if (ret == 1) ret=IDYES;
    else if (ret==-1) ret=IDNO;
    else ret=IDCANCEL;
  }
  
  return ret; 
#endif
}

#endif
