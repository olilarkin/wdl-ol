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

/*
////////////////////There are 3 main GUI resize modes:////////////////////////

1. View Resize: 
With this you will be able to set up for example mini view, normal view 
where you will have more knobs etc.

-----------------------------------------------------------------------------

   Normal View:                             Mini View:
-------------------                         ----------
|  Knob1   Knob2  |                         |  Knob1 |
|                 |                         |        |
|                 |                         ----------
-------------------    

-----------------------------------------------------------------------------

2. Window resize:
This will resize window of different view. For example you could have mini 
view and on that mini view you will have some spectrum analyzer, now when
you resize window you can make spectrum analyzer to be resized but the rest
of controls won't be enlarged, and view won't jump to the advanced view.

-----------------------------------------------------------------------------

Normal View + Window Resize:           Mini View + Window Resize:
-------------------////                     ----------////
|  Knob1   Knob2  |////                     |  Knob1 |////
|                 |////                     |        |////
|                 |////                     ----------////
-------------------////                     //////////////
///////////////////////

-----------------------------------------------------------------------------

3. GUI Scaling:
This will take everything you have done with the view and enlarge it. 
This is mainly for monitors with higher resolutions (retina etc.)

-----------------------------------------------------------------------------

    Normal View:               Normal View - Scaled 2X:      
-------------------     -------------------------------------- 
|  Knob1   Knob2  |     |                                    |
|                 |     |        Knob1          Knob2        | 
|                 |     |                                    |
-------------------     |                                    |
                        |                                    |
						|                                    |
						--------------------------------------

-----------------------------------------------------------------------------

*/

#include <vector>
#include "IGraphics.h"
#include "IControl.h"

using namespace std;

struct DRECT
{
	double L, T, R, B;

	DRECT() { L = T = R = B = 0.0; }
	DRECT(double l, double t, double r, double b) : L(l), R(r), T(t), B(b) {}
	inline double W() const { return R - L; }
	inline double H() const { return B - T; }
};

