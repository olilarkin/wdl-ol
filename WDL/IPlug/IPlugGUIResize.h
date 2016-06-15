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

struct viewContainer
{
	vector <int> view_mode;
	vector <int> view_width;
	vector <int> view_height;
	vector <double> min_window_width_normalized; 
	vector <double> min_window_height_normalized;
	vector <double> max_window_width_normalized;
	vector <double> max_window_height_normalized;
};

struct layoutContainer
{
	vector <IControl*> org_pointer;
	vector <DRECT> org_draw_area;
	vector <DRECT> org_target_area;
	vector <int> org_is_hidden;
};

typedef enum _resizeFlag { drawAndTargetArea, drawAreaOnly, targetAreaOnly } resizeFlag;
typedef enum _resizeOneSide { justHorisontalResizing, justVerticalResizing, horisontalAndVerticalResizing } resizeOneSide;

static bool plugin_resized = false;
static bool bitmaps_rescaled_at_load = false;
static double global_gui_scale_ratio = 1.0;
static vector <layoutContainer> global_layout_container;


class IPlugGUIResize : public IControl
{
public:
	IPlugGUIResize(IPlugBase *pPlug, IGraphics *pGraphics, bool useHandle = true, int controlSize = 0, int minimumControlSize = 10);
	~IPlugGUIResize(){}

	
	// These must be called in your plugin constructor ---------------------------------------------------------------------------------------------
	void UsingBitmaps();
	void DisableFastBitmapResizing();
	void SmoothResizedBitmaps();
	void AddNewView(int viewMode, int viewWidth, int viewHeight);
	void UseOneSideResizing(int handleSize, int minHandleSize = 5, resizeOneSide flag = horisontalAndVerticalResizing);
	// ---------------------------------------------------------------------------------------------------------------------------------------------


	
	// These can be called from your custom controls -----------------------------------------------------------------------------------------------
	void UseHandleForGUIScaling(bool statement = false);

	void EnableOneSideResizing(resizeOneSide flag = horisontalAndVerticalResizing);
	void DisableOneSideResizing(resizeOneSide flag = horisontalAndVerticalResizing);

	void SelectViewMode(int viewMode);
	void SetGUIScaleRatio(double guiScaleRatio);
	void SetWindowSize(double width, double height);
	void SetWindowWidth(double width);
	void SetWindowHeight(double height);
	void SetGUIScaleLimits(double minSizeInPercentage, double maxSizeInPercentage);
	void SetWindowSizeLimits(int viewMode, double minWindowWidth, double minWindowHeight, double maxWindowWidth, double maxWindowHeight);

	// Manipulate controls
	void HideControl(int index);
	void ShowControl(int index);
	void MoveControl(int index, double x, double y, resizeFlag flag = drawAndTargetArea);
	void MoveControlRightEdge(int index, double R, resizeFlag flag = drawAndTargetArea);
	void MoveControlBottomEdge(int index, double B, resizeFlag flag = drawAndTargetArea);
	
	double GetGUIScaleRatio();
	int GetViewMode();
	int GetViewModeSize();
	
	// You can override this to use in your custom resizing control
	virtual void DrawBackgroundAtFastResizing(IGraphics* pGraphics, IRECT *pRECT);
	virtual void DrawReopenPluginInterface(IGraphics* pGraphics, IRECT *pRECT);
	virtual void DrawHandle(IGraphics* pGraphics, IRECT *pRECT);

	// Call this to resize GUI
	void ResizeGraphics();
	// ---------------------------------------------------------------------------------------------------------------------------------------------


	// Used by framework ---------------------------------------------------------------------------------------------------------------------------
	void ResizeAtGUIOpen();
	bool Draw(IGraphics* pGraphics);
	void RescaleBitmapsAtLoad();
	IPlugGUIResize *AttachGUIResize();
	void LiveEditSetLayout(int viewMode, IControl * pControl, IRECT drawRECT, IRECT targetRECT, bool isHidden);
	// ---------------------------------------------------------------------------------------------------------------------------------------------



