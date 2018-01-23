#pragma once

#include "IControl.h"
#include "wdlstring.h"

#define SLIDER_HANDLE_WIDTH 5

struct AGTab 
{
  IRECT mRECT;
  WDL_TypedBuf<int> mParamsToMux;
  WDL_String mLabel;
  
  AGTab(IRECT rect, const char* pLabel)
  {
    mRECT = rect;
    mLabel.Set(pLabel);
  }
  
};

class AGPanelTabs : public IControl
{
private:
  WDL_PtrList<AGTab> mTabs;
  IColor mbgcolor, mfgcolor, mOnColor;
  int mActive;
  
public:
  
  AGPanelTabs(IPlugBaseGraphics& plug, IRECT tabsRect, IText& text, const IColor& bgcolor, const IColor& fgcolor, const IColor& onColor)
  : IControl(plug, tabsRect, kNoParameter)
  , mbgcolor(bgcolor)
  , mfgcolor(fgcolor)
  , mOnColor(onColor)
  , mActive(0)
  {
    mDblAsSingleClick = true;
    mText = text;
    mText.mAlign = IText::kAlignCenter;
  }
  
  ~AGPanelTabs()
  {
    mTabs.Empty(true);
  }
  
  void AddTab(AGTab* tab)
  {
    mTabs.Add(tab);
  }
  
  void OnMouseWheel(float x, float y, const IMouseMod& mod, float d) override {}
  
  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    int i, n = mTabs.GetSize();
    int hit = -1;
    
    for (i = 0; i < n; ++i) 
    {
      if (mTabs.Get(i)->mRECT.Contains(x, y)) 
      {
        hit = i;
        mValue = (double) i / (double) (n - 1);
        
        for (int t = 0; t < n; t++) 
        {
          if (t == i) 
          {
            for (int p = 0; p < mTabs.Get(t)->mParamsToMux.GetSize(); p++) 
            {
              mPlug.GetGUI()->HideControl(mTabs.Get(t)->mParamsToMux.Get()[p], false);
            }
          }
          else 
          {
            for (int p = 0; p < mTabs.Get(t)->mParamsToMux.GetSize(); p++) 
            {
              mPlug.GetGUI()->HideControl(mTabs.Get(t)->mParamsToMux.Get()[p], true);
            }
          }
          
        }
        
        break;
      }
    }
    
    if (hit != -1) 
    {
      mActive = hit;
    }
    
    SetDirty();
  }
  
  void Draw(IGraphics& graphics) override
  {
    for (int t = 0; t < mTabs.GetSize(); t++) 
    {
      if (t == mActive) {
        graphics.FillRect(mOnColor, mTabs.Get(t)->mRECT);
      }
      graphics.DrawRect(mfgcolor, mTabs.Get(t)->mRECT);
      graphics.DrawText(mText, mTabs.Get(t)->mLabel.Get(), mTabs.Get(t)->mRECT);
    }
  }
};

class AGHSliderControl: public IControl
{
public:
  AGHSliderControl(IPlugBaseGraphics& plug,
                   IRECT rect,
                   int paramIdx,
                   IText& text,
                   const IColor &bgcolor,
                   const IColor &fgcolor,
                   int paramNameWidth,
                   int paramValWidth)
  : IControl(plug, rect, paramIdx)
  , mbgcolor(bgcolor)
  , mfgcolor(fgcolor)
  {
    mText = text;
    mText.mAlign = IText::kAlignNear;
    mDisablePrompt = false;
    
    mParamNameRECT =  IRECT(mRECT.L, mRECT.T, mRECT.L + paramNameWidth, mRECT.B);
    mParamValueRECT = IRECT(mRECT.R - paramValWidth, mRECT.T, mRECT.R, mRECT.B);
    mSliderRECT =     IRECT(mParamNameRECT.R + 2, mRECT.T, mParamValueRECT.L - 2, mRECT.B);
    mTextEntryRect =  mParamValueRECT;//IRECT(mParamValueRECT.L+3, mParamValueRECT.T+3, mParamValueRECT.R, mParamValueRECT.B-3);
    mBlend = IBlend::kBlendNone;
    
    mParamNameStr.Set(mPlug.GetParam(mParamIdx)->GetNameForHost());
  }
  
