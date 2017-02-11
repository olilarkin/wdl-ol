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
#include <math.h> 
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
	vector <IControl*> moved_pointer;
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
	void AddNewView(int viewMode, int viewWidth, int viewHeight);
	void UseOneSideResizing(int handleSize, int minHandleSize = 5, resizeOneSide flag = horisontalAndVerticalResizing);
	// ---------------------------------------------------------------------------------------------------------------------------------------------


	
	// These can be called from your custom controls -----------------------------------------------------------------------------------------------
	void UseHandleForGUIScaling(bool statement = false);
	void UseControlAndClickOnHandleForGUIScaling(bool statement = false);

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

	void MoveControlRelativeToControlDrawRect(int moveControlIndex, int relativeToControlIndex, double xRatio, double yRatio, resizeFlag flag = drawAndTargetArea);
	void MoveControlRelativeToControlDrawRect(IControl * moveControl, IControl * relativeToControl, double xRatio, double yRatio, resizeFlag flag = drawAndTargetArea);
	void MoveControlHorizontallyRelativeToControlDrawRect(int moveControlIndex, int relativeToControlIndex, double xRatio, resizeFlag flag = drawAndTargetArea);
	void MoveControlHorizontallyRelativeToControlDrawRect(IControl * moveControl, IControl * relativeToControl, double xRatio, resizeFlag flag = drawAndTargetArea);
	void MoveControlVerticallyRelativeToControlDrawRect(int moveControlIndex, int relativeToControlIndex, double yRatio, resizeFlag flag = drawAndTargetArea);
	void MoveControlVerticallyRelativeToControlDrawRect(IControl * moveControl, IControl * relativeToControl, double yRatio, resizeFlag flag = drawAndTargetArea);

	void MoveControlRelativeToNonScaledDRECT(int index, DRECT relativeTo, double xRatio, double yRatio, resizeFlag flag = drawAndTargetArea);
	void MoveControlRelativeToNonScaledDRECT(IControl * pControl, DRECT relativeTo, double xRatio, double yRatio, resizeFlag flag = drawAndTargetArea);
	void MoveControlHorizontallyRelativeToNonScaledDRECT(int index, DRECT relativeTo, double xRatio, resizeFlag flag = drawAndTargetArea);
	void MoveControlHorizontallyRelativeToNonScaledDRECT(IControl * pControl, DRECT relativeTo, double xRatio, resizeFlag flag = drawAndTargetArea);
	void MoveControlVerticallyRelativeToNonScaledDRECT(int index, DRECT relativeTo, double yRatio, resizeFlag flag = drawAndTargetArea);
	void MoveControlVerticallyRelativeToNonScaledDRECT(IControl * pControl, DRECT relativeTo, double yRatio, resizeFlag flag = drawAndTargetArea);
	
	void MoveControl(int index, double x, double y, resizeFlag flag = drawAndTargetArea);
	void MoveControl(IControl * pControl, double x, double y, resizeFlag flag);
	void MoveControlHorizontally(int index, double x, resizeFlag flag);
	void MoveControlHorizontally(IControl * pControl, double x, resizeFlag flag);
	void MoveControlVertically(int index, double y, resizeFlag flag);
	void MoveControlVertically(IControl * pControl, double y, resizeFlag flag);

	void MoveControlTopEdge(int index, double T, resizeFlag flag = drawAndTargetArea);
	void MoveControlTopEdge(IControl * pControl, double T, resizeFlag flag);
	void MoveControlLeftEdge(int index, double L, resizeFlag flag = drawAndTargetArea);
	void MoveControlLeftEdge(IControl * pControl, double L, resizeFlag flag);
	void MoveControlRightEdge(int index, double R, resizeFlag flag = drawAndTargetArea);
	void MoveControlRightEdge(IControl * pControl, double R, resizeFlag flag);
	void MoveControlBottomEdge(int index, double B, resizeFlag flag = drawAndTargetArea);
	void MoveControlBottomEdge(IControl * pControl, double B, resizeFlag flag);

	void SetNormalizedDrawRect(int index, double L, double T, double R, double B);
	void SetNormalizedDrawRect(IControl *pControl, double L, double T, double R, double B);
	void SetNormalizedDrawRect(int index, DRECT r);
	void SetNormalizedDrawRect(IControl *pControl, DRECT r);

	void SetNormalizedTargetRect(int index, double L, double T, double R, double B);
	void SetNormalizedTargetRect(IControl *pControl, double L, double T, double R, double B);
	void SetNormalizedTargetRect(int index, DRECT r);
	void SetNormalizedTargetRect(IControl *pControl, DRECT r);
	
	// Get values
	double GetGUIScaleRatio();
	int GetViewMode();
	int GetViewModeSize();
	bool CurrentlyFastResizing();
	
	// You can override this to use in your custom resizing control
	virtual void DrawBackgroundAtFastResizing(IGraphics* pGraphics, IRECT *pRECT);
	virtual void DrawReopenPluginInterface(IGraphics* pGraphics, IRECT *pRECT);
	virtual void DrawHandle(IGraphics* pGraphics, IRECT *pRECT);
	virtual void DoPopupMenu() {}

	// Call this to resize GUI
	void ResizeGraphics();
	// ---------------------------------------------------------------------------------------------------------------------------------------------


	// Used by the framework -----------------------------------------------------------------------------------------------------------------------
	IParam* GetGUIResizeParameter(int index);
	int GetGUIResizeParameterSize();
	void ResizeAtGUIOpen();
	bool Draw(IGraphics* pGraphics);
	void RescaleBitmapsAtLoad();
	IPlugGUIResize *AttachGUIResize();
	void LiveEditSetLayout(int viewMode, int moveToIndex, int moveFromIndex, IRECT drawRECT, IRECT targetRECT, bool isHidden);
	void LiveRemoveLayer(IControl* pControl);
	bool IsDirty();
	// ---------------------------------------------------------------------------------------------------------------------------------------------
		
