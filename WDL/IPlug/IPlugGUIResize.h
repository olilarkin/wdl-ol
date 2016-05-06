#ifndef _IPLUGGUIRESIZE_
#define _IPLUGGUIRESIZE_

/*
Youlean - IPlugGUIResize - GUI resizing class

Copyright (C) 2016 and later, Youlean

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
2. This notice may not be removed or altered from any source distribution.

*/


#include <vector>
#include "IGraphics.h"
#include "IControl.h"

using namespace std;

static bool plugin_resized = false;
static int global_width = 0, global_height = 0;

class IPlugGUIResize : public IControl
{
public:
	IPlugGUIResize(IPlugBase *pPlug, IGraphics *pGraphics, int guiWidth, int guiHeight, const char *bundleName, int controlSize, int minimumControlSize = 10)
		: IControl(pPlug, IRECT(0, 0, 0, 0))
	{
		default_gui_width = guiWidth;
		default_gui_height = guiHeight;

		pGraphics->HandleMouseOver(true);

		control_size = controlSize;
		min_control_size = minimumControlSize;

		// Set IPlugGUIResize area
		gui_resize_area.L = pGraphics->Width() - controlSize;
		gui_resize_area.T = pGraphics->Height() - controlSize;
		gui_resize_area.R = pGraphics->Width();
		gui_resize_area.B = pGraphics->Height();

		// Set target and draw area
		mTargetRECT = mRECT = IRECT(gui_resize_area.L, gui_resize_area.T, gui_resize_area.R, gui_resize_area.B);

		// Backup original controls size
		for (int i = 0; i < pGraphics->GetNControls(); i++)
		{
			IControl* pControl = pGraphics->GetControl(i);

			org_draw_area.push_back(*pControl->GetRECT());
			org_target_area.push_back(*pControl->GetTargetRECT());
			org_text_size.push_back(*pControl->GetText());
		}

		// Add IPlugGUIResize control size
		org_draw_area.push_back(gui_resize_area);
		org_target_area.push_back(gui_resize_area);
		IText tmpIText;
		org_text_size.push_back(tmpIText);

		mouse_x = pGraphics->Width();
		mouse_y = pGraphics->Height();

		plugin_width = pGraphics->Width();
		plugin_height = pGraphics->Height();


		// Set settings.ini file path
		pGraphics->AppSupportPath(&settings_ini_path);
		settings_ini_path.Append("/");
		settings_ini_path.Append(bundleName);
		settings_ini_path.Append("/settings.ini");

		// Check if gui size was written in settings.ini, if not write defaults
		if (GetIntFromFile("guiwidth") == negative_int_limit)
		{
			SetIntToFile("guiwidth", pGraphics->Width());
		}
		if (GetIntFromFile("guiheight") == negative_int_limit)
		{
			SetIntToFile("guiheight", pGraphics->Height());
		}

		mPlug->GetParam(2)->InitInt("test", 0, 0, 10000);
	}

	~IPlugGUIResize()
	{
	}

	void HideControl(int index)
	{
		IControl* pControl = GetGUI()->GetControl(index);
		pControl->Hide(true);
	}

	void ShowControl(int index)
	{
		IControl* pControl = GetGUI()->GetControl(index);
		pControl->Hide(false);
	}

	void MoveControl(int index, int x, int y)
	{
		IControl* pControl = GetGUI()->GetControl(index);

		int drawAreaW = pControl->GetRECT()->W();
		int drawAreaH = pControl->GetRECT()->H();

		int targetAreaW = pControl->GetTargetRECT()->W();
		int targetAreaH = pControl->GetTargetRECT()->H();

		IRECT drawArea = IRECT(x, y, x + drawAreaW, y + drawAreaH);
		IRECT targetArea = IRECT(x, y, x + targetAreaW, y + targetAreaH);

		org_draw_area[index] = drawArea;
		pControl->SetDrawArea(drawArea);

		org_target_area[index] = targetArea;
		pControl->SetTargetArea(targetArea);
	}

	void SetViewMode(int viewMode) 
	{
		view_mode = viewMode;
	}

	int GetViewMode()
	{
		return view_mode;
	}

	void UsingBitmaps(bool fastBitmapResizing = true)
	{
		using_bitmaps = true;
		fast_bitmap_resizing = fastBitmapResizing;
	}

	void SetIntToFile(const char *name, int x)
	{
		sprintf(buf, "%u", x);
		WritePrivateProfileString("gui", name, buf, settings_ini_path.Get());
	}

	int GetIntFromFile(const char *name)
	{
		return GetPrivateProfileInt("gui", name, negative_int_limit, settings_ini_path.Get());
	}

	void SetDoubleToFile(const char *name, double x)
	{
		sprintf(buf, "%.15f", x);
		WritePrivateProfileString("gui", name, buf, settings_ini_path.Get());
	}