  void Draw(IGraphics& graphics) override
  {
    //graphics.RoundRect(&mfgcolor, &mRECT, &mBlend, 2, true);

    graphics.DrawText(mText, mParamNameStr.Get(), mParamNameRECT);
    
    // Draw Slider track
    graphics.DrawLine(mfgcolor, (float) mSliderRECT.L, (float) mSliderRECT.MH(), (float) mSliderRECT.R, (float) mSliderRECT.MH(), &mBlend, false);
    
    // Draw Slider handle
    float xPos = mValue * (mSliderRECT.W() - (SLIDER_HANDLE_WIDTH-1));
  
    IRECT sliderHandleRect = IRECT(mSliderRECT.L + xPos, mRECT.T+4, mSliderRECT.L + xPos + SLIDER_HANDLE_WIDTH, mRECT.B-4);
    graphics.FillRoundRect(mfgcolor, sliderHandleRect, &mBlend, 2, true);

    char cstr[32];    
    mPlug.GetParam(mParamIdx)->GetDisplayForHost(cstr);
    mParamValueStr.Set(cstr);
    mParamValueStr.Append(" ");
    mParamValueStr.Append(mPlug.GetParam(mParamIdx)->GetLabelForHost());
    graphics.DrawText(mText, mParamValueStr.Get(), mParamValueRECT);
  }
  
  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    if (mParamValueRECT.Contains(x, y)) 
    {
      PromptUserInput(mTextEntryRect);
    }
    else SnapToMouse(x, y);      
  }
  
  void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod) override
  {
    SnapToMouse(x, y);
  }
  
private:
  IColor mbgcolor, mfgcolor;
  int mHandleWidth;
  IRECT mSliderRECT;
  IRECT mParamValueRECT;  
  IRECT mParamNameRECT;
  IRECT mUnitRECT;
  IRECT mTextEntryRect;
  WDL_String mParamNameStr, mParamValueStr;
  
  void SnapToMouse(float x, float y)
  {
    if (mSliderRECT.Contains(x, mSliderRECT.T+3))
    {
      float xValue = (x-mSliderRECT.L -2.f) / (mSliderRECT.W() - 4.f);
      mValue = BOUNDED(xValue, 0., 1.);
    }
    
    SetDirty(); 
  }
};

class AGKnobControl : public IKnobControl
{
public:
  AGKnobControl(IPlugBaseGraphics& plug,
                IRECT rect,
                int paramIdx,
                IText& text,
                const IColor& bgcolor,
                const IColor& fgcolor,
                int textHeight)
  :   IKnobControl(plug, rect, paramIdx, kVertical, DEFAULT_GEARING)
  , mbgcolor(bgcolor)
  , mfgcolor(fgcolor)
  {
    mText = text;
    mText.mAlign = IText::kAlignCenter;

    mMinAngle = -0.75f * float(PI);
    mMaxAngle = 0.75f * float(PI);
    
    mDisablePrompt = false;
    
    mParamNameRECT =  IRECT(mRECT.L, mRECT.T, mRECT.R, mRECT.T + textHeight);
    mParamValueRECT = IRECT(mRECT.L, mRECT.B - textHeight, mRECT.R, mRECT.B);
    mKnobRECT =       IRECT(mRECT.L, mParamNameRECT.B, mRECT.R, mParamValueRECT.T);

    //mUnitRECT =       IRECT(mRECT.R - unitWidth, mRECT.T, mRECT.R, mRECT.B);
    mTextEntryRect =  IRECT(mParamValueRECT.L+2, mParamValueRECT.T+3, mParamValueRECT.R - 2, mParamValueRECT.B-3);
    
    mInnerRadius = 0.;
    mOuterRadius = 0.5f * (float) mKnobRECT.H();
    
    mBlend = IBlend(IBlend::kBlendNone);
    
    mParamNameStr.Set(mPlug.GetParam(mParamIdx)->GetNameForHost());
  }
  
  ~AGKnobControl() {}
  