private:
	// Functions that are used internally ----------------------------------------------------------------------------------------------------------
	bool double_equals(double a, double b, double epsilon = 0.0000000001);
	DRECT IRECT_to_DRECT(IRECT * iRECT);
	IRECT DRECT_to_IRECT(DRECT * dRECT);
	IRECT ResizeIRECT(DRECT * old_IRECT, double width_ratio, double height_ratio);
	DRECT* GetLayoutContainerDrawRECT(int viewMode, IControl* pControl);
	DRECT* GetLayoutContainerTargetRECT(int viewMode, IControl* pControl);
	int* GetLayoutContainerIsHidden(int viewMode, IControl* pControl);
	void SetLayoutContainerAt(int viewMode, IControl* pControl, DRECT drawIn, DRECT targetIn, int isHiddenIn);
	int FindLayoutPointerPosition(int viewMode, IControl* pControl);
	void RearrangeLayers();

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
	void OnMouseDblClick(int x, int y, IMouseMod * pMod);
	void OnMouseUp(int x, int y, IMouseMod* pMod);
	// ---------------------------------------------------------------------------------------------------------------------------------------------

	int current_view_mode;

	viewContainer view_container;
	vector <layoutContainer> layout_container;

	vector <bool> controls_visibility;

	bool use_handle = true;
	bool handle_controls_gui_scaling = false;
	bool control_and_click_on_handle_controls_gui_scaling = false;

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
	bool currentlyFastResizing = false;
	bool mouse_is_dragging = false;
	bool gui_should_be_closed = false;
	bool using_bitmaps = false;
	bool fast_bitmap_resizing = true;
	bool bitmaps_rescaled_at_load_skip = false;
	bool presets_loaded = false;
	double* backup_parameters;
	IRECT gui_resize_area;
	WDL_String settings_ini_path;
	char buf[128]; // temp buffer for writing integers to profile strings
	resizeOneSide one_side_flag;
	WDL_PtrList<IParam> guiResizeParameters;

	friend class IPlugGUILiveEdit;
};


// One side handle classes
// NOTE: Horizontal control position is control size - 2
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