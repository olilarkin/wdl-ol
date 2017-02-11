#include "IPlugGUIResize.h"

// Helpers -------------------------------------------------------------------------------------------------------------
bool IPlugGUIResize::double_equals(double a, double b, double epsilon)
{
	return abs(a - b) < epsilon;
}

DRECT IPlugGUIResize::IRECT_to_DRECT(IRECT *iRECT)
{
	return DRECT((double)(iRECT->L), (double)(iRECT->T), (double)(iRECT->R), (double)(iRECT->B));
}

IRECT IPlugGUIResize::DRECT_to_IRECT(DRECT *dRECT)
{
	return IRECT((int)(dRECT->L), (int)(dRECT->T), (int)(dRECT->R), (int)(dRECT->B));
}

IRECT IPlugGUIResize::ResizeIRECT(DRECT *old_IRECT, double width_ratio, double height_ratio)
{
	return IRECT((int)((old_IRECT->L)* width_ratio), (int)((old_IRECT->T) * height_ratio), (int)((old_IRECT->R)* width_ratio), (int)((old_IRECT->B) * height_ratio));
}

DRECT * IPlugGUIResize::GetLayoutContainerDrawRECT(int viewMode, IControl * pControl)
{
	int position = FindLayoutPointerPosition(viewMode, pControl);
	return &layout_container[viewMode].org_draw_area[position];
}

DRECT * IPlugGUIResize::GetLayoutContainerTargetRECT(int viewMode, IControl * pControl)
{
	int position = FindLayoutPointerPosition(viewMode, pControl);
	return &layout_container[viewMode].org_target_area[position];
}

int * IPlugGUIResize::GetLayoutContainerIsHidden(int viewMode, IControl * pControl)
{
	int position = FindLayoutPointerPosition(viewMode, pControl);
	return &layout_container[viewMode].org_is_hidden[position];
}

void IPlugGUIResize::SetLayoutContainerAt(int viewMode, IControl * pControl, DRECT drawIn, DRECT targetIn, int isHiddenIn)
{
	int position = FindLayoutPointerPosition(viewMode, pControl);

	layout_container[viewMode].org_draw_area[position] = drawIn;
	layout_container[viewMode].org_target_area[position] = targetIn;
	layout_container[viewMode].org_is_hidden[position] = isHiddenIn;
}

int IPlugGUIResize::FindLayoutPointerPosition(int viewMode, IControl * pControl)
{
	for (int i = 0; i < layout_container[0].org_pointer.size(); i++)
	{
		if (pControl == layout_container[viewMode].org_pointer[i]) return i;
	}
	return -1;
}

void IPlugGUIResize::RearrangeLayers()
{
	for (int i = 1; i < mGraphics->GetNControls(); i++)
	{
		int position = FindLayoutPointerPosition(viewMode, layout_container[current_view_mode].moved_pointer[i]);

		mGraphics->ReplaceControl(position, layout_container[current_view_mode].org_pointer[i]);
	}
}
// --------------------------------------------------------------------------------------------------------------------

// IPlugGUIResize -----------------------------------------------------------------------------------------------------
IPlugGUIResize::IPlugGUIResize(IPlugBase* pPlug, IGraphics* pGraphics, bool useHandle, int controlSize, int minimumControlSize)
	: IControl(pPlug, IRECT(0, 0, 0, 0))
{
	mGraphics = pGraphics;
	layout_container.resize(1);
	use_handle = useHandle;

	default_gui_width = pGraphics->Width();
	default_gui_height = pGraphics->Height();
	window_width_normalized = (double)default_gui_width;
	window_height_normalized = (double)default_gui_height;

	// Set default view dimensions
	view_container.view_mode.push_back(0);
	view_container.view_width.push_back(default_gui_width);
	view_container.view_height.push_back(default_gui_height);
	view_container.min_window_width_normalized.push_back(0.0);
	view_container.min_window_height_normalized.push_back(0.0);
	view_container.max_window_width_normalized.push_back(999999999999.0);
	view_container.max_window_height_normalized.push_back(999999999999.0);

	current_view_mode = 0;

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

	plugin_width = pGraphics->Width();
	plugin_height = pGraphics->Height();

	// Set settings.ini file path
	pGraphics->DocumentsPath(&settings_ini_path);
	settings_ini_path.Append("/");
	settings_ini_path.Append(pPlug->GetMfrName());

	// Create directory if it doesn't exist
	CreateDirectory(settings_ini_path.Get(), NULL);

	settings_ini_path.Append("/");
	settings_ini_path.Append(pPlug->GetEffectName());
	
	// Create directory if it doesn't exist
	CreateDirectory(settings_ini_path.Get(), NULL);

	settings_ini_path.Append("/Settings.ini");

	gui_scale_ratio = GetDoubleFromFile("guiscale");

	if (gui_scale_ratio < 0.0)
	{
		SetDoubleToFile("guiscale", 1.0);
		gui_scale_ratio = 1.0;
	}
	else
	{
		gui_scale_ratio = GetDoubleFromFile("guiscale");
	}

	// Initialize parameters
	guiResizeParameters.Add(new IParam);
	guiResizeParameters.Get(0)->InitInt("", -1, -1, 1000000);
	guiResizeParameters.Get(0)->SetCanAutomate(false);
	guiResizeParameters.Add(new IParam);
	guiResizeParameters.Get(1)->InitDouble("", -1.0, -1.0, 1000000, 1.0);
	guiResizeParameters.Get(1)->SetCanAutomate(false);
	guiResizeParameters.Add(new IParam);
	guiResizeParameters.Get(2)->InitDouble("", -1.0, -1.0, 1000000, 1.0);
	guiResizeParameters.Get(2)->SetCanAutomate(false);
}

bool IPlugGUIResize::Draw(IGraphics * pGraphics)
{
	if (gui_should_be_closed)
	{
		mTargetRECT = mRECT = IRECT(0, 0, plugin_width, plugin_height);
		DrawReopenPluginInterface(pGraphics, &mRECT);
	}
	else
	{
		DrawHandle(pGraphics, &mRECT);
	}

	return true;
}

void IPlugGUIResize::DrawBackgroundAtFastResizing(IGraphics * pGraphics, IRECT * pRECT)
{
	IColor backgroundColor = IColor(255, 25, 25, 25);
	pGraphics->FillIRect(&backgroundColor, pRECT);
}

void IPlugGUIResize::DrawReopenPluginInterface(IGraphics * pGraphics, IRECT * pRECT)
{
	IColor backgroundColor = IColor(255, 25, 25, 25);
	pGraphics->FillIRect(&backgroundColor, &mRECT);


	IColor color = IColor(255, 255, 255, 255);

	int textSize = int(77.0 * (double)pGraphics->Width() / (double)default_gui_width);

	IText textProps = IText(textSize, &color, "Arial", IText::kStyleItalic, IText::kAlignNear);

	IRECT textPosition = IRECT(0, (pGraphics->Height() / 2) - textSize, mRECT.R, mRECT.B);
	int position_correction = 0;

	if (textPosition.T < 0) position_correction = textPosition.T * -1;

	textPosition.T += position_correction;
	pGraphics->DrawIText(&textProps, "  Reopen plugin interface", &textPosition);

	textPosition = IRECT(0, (pGraphics->Height() / 2), mRECT.R, mRECT.B);
	textPosition.T += position_correction;
	pGraphics->DrawIText(&textProps, "  to get new size...", &textPosition);
}