	// Testing
	void DoPopupMenu()
	{

		IPopupMenu menu;

		IGraphics* gui = mPlug->GetGUI();

		menu.AddItem("Test 1");
		menu.AddItem("Test 2");
		menu.AddSeparator();
		menu.AddItem("Test 3");
		menu.AddItem("Test 4");

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
		
	bool IsDirty();

private:

	// Functions that are used internaly -----------------------------------------------------------------------------------------------
	bool double_equals(double a, double b, double epsilon = 0.0000000001);
	DRECT IRECT_to_DRECT(IRECT * iRECT);
	IRECT DRECT_to_IRECT(DRECT * dRECT);
	IRECT ResizeIRECT(DRECT * old_IRECT, double width_ratio, double height_ratio);

	DRECT* GetLayoutContainerDrawRECT(int viewMode, IControl* pControl)
	{
		int position = FindLayoutPointerPosition(viewMode, pControl);
		return &layout_container[viewMode].org_draw_area[position];
	}

	DRECT* GetLayoutContainerTargetRECT(int viewMode, IControl* pControl)
	{
		int position = FindLayoutPointerPosition(viewMode, pControl);
		return &layout_container[viewMode].org_target_area[position];
	}

	int* GetLayoutContainerIsHidden(int viewMode, IControl* pControl)
	{
		int position = FindLayoutPointerPosition(viewMode, pControl);
		return &layout_container[viewMode].org_is_hidden[position];
	}

	void SetLayoutContainerAt(int viewMode, IControl* pControl, DRECT drawIn, DRECT targetIn, int isHiddenIn)
	{
		int position = FindLayoutPointerPosition(viewMode, pControl);

		layout_container[viewMode].org_draw_area[position] = drawIn;
		layout_container[viewMode].org_target_area[position] = targetIn;
		layout_container[viewMode].org_is_hidden[position] = isHiddenIn;
	}

	int FindLayoutPointerPosition(int viewMode, IControl* pControl)
	{
		for (int i = 0; i < layout_container[0].org_pointer.size(); i++)
		{
			if (pControl == layout_container[viewMode].org_pointer[i]) return i;
		}
		return -1;
	}

	void SetIntToFile(const char *name, int x);
	int GetIntFromFile(const char *name);
	void SetDoubleToFile(const char *name, double x);
	double GetDoubleFromFile(const char *name);

	void ResizeBackground();
	void ResizeControlRects();
	void InitializeGUIControls(IGraphics *pGraphics);
	void ResetControlsVisibility();

	void MoveHandle();

	void OnMouseDrag(int x, int y, int dX, int dY, IMouseMod* pMod);
	void OnMouseOver(int x, int y, IMouseMod* pMod);
	void OnMouseOut();
	void OnMouseDown(int x, int y, IMouseMod* pMod);
	void OnMouseUp(int x, int y, IMouseMod* pMod);
	// ---------------------------------------------------------------------------------------------------------------------------------

	int current_view_mode;

	viewContainer view_container;
	vector <layoutContainer> layout_container;

	vector <bool> controls_visibility;

	bool use_handle = true;
	bool handle_controls_gui_scaling = false;

	// Parameters set
	int viewMode = 0,
		windowWidth = 1,
		windowHeight = 2;

	IGraphics *mGraphics;

	// GUI scale variables
	double gui_scale_ratio = 1.0;
	double min_gui_scale_ratio = 0.0;
	double max_gui_scale_ratio = 1000000000.0;

	// Window size variables
	double window_width_normalized, window_height_normalized;

	// One side resizing variables
	int one_side_handle_size = 0, one_side_handle_min_size = 0;
	bool using_one_size_resize = false;

	int default_gui_width, default_gui_height;
	int plugin_width, plugin_height; // This is current plugin instance width
	int min_control_size, control_size;
	bool mouse_is_down = false;
	bool mouse_is_dragging = false;
	bool gui_should_be_closed = false;
	bool using_bitmaps = false;
	bool fast_bitmap_resizing = true;
	bool bitmaps_rescaled_at_load_skip = false;
	bool presets_loaded = false;
	bool smooth_bitmap_resizing = false;
	double* backup_parameters;
	IRECT gui_resize_area;
	WDL_String settings_ini_path;
	char buf[128]; // temp buffer for writing integers to profile strings
	resizeOneSide one_side_flag;
};


// One side handle classes
// NOTE: Horisontal control position is control size - 2
class HorisontalResizing : public IControl
{
public:
	HorisontalResizing(IPlugBase *pPlug, IGraphics *pGraphics, int width);

	~HorisontalResizing() {}

	bool Draw(IGraphics* pGraphics);
	void OnMouseDown(int x, int y, IMouseMod * pMod);
	void OnMouseDrag(int x, int y, int dX, int dY, IMouseMod * pMod);
	void OnMouseOver(int x, int y, IMouseMod * pMod);
	void OnMouseOut();

private:
	IGraphics* mGraphics;
};

// NOTE: Vertical control position is control size - 3
class VerticalResizing : public IControl
{
public:
	VerticalResizing(IPlugBase *pPlug, IGraphics *pGraphics, int height);

	~VerticalResizing() {}

	bool Draw(IGraphics* pGraphics);
	void OnMouseDown(int x, int y, IMouseMod * pMod);
	void OnMouseDrag(int x, int y, int dX, int dY, IMouseMod * pMod);
	void OnMouseOver(int x, int y, IMouseMod * pMod);
	void OnMouseOut();

private:
	IGraphics* mGraphics;
};

#endif