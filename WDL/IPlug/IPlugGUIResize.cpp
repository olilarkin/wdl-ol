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
	return IRECT((int)(old_IRECT->L * width_ratio), (int)(old_IRECT->T * height_ratio), (int)(old_IRECT->R * width_ratio), (int)(old_IRECT->B * height_ratio));
}
// --------------------------------------------------------------------------------------------------------------------



// IPlugGUIResize -----------------------------------------------------------------------------------------------------
IPlugGUIResize::IPlugGUIResize(IPlugBase * pPlug, IGraphics * pGraphics, int guiWidth, int guiHeight, const char * bundleName, bool useHandle, int controlSize, int minimumControlSize)
	: IControl(pPlug, IRECT(0, 0, 0, 0))
{
	mGraphics = pGraphics;
	layout_container.resize(1);
	use_handle = useHandle;

	default_gui_width = guiWidth;
	default_gui_height = guiHeight;
	window_width_normalized = (double)guiWidth;
	window_height_normalized = (double)guiHeight;

	// Set default view dimensions
	view_container.view_mode.push_back(0);
	view_container.view_width.push_back(guiWidth);
	view_container.view_height.push_back(guiHeight);

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
	pGraphics->AppSupportPath(&settings_ini_path);
	settings_ini_path.Append("/");
	settings_ini_path.Append(bundleName);
	settings_ini_path.Append("/settings.ini");

	// Check if gui size was written in settings.ini, if not write defaults
	gui_scale_ratio = GetDoubleFromFile("guiscale");
	if (gui_scale_ratio < 0.0)
	{
		SetDoubleToFile("guiscale", 1.0);
		gui_scale_ratio = 1.0;
	}

	// Initiaize parameters
	mPlug->GetParam(viewMode)->InitInt("", -1, -1, 1000000);
	mPlug->GetParam(viewMode)->SetCanAutomate(false);

	mPlug->GetParam(windowWidth)->InitDouble("", -1.0, -1.0, 1000000, 1.0);
	mPlug->GetParam(windowWidth)->SetCanAutomate(false);

	mPlug->GetParam(windowHeight)->InitDouble("", -1.0, -1.0, 1000000, 1.0);
	mPlug->GetParam(windowHeight)->SetCanAutomate(false);
}

bool IPlugGUIResize::Draw(IGraphics * pGraphics)
{
	if (mouse_is_down)
	{
		IRECT backgroundRECT = IRECT(0, 0, plugin_width, plugin_height);
		DrawBackgroundAtFastResizing(pGraphics, &backgroundRECT);
	}
	else
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


	IColor textColor = IColor(255, 255, 255, 255);

	int textSize = int(77.0 * (double)pGraphics->Width() / (double)default_gui_width);

	IText textProps = IText(textSize, &textColor, "Arial", IText::kStyleItalic, IText::kAlignNear);

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
	// Add control sizes to a global container. This is to fix the problem of bitmap resizing on load
	if (global_layout_container.size() == 0)
	{
		global_layout_container.resize(1);

		// Backup original controls sizes
		for (int i = 0; i < mGraphics->GetNControls(); i++)
		{
			IControl* pControl = mGraphics->GetControl(i);

			global_layout_container[0].org_draw_area.push_back(IRECT_to_DRECT(&*pControl->GetRECT()));
			global_layout_container[0].org_target_area.push_back(IRECT_to_DRECT(&*pControl->GetTargetRECT()));
			global_layout_container[0].org_text_size.push_back(*pControl->GetText());
		}

		// Add IPlugGUIResize control size
		global_layout_container[0].org_draw_area.push_back(IRECT_to_DRECT(&gui_resize_area));
		global_layout_container[0].org_target_area.push_back(IRECT_to_DRECT(&gui_resize_area));
		IText tmpIText;
		global_layout_container[0].org_text_size.push_back(tmpIText);
	}

	// Adding global layout container to a local one
	layout_container[0] = global_layout_container[0];

	// Backup original controls visibility
	for (int i = 0; i < mGraphics->GetNControls(); i++)
	{
		IControl* pControl = mGraphics->GetControl(i);

		controls_visibility.push_back(pControl->IsHidden());
	}

	// Adding new layout for this view. By default it is copying default view layout
	for (int i = 0; i < view_container.view_mode.size(); i++)
	{
		layout_container.push_back(layout_container[0]);
	}

	RescaleBitmapsAtLoad(mGraphics);

	InitializeGUIControls(mGraphics);

	mGraphics->SetAllControlsDirty();

	return this;
}

void IPlugGUIResize::UseHandleForGUIScaling(bool statement) 
{ 
	handle_gui_scaling = statement; 
}