void IPlugGUIResize::DrawHandle(IGraphics * pGraphics, IRECT * pRECT)
{
	// Draw triangle handle for resizing
	IColor lineColor = IColor(255, 255, 255, 255);
	double gradient = ((double)lineColor.A / 255.0) / (double)mRECT.W();

	for (int i = 0; i < mRECT.W(); i++)
	{
		double alpha = gradient * (double)(mRECT.W() - i);
		alpha = alpha * alpha * alpha;
		alpha = 1 - alpha;

		LICE_Line(pGraphics->GetDrawBitmap(), mRECT.L + i, mRECT.B, mRECT.R, mRECT.B - mRECT.W() + i, LICE_RGBA(lineColor.R, lineColor.G, lineColor.B, 255), (float)alpha);
	}
}

IPlugGUIResize* IPlugGUIResize::AttachGUIResize()
{
	// Check if we need to attach horizontal and vertical handles
	int* verticalControl = mGraphics->AttachControl(new VerticalResizing(mPlug, mGraphics, one_side_handle_size));
	int* horisontalControl = mGraphics->AttachControl(new HorisontalResizing(mPlug, mGraphics, one_side_handle_size));

	if (using_one_size_resize)
	{
		// Hide control that is not yet enabled
		if (one_side_flag == justHorisontalResizing)
		{
			HideControl(*verticalControl);
		}
		else if (one_side_flag == justVerticalResizing)
		{
			HideControl(*horisontalControl);
		}
	}
	else
	{
		HideControl(*verticalControl);
		HideControl(*horisontalControl);
	}

	// Add control sizes to a global container. This is to fix the problem of bitmap resizing on load
	if (global_layout_container.size() == 0)
	{
		global_layout_container.resize(1);

		// Backup original controls sizes
		for (int i = 0; i < mGraphics->GetNControls(); i++)
		{
			IControl* pControl = mGraphics->GetControl(i);

			global_layout_container[0].org_pointer.push_back(pControl);
			global_layout_container[0].moved_pointer.push_back(pControl);
			global_layout_container[0].org_draw_area.push_back(IRECT_to_DRECT(&*pControl->GetRECT()));
			global_layout_container[0].org_target_area.push_back(IRECT_to_DRECT(&*pControl->GetTargetRECT()));
			global_layout_container[0].org_is_hidden.push_back((int)pControl->IsHidden());
		}

		// Add IPlugGUIResize control size
		global_layout_container[0].org_pointer.push_back(this);
		global_layout_container[0].moved_pointer.push_back(this);
		global_layout_container[0].org_draw_area.push_back(IRECT_to_DRECT(&gui_resize_area));
		global_layout_container[0].org_target_area.push_back(IRECT_to_DRECT(&gui_resize_area));
		global_layout_container[0].org_is_hidden.push_back(0);
	}

	// Push pointers to layout control so that we can search by pointer
	for (int i = 0; i < mGraphics->GetNControls(); i++)
	{
		layout_container[0].org_pointer.push_back(mGraphics->GetControl(i));
		layout_container[0].moved_pointer.push_back(mGraphics->GetControl(i));
	}
	// Add IPlugGUIResize control
	layout_container[0].org_pointer.push_back(this);
	layout_container[0].moved_pointer.push_back(this);

	// Adding global layout container to a local one
	layout_container[0].org_draw_area = global_layout_container[0].org_draw_area;
	layout_container[0].org_target_area = global_layout_container[0].org_target_area;
	layout_container[0].org_is_hidden = global_layout_container[0].org_is_hidden;

	// Adding new layout for this view. By default it is copying default view layout
	for (int i = 0; i < view_container.view_mode.size(); i++)
	{
		layout_container.push_back(layout_container[0]);
	}

	RescaleBitmapsAtLoad();

	InitializeGUIControls(mGraphics);

	mGraphics->SetAllControlsDirty();

	return this;
}

void IPlugGUIResize::LiveEditSetLayout(int viewMode, int moveToIndex, int moveFromIndex, IRECT drawRECT, IRECT targetRECT, bool isHidden)
{
	DRECT drawDRECT, targetDRECT;
	drawDRECT = IRECT_to_DRECT(&drawRECT);
	targetDRECT = IRECT_to_DRECT(&targetRECT);

	layout_container[viewMode].moved_pointer[moveToIndex] = layout_container[viewMode].org_pointer[moveFromIndex];
	layout_container[viewMode].org_draw_area[moveToIndex] = drawDRECT;
	layout_container[viewMode].org_target_area[moveToIndex] = targetDRECT;
	layout_container[viewMode].org_is_hidden[moveToIndex] = isHidden;
}

void IPlugGUIResize::LiveRemoveLayer(IControl * pControl)
{
	// This will remove control for every view
	for (int i = 0; i < GetViewModeSize(); i++)
	{
		int position = FindLayoutPointerPosition(i, pControl);

		layout_container[i].moved_pointer.erase(layout_container[i].moved_pointer.begin() + position);
		layout_container[i].org_pointer.erase(layout_container[i].org_pointer.begin() + position);
		layout_container[i].org_draw_area.erase(layout_container[i].org_draw_area.begin() + position);
		layout_container[i].org_target_area.erase(layout_container[i].org_target_area.begin() + position);
		layout_container[i].org_is_hidden.erase(layout_container[i].org_is_hidden.begin() + position);
	}
}

void IPlugGUIResize::UseHandleForGUIScaling(bool statement)
{
	handle_controls_gui_scaling = statement;
}

void IPlugGUIResize::UseControlAndClickOnHandleForGUIScaling(bool statement)
{
	control_and_click_on_handle_controls_gui_scaling = statement;
}

void IPlugGUIResize::AddNewView(int viewMode, int viewWidth, int viewHeight)
{
	view_container.view_mode.push_back(viewMode);
	view_container.view_width.push_back(viewWidth);
	view_container.view_height.push_back(viewHeight);
	view_container.min_window_width_normalized.push_back(0.0);
	view_container.min_window_height_normalized.push_back(0.0);
	view_container.max_window_width_normalized.push_back(999999999999.0);
	view_container.max_window_height_normalized.push_back(999999999999.0);
}

void IPlugGUIResize::UseOneSideResizing(int handleSize, int minHandleSize, resizeOneSide flag)
{
	one_side_handle_size = handleSize;
	one_side_handle_min_size = minHandleSize;

	using_one_size_resize = true;
	one_side_flag = flag;
}

void IPlugGUIResize::EnableOneSideResizing(resizeOneSide flag)
{
	if (using_one_size_resize)
	{
		int index = mGraphics->GetNControls() - 1;
		int horisontalControl = (index - 1);
		int verticalControl = (index - 2);

		if (flag == horisontalAndVerticalResizing)
		{
			ShowControl(horisontalControl);
			ShowControl(verticalControl);
		}
		else if (flag == justHorisontalResizing)
		{
			ShowControl(horisontalControl);;
		}
		else if (flag == justVerticalResizing)
		{
			ShowControl(verticalControl);
		}
	}
}