  void Draw(IGraphics& graphics) override
  {
    graphics.DrawRoundRect(mfgcolor, mRECT, &mBlend, 2, true);

    // Draw Param Name
    graphics.DrawText(mText, mParamNameStr.Get(), mParamNameRECT);

    // Draw Knob
    double v = mMinAngle + mValue * (mMaxAngle - mMinAngle);
    float sinV = (float) sin(v);
    float cosV = (float) cos(v);
    float cx = mKnobRECT.MW(), cy = mKnobRECT.MH();
    float x1 = cx + mInnerRadius * sinV, y1 = cy - mInnerRadius * cosV;
    float x2 = cx + (mOuterRadius) * sinV, y2 = cy - (mOuterRadius) * cosV;

    graphics.FillCircle(mbgcolor, (int) cx, (int) cy, mOuterRadius - 5, &mBlend, true);
    graphics.DrawArc(mfgcolor, cx, cy, mOuterRadius, mMinAngle, mMaxAngle, &mBlend, true);
    graphics.DrawArc(mfgcolor, cx, cy, mOuterRadius+1, mMinAngle, mMaxAngle, &mBlend, true);

    graphics.DrawLine(mfgcolor, x1, y1, x2, y2, &mBlend, true);
    
    if (fabs(x2-x1) > fabs(y2-y1))
    {
      ++y1;
      ++y2;
    }
    else
    {
      ++x1;
      ++x2;
    }
    
    // thicken line
    graphics.DrawLine(mfgcolor, x1, y1, x2, y2, &mBlend, true);
    
    char cstr[32];    
    mPlug.GetParam(mParamIdx)->GetDisplayForHost(cstr);
    mParamValueStr.Set(cstr);
    mParamValueStr.Append(" ");
    mParamValueStr.Append(mPlug.GetParam(mParamIdx)->GetLabelForHost());
    graphics.DrawText(mText, mParamValueStr.Get(), mParamValueRECT);
  }
  
  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    if (mParamValueRECT.Contains(x, y)) 
    {
      PromptUserInput(mTextEntryRect);
    }
  }
  
private:
  IColor mbgcolor, mfgcolor;
  float mMinAngle, mMaxAngle, mInnerRadius, mOuterRadius;
  
  IRECT mKnobRECT;
  IRECT mParamValueRECT;  
  IRECT mParamNameRECT;
  IRECT mUnitRECT;
  IRECT mTextEntryRect;
  
  WDL_String mParamNameStr, mParamValueStr;
};

class AGPresetSaveButtonControl : public IPanelControl
{
private:
  const char** mParamNameStrings;
  
public:
  AGPresetSaveButtonControl(IPlugBaseGraphics& plug, IRECT rect, IText& text, const char** ppParamNameStrings)
  : IPanelControl(plug, rect, COLOR_RED)
  , mParamNameStrings(ppParamNameStrings)
  {
    mText = text;
    mText.mAlign = IText::kAlignCenter;
  }
  
  void OnMouseDown(float x, float y, IMouseMod& mod)
  {
    WDL_String presetFilePath, desktopPath;
    
    mPlug.GetGUI()->DesktopPath(desktopPath);
    mPlug.GetGUI()->PromptForFile(presetFilePath, desktopPath, kFileSave, "txt");
    
    if (strcmp(presetFilePath.Get(), "") != 0) {
      mPlug.DumpPresetSrcCode(presetFilePath.Get(), mParamNameStrings);
    }
  }
  
  void Draw(IGraphics& graphics) override
  {
    graphics.FillRect(mColor, mRECT);
    graphics.DrawText(mText, "Dump preset", mRECT);
  }
};

#define WIDTH 48
#define HEIGHT 50
#define GAP 2