	double GetDoubleFromFile(const char *name)
	{
		GetPrivateProfileString("gui", name, "0.0", buf, 128, settings_ini_path.Get());
		return atof(buf);
	}

	IRECT ResizeIRECT(IRECT old_IRECT, double width_ratio, double height_ratio)
	{
		int L = (int)((double)old_IRECT.L * width_ratio);
		int T = (int)((double)old_IRECT.T * height_ratio);
		int R = (int)((double)old_IRECT.R * width_ratio);
		int B = (int)((double)old_IRECT.B * height_ratio);

		return IRECT(L, T, R, B);
	}

	void ResizeControlRects()
	{
		// Set new target and draw area
		for (int i = 0; i < GetGUI()->GetNControls(); i++)
		{
			// This updates draw and control rect
			IControl* pControl = GetGUI()->GetControl(i);
			pControl->SetDrawArea(ResizeIRECT(org_draw_area[i], scale_ratio, scale_ratio));
			pControl->SetTargetArea(ResizeIRECT(org_target_area[i], scale_ratio, scale_ratio));

			// This updates IText size
			IText tmpText = IText((int)((double)org_text_size[i].mSize * scale_ratio), &org_text_size[i].mColor,
				org_text_size[i].mFont, org_text_size[i].mStyle, org_text_size[i].mAlign, org_text_size[i].mOrientation,
				org_text_size[i].mQuality, &org_text_size[i].mTextEntryBGColor, &org_text_size[i].mTextEntryFGColor);

			pControl->SetText(&tmpText);
		}

		// Keeps control rect uniform and prevent it to go below specified size
		int minSize = IPMIN(mRECT.W(), mRECT.H());
		if (mRECT.W() != mRECT.H())
		{
			mTargetRECT = mRECT = IRECT(mRECT.R - minSize, mRECT.B - minSize, mRECT.R, mRECT.B);
		}

		if (minSize < min_control_size)
		{
			mTargetRECT = mRECT = IRECT(mRECT.R - min_control_size, mRECT.B - min_control_size, mRECT.R, mRECT.B);
		}

	}

	void InitializeGUIControls()
	{
		// Call GUI initializer
		for (int i = 0; i < GetGUI()->GetNControls(); i++)
		{
			IControl* pControl = GetGUI()->GetControl(i);
			pControl->InitializeGUI(scale_ratio);
		}
		GetGUI()->SetAllControlsDirty();
	}

	void ResizeAtGUIOpen()
	{
		int w = GetIntFromFile("guiwidth");
		int h = GetIntFromFile("guiheight");

		scale_ratio = (double)w / (double)default_gui_width;
		scale_ratio = (double)h / (double)default_gui_height;

		global_width = w;
		global_height = h;

		// Prevent resizing if it is not needed
		if (w != plugin_width || h != plugin_height)
		{
			if (using_bitmaps)
			{
				GetGUI()->RescaleBitmaps(w, h, scale_ratio);
			}

			ResizeControlRects();
			GetGUI()->Resize(w, h);

			plugin_width = global_width;
			plugin_height = global_height;

			plugin_resized = true;

			gui_should_be_closed = false;
		}

		InitializeGUIControls();
	}

	void ResizeGraphics()
	{
		SetIntToFile("guiwidth", mouse_x);
		SetIntToFile("guiheight", mouse_y);

		scale_ratio = (double)mouse_x / (double)default_gui_width;
		scale_ratio = (double)mouse_y / (double)default_gui_height;


		if (using_bitmaps)
		{
			if (!fast_bitmap_resizing)
			{
				GetGUI()->RescaleBitmaps(mouse_x, mouse_y, scale_ratio);
				ResizeControlRects();
				InitializeGUIControls();
				GetGUI()->Resize(mouse_x, mouse_y);
			}
			else
			{
				InitializeGUIControls();
				GetGUI()->Resize(mouse_x, mouse_y);
			}
		}
		else
		{
			ResizeControlRects();
			InitializeGUIControls();
			GetGUI()->Resize(mouse_x, mouse_y);
		}


		global_width = mouse_x;
		global_height = mouse_y;

		plugin_width = mouse_x;
		plugin_height = mouse_y;

		plugin_resized = true;

		mPlug->SetParameterFromGUI(2, 0.77);
	}

	void OnMouseDrag(int x, int y, int dX, int dY, IMouseMod* pMod)
	{
		if (!gui_should_be_closed)
		{
			SetCursor(LoadCursor(NULL, IDC_SIZENWSE));

			// Resize window uniform or not
			// Scaling will always be uniform
			if (true)
			{
				mouse_x = (x + y) / 2;
				mouse_y = (x + y) / 2;
			}
			else
			{
				mouse_x = x;
				mouse_y = y;
			}

			if (GetGUI()->Width() != mouse_x || GetGUI()->Height() != mouse_y)
			{
				ResizeGraphics();
			}
			mouse_is_dragging = true;
		}

		if (using_bitmaps && fast_bitmap_resizing)
		{
			mTargetRECT = mRECT = IRECT(0, 0, mouse_x, mouse_y);
		}
	}