typedef enum _resizeFlag { drawAndTargetArea, drawAreaOnly, targetAreaOnly } resizeFlag;

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

		// Set default view dimensions
		view_container.view_mode.push_back(0);
		view_container.view_width.push_back(guiWidth);
		view_container.view_height.push_back(guiHeight);
		current_view_mode = 0;
		current_view_mode = guiWidth;
		current_view_height = guiHeight;

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

	IPlugGUIResize *Attach(IGraphics *pGraphics)
	{
		// Backup original controls sizes
		for (int i = 0; i < pGraphics->GetNControls(); i++)
		{
			IControl* pControl = pGraphics->GetControl(i);

			org_draw_area.push_back(IRECT_to_DRECT(&*pControl->GetRECT()));
			org_target_area.push_back(IRECT_to_DRECT(&*pControl->GetTargetRECT()));
			org_text_size.push_back(*pControl->GetText());
		}

		// Add IPlugGUIResize control size
		org_draw_area.push_back(IRECT_to_DRECT(&gui_resize_area));
		org_target_area.push_back(IRECT_to_DRECT(&gui_resize_area));
		IText tmpIText;
		org_text_size.push_back(tmpIText);

		InitializeGUIControls(pGraphics);

		return this;
	}

	IRECT DRECT_to_IRECT(DRECT *dRECT)
	{
		return IRECT((int)(dRECT->L), (int)(dRECT->T), (int)(dRECT->R), (int)(dRECT->B));
	}

	DRECT IRECT_to_DRECT(IRECT *iRECT)
	{
		return DRECT((double)(iRECT->L), (double)(iRECT->T), (double)(iRECT->R), (double)(iRECT->B));
	}

	void AddNewView(int viewMode, int viewWidth, int viewHeight)
	{
		view_container.view_mode.push_back(viewMode);
		view_container.view_width.push_back(viewWidth);
		view_container.view_height.push_back(viewHeight);
	}

	void SelectViewMode(int viewMode)
	{
		int position = 0;
		
		for (int i = 0; i < view_container.view_mode.size(); i++)
		{
			if (view_container.view_mode[i] == viewMode)
			{
				position = i;
				break;
			}
		}

		current_view_mode = viewMode;
		current_view_width = view_container.view_width[position];
		current_view_height = view_container.view_height[position];
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

	void MoveControl(int index, double x, double y, resizeFlag flag = drawAndTargetArea)
	{
		IControl* pControl = GetGUI()->GetControl(index);

		double x_relative = x * scale_ratio;
		double y_relative = y * scale_ratio;

		if (flag == drawAndTargetArea || flag == drawAreaOnly)
		{
			double drawAreaW = (double)pControl->GetRECT()->W();
			double drawAreaH = (double)pControl->GetRECT()->H();

			DRECT drawArea = DRECT(x_relative, y_relative, x_relative + drawAreaW, y_relative + drawAreaH);
			pControl->SetDrawArea(DRECT_to_IRECT(&drawArea));

			double org_draw_width = org_draw_area[index].W();
			double org_draw_height = org_draw_area[index].H();

			org_draw_area[index].L = x;
			org_draw_area[index].T = y;
			org_draw_area[index].R = x + org_draw_width;
			org_draw_area[index].B = y + org_draw_height;

		}

		if (flag == drawAndTargetArea || flag == targetAreaOnly)
		{
			double targetAreaW = (double)pControl->GetTargetRECT()->W();
			double targetAreaH = (double)pControl->GetTargetRECT()->H();

			DRECT targetArea = DRECT(x_relative, y_relative, x_relative + targetAreaW, y_relative + targetAreaH);
			pControl->SetTargetArea(DRECT_to_IRECT(&targetArea));

			double org_target_width = org_draw_area[index].W();
			double org_target_height = org_draw_area[index].H();

			org_target_area[index].L = x;
			org_target_area[index].T = y;
			org_target_area[index].R = x + org_target_width;
			org_target_area[index].B = y + org_target_height;

		}
	}

	void MoveControlEnd(int index, double R, double B, resizeFlag flag = drawAndTargetArea) // to do: fix this function
	{
		IControl* pControl = GetGUI()->GetControl(index);

		if (flag == drawAndTargetArea || flag == drawAreaOnly)
		{
			double drawAreaX = (double)pControl->GetRECT()->L;
			double drawAreaY = (double)pControl->GetRECT()->T;

			DRECT drawArea = DRECT(drawAreaX, drawAreaY, R, B);

			org_draw_area[index] = drawArea;
			pControl->SetDrawArea(DRECT_to_IRECT(&drawArea));
		}

		if (flag == drawAndTargetArea || flag == targetAreaOnly)
		{
			double targetAreaX = (double)pControl->GetTargetRECT()->L;
			double targetAreaY = (double)pControl->GetTargetRECT()->T;

			DRECT targetArea = DRECT(targetAreaX, targetAreaY, R, B);

			org_target_area[index] = targetArea;
			pControl->SetTargetArea(DRECT_to_IRECT(&targetArea));
		}
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

	IRECT ResizeIRECT(DRECT *old_IRECT, double width_ratio, double height_ratio)
	{
		return IRECT((int)(old_IRECT->L * width_ratio), (int)(old_IRECT->T * height_ratio), (int)(old_IRECT->R * width_ratio), (int)(old_IRECT->B * height_ratio));
	}

	void ResizeControlRects()
	{
		// Set new target and draw area
		for (int i = 0; i < GetGUI()->GetNControls(); i++)
		{
			// This updates draw and control rect
			IControl* pControl = GetGUI()->GetControl(i);
			pControl->SetDrawArea(ResizeIRECT(&org_draw_area[i], scale_ratio, scale_ratio));
			pControl->SetTargetArea(ResizeIRECT(&org_target_area[i], scale_ratio, scale_ratio));

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

	void InitializeGUIControls(IGraphics *pGraphics)
	{
		// Call GUI initializer
		for (int i = 0; i < pGraphics->GetNControls(); i++)
		{
			IControl* pControl = pGraphics->GetControl(i);
			pControl->InitializeGUI(scale_ratio);
		}
		pGraphics->SetAllControlsDirty();
	}

	void ResizeAtGUIOpen()
	{
		int w = GetIntFromFile("guiwidth");
		int h = GetIntFromFile("guiheight");

		scale_ratio = (double)w / (double)default_gui_width;
		scale_ratio = (double)h / (double)default_gui_height;

        // to do: implement all calls
		mPlug->SetGUILayout(current_view_mode, 800, 800);

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

		InitializeGUIControls(GetGUI());
	}

	void ResizeGraphics()
	{
		SetIntToFile("guiwidth", mouse_x);
		SetIntToFile("guiheight", mouse_y);

		scale_ratio = (double)mouse_x / (double)default_gui_width;
		scale_ratio = (double)mouse_y / (double)default_gui_height;

		// to do: implement all calls
		mPlug->SetGUILayout(current_view_mode, 800, 800);

		if (using_bitmaps)
		{
			if (!fast_bitmap_resizing)
			{
				GetGUI()->RescaleBitmaps(mouse_x, mouse_y, scale_ratio);
				ResizeControlRects();
				InitializeGUIControls(GetGUI());
				GetGUI()->Resize(mouse_x, mouse_y);
			}
			else
			{
				InitializeGUIControls(GetGUI());
				GetGUI()->Resize(mouse_x, mouse_y);
			}
		}
		else
		{
			ResizeControlRects();
			InitializeGUIControls(GetGUI());
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

		if (pMod->L && using_bitmaps && fast_bitmap_resizing)
		{
			mTargetRECT = mRECT = IRECT(0, 0, plugin_width, plugin_height);

			mouse_is_down = true;
		}

		mouse_x = plugin_width;
		mouse_y = plugin_height;

		if (pMod->R)
		{
			doPopupMenu();
		}
	}

	void doPopupMenu()
	{
		IPopupMenu menu;

		IGraphics* gui = mPlug->GetGUI();

		menu.AddItem("Save Program...");
		menu.AddItem("Save Bank...");
		menu.AddSeparator();
		menu.AddItem("Load Program...");
		menu.AddItem("Load Bank...");

		if (gui->CreateIPopupMenu(&menu, &mRECT))
		{
			int itemChosen = menu.GetChosenItemIdx();
			WDL_String fileName;

			//printf("chosen %i /n", itemChosen);
			switch (itemChosen)
			{
			case 0: //Save Program

				break;
			case 1: //Save Bank

				break;
			case 3: //Load Preset

				break;
			case 4: // Load Bank

				break;
			default:
				break;
			}
		}
	}

	void OnMouseUp(int x, int y, IMouseMod* pMod)
	{
		SetCursor(LoadCursor(NULL, IDC_ARROW));

		if (using_bitmaps && fast_bitmap_resizing)
		{
			GetGUI()->RescaleBitmaps(plugin_width, plugin_height, scale_ratio);
		    ResizeControlRects();
			InitializeGUIControls(GetGUI());
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
	struct viewContainer
	{
		vector <int> view_mode;
		vector <int> view_width;
		vector <int> view_height;
	};

	int current_view_mode, current_view_width, current_view_height;

	viewContainer view_container;

	vector <DRECT> org_draw_area;
	vector <DRECT> org_target_area;
	vector <IText> org_text_size;


	double window_width_normalized = 1.0, window_height_normalized = 1.0;
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