void GenerateKnobGUI(IGraphics& graphics,
                     IPlugBaseGraphics& plug,
                     IText& text,
                     const IColor& bgcolor,
                     const IColor& fgcolor,
                     int minWidth,
                     int minHeight)
{
  graphics.AttachPanelBackground(bgcolor);
  
  const int w = graphics.Width();
  
  // Calculate max bounds
  WDL_String tmtext;
  IRECT paramNameMaxBounds;
  IRECT paramValueMaxBounds;
  
  for(int p = 0; p < plug.NParams(); p++)
  {
    IRECT thisParamNameMaxBounds;
    tmtext.Set(plug.GetParam(p)->GetNameForHost());
    graphics.MeasureText(text, tmtext.Get(), thisParamNameMaxBounds);
    paramNameMaxBounds = paramNameMaxBounds.Union(thisParamNameMaxBounds);
    
    // hope that the display texts are longer than normal values for double params etc
    // TODO: account for length of normal param values
    for(int dt = 0; dt < plug.GetParam(p)->GetNDisplayTexts(); dt++)
    {
      IRECT thisParamValueMaxBounds;
      tmtext.Set(plug.GetParam(p)->GetDisplayTextAtIdx(dt));
      graphics.MeasureText(text, tmtext.Get(), thisParamValueMaxBounds);
      paramValueMaxBounds = paramValueMaxBounds.Union(thisParamValueMaxBounds);
    }
  }

  paramNameMaxBounds = paramNameMaxBounds.Union(paramValueMaxBounds);
  
  int width = std::max(paramNameMaxBounds.W(), float(minWidth));
  
  width = (width % 2 == 0) ? width : (width + 1); // make sure it's an even number, otherwise LICE draw errors
  
  int height = std::max(paramNameMaxBounds.H(), float(minHeight));
  int row = 0;
  int column = 0;
  int xoffs = 2;
  
  for(int p = 0; p < plug.NParams(); p++)
  {
    if ((((width + GAP) * column) + 2) + width >= w) 
    {
      column = 0;
      row++;
      xoffs = 2;
    }

    xoffs = ((width + GAP) * column++) + 2;

    int yoffs = ((height + GAP) * row) + 2;
    
    IRECT paramRect = IRECT(xoffs, yoffs, xoffs+width, yoffs + height);
    
    switch (plug.GetParam(p)->Type())
    {
      case IParam::kTypeBool:
        graphics.AttachControl(new AGKnobControl(plug, paramRect, p, text, bgcolor, fgcolor, paramNameMaxBounds.H()));
        break;
      case IParam::kTypeInt:
        graphics.AttachControl(new AGKnobControl(plug, paramRect, p, text, bgcolor, fgcolor, paramNameMaxBounds.H()));
        break;
      case IParam::kTypeEnum:
        graphics.AttachControl(new AGKnobControl(plug, paramRect, p, text, bgcolor, fgcolor, paramNameMaxBounds.H()));
        break;
      case IParam::kTypeDouble:
        graphics.AttachControl(new AGKnobControl(plug, paramRect, p, text, bgcolor, fgcolor, paramNameMaxBounds.H()));
        break;
      default:
        break;
    }
  }
}

