#ifndef __IPLUGBETTERGUIRESIZE__
#define __IPLUGBETTERGUIRESIZE__

#include "IPlug_include_in_plug_hdr.h"
#include <time.h>

class IPlugBetterGUIResize : public IPlug
{
public:
  IPlugBetterGUIResize(IPlugInstanceInfo instanceInfo);
  ~IPlugBetterGUIResize();

  void Reset();
  void OnParamChange(int paramIdx);
  void ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames);
  void SetGUILayout(int viewMode, double windowWidth, double windowHeight);

private:
  IGraphics* pGraphics;

  // Get control numbers. Do this to make gui layout easier
  int *background, *redKnob, *grayKnob, *infoText, *customControl;
};

// This is custom GUI resize control that allowes you to customize graphics
class IPlugGUIResizeCustom : public IPlugGUIResize
{
public:
	IPlugGUIResizeCustom(IPlugBase *pPlug, IGraphics *pGraphics, bool useHandle = true, int controlSize = 0, int minimumControlSize = 10)
		: IPlugGUIResize(pPlug, pGraphics, controlSize, minimumControlSize) {}
	~IPlugGUIResizeCustom() {}

	void DrawBackgroundAtFastResizing(IGraphics * pGraphics, IRECT * pRECT)
	{
		IColor backgroundColor = IColor(255, 255, 25, 25);
		pGraphics->FillIRect(&backgroundColor, pRECT);
	}
};

class CustomControl : public IControl
{
public:
	CustomControl(IPlugBase *pPlug, IRECT pR, IColor color)
		: IControl(pPlug, pR), mColor(color) {}

	~CustomControl() {}

	// Initialize GUI size here. This will be called after gui resize
	void InitializeGUI(double guiScaleRatio)
	{
		// We could actually just use mRECT, but this is just to demonstrate this function

		drawRect.L = mRECT.L;
		drawRect.T = mRECT.T;
		drawRect.R = mRECT.R;
		drawRect.B = mRECT.B;
		
	}

	bool Draw(IGraphics* pGraphics)
	{
		pGraphics->FillIRect(&mColor, &drawRect);
		return true;
	}

private:
	IRECT drawRect;
	IColor mColor;
};

class viewSelector : public IControl
{
private:
	WDL_String mStr;
	int view_mode;

public:
	viewSelector(IPlugBase* pPlug, IRECT pR, const char* label, int viewMode)
		: IControl(pPlug, pR)
	{
		view_mode = viewMode;
		mStr.Set(label);
		mText.mColor = COLOR_WHITE;
		mText.mSize = 24;
	}

	~viewSelector() {}

	void InitializeGUI(double guiScaleRatio)
	{
		mText = IText((int)((double)24 * guiScaleRatio), &mText.mColor,
			mText.mFont, mText.mStyle, mText.mAlign, mText.mOrientation,
			mText.mQuality, &mText.mTextEntryBGColor, &mText.mTextEntryFGColor);
	}

	bool Draw(IGraphics* pGraphics)
	{
		pGraphics->FillIRect(&COLOR_GRAY, &mRECT, &mBlend);
		char* cStr = mStr.Get();
		pGraphics->DrawIText(&mText, cStr, &mRECT);
		return true;
	}

	void OnMouseDown(int x, int y, IMouseMod* pMod)
	{
		GetGUIResize()->SelectViewMode(view_mode);
		GetGUIResize()->ResizeGraphics();
	}

};

class handleSelector : public IControl
{
public:
	handleSelector(IPlugBase* pPlug, IRECT pR)
		: IControl(pPlug, pR)
	{
		scaling.Set("guiScaling");
		resize.Set("windowResizing");
		mText.mColor = COLOR_WHITE;
		mText.mSize = 24;
	}

	~handleSelector() {}

	void InitializeGUI(double guiScaleRatio)
	{
		mText = IText((int)((double)24 * guiScaleRatio), &mText.mColor,
			mText.mFont, mText.mStyle, mText.mAlign, mText.mOrientation,
			mText.mQuality, &mText.mTextEntryBGColor, &mText.mTextEntryFGColor);
	}

	bool Draw(IGraphics* pGraphics)
	{
		pGraphics->FillIRect(&COLOR_BLUE, &mRECT, &mBlend);

		//GetAnimation()->DrawAnimationCurve_DEBUG(pGraphics, animationFlag::_BounceEaseOut);

		double animateWidth = GetAnimation()->Animation("animatePosition", button, 0, mRECT.W(), 60, 60, animationFlag::_QuinticEaseInOut, animationFlag::_QuarticEaseInOut);
		double animateHeight = mRECT.H();// = GetAnimation()->Animation("animateHeight", button, 0, mRECT.H(), 10, 10, animationFlag::_CubicEaseIn, animationFlag::_CubicEaseIn);

		pGraphics->FillIRect(&COLOR_BLACK, &IRECT(mRECT.L, mRECT.T, mRECT.L + (int)animateWidth, mRECT.T + (int)animateHeight), &mBlend);


		if (!button)
		{
			pGraphics->DrawIText(&mText, resize.Get(), &mRECT);
		}
		else
		{
			pGraphics->DrawIText(&mText, scaling.Get(), &mRECT);
		}

		// Print selected control
		WDL_String controlNumber;
		controlNumber.SetFormatted(100, "Redraws: %i", redrawTest);
		IText txtControlNumber(17, &COLOR_WHITE);
		txtControlNumber.mAlign = IText::kAlignNear;
		//pGraphics->DrawIText(&txtControlNumber, controlNumber.Get(), &mRECT);
		redrawTest++;
		return true;
	}

	void OnMouseDown(int x, int y, IMouseMod* pMod)
	{
		if (button)
			button = false;
		else
			button = true;
		
		GetGUIResize()->UseHandleForGUIScaling(button);

		SetDirty();
	}

	//bool IsDirty() {  return true; }


private:
	WDL_String scaling;
	WDL_String resize;
	IPlugGUIResize *GUIResize;
	bool button = false;
	int redrawTest = 0;
};
#endif