void IPlugGUIResize::AddNewView(int viewMode, int viewWidth, int viewHeight)
{
	view_container.view_mode.push_back(viewMode);
	view_container.view_width.push_back(viewWidth);
	view_container.view_height.push_back(viewHeight);
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

void IPlugGUIResize::SetWindowSize(int width, int height)
{
	window_width_normalized = (double)width;
	window_height_normalized = (double)height;
}

void  IPlugGUIResize::UsingBitmaps()
{
	using_bitmaps = true;
}

void IPlugGUIResize::DisableFastBitmapResizing()
{
	fast_bitmap_resizing = false;
}

void IPlugGUIResize::SmoothResizedBitmaps()
{
	smooth_bitmap_resizing = true;
}

void IPlugGUIResize::HideControl(int index)
{
	IControl* pControl = mGraphics->GetControl(index);
	pControl->Hide(true);
}

void IPlugGUIResize::ShowControl(int index)
{
	IControl* pControl = mGraphics->GetControl(index);
	pControl->Hide(false);
}

void IPlugGUIResize::MoveControl(int index, double x, double y, resizeFlag flag)
{
	IControl* pControl = mGraphics->GetControl(index);

	double x_relative = x * gui_scale_ratio;
	double y_relative = y * gui_scale_ratio;

	if (flag == drawAndTargetArea || flag == drawAreaOnly)
	{
		double drawAreaW = (double)pControl->GetRECT()->W();
		double drawAreaH = (double)pControl->GetRECT()->H();

		DRECT drawArea = DRECT(x_relative, y_relative, x_relative + drawAreaW, y_relative + drawAreaH);
		pControl->SetDrawArea(DRECT_to_IRECT(&drawArea));

		double org_draw_width = layout_container[current_view_mode].org_draw_area[index].W();
		double org_draw_height = layout_container[current_view_mode].org_draw_area[index].H();

		layout_container[current_view_mode].org_draw_area[index].L = x;
		layout_container[current_view_mode].org_draw_area[index].T = y;
		layout_container[current_view_mode].org_draw_area[index].R = x + org_draw_width;
		layout_container[current_view_mode].org_draw_area[index].B = y + org_draw_height;

	}

	if (flag == drawAndTargetArea || flag == targetAreaOnly)
	{
		double targetAreaW = (double)pControl->GetTargetRECT()->W();
		double targetAreaH = (double)pControl->GetTargetRECT()->H();

		DRECT targetArea = DRECT(x_relative, y_relative, x_relative + targetAreaW, y_relative + targetAreaH);
		pControl->SetTargetArea(DRECT_to_IRECT(&targetArea));

		double org_target_width = layout_container[current_view_mode].org_draw_area[index].W();
		double org_target_height = layout_container[current_view_mode].org_draw_area[index].H();

		layout_container[current_view_mode].org_target_area[index].L = x;
		layout_container[current_view_mode].org_target_area[index].T = y;
		layout_container[current_view_mode].org_target_area[index].R = x + org_target_width;
		layout_container[current_view_mode].org_target_area[index].B = y + org_target_height;

	}
}

void IPlugGUIResize::MoveControlRightEdge(int index, double R, resizeFlag flag)
{
	IControl* pControl = mGraphics->GetControl(index);

	double R_relative = R * gui_scale_ratio;

	if (flag == drawAndTargetArea || flag == drawAreaOnly)
	{
		double drawAreaL = (double)pControl->GetRECT()->L;
		double drawAreaT = (double)pControl->GetRECT()->T;
		double drawAreaB = (double)pControl->GetRECT()->B;

		DRECT drawArea = DRECT(drawAreaL, drawAreaT, R_relative, drawAreaB);
		pControl->SetDrawArea(DRECT_to_IRECT(&drawArea));

		layout_container[current_view_mode].org_draw_area[index].R = R;
	}

	if (flag == drawAndTargetArea || flag == targetAreaOnly)
	{
		double targetAreaL = (double)pControl->GetTargetRECT()->L;
		double targetAreaT = (double)pControl->GetTargetRECT()->T;
		double targetAreaB = (double)pControl->GetTargetRECT()->B;

		DRECT targetArea = DRECT(targetAreaL, targetAreaT, R_relative, targetAreaB);
		pControl->SetTargetArea(DRECT_to_IRECT(&targetArea));

		layout_container[current_view_mode].org_target_area[index].R = R;
	}
}

void IPlugGUIResize::MoveControlBottomEdge(int index, double B, resizeFlag flag)
{
	IControl* pControl = mGraphics->GetControl(index);

	double B_relative = B * gui_scale_ratio;

	if (flag == drawAndTargetArea || flag == drawAreaOnly)
	{
		double drawAreaL = (double)pControl->GetRECT()->L;
		double drawAreaR = (double)pControl->GetRECT()->R;
		double drawAreaT = (double)pControl->GetRECT()->T;

		DRECT drawArea = DRECT(drawAreaL, drawAreaT, drawAreaR, B_relative);
		pControl->SetDrawArea(DRECT_to_IRECT(&drawArea));

		layout_container[current_view_mode].org_draw_area[index].B = B;

	}

	if (flag == drawAndTargetArea || flag == targetAreaOnly)
	{
		double targetAreaL = (double)pControl->GetTargetRECT()->L;
		double targetAreaR = (double)pControl->GetTargetRECT()->R;
		double targetAreaT = (double)pControl->GetTargetRECT()->T;

		DRECT targetArea = DRECT(targetAreaL, targetAreaT, targetAreaR, B_relative);
		pControl->SetTargetArea(DRECT_to_IRECT(&targetArea));

		layout_container[current_view_mode].org_target_area[index].B = B;
	}
}

double IPlugGUIResize::GetGUIScaleRatio()
{
	return gui_scale_ratio;
}

int IPlugGUIResize::GetViewMode()
{
	return view_mode;
}

void IPlugGUIResize::ResizeControlRects()
{
	// Set new target and draw area
	for (int i = 1; i < mGraphics->GetNControls(); i++)
	{
		// This updates draw and control rect
		IControl* pControl = mGraphics->GetControl(i);
		pControl->SetDrawArea(ResizeIRECT(&layout_container[current_view_mode].org_draw_area[i], gui_scale_ratio, gui_scale_ratio));
		pControl->SetTargetArea(ResizeIRECT(&layout_container[current_view_mode].org_target_area[i], gui_scale_ratio, gui_scale_ratio));

		// This updates IText size
		IText tmpText = IText((int)((double)layout_container[current_view_mode].org_text_size[i].mSize * gui_scale_ratio), &layout_container[current_view_mode].org_text_size[i].mColor,
			layout_container[current_view_mode].org_text_size[i].mFont, layout_container[current_view_mode].org_text_size[i].mStyle, layout_container[current_view_mode].org_text_size[i].mAlign, layout_container[current_view_mode].org_text_size[i].mOrientation,
			layout_container[current_view_mode].org_text_size[i].mQuality, &layout_container[current_view_mode].org_text_size[i].mTextEntryBGColor, &layout_container[current_view_mode].org_text_size[i].mTextEntryFGColor);

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

void IPlugGUIResize::InitializeGUIControls(IGraphics * pGraphics)
{
	// Call GUI initializer
	for (int i = 0; i < pGraphics->GetNControls(); i++)
	{
		IControl* pControl = pGraphics->GetControl(i);
		pControl->InitializeGUI(gui_scale_ratio);
	}
	pGraphics->SetAllControlsDirty();
}

void IPlugGUIResize::ResetControlsVisibility()
{
	for (int i = 0; i < mGraphics->GetNControls(); i++)
	{
		IControl* pControl = mGraphics->GetControl(i);
		pControl->Hide(controls_visibility[0]);
	}
}

void IPlugGUIResize::RescaleBitmapsAtLoad(IGraphics * pGraphics)
{
	if (!bitmaps_rescaled_at_load)
	{
		pGraphics->RescaleBitmaps(gui_scale_ratio);

		if (smooth_bitmap_resizing)
		{
			mGraphics->SmoothResizedBitmaps();
		}

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
		if (mPlug->GetParam(viewMode)->Value() > -0.5)
			current_view_mode = (int)mPlug->GetParam(viewMode)->Value();

		if (mPlug->GetParam(windowWidth)->Value() > -0.5)
			window_width_normalized = mPlug->GetParam(windowWidth)->Value();

		if (mPlug->GetParam(windowHeight)->Value() > -0.5)
			window_height_normalized = mPlug->GetParam(windowHeight)->Value();

		presets_loaded = true;
	}

	gui_scale_ratio = GetDoubleFromFile("guiscale");

	plugin_width = (int)(window_width_normalized * gui_scale_ratio);
	plugin_height = (int)(window_height_normalized * gui_scale_ratio);

	MoveHandle();
	ResizeBackground();
	ResetControlsVisibility();
	mPlug->SetGUILayout(current_view_mode, window_width_normalized, window_height_normalized);

	// Prevent resizing if it is not needed
	if (prev_plugin_width != plugin_width || prev_plugin_height != plugin_height)
	{
		if (using_bitmaps && !bitmaps_rescaled_at_load_skip && !double_equals(global_gui_scale_ratio, gui_scale_ratio))
		{
			mGraphics->RescaleBitmaps(gui_scale_ratio);

			if (smooth_bitmap_resizing)
			{
				mGraphics->SmoothResizedBitmaps();
			}
		}
		bitmaps_rescaled_at_load_skip = false;

		ResizeControlRects();
		InitializeGUIControls(mGraphics);
		mGraphics->Resize(plugin_width, plugin_height);

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

	// Set parameters
	mPlug->GetParam(viewMode)->Set(current_view_mode);
	mPlug->GetParam(windowWidth)->Set(window_width_normalized);
	mPlug->GetParam(windowHeight)->Set(window_height_normalized);

	plugin_width = (int)(window_width_normalized * gui_scale_ratio);
	plugin_height = (int)(window_height_normalized * gui_scale_ratio);

	MoveHandle();
	ResizeBackground();
	ResetControlsVisibility();
	mPlug->SetGUILayout(current_view_mode, window_width_normalized, window_height_normalized);

	if (using_bitmaps)
	{
		if (!fast_bitmap_resizing)
		{
			if (!window_resizing)
			{
				mGraphics->RescaleBitmaps(gui_scale_ratio);

				if (smooth_bitmap_resizing)
				{
					mGraphics->SmoothResizedBitmaps();
				}
			}

			ResizeControlRects();
			InitializeGUIControls(mGraphics);
			mGraphics->Resize(plugin_width, plugin_height);
		}
		else
		{
			if (!mouse_is_down)
			{
				ResizeControlRects();

				InitializeGUIControls(mGraphics);
			}

			mGraphics->Resize(plugin_width, plugin_height);
		}

		plugin_resized = true;
	}
	else
	{
		ResizeControlRects();

		InitializeGUIControls(mGraphics);
		mGraphics->Resize(plugin_width, plugin_height);
	}

	mGraphics->SetAllControlsDirty();
}

void IPlugGUIResize::MoveHandle()
{
	int index = layout_container[current_view_mode].org_draw_area.size() - 1;
	double x = (double)plugin_width / gui_scale_ratio - control_size;
	double y = (double)plugin_height / gui_scale_ratio - control_size;

	double org_draw_width = layout_container[current_view_mode].org_draw_area[index].W();
	double org_draw_height = layout_container[current_view_mode].org_draw_area[index].H();

	layout_container[current_view_mode].org_draw_area[index].L = x;
	layout_container[current_view_mode].org_draw_area[index].T = y;
	layout_container[current_view_mode].org_draw_area[index].R = x + org_draw_width;
	layout_container[current_view_mode].org_draw_area[index].B = y + org_draw_height;

	double org_target_width = layout_container[current_view_mode].org_draw_area[index].W();
	double org_target_height = layout_container[current_view_mode].org_draw_area[index].H();

	layout_container[current_view_mode].org_target_area[index].L = x;
	layout_container[current_view_mode].org_target_area[index].T = y;
	layout_container[current_view_mode].org_target_area[index].R = x + org_target_width;
	layout_container[current_view_mode].org_target_area[index].B = y + org_target_height;
}

void IPlugGUIResize::OnMouseDrag(int x, int y, int dX, int dY, IMouseMod * pMod)
{
	double prev_plugin_width = plugin_width;
	double prev_plugin_height = plugin_height;

	if (!gui_should_be_closed)
	{
		SetCursor(LoadCursor(NULL, IDC_SIZENWSE));

		if (handle_gui_scaling)
		{
			// Sets GUI scale
			gui_scale_ratio = (((double)x + (double)y) / 2.0) / ((window_width_normalized + window_height_normalized) / 2.0);
			global_gui_scale_ratio = gui_scale_ratio;
			plugin_width = (int)(window_width_normalized * gui_scale_ratio);
			plugin_height = (int)(window_height_normalized * gui_scale_ratio);
		}
		else
		{
			window_width_normalized = (double)x / gui_scale_ratio;
			window_height_normalized = (double)y / gui_scale_ratio;
		}


		if (using_bitmaps && fast_bitmap_resizing && handle_gui_scaling)
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

	if (pMod->L && using_bitmaps && fast_bitmap_resizing && handle_gui_scaling && !gui_should_be_closed)
	{
		mTargetRECT = mRECT = IRECT(0, 0, plugin_width, plugin_height);

		mouse_is_down = true;
	}

	if (pMod->R)
	{
		DoPopupMenu();
	}
}

void IPlugGUIResize::ResizeBackground()
{
	// Resize background to plugin width/height
	mGraphics->GetControl(0)->SetDrawArea(IRECT(0, 0, plugin_width, plugin_height));
	mGraphics->GetControl(0)->SetTargetArea(IRECT(0, 0, plugin_width, plugin_height));
}

void IPlugGUIResize::OnMouseUp(int x, int y, IMouseMod * pMod)
{
	SetCursor(LoadCursor(NULL, IDC_ARROW));

	if (using_bitmaps && fast_bitmap_resizing && !gui_should_be_closed)
	{
		mGraphics->RescaleBitmaps(gui_scale_ratio);

		if (smooth_bitmap_resizing)
		{
			mGraphics->SmoothResizedBitmaps();
		}

		ResizeControlRects();
		InitializeGUIControls(mGraphics);
		mouse_is_down = false;
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