void GenerateSliderGUI(IGraphics& graphics, IPlug& plug, IText& text, const IColor& bgcolor, const IColor& fgcolor, int colWidth = 300, int tabs = 0, const char** pParamNameStrings = 0)
{
  graphics.AttachPanelBackground(bgcolor);

  WDL_PtrList<const char> groupNames;
  WDL_String thisGroup("");
  
  // Calculate max bounds
  WDL_String tmtext;
  IRECT paramNameMaxBounds;
  IRECT paramValueMaxBounds = IRECT(0, 0, 70, 10);  // the values here are a hack to make a minimum bounds 
  
  for(int p = 0; p < plug.NParams(); p++)
  {
    IRECT thisParamNameMaxBounds;
    tmtext.Set(plug.GetParam(p)->GetNameForHost());
    graphics.MeasureText(text, tmtext.Get(), thisParamNameMaxBounds);
    paramNameMaxBounds = paramNameMaxBounds.Union(thisParamNameMaxBounds);
    
    // hope that the display texts are longer than normal values for double params etc
    // TODO: account for length of normal param values
    for(int dt = 0; dt < plug.GetParam(p)->GetNDisplayTexts(); dt++)
    {
      IRECT thisParamValueMaxBounds;
      tmtext.Set(plug.GetParam(p)->GetDisplayTextAtIdx(dt));
      graphics.MeasureText(text, tmtext.Get(), thisParamValueMaxBounds);
      paramValueMaxBounds = paramValueMaxBounds.Union(thisParamValueMaxBounds);
    }
    
    const char* label = plug.GetParam(p)->GetParamGroupForHost();
    
    if (strcmp(label, thisGroup.Get()) != 0) 
    {
      groupNames.Add(label);
      thisGroup.Set(label);
    }
  }
  
  //printf("%i groups\n", groupNames.GetSize());
  
  int yoffs = 2;
  int row = 0;
  int col = 0;
  
  if (pParamNameStrings) 
  {
    IRECT buttonsRect = IRECT(2, yoffs, colWidth-2, yoffs + paramNameMaxBounds.H());
    
    graphics.AttachControl(new AGPresetSaveButtonControl(plug, buttonsRect, text, pParamNameStrings));
    
    yoffs += 20;
  }

  AGPanelTabs* pTabsControl = 0;
  IRECT tabsRect = IRECT(2, yoffs, colWidth-2, yoffs + paramNameMaxBounds.H());
  
  if (tabs) 
  {
    pTabsControl = new AGPanelTabs(plug, tabsRect, text, bgcolor, fgcolor, COLOR_RED);
    graphics.AttachControl(pTabsControl);
    yoffs += 20;
  }
  
  AGTab* pTab = 0;
  thisGroup.Set("");
  IRECT thisTabRect;
  int groupIdx = 0;
  char buf[32];
  
  int paramStartYoffs = yoffs;
  
  for(int p = 0; p < plug.NParams(); p++)
  {
    if (tabs && groupNames.GetSize()) 
    {
      const char* label = plug.GetParam(p)->GetParamGroupForHost();

      if (strcmp(label, thisGroup.Get()) != 0) 
      {
        thisTabRect = tabsRect.SubRectHorizontal(groupNames.GetSize(), groupIdx);
        thisGroup.Set(label);
        if (tabs == 1)
        {
          sprintf(buf, "%i", groupIdx+1);
        }
        else {
          strcpy(buf, label);
        }
        pTab = new AGTab(thisTabRect, buf);
        pTabsControl->AddTab(pTab);
        groupIdx++;
        
        col = 0;
        yoffs = paramStartYoffs;
      }
      
      pTab->mParamsToMux.Add(p);
    }

    IRECT paramRect = IRECT(2 + (col * colWidth), yoffs, (col+1) * colWidth, yoffs + paramNameMaxBounds.H());
    
//    switch (plug.GetParam(p)->Type())
//    {
//      case IParam::kTypeBool:
//        graphics.AttachControl(new AGHSliderControl(plug, paramRect, p, text, bgcolor, fgcolor, paramNameMaxBounds.W(), paramValueMaxBounds.W()));
//        break;
//      case IParam::kTypeInt:
//        graphics.AttachControl(new AGHSliderControl(plug, paramRect, p, text, bgcolor, fgcolor, paramNameMaxBounds.W(), paramValueMaxBounds.W()));
//        break;
//      case IParam::kTypeEnum:
//        graphics.AttachControl(new AGHSliderControl(plug, paramRect, p, text, bgcolor, fgcolor, paramNameMaxBounds.W(), paramValueMaxBounds.W()));
//        break;
//      case IParam::kTypeDouble:
//        graphics.AttachControl(new AGHSliderControl(plug, paramRect, p, text, bgcolor, fgcolor, paramNameMaxBounds.W(), paramValueMaxBounds.W()));
//        break;
//      default:
//        break;
//    }
    
    IControl* pControl = new AGHSliderControl(plug, paramRect, p, text, bgcolor, fgcolor, paramNameMaxBounds.W(), paramValueMaxBounds.W());
    graphics.AttachControl(pControl);
    
    if (tabs && groupIdx != 1) 
    {
      pControl->Hide(true);
    }
    
    if (yoffs + paramNameMaxBounds.H() >= graphics.Height() - 5)
    {
      col++;
      yoffs = 2;
      row = 0;
    }
    else 
    {
      yoffs += paramNameMaxBounds.H();
    }
  }
}