void IPlugGUIResize::DisableOneSideResizing(resizeOneSide flag)
{
	if (using_one_size_resize)
	{
		int index = mGraphics->GetNControls() - 1;
		int horisontalControl = (index - 1);
		int verticalControl = (index - 2);

		if (flag == horisontalAndVerticalResizing)
		{
			HideControl(horisontalControl);
			HideControl(verticalControl);
		}
		else if (flag == justHorisontalResizing)
		{
			HideControl(horisontalControl);
		}
		else if (flag == justVerticalResizing)
		{
			HideControl(verticalControl);
		}
	}
}

void IPlugGUIResize::SelectViewMode(int viewMode)
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

	current_view_mode = position;
	window_width_normalized = (double)view_container.view_width[position];
	window_height_normalized = (double)view_container.view_height[position];
}

void IPlugGUIResize::SetGUIScaleRatio(double guiScaleRatio)
{
	gui_scale_ratio = guiScaleRatio;
	gui_scale_ratio = BOUNDED(gui_scale_ratio, min_gui_scale_ratio, max_gui_scale_ratio);

	global_gui_scale_ratio = gui_scale_ratio;
	plugin_width = (int)(window_width_normalized * gui_scale_ratio);
	plugin_height = (int)(window_height_normalized * gui_scale_ratio);
}

void IPlugGUIResize::SetWindowSize(double width, double height)
{
	window_width_normalized = width;
	window_height_normalized = height;

	window_width_normalized = BOUNDED(window_width_normalized, view_container.min_window_width_normalized[current_view_mode], view_container.max_window_width_normalized[current_view_mode]);
	window_height_normalized = BOUNDED(window_height_normalized, view_container.min_window_height_normalized[current_view_mode], view_container.max_window_height_normalized[current_view_mode]);
}

void IPlugGUIResize::SetWindowWidth(double width)
{
	window_width_normalized = width;

	window_width_normalized = BOUNDED(window_width_normalized, view_container.min_window_width_normalized[current_view_mode], view_container.max_window_width_normalized[current_view_mode]);
}

void IPlugGUIResize::SetWindowHeight(double height)
{
	window_height_normalized = height;

	window_height_normalized = BOUNDED(window_height_normalized, view_container.min_window_height_normalized[current_view_mode], view_container.max_window_height_normalized[current_view_mode]);
}

void IPlugGUIResize::SetGUIScaleLimits(double minSizeInPercentage, double maxSizeInPercentage)
{
	min_gui_scale_ratio = minSizeInPercentage / 100.0;
	max_gui_scale_ratio = maxSizeInPercentage / 100.0;

	gui_scale_ratio = BOUNDED(gui_scale_ratio, min_gui_scale_ratio, max_gui_scale_ratio);
}

void IPlugGUIResize::SetWindowSizeLimits(int viewMode, double minWindowWidth, double minWindowHeight, double maxWindowWidth, double maxWindowHeight)
{
	view_container.min_window_width_normalized[viewMode] = minWindowWidth;
	view_container.max_window_width_normalized[viewMode] = maxWindowWidth;

	view_container.min_window_height_normalized[viewMode] = minWindowHeight;
	view_container.max_window_height_normalized[viewMode] = maxWindowHeight;

	window_width_normalized = BOUNDED(window_width_normalized, view_container.min_window_width_normalized[current_view_mode], view_container.max_window_width_normalized[current_view_mode]);
	window_height_normalized = BOUNDED(window_height_normalized, view_container.min_window_height_normalized[current_view_mode], view_container.max_window_height_normalized[current_view_mode]);
}

void  IPlugGUIResize::UsingBitmaps()
{
	using_bitmaps = true;
}

void IPlugGUIResize::DisableFastBitmapResizing()
{
	fast_bitmap_resizing = false;
}

void IPlugGUIResize::HideControl(int index)
{
	IControl* pControl = mGraphics->GetControl(index);

	int position = FindLayoutPointerPosition(current_view_mode, pControl);
	if (position >= 0) layout_container[current_view_mode].org_is_hidden[position] = true;

	pControl->Hide(true);
}

void IPlugGUIResize::ShowControl(int index)
{
	IControl* pControl = mGraphics->GetControl(index);

	int position = FindLayoutPointerPosition(current_view_mode, pControl);
	if (position >= 0) layout_container[current_view_mode].org_is_hidden[position] = false;

	pControl->Hide(false);
}




void IPlugGUIResize::MoveControlRelativeToControlDrawRect(int moveControlIndex, int relativeToControlIndex, double xRatio, double yRatio, resizeFlag flag)
{
	IControl* moveControl = mGraphics->GetControl(moveControlIndex);
	IControl* relativeToControl = mGraphics->GetControl(relativeToControlIndex);

	MoveControlRelativeToControlDrawRect(moveControl, relativeToControl, xRatio, yRatio, flag);
}

void IPlugGUIResize::MoveControlRelativeToControlDrawRect(IControl* moveControl, IControl* relativeToControl, double xRatio, double yRatio, resizeFlag flag)
{
	IRECT relativeTo = *relativeToControl->GetNonScaledDrawRECT();

	// Get control width and height
	double nonScaledControlWidth = moveControl->GetRECT()->W() / gui_scale_ratio;
	double nonScaledControlHeight = moveControl->GetRECT()->H() / gui_scale_ratio;

	// Make x and y relative to IRECT
	double x = relativeTo.L + (relativeTo.W() - nonScaledControlWidth) * xRatio;
	double y = relativeTo.T + (relativeTo.H() - nonScaledControlHeight) * yRatio;

	MoveControl(moveControl, x, y, flag);
}

void IPlugGUIResize::MoveControlHorizontallyRelativeToControlDrawRect(int moveControlIndex, int relativeToControlIndex, double xRatio, resizeFlag flag)
{
	IControl* moveControl = mGraphics->GetControl(moveControlIndex);
	IControl* relativeToControl = mGraphics->GetControl(relativeToControlIndex);

	MoveControlHorizontallyRelativeToControlDrawRect(moveControl, relativeToControl, xRatio, flag);
}

void IPlugGUIResize::MoveControlHorizontallyRelativeToControlDrawRect(IControl* moveControl, IControl* relativeToControl, double xRatio, resizeFlag flag)
{
	IRECT relativeTo = *relativeToControl->GetNonScaledDrawRECT();

	// Get control width and height
	double nonScaledControlWidth = moveControl->GetRECT()->W() / gui_scale_ratio;

	// Make x and y relative to IRECT
	double x = relativeTo.L + (relativeTo.W() - nonScaledControlWidth) * xRatio;

	MoveControlHorizontally(moveControl, x, flag);
}

void IPlugGUIResize::MoveControlVerticallyRelativeToControlDrawRect(int moveControlIndex, int relativeToControlIndex, double yRatio, resizeFlag flag)
{
	IControl* moveControl = mGraphics->GetControl(moveControlIndex);
	IControl* relativeToControl = mGraphics->GetControl(relativeToControlIndex);

	MoveControlVerticallyRelativeToControlDrawRect(moveControl, relativeToControl,  yRatio, flag);
}

void IPlugGUIResize::MoveControlVerticallyRelativeToControlDrawRect(IControl* moveControl, IControl* relativeToControl, double yRatio, resizeFlag flag)
{
	IRECT relativeTo = *relativeToControl->GetNonScaledDrawRECT();

	// Get control width and height
	double nonScaledControlHeight = moveControl->GetRECT()->H() / gui_scale_ratio;

	// Make x and y relative to IRECT
	double y = relativeTo.T + (relativeTo.H() - nonScaledControlHeight) * yRatio;

	MoveControlVertically(moveControl, y, flag);
}

void IPlugGUIResize::MoveControlRelativeToNonScaledDRECT(int index, DRECT relativeTo, double xRatio, double yRatio, resizeFlag flag)
{
	IControl* pControl = mGraphics->GetControl(index);

	MoveControlRelativeToNonScaledDRECT(pControl, relativeTo, xRatio, yRatio, flag);
}

void IPlugGUIResize::MoveControlRelativeToNonScaledDRECT(IControl* pControl, DRECT relativeTo, double xRatio, double yRatio, resizeFlag flag)
{
	// Get control width and height
	double nonScaledControlWidth = pControl->GetRECT()->W() / gui_scale_ratio;
	double nonScaledControlHeight = pControl->GetRECT()->H() / gui_scale_ratio;

	// Make x and y relative to IRECT
	double x = relativeTo.L + (relativeTo.W() - nonScaledControlWidth) * xRatio;
	double y = relativeTo.T + (relativeTo.H() - nonScaledControlHeight) * yRatio;

	MoveControl(pControl, x, y, flag);
}

void IPlugGUIResize::MoveControlHorizontallyRelativeToNonScaledDRECT(int index, DRECT relativeTo, double xRatio, resizeFlag flag)
{
	IControl* pControl = mGraphics->GetControl(index);

	MoveControlHorizontallyRelativeToNonScaledDRECT(pControl, relativeTo, xRatio, flag);
}

void IPlugGUIResize::MoveControlHorizontallyRelativeToNonScaledDRECT(IControl* pControl, DRECT relativeTo, double xRatio, resizeFlag flag)
{
	// Get control width and height
	double nonScaledControlWidth = pControl->GetRECT()->W() / gui_scale_ratio;

	// Make x and y relative to IRECT
	double x = relativeTo.L + (relativeTo.W() - nonScaledControlWidth) * xRatio;

	MoveControlHorizontally(pControl, x, flag);
}

void IPlugGUIResize::MoveControlVerticallyRelativeToNonScaledDRECT(int index, DRECT relativeTo, double yRatio, resizeFlag flag)
{
	IControl* pControl = mGraphics->GetControl(index);

	MoveControlVerticallyRelativeToNonScaledDRECT(pControl, relativeTo, yRatio, flag);
}

void IPlugGUIResize::MoveControlVerticallyRelativeToNonScaledDRECT(IControl* pControl, DRECT relativeTo, double yRatio, resizeFlag flag)
{
	// Get control width and height
	double nonScaledControlHeight = pControl->GetRECT()->H() / gui_scale_ratio;

	// Make x and y relative to IRECT
	double y = relativeTo.T + (relativeTo.H() - nonScaledControlHeight) * yRatio;

	MoveControlVertically(pControl, y, flag);
}

void IPlugGUIResize::MoveControl(int index, double x, double y, resizeFlag flag)
{
	IControl* pControl = mGraphics->GetControl(index);
	MoveControl(pControl, x, y, flag);
}

void IPlugGUIResize::MoveControl(IControl* pControl, double x, double y, resizeFlag flag)
{
	double x_relative = x * gui_scale_ratio;
	double y_relative = y * gui_scale_ratio;

	DRECT *orgDrawArea = NULL, *orgTargetArea = NULL;
	orgDrawArea = GetLayoutContainerDrawRECT(current_view_mode, pControl);
	orgTargetArea = GetLayoutContainerTargetRECT(current_view_mode, pControl);

	if (flag == drawAndTargetArea || flag == drawAreaOnly)
	{
		double drawAreaW = (double)pControl->GetRECT()->W();
		double drawAreaH = (double)pControl->GetRECT()->H();
		
		DRECT drawArea = DRECT(x_relative, y_relative, x_relative + drawAreaW, y_relative + drawAreaH);
		pControl->SetDrawRECT(DRECT_to_IRECT(&drawArea));
		
		double org_draw_width = orgDrawArea->W();
		double org_draw_height = orgDrawArea->H();

		orgDrawArea->L = x;
		orgDrawArea->T = y;
		orgDrawArea->R = x + org_draw_width;
		orgDrawArea->B = y + org_draw_height;

		pControl->SetNonScaledDrawRECT(DRECT_to_IRECT(orgDrawArea));
	}

	if (flag == drawAndTargetArea || flag == targetAreaOnly)
	{
		double targetAreaW = (double)pControl->GetTargetRECT()->W();
		double targetAreaH = (double)pControl->GetTargetRECT()->H();

		DRECT targetArea = DRECT(x_relative, y_relative, x_relative + targetAreaW, y_relative + targetAreaH);
		pControl->SetTargetRECT(DRECT_to_IRECT(&targetArea));

		double org_target_width = orgTargetArea->W();
		double org_target_height = orgTargetArea->H();

		orgTargetArea->L = x;
		orgTargetArea->T = y;
		orgTargetArea->R = x + org_target_width;
		orgTargetArea->B = y + org_target_height;
	}
}

void IPlugGUIResize::MoveControlHorizontally(int index, double x, resizeFlag flag)
{
	IControl* pControl = mGraphics->GetControl(index);
	MoveControlHorizontally(pControl, x, flag);
}

void IPlugGUIResize::MoveControlHorizontally(IControl* pControl, double x, resizeFlag flag)
{
	double x_relative = x * gui_scale_ratio;

	DRECT *orgDrawArea = NULL, *orgTargetArea = NULL;
	orgDrawArea = GetLayoutContainerDrawRECT(current_view_mode, pControl);
	orgTargetArea = GetLayoutContainerTargetRECT(current_view_mode, pControl);

	if (flag == drawAndTargetArea || flag == drawAreaOnly)
	{
		double drawAreaT = (double)pControl->GetRECT()->T;
		double drawAreaB = (double)pControl->GetRECT()->B;
		double drawAreaW = (double)pControl->GetRECT()->W();

		DRECT drawArea = DRECT(x_relative, drawAreaT, x_relative + drawAreaW, drawAreaB);
		pControl->SetDrawRECT(DRECT_to_IRECT(&drawArea));

		double org_draw_width = orgDrawArea->W();

		orgDrawArea->L = x;
		orgDrawArea->R = x + org_draw_width;

		pControl->SetNonScaledDrawRECT(DRECT_to_IRECT(orgDrawArea));
	}

	if (flag == drawAndTargetArea || flag == targetAreaOnly)
	{
		double targetAreaT = (double)pControl->GetTargetRECT()->T;
		double targetAreaB = (double)pControl->GetTargetRECT()->B;
		double targetAreaW = (double)pControl->GetTargetRECT()->W();

		DRECT targetArea = DRECT(x_relative, targetAreaT, x_relative + targetAreaW, targetAreaB);
		pControl->SetTargetRECT(DRECT_to_IRECT(&targetArea));

		double org_target_width = orgTargetArea->W();

		orgTargetArea->L = x;
		orgTargetArea->R = x + org_target_width;
	}
}