	void OnMouseOver(int x, int y, IMouseMod* pMod)
	{
		if (!gui_should_be_closed)
			SetCursor(LoadCursor(NULL, IDC_SIZENWSE));
	}

	void OnMouseOut()
	{
		SetCursor(LoadCursor(NULL, IDC_ARROW));
	}

	void OnMouseDown(int x, int y, IMouseMod* pMod)
	{
		if (!gui_should_be_closed)
			SetCursor(LoadCursor(NULL, IDC_SIZENWSE));

		if (using_bitmaps && fast_bitmap_resizing)
		{
			mTargetRECT = mRECT = IRECT(0, 0, plugin_width, plugin_height);

			mouse_is_down = true;
		}

		mouse_x = plugin_width;
		mouse_y = plugin_height;
	}

	void OnMouseUp(int x, int y, IMouseMod* pMod)
	{
		SetCursor(LoadCursor(NULL, IDC_ARROW));

		if (using_bitmaps && fast_bitmap_resizing)
		{
			GetGUI()->RescaleBitmaps(plugin_width, plugin_height, scale_ratio);
			ResizeControlRects();
			InitializeGUIControls();
			mouse_is_down = false;

			GetGUI()->SetAllControlsDirty();
		}
	}

	bool Draw(IGraphics* pGraphics)
	{
		if (mouse_is_down)
		{
			IRECT backgroundRECT = IRECT(0, 0, mouse_x, mouse_y);
			IColor backgroundColor = IColor(255, 25, 25, 25);
			pGraphics->FillIRect(&backgroundColor, &backgroundRECT);

		}
		else
		{
			if ((global_width != plugin_width || global_height != plugin_height) && using_bitmaps)
			{
				gui_should_be_closed = true;

				mTargetRECT = mRECT = IRECT(0, 0, plugin_width, plugin_height);
				int textSize = 48;
				IColor backgroundColor = IColor(255, 25, 25, 25);
				IColor textColor = IColor(255, 255, 255, 255);
				IRECT textPosition = IRECT(0, (plugin_height / 2) - (int)((double)textSize * scale_ratio), mRECT.R, mRECT.B);
				IText textProps = IText((int)((double)textSize * scale_ratio), &textColor, "Arial", IText::kStyleItalic, IText::kAlignNear);

				pGraphics->FillIRect(&backgroundColor, &mRECT);
				pGraphics->DrawIText(&textProps, "  Reopen plugin interface to get new size...", &textPosition);
			}
			else
			{
				// Draw triangle handle for resizing
				IColor lineColor = IColor(255, 255, 255, 255);
				double gradient = ((double)lineColor.A / 255.0) / (double)mRECT.W();

				for (int i = 0; i < mRECT.W(); i++)
				{
					double alpha = gradient * (double)(mRECT.W() - i);
					alpha = alpha * alpha;
					alpha = 1 - alpha;

					LICE_Line(pGraphics->GetDrawBitmap(), mRECT.L + i, mRECT.B, mRECT.R, mRECT.B - mRECT.W() + i, LICE_RGBA(lineColor.R, lineColor.G, lineColor.B, 255), (float)alpha);
				}

			}
		}

		double ttt = mPlug->GetParam(2)->Value();
		sprintf(buf, "%u", (int)ttt);

		int textSize = 48;
		IColor textColor = IColor(255, 255, 255, 255);
		IRECT textPosition = IRECT(0, (plugin_height / 2) - (int)((double)textSize * scale_ratio), mRECT.R, mRECT.B);
		IText textProps = IText((int)((double)textSize * scale_ratio), &textColor, "Arial", IText::kStyleItalic, IText::kAlignNear);

		pGraphics->DrawIText(&textProps, buf, &textPosition);
		return false;
	}

	void ResizeOnReset()
	{
		ResizeAtGUIOpen();
	}

	bool IsDirty()
	{
		return plugin_resized;
	}

private:
	vector <IRECT> org_draw_area;
	vector <IRECT> org_target_area;
	vector <IText> org_text_size;
	int view_mode;
	int mouse_x, mouse_y;
	int default_gui_width, default_gui_height;
	int plugin_width, plugin_height; // This is current plugin instance width
	int min_control_size, control_size;
	bool mouse_is_down = false;
	bool mouse_is_dragging = false;
	bool gui_should_be_closed = false;
	bool using_bitmaps = false;
	bool fast_bitmap_resizing = false;
	double* backup_parameters;
	double scale_ratio;
	IRECT gui_resize_area;
	WDL_String settings_ini_path;
	int negative_int_limit = -2147483647;
	char buf[128]; // temp buffer for writing integers to profile strings
	LICE_IBitmap *draggingDisplay;
	IRECT mRECT_backup;
};

#endif