void IPlugGUIResize::MoveControlVertically(int index, double y, resizeFlag flag)
{
	IControl* pControl = mGraphics->GetControl(index);
	MoveControlVertically(pControl, y, flag);
}

void IPlugGUIResize::MoveControlVertically(IControl* pControl, double y, resizeFlag flag)
{
	double y_relative = y * gui_scale_ratio;

	DRECT *orgDrawArea = NULL, *orgTargetArea = NULL;
	orgDrawArea = GetLayoutContainerDrawRECT(current_view_mode, pControl);
	orgTargetArea = GetLayoutContainerTargetRECT(current_view_mode, pControl);

	if (flag == drawAndTargetArea || flag == drawAreaOnly)
	{
		double drawAreaL = (double)pControl->GetRECT()->L;
		double drawAreaR = (double)pControl->GetRECT()->R;
		double drawAreaH = (double)pControl->GetRECT()->H();

		DRECT drawArea = DRECT(drawAreaL, y_relative, drawAreaR, y_relative + drawAreaH);
		pControl->SetDrawRECT(DRECT_to_IRECT(&drawArea));

		double org_draw_height = orgDrawArea->H();

		orgDrawArea->T = y;
		orgDrawArea->B = y + org_draw_height;

		pControl->SetNonScaledDrawRECT(DRECT_to_IRECT(orgDrawArea));
	}

	if (flag == drawAndTargetArea || flag == targetAreaOnly)
	{
		double targetAreaL = (double)pControl->GetTargetRECT()->L;
		double targetAreaR = (double)pControl->GetTargetRECT()->R;
		double targetAreaH = (double)pControl->GetTargetRECT()->H();

		DRECT targetArea = DRECT(targetAreaL, y_relative, targetAreaR, y_relative + targetAreaH);
		pControl->SetTargetRECT(DRECT_to_IRECT(&targetArea));

		double org_target_height = orgTargetArea->H();

		orgTargetArea->T = y;
		orgTargetArea->B = y + org_target_height;
	}
}

void IPlugGUIResize::MoveControlTopEdge(int index, double T, resizeFlag flag)
{
	IControl* pControl = mGraphics->GetControl(index);
	MoveControlTopEdge(pControl, T, flag);
}

void IPlugGUIResize::MoveControlTopEdge(IControl* pControl, double T, resizeFlag flag)
{
	DRECT *orgDrawArea = NULL, *orgTargetArea = NULL;
	orgDrawArea = GetLayoutContainerDrawRECT(current_view_mode, pControl);
	orgTargetArea = GetLayoutContainerTargetRECT(current_view_mode, pControl);

	double T_relative = T * gui_scale_ratio;

	if (flag == drawAndTargetArea || flag == drawAreaOnly)
	{
		double drawAreaL = (double)pControl->GetRECT()->L;
		double drawAreaR = (double)pControl->GetRECT()->R;
		double drawAreaB = (double)pControl->GetRECT()->B;

		DRECT drawArea = DRECT(drawAreaL, T_relative, drawAreaR, drawAreaB);
		pControl->SetDrawRECT(DRECT_to_IRECT(&drawArea));

		orgDrawArea->T = T;

		pControl->SetNonScaledDrawRECT(DRECT_to_IRECT(orgDrawArea));
	}

	if (flag == drawAndTargetArea || flag == targetAreaOnly)
	{
		double targetAreaL = (double)pControl->GetTargetRECT()->L;
		double targetAreaR = (double)pControl->GetTargetRECT()->R;
		double targetAreaB = (double)pControl->GetTargetRECT()->B;

		DRECT targetArea = DRECT(targetAreaL, T_relative, targetAreaR, targetAreaB);
		pControl->SetTargetRECT(DRECT_to_IRECT(&targetArea));

		orgTargetArea->T = T;
	}
}

void IPlugGUIResize::MoveControlLeftEdge(int index, double L, resizeFlag flag)
{
	IControl* pControl = mGraphics->GetControl(index);
	MoveControlLeftEdge(pControl, L, flag);
}

void IPlugGUIResize::MoveControlLeftEdge(IControl* pControl, double L, resizeFlag flag)
{
	DRECT *orgDrawArea = NULL, *orgTargetArea = NULL;
	orgDrawArea = GetLayoutContainerDrawRECT(current_view_mode, pControl);
	orgTargetArea = GetLayoutContainerTargetRECT(current_view_mode, pControl);

	double L_relative = L * gui_scale_ratio;

	if (flag == drawAndTargetArea || flag == drawAreaOnly)
	{
		double drawAreaT = (double)pControl->GetRECT()->T;
		double drawAreaR = (double)pControl->GetRECT()->R;
		double drawAreaB = (double)pControl->GetRECT()->B;

		DRECT drawArea = DRECT(L_relative, drawAreaT, drawAreaR, drawAreaB);
		pControl->SetDrawRECT(DRECT_to_IRECT(&drawArea));

		orgDrawArea->L = L;

		pControl->SetNonScaledDrawRECT(DRECT_to_IRECT(orgDrawArea));
	}

	if (flag == drawAndTargetArea || flag == targetAreaOnly)
	{
		double targetAreaT = (double)pControl->GetTargetRECT()->T;
		double targetAreaR = (double)pControl->GetTargetRECT()->R;
		double targetAreaB = (double)pControl->GetTargetRECT()->B;

		DRECT targetArea = DRECT(L_relative, targetAreaT, targetAreaR, targetAreaB);
		pControl->SetTargetRECT(DRECT_to_IRECT(&targetArea));

		orgTargetArea->L = L;
	}
}

void IPlugGUIResize::MoveControlRightEdge(int index, double R, resizeFlag flag)
{
	IControl* pControl = mGraphics->GetControl(index);
	MoveControlRightEdge(pControl, R, flag);
}

void IPlugGUIResize::MoveControlRightEdge(IControl* pControl, double R, resizeFlag flag)
{
	DRECT *orgDrawArea = NULL, *orgTargetArea = NULL;
	orgDrawArea = GetLayoutContainerDrawRECT(current_view_mode, pControl);
	orgTargetArea = GetLayoutContainerTargetRECT(current_view_mode, pControl);

	double R_relative = R * gui_scale_ratio;

	if (flag == drawAndTargetArea || flag == drawAreaOnly)
	{
		double drawAreaL = (double)pControl->GetRECT()->L;
		double drawAreaT = (double)pControl->GetRECT()->T;
		double drawAreaB = (double)pControl->GetRECT()->B;

		DRECT drawArea = DRECT(drawAreaL, drawAreaT, R_relative, drawAreaB);
		pControl->SetDrawRECT(DRECT_to_IRECT(&drawArea));

		orgDrawArea->R = R;

		pControl->SetNonScaledDrawRECT(DRECT_to_IRECT(orgDrawArea));
	}

	if (flag == drawAndTargetArea || flag == targetAreaOnly)
	{
		double targetAreaL = (double)pControl->GetTargetRECT()->L;
		double targetAreaT = (double)pControl->GetTargetRECT()->T;
		double targetAreaB = (double)pControl->GetTargetRECT()->B;

		DRECT targetArea = DRECT(targetAreaL, targetAreaT, R_relative, targetAreaB);
		pControl->SetTargetRECT(DRECT_to_IRECT(&targetArea));

		orgTargetArea->R = R;
	}
}

void IPlugGUIResize::MoveControlBottomEdge(int index, double B, resizeFlag flag)
{
	IControl* pControl = mGraphics->GetControl(index);
	MoveControlBottomEdge(pControl, B, flag);
}

void IPlugGUIResize::MoveControlBottomEdge(IControl* pControl, double B, resizeFlag flag)
{
	DRECT *orgDrawArea = NULL, *orgTargetArea = NULL;
	orgDrawArea = GetLayoutContainerDrawRECT(current_view_mode, pControl);
	orgTargetArea = GetLayoutContainerTargetRECT(current_view_mode, pControl);

	double B_relative = B * gui_scale_ratio;

	if (flag == drawAndTargetArea || flag == drawAreaOnly)
	{
		double drawAreaL = (double)pControl->GetRECT()->L;
		double drawAreaR = (double)pControl->GetRECT()->R;
		double drawAreaT = (double)pControl->GetRECT()->T;

		DRECT drawArea = DRECT(drawAreaL, drawAreaT, drawAreaR, B_relative);
		pControl->SetDrawRECT(DRECT_to_IRECT(&drawArea));

		orgDrawArea->B = B;

		pControl->SetNonScaledDrawRECT(DRECT_to_IRECT(orgDrawArea));
	}

	if (flag == drawAndTargetArea || flag == targetAreaOnly)
	{
		double targetAreaL = (double)pControl->GetTargetRECT()->L;
		double targetAreaR = (double)pControl->GetTargetRECT()->R;
		double targetAreaT = (double)pControl->GetTargetRECT()->T;

		DRECT targetArea = DRECT(targetAreaL, targetAreaT, targetAreaR, B_relative);
		pControl->SetTargetRECT(DRECT_to_IRECT(&targetArea));

		orgTargetArea->B = B;
	}
}




void IPlugGUIResize::SetNormalizedDrawRect(int index, double L, double T, double R, double B)
{
	IControl *tmpControl = mGraphics->GetControl(index);
	tmpControl->SetDrawRECT(IRECT(int(L * gui_scale_ratio), int(T * gui_scale_ratio), int(R * gui_scale_ratio), int(B * gui_scale_ratio)));
}

void IPlugGUIResize::SetNormalizedDrawRect(int index, DRECT r)
{
	IControl *tmpControl = mGraphics->GetControl(index);
	tmpControl->SetDrawRECT(IRECT(int(r.L * gui_scale_ratio), int(r.T * gui_scale_ratio), int(r.R * gui_scale_ratio), int(r.B * gui_scale_ratio)));
}

void IPlugGUIResize::SetNormalizedDrawRect(IControl * pControl, double L, double T, double R, double B)
{
	pControl->SetDrawRECT(IRECT(int(L * gui_scale_ratio), int(T * gui_scale_ratio), int(R * gui_scale_ratio), int(B * gui_scale_ratio)));
}

void IPlugGUIResize::SetNormalizedDrawRect(IControl * pControl, DRECT r)
{
	pControl->SetDrawRECT(IRECT(int(r.L * gui_scale_ratio), int(r.T * gui_scale_ratio), int(r.R * gui_scale_ratio), int(r.B * gui_scale_ratio)));
}

void IPlugGUIResize::SetNormalizedTargetRect(int index, double L, double T, double R, double B)
{
	IControl *tmpControl = mGraphics->GetControl(index);
	tmpControl->SetTargetRECT(IRECT(int(L * gui_scale_ratio), int(T * gui_scale_ratio), int(R * gui_scale_ratio), int(B * gui_scale_ratio)));
}

void IPlugGUIResize::SetNormalizedTargetRect(int index, DRECT r)
{
	IControl *tmpControl = mGraphics->GetControl(index);
	tmpControl->SetTargetRECT(IRECT(int(r.L * gui_scale_ratio), int(r.T * gui_scale_ratio), int(r.R * gui_scale_ratio), int(r.B * gui_scale_ratio)));
}

void IPlugGUIResize::SetNormalizedTargetRect(IControl * pControl, double L, double T, double R, double B)
{
	pControl->SetTargetRECT(IRECT(int(L * gui_scale_ratio), int(T * gui_scale_ratio), int(R * gui_scale_ratio), int(B * gui_scale_ratio)));
}

void IPlugGUIResize::SetNormalizedTargetRect(IControl * pControl, DRECT r)
{
	pControl->SetTargetRECT(IRECT(int(r.L * gui_scale_ratio), int(r.T * gui_scale_ratio), int(r.R * gui_scale_ratio), int(r.B * gui_scale_ratio)));
}

double IPlugGUIResize::GetGUIScaleRatio()
{
	return gui_scale_ratio;
}

int IPlugGUIResize::GetViewMode()
{
	return current_view_mode;
}

int IPlugGUIResize::GetViewModeSize()
{
	return view_container.view_mode.size();
}

bool IPlugGUIResize::CurrentlyFastResizing()
{
	return currentlyFastResizing;
}

void IPlugGUIResize::ResizeControlRects()
{
	// Set new target and draw area
	for (int i = 1; i < mGraphics->GetNControls(); i++)
	{
		IControl* pControl = mGraphics->GetControl(i);
		DRECT *orgDrawArea = NULL, *orgTargetArea = NULL; int* isHidden = NULL;
		orgDrawArea = GetLayoutContainerDrawRECT(current_view_mode, pControl);
		orgTargetArea = GetLayoutContainerTargetRECT(current_view_mode, pControl);
		isHidden = GetLayoutContainerIsHidden(current_view_mode, pControl);

		// This updates draw and control rect
		pControl->SetNonScaledDrawRECT(DRECT_to_IRECT(orgDrawArea));
		pControl->SetDrawRECT(ResizeIRECT(orgDrawArea, gui_scale_ratio, gui_scale_ratio));
		pControl->SetTargetRECT(ResizeIRECT(orgTargetArea, gui_scale_ratio, gui_scale_ratio));
		pControl->Hide((bool)*isHidden);
	}

	// Keeps one side handle above or at minimum size
	int index = mGraphics->GetNControls() - 1;
	IControl* horisontalControl = mGraphics->GetControl(index - 1);
	IControl* verticalControl = mGraphics->GetControl(index - 2);

	// Take care of one side handles
	// We will treat draw and target rect the same
	if (using_one_size_resize)
	{
		if (horisontalControl->GetTargetRECT()->W() < one_side_handle_min_size)
		{
			IRECT rect = IRECT(horisontalControl->GetTargetRECT()->R - one_side_handle_min_size, 0, horisontalControl->GetTargetRECT()->R, horisontalControl->GetTargetRECT()->B);
			horisontalControl->SetDrawRECT(rect);
			horisontalControl->SetTargetRECT(rect);
		}

		if (verticalControl->GetTargetRECT()->H() < one_side_handle_min_size)
		{
			IRECT rect = IRECT(0, verticalControl->GetTargetRECT()->B - one_side_handle_min_size, verticalControl->GetTargetRECT()->R, verticalControl->GetTargetRECT()->B);
			verticalControl->SetDrawRECT(rect);
			verticalControl->SetTargetRECT(rect);
		}
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

void IPlugGUIResize::InitializeGUIControls(IGraphics * pGraphics)
{
	// Call GUI initializer
	for (int i = 0; i < pGraphics->GetNControls(); i++)
	{
		IControl* pControl = pGraphics->GetControl(i);
		pControl->AfterGUIResize(gui_scale_ratio);
	}
	pGraphics->SetAllControlsDirty();
}

void IPlugGUIResize::ResetControlsVisibility()
{
	for (int i = 0; i < mGraphics->GetNControls(); i++)
	{
		IControl* pControl = mGraphics->GetControl(i);
		pControl->Hide(layout_container[current_view_mode].org_is_hidden[i]);
	}
}

void IPlugGUIResize::RescaleBitmapsAtLoad()
{
	if (!bitmaps_rescaled_at_load)
	{
		mGraphics->RescaleBitmaps(gui_scale_ratio);

		bitmaps_rescaled_at_load = true;
	}
	bitmaps_rescaled_at_load_skip = true;
}

void IPlugGUIResize::ResizeAtGUIOpen()
{
	double prev_plugin_width = plugin_width;
	double prev_plugin_height = plugin_height;

	if (!presets_loaded)
	{
		if (guiResizeParameters.Get(0)->Value() > -0.5)
			current_view_mode = IPMIN((int)guiResizeParameters.Get(0)->Value(), view_container.view_mode.size());

		if (guiResizeParameters.Get(1)->Value() > -0.5)
			window_width_normalized = guiResizeParameters.Get(1)->Value();

		if (guiResizeParameters.Get(2)->Value() > -0.5)
			window_height_normalized = guiResizeParameters.Get(2)->Value();

		presets_loaded = true;
	}

	gui_scale_ratio = GetDoubleFromFile("guiscale");
	mGraphics->guiScaleRatio = gui_scale_ratio;

	plugin_width = (int)(window_width_normalized * gui_scale_ratio);
	plugin_height = (int)(window_height_normalized * gui_scale_ratio);

	RearrangeLayers();
	MoveHandle();
	ResizeBackground();
    ResizeControlRects();
	mPlug->SetGUILayout(current_view_mode, window_width_normalized, window_height_normalized);

	// Prevent resizing if it is not needed
	if (prev_plugin_width != plugin_width || prev_plugin_height != plugin_height)
	{
		if (using_bitmaps && !bitmaps_rescaled_at_load_skip && !double_equals(global_gui_scale_ratio, gui_scale_ratio))
		{
			mGraphics->RescaleBitmaps(gui_scale_ratio);
		}
		bitmaps_rescaled_at_load_skip = false;

		mGraphics->Resize(plugin_width, plugin_height);
		InitializeGUIControls(mGraphics);

		plugin_resized = false;
		gui_should_be_closed = false;
	}

	global_gui_scale_ratio = gui_scale_ratio;
}

void IPlugGUIResize::ResizeGraphics()
{
	bool window_resizing = false;

	if (double_equals(GetDoubleFromFile("guiscale"), gui_scale_ratio))
	{
		window_resizing = true;
	}
	else
	{
		SetDoubleToFile("guiscale", gui_scale_ratio);
	}

	mGraphics->guiScaleRatio = gui_scale_ratio;

	// Set parameters
	guiResizeParameters.Get(0)->Set(current_view_mode);
	guiResizeParameters.Get(1)->Set(window_width_normalized);
	guiResizeParameters.Get(2)->Set(window_height_normalized);

	plugin_width = (int)(window_width_normalized * gui_scale_ratio);
	plugin_height = (int)(window_height_normalized * gui_scale_ratio);

	RearrangeLayers();
	MoveHandle();
	ResizeBackground();
	ResizeControlRects();
	mPlug->SetGUILayout(current_view_mode, window_width_normalized, window_height_normalized);

	if (using_bitmaps)
	{
		if (!fast_bitmap_resizing)
		{
			if (!window_resizing)
			{
				mGraphics->RescaleBitmaps(gui_scale_ratio);
			}

			mGraphics->Resize(plugin_width, plugin_height);
			InitializeGUIControls(mGraphics);
		}
		else
		{
			mGraphics->Resize(plugin_width, plugin_height);

			if (!currentlyFastResizing)
			{
				InitializeGUIControls(mGraphics);
			}
		}

		plugin_resized = true;
	}
	else
	{
		mGraphics->Resize(plugin_width, plugin_height);
		InitializeGUIControls(mGraphics);
	}

	mGraphics->SetAllControlsDirty();
}

// Used by the framework ---------------------------------------------------------------------------------------------------------------------------

IParam * IPlugGUIResize::GetGUIResizeParameter(int index)
{
	return guiResizeParameters.Get(index);
}

int IPlugGUIResize::GetGUIResizeParameterSize()
{
	return guiResizeParameters.GetSize();
}

void IPlugGUIResize::MoveHandle()
{
	double x;
	double y;

	int index = mGraphics->GetNControls() - 1;

	// Take care of one side handles
	if (using_one_size_resize)
	{
		// Set horizontal handle
		x = (double)plugin_width / gui_scale_ratio - one_side_handle_size;
		y = (double)plugin_height / gui_scale_ratio;
		MoveControl(index - 1, x, 0);
		MoveControlBottomEdge(index - 1, y);

		// Set vertical handle
		x = (double)plugin_width / gui_scale_ratio;
		y = (double)plugin_height / gui_scale_ratio - one_side_handle_size;
		MoveControl(index - 2, 0, y);
		MoveControlRightEdge(index - 2, x);
	}

	// Take care of main handle
	x = (double)plugin_width / gui_scale_ratio - control_size;
	y = (double)plugin_height / gui_scale_ratio - control_size;
	MoveControl(index, x, y);
}

void IPlugGUIResize::OnMouseDrag(int x, int y, int dX, int dY, IMouseMod * pMod)
{
	if (!gui_should_be_closed)
	{
		SetCursor(LoadCursor(NULL, IDC_SIZENWSE));

		if (handle_controls_gui_scaling || (control_and_click_on_handle_controls_gui_scaling && pMod->C))
		{
			// Sets GUI scale
			gui_scale_ratio = (((double)x + (double)y) / 2.0) / ((window_width_normalized + window_height_normalized) / 2.0);
			gui_scale_ratio = BOUNDED(gui_scale_ratio, min_gui_scale_ratio, max_gui_scale_ratio);
			
			global_gui_scale_ratio = gui_scale_ratio;
			plugin_width = (int)(window_width_normalized * gui_scale_ratio);
			plugin_height = (int)(window_height_normalized * gui_scale_ratio);
		}
		else
		{
			window_width_normalized = (double)x / gui_scale_ratio;
			window_height_normalized = (double)y / gui_scale_ratio;

			window_width_normalized = BOUNDED(window_width_normalized, view_container.min_window_width_normalized[current_view_mode], view_container.max_window_width_normalized[current_view_mode]);
			window_height_normalized = BOUNDED(window_height_normalized, view_container.min_window_height_normalized[current_view_mode], view_container.max_window_height_normalized[current_view_mode]);
		}


		if ((using_bitmaps && fast_bitmap_resizing && handle_controls_gui_scaling) || (control_and_click_on_handle_controls_gui_scaling && pMod->C))
		{
			mTargetRECT = mRECT = IRECT(0, 0, plugin_width, plugin_height);
		}
		else
		{
			mTargetRECT.L = mRECT.L = x - IPMAX((int)((double)control_size * gui_scale_ratio), min_control_size);
			mTargetRECT.T = mRECT.T = y - IPMAX((int)((double)control_size * gui_scale_ratio), min_control_size);
			mTargetRECT.R = mRECT.R = x;
			mTargetRECT.B = mRECT.B = y;
		}

		ResizeGraphics();

		mouse_is_dragging = true;
	}
}

void IPlugGUIResize::OnMouseOver(int x, int y, IMouseMod * pMod)
{
	if (!gui_should_be_closed)
		SetCursor(LoadCursor(NULL, IDC_SIZENWSE));
}

void IPlugGUIResize::OnMouseOut()
{
	SetCursor(LoadCursor(NULL, IDC_ARROW));
}

void IPlugGUIResize::OnMouseDown(int x, int y, IMouseMod * pMod)
{
	if (!gui_should_be_closed)
		SetCursor(LoadCursor(NULL, IDC_SIZENWSE));

	if (pMod->L && using_bitmaps && fast_bitmap_resizing && handle_controls_gui_scaling && !gui_should_be_closed)
	{
		mTargetRECT = mRECT = IRECT(0, 0, plugin_width, plugin_height);

		currentlyFastResizing = true;
	}

	if (pMod->R)
	{
		DoPopupMenu();
	}
}

void IPlugGUIResize::OnMouseDblClick(int x, int y, IMouseMod* pMod)
{
	gui_scale_ratio = 1.0;
	ResizeGraphics();
}

void IPlugGUIResize::ResizeBackground()
{
	// Resize background to plugin width/height
	DRECT nonScaledDrawRect = DRECT(0, 0, window_width_normalized, window_height_normalized);

	mGraphics->GetControl(0)->SetNonScaledDrawRECT(DRECT_to_IRECT(&nonScaledDrawRect));

	mGraphics->GetControl(0)->SetDrawRECT(ResizeIRECT(&nonScaledDrawRect, gui_scale_ratio, gui_scale_ratio));
	mGraphics->GetControl(0)->SetTargetRECT(ResizeIRECT(&nonScaledDrawRect, gui_scale_ratio, gui_scale_ratio));
}

void IPlugGUIResize::OnMouseUp(int x, int y, IMouseMod * pMod)
{
	SetCursor(LoadCursor(NULL, IDC_ARROW));

	if (using_bitmaps && fast_bitmap_resizing && !gui_should_be_closed)
	{
		mGraphics->RescaleBitmaps(gui_scale_ratio);

		ResizeControlRects();
		InitializeGUIControls(mGraphics);
		currentlyFastResizing = false;
		mouse_is_dragging = false;
		global_gui_scale_ratio = gui_scale_ratio;
		mGraphics->SetAllControlsDirty();
	}
}

void IPlugGUIResize::SetIntToFile(const char * name, int x)
{
	sprintf(buf, "%u", x);
	WritePrivateProfileString("gui", name, buf, settings_ini_path.Get());
}

int IPlugGUIResize::GetIntFromFile(const char * name)
{
	return GetPrivateProfileInt("gui", name, -1, settings_ini_path.Get());
}

void IPlugGUIResize::SetDoubleToFile(const char * name, double x)
{
	sprintf(buf, "%.15f", x);
	WritePrivateProfileString("gui", name, buf, settings_ini_path.Get());
}

double IPlugGUIResize::GetDoubleFromFile(const char * name)
{
	GetPrivateProfileString("gui", name, "-1.0", buf, 128, settings_ini_path.Get());
	return atof(buf);
}

bool IPlugGUIResize::IsDirty()
{
	if (using_bitmaps && plugin_resized && !gui_should_be_closed && !currentlyFastResizing)
	{
		gui_should_be_closed = !double_equals(global_gui_scale_ratio, gui_scale_ratio);

		if (gui_should_be_closed)
		{
			mTargetRECT = mRECT = IRECT(0, 0, plugin_width, plugin_height);
		}
	}

	return gui_should_be_closed;
}


// Horizontal handle ------------------------------------------------------------------------------------
HorisontalResizing::HorisontalResizing(IPlugBase *pPlug, IGraphics *pGraphics, int width)
	: IControl(pPlug, IRECT(pGraphics->Width() - width, 0, pGraphics->Width(), pGraphics->Height()))
{
	mGraphics = pGraphics;
	mGraphics->HandleMouseOver(true);
}

bool HorisontalResizing::Draw(IGraphics * pGraphics)
{
	//pGraphics->FillIRect(&COLOR_RED, &mRECT);
	return true;
}

void HorisontalResizing::OnMouseDown(int x, int y, IMouseMod * pMod)
{
	SetCursor(LoadCursor(NULL, IDC_SIZEWE));
}

void HorisontalResizing::OnMouseDrag(int x, int y, int dX, int dY, IMouseMod * pMod)
{
	SetCursor(LoadCursor(NULL, IDC_SIZEWE));

	double window_width_normalized = (double)x / GetGUIResize()->GetGUIScaleRatio();

	GetGUIResize()->SetWindowWidth(window_width_normalized);

	GetGUIResize()->ResizeGraphics();
}

void HorisontalResizing::OnMouseOver(int x, int y, IMouseMod * pMod)
{
	SetCursor(LoadCursor(NULL, IDC_SIZEWE));
}

void HorisontalResizing::OnMouseOut()
{
	SetCursor(LoadCursor(NULL, IDC_ARROW));
}


// Vertical handle ------------------------------------------------------------------------------------
VerticalResizing::VerticalResizing(IPlugBase *pPlug, IGraphics *pGraphics, int height)
	: IControl(pPlug, IRECT(0, pGraphics->Height() - height, pGraphics->Width(), pGraphics->Height()))
{
	mGraphics = pGraphics;
	mGraphics->HandleMouseOver(true);
}

bool VerticalResizing::Draw(IGraphics * pGraphics)
{
	//pGraphics->FillIRect(&COLOR_GREEN, &mRECT);
	return true;
}

void VerticalResizing::OnMouseDown(int x, int y, IMouseMod * pMod)
{
	SetCursor(LoadCursor(NULL, IDC_SIZENS));
}

void VerticalResizing::OnMouseDrag(int x, int y, int dX, int dY, IMouseMod * pMod)
{
	SetCursor(LoadCursor(NULL, IDC_SIZENS));

	double window_height_normalized = (double)y / GetGUIResize()->GetGUIScaleRatio();

	GetGUIResize()->SetWindowHeight(window_height_normalized);

	GetGUIResize()->ResizeGraphics();
}

void VerticalResizing::OnMouseOver(int x, int y, IMouseMod * pMod)
{
	SetCursor(LoadCursor(NULL, IDC_SIZENS));
}

void VerticalResizing::OnMouseOut()
{
	SetCursor(LoadCursor(NULL, IDC_ARROW));
}
