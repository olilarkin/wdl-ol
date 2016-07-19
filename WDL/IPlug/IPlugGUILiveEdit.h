#ifndef _IPLUGGUILIVEEDIT_
#define _IPLUGGUILIVEEDIT_

/*
Youlean - IPlugGUILiveEdit - live GUI editing class

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
USE:

Press E key to activate;
Press B to change editing mode to include both draw and terget rect;
Press D to change editing mode to include both draw;
Press T to change editing mode to include terget rect;
Press ALT while moving control to unsnap from grid;
Press SHIFT while moving control to snap to other controls;
Press SHIFT + B to force snapping to controls that have same draw and target rect;
Press SHIFT + D to force snapping to controls to draw rect;
Press SHIFT + T to force snapping to controls to target rect;
Press CTRL + LEFT MOUSE BUTTON and drag to make a selection;
*/

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <typeinfo>
#include "IControl.h"
#include "IGraphics.h"
#include "IPlugGUIResize.h"

using namespace std;

class IPlugGUILiveEdit
{
public:
	IPlugGUILiveEdit() {}
	~IPlugGUILiveEdit() {}

	void EditGUI(IPlugBase* pPlug, IGraphics* pGraphics, WDL_PtrList<IControl>* pControls, LICE_IBitmap* pDrawBitmap, 
		IMouseMod* liveEditingMod, int* liveGridSize, int* liveSnap, int* liveKeyDown, bool* liveToogleEditing, int* liveMouseCapture,
		bool* liveMouseDragging, int* liveMode, int* mMouseX, int* mMouseY, int width, int height, double guiScaleRatio)
	{
		// Moving controls --------------------------------------------------------------------
	
		if (pPlug->GetGUIResize()) liveScaledGridSize = int((double)*liveGridSize * guiScaleRatio);
		else liveScaledGridSize = *liveGridSize;

		if (pPlug->GetGUIResize())
		{
			currentViewMode = pPlug->GetGUIResize()->GetViewMode();
			viewModeSize = pPlug->GetGUIResize()->GetViewModeSize();

			// Resize container to have room for all views
			code_view_mode.resize(viewModeSize);
		}
		else
		{
			currentViewMode = 0;
			viewModeSize = 1;

			// Resize container to have room for all views
			code_view_mode.resize(viewModeSize);
		}

		if (!retrieveOldLayoutChanges)
		{
			RetrieveOldLayoutChanges();
			retrieveOldLayoutChanges = true;
		}

		// Toogle live editing
		if (*liveToogleEditing)
		{
			if (*liveKeyDown == 19)
			{
				*liveToogleEditing = false;
				*liveKeyDown = -1;
			}
		}
		else
		{
			if (*liveKeyDown == 19)
			{
				*liveToogleEditing = true;
				*liveKeyDown = -1;
			}
		}

		// Toogle snap mode
		if (liveEditingMod->S && *liveKeyDown == 16)
		{
			snapMode = 0;
		}

		else if (liveEditingMod->S && *liveKeyDown == 18)
		{
			snapMode = 1;
		}

		else if (liveEditingMod->S && *liveKeyDown == 34)
		{
			snapMode = 2;
		}

		// Toogle mode
		if (!liveEditingMod->S && *liveKeyDown == 16)
		{
			*liveMode = 0;
			liveControlNumber = -1;
			*liveKeyDown = -1;
			selectedControlsRECT = IRECT(999999999, 999999999, 0, 0);
		}

		else if (!liveEditingMod->S && *liveKeyDown == 18)
		{
			*liveMode = 1;
			liveControlNumber = -1;
			*liveKeyDown = -1;
			selectedControlsRECT = IRECT(999999999, 999999999, 0, 0);
		}

		else if (!liveEditingMod->S && *liveKeyDown == 34)
		{
			*liveMode = 2;
			liveControlNumber = -1;
			*liveKeyDown = -1;
			selectedControlsRECT = IRECT(999999999, 999999999, 0, 0);
		}

		// If snap mode is not forced to change
		if (!liveEditingMod->S) snapMode = *liveMode;
	

		// If live editing is not active disable selected rect
		if (!*liveToogleEditing) selectedControlsRECT = IRECT(999999999, 999999999, 0, 0);

		// If mouse was clicked
		if (*liveToogleEditing)
		{
			pGraphics->SetAllControlsDirty();

			PrintDeletedControls(pGraphics);

			// Draw control rects
			int controlSize = pControls->GetSize();

			if (pPlug->GetGUIResize())
			{
				controlSize -= 3;
				if (*liveMouseCapture > controlSize) *liveMouseCapture = -1;
			}

			// Draw resizing handles
			liveHandleSize = int(8.0 * guiScaleRatio);

			bool overControlHandle = false;
			if (selectedControlsRECT == IRECT(999999999, 999999999, 0, 0))
			{
				// Find if over control handle
				for (int j = 1; j < controlSize; j++)
				{
					IControl* pControl = pControls->Get(j);

					if (!IsControlDeleted(pControl))
					{
						IRECT tmpRECT(0, 0, 0, 0);
						if (*liveMode == 0 && (*pControl->GetRECT() == *pControl->GetTargetRECT())) tmpRECT = *pControl->GetRECT();
						else if (*liveMode == 1) tmpRECT = *pControl->GetRECT();
						else if (*liveMode == 2) tmpRECT = *pControl->GetTargetRECT();

						IRECT handle = IRECT(tmpRECT.R - liveHandleSize, tmpRECT.B - liveHandleSize, tmpRECT.R, tmpRECT.B);

						if (tmpRECT.Contains(*mMouseX, *mMouseY))
						{
							overControlHandle = handle.Contains(*mMouseX, *mMouseY);
						}
					}
				}
			}

			if (!*liveMouseDragging)
			{
				// Change cursor when over handle
				if (overControlHandle) SetCursor(LoadCursor(NULL, IDC_SIZENWSE));
				else SetCursor(LoadCursor(NULL, IDC_ARROW));
			}


			if (liveEditingMod->R && !IsControlDeleted(pControls->Get(*liveMouseCapture)))
			{
				IControl* pControl = pControls->Get(*liveMouseCapture);

				liveSelectedRECT = *pControl->GetRECT();
				liveSelectedTargetRECT = *pControl->GetTargetRECT();
				liveControlNumber = *liveMouseCapture;

				// Find where mouse was clicked
				if (*liveMouseCapture != lastliveMouseCapture)
				{
					liveClickedX = *mMouseX;
					liveClickedY = *mMouseY;
					liveClickedRECT = liveSelectedRECT;
					liveClickedTargetRECT = liveSelectedTargetRECT;

					IRECT tmpRECT(0, 0, 0, 0);
					if (*liveMode == 0 && (*pControl->GetRECT() == *pControl->GetTargetRECT())) tmpRECT = *pControl->GetRECT();
					else if (*liveMode == 1) tmpRECT = *pControl->GetRECT();
					else if (*liveMode == 2) tmpRECT = *pControl->GetTargetRECT();

					IRECT handle = IRECT(tmpRECT.R - liveHandleSize, tmpRECT.B - liveHandleSize, tmpRECT.R, tmpRECT.B);
					liveClickedOnHandle = handle.Contains(liveClickedX, liveClickedY);
				}
			}

			// Prepare undo to be executed on next mose click if nedeed
			if (!liveEditingMod->L) 
				undoMove = true;
			
			// Check if clicked on selection
			if ((liveEditingMod->L != lastLDown) || (liveEditingMod->R != lastRDown))
			{
				clickedX = *mMouseX;
				clickedY = *mMouseY;

				lastLDown = liveEditingMod->L;
				lastRDown = liveEditingMod->R;
			}

			if (liveEditingMod->L && !liveEditingMod->C)
			{
				if (!selectionRECT.Contains(clickedX, clickedY)) selectedControlsRECT = IRECT(999999999, 999999999, 0, 0);
			}

            if (selectedControlsRECT == IRECT(999999999, 999999999, 0, 0)) multipleControlsSelected = false;
			else multipleControlsSelected = true;

			// Move controls
			if (liveEditingMod->L && !liveEditingMod->C && !multipleControlsSelected && !IsControlDeleted(pControls->Get(*liveMouseCapture)))
			{
				MoveControl(pPlug, pGraphics, pDrawBitmap, pControls, liveEditingMod, liveSnap, 
					liveMouseCapture, controlSize, overControlHandle, liveHandleSize, liveMouseDragging, liveMode, mMouseX, mMouseY);
			}

			// Move selection of controls
			if (liveEditingMod->C || multipleControlsSelected)
			{
				MoveSelectionOfControls(pPlug, pGraphics, pDrawBitmap, pControls, liveEditingMod, liveSnap,
					liveMouseCapture, controlSize, overControlHandle, liveHandleSize, liveMouseDragging, liveMode, mMouseX, mMouseY);
			}
			else
			{
				prevSelectingControls = false;
			}


			*liveMouseDragging = false;
			lastliveMouseCapture = *liveMouseCapture;
		}

		if (*liveToogleEditing)
		{
			// Draw control rects
			int controlSize = pControls->GetSize();

			if (pPlug->GetGUIResize())
			{
				controlSize -= 3;
				if (*liveMouseCapture > controlSize) *liveMouseCapture = -1;
			}

			for (int j = 1; j < controlSize; j++)
			{
				IControl* pControl = pControls->Get(j);

				IRECT tmpRECT(0, 0, 0, 0);
				IColor color;
				if (*liveMode == 0) color = EDIT_COLOR_BOTH;
				else if (*liveMode == 1) color = EDIT_COLOR_DRAW;
				else if (*liveMode == 2) color = EDIT_COLOR_TARGET;

				// Dont outline deleted control
				if (!IsControlDeleted(pControl))
				{
					if (*pControl->GetRECT() == *pControl->GetTargetRECT())
					{
						tmpRECT = *pControl->GetRECT();

						// T
						LICE_DashedLine(pDrawBitmap, tmpRECT.L, tmpRECT.T, tmpRECT.R, tmpRECT.T, 2, 2,
							LICE_RGBA(EDIT_COLOR_BOTH.R, EDIT_COLOR_BOTH.G, EDIT_COLOR_BOTH.B, EDIT_COLOR_BOTH.A));
						//B
						LICE_DashedLine(pDrawBitmap, tmpRECT.L, tmpRECT.B, tmpRECT.R, tmpRECT.B, 2, 2,
							LICE_RGBA(EDIT_COLOR_BOTH.R, EDIT_COLOR_BOTH.G, EDIT_COLOR_BOTH.B, EDIT_COLOR_BOTH.A));
						//L
						LICE_DashedLine(pDrawBitmap, tmpRECT.L, tmpRECT.T, tmpRECT.L, tmpRECT.B, 2, 2,
							LICE_RGBA(EDIT_COLOR_BOTH.R, EDIT_COLOR_BOTH.G, EDIT_COLOR_BOTH.B, EDIT_COLOR_BOTH.A));
						//R
						LICE_DashedLine(pDrawBitmap, tmpRECT.R, tmpRECT.T, tmpRECT.R, tmpRECT.B, 2, 2,
							LICE_RGBA(EDIT_COLOR_BOTH.R, EDIT_COLOR_BOTH.G, EDIT_COLOR_BOTH.B, EDIT_COLOR_BOTH.A));
					}
					else
					{
						tmpRECT = *pControl->GetTargetRECT();

						// T
						LICE_DashedLine(pDrawBitmap, tmpRECT.L, tmpRECT.T, tmpRECT.R, tmpRECT.T, 2, 2,
							LICE_RGBA(EDIT_COLOR_TARGET.R, EDIT_COLOR_TARGET.G, EDIT_COLOR_TARGET.B, EDIT_COLOR_TARGET.A));
						//B
						LICE_DashedLine(pDrawBitmap, tmpRECT.L, tmpRECT.B, tmpRECT.R, tmpRECT.B, 2, 2,
							LICE_RGBA(EDIT_COLOR_TARGET.R, EDIT_COLOR_TARGET.G, EDIT_COLOR_TARGET.B, EDIT_COLOR_TARGET.A));
						//L
						LICE_DashedLine(pDrawBitmap, tmpRECT.L, tmpRECT.T, tmpRECT.L, tmpRECT.B, 2, 2,
							LICE_RGBA(EDIT_COLOR_TARGET.R, EDIT_COLOR_TARGET.G, EDIT_COLOR_TARGET.B, EDIT_COLOR_TARGET.A));
						//R
						LICE_DashedLine(pDrawBitmap, tmpRECT.R, tmpRECT.T, tmpRECT.R, tmpRECT.B, 2, 2,
							LICE_RGBA(EDIT_COLOR_TARGET.R, EDIT_COLOR_TARGET.G, EDIT_COLOR_TARGET.B, EDIT_COLOR_TARGET.A));


						tmpRECT = *pControl->GetRECT();

						// T
						LICE_DashedLine(pDrawBitmap, tmpRECT.L, tmpRECT.T, tmpRECT.R, tmpRECT.T, 2, 2,
							LICE_RGBA(EDIT_COLOR_DRAW.R, EDIT_COLOR_DRAW.G, EDIT_COLOR_DRAW.B, EDIT_COLOR_DRAW.A));
						//B
						LICE_DashedLine(pDrawBitmap, tmpRECT.L, tmpRECT.B, tmpRECT.R, tmpRECT.B, 2, 2,
							LICE_RGBA(EDIT_COLOR_DRAW.R, EDIT_COLOR_DRAW.G, EDIT_COLOR_DRAW.B, EDIT_COLOR_DRAW.A));
						//L
						LICE_DashedLine(pDrawBitmap, tmpRECT.L, tmpRECT.T, tmpRECT.L, tmpRECT.B, 2, 2,
							LICE_RGBA(EDIT_COLOR_DRAW.R, EDIT_COLOR_DRAW.G, EDIT_COLOR_DRAW.B, EDIT_COLOR_DRAW.A));
						//R
						LICE_DashedLine(pDrawBitmap, tmpRECT.R, tmpRECT.T, tmpRECT.R, tmpRECT.B, 2, 2,
							LICE_RGBA(EDIT_COLOR_DRAW.R, EDIT_COLOR_DRAW.G, EDIT_COLOR_DRAW.B, EDIT_COLOR_DRAW.A));
					}

				}
			}


			// Outline selected rect
			if (liveControlNumber > 0 && *liveMode == 0 && *pControls->Get(liveControlNumber)->GetRECT() == *pControls->Get(liveControlNumber)->GetTargetRECT()) 
				pGraphics->DrawRect(&EDIT_COLOR_BOTH, pControls->Get(liveControlNumber)->GetRECT());

			else if (liveControlNumber > 0)
			{
				pGraphics->DrawRect(&EDIT_COLOR_TARGET, pControls->Get(liveControlNumber)->GetTargetRECT());
				pGraphics->DrawRect(&EDIT_COLOR_DRAW, pControls->Get(liveControlNumber)->GetRECT());
			}

			// Draw connection between draw and target rect if it is not the same
			if (liveControlNumber > 0  && *pControls->Get(liveControlNumber)->GetRECT() != *pControls->Get(liveControlNumber)->GetTargetRECT())
			{
				IRECT drawRECT = *pControls->Get(liveControlNumber)->GetRECT();
				IRECT targetRECT = *pControls->Get(liveControlNumber)->GetTargetRECT();

					LICE_Line(pDrawBitmap, drawRECT.L + drawRECT.W() / 2, drawRECT.T + drawRECT.H() / 2,
						targetRECT.L + targetRECT.W() / 2, targetRECT.T + targetRECT.H() / 2,
						LICE_RGBA(EDIT_COLOR_BOTH.R, EDIT_COLOR_BOTH.G, EDIT_COLOR_BOTH.B, EDIT_COLOR_BOTH.A));

			}

			// Draw handles
			for (int j = 1; j < controlSize; j++)
			{
				IControl* pControl = pControls->Get(j);

				if (!IsControlDeleted(pControl))
				{
					IRECT tmpRECT(0,0,0,0);
					if (*liveMode == 0 && (*pControl->GetRECT() == *pControl->GetTargetRECT())) tmpRECT = *pControl->GetRECT();
					else if (*liveMode == 1) tmpRECT = *pControl->GetRECT();
					else if (*liveMode == 2) tmpRECT = *pControl->GetTargetRECT();

					IColor color;
					if (*liveMode == 0) color = EDIT_COLOR_BOTH;
					else if (*liveMode == 1) color = EDIT_COLOR_DRAW;
					else if (*liveMode == 2) color = EDIT_COLOR_TARGET;

					IRECT handle = IRECT(tmpRECT.R - liveHandleSize, tmpRECT.B - liveHandleSize, tmpRECT.R, tmpRECT.B);
					pGraphics->FillTriangle(&color, handle.L, handle.B, handle.R, handle.B, handle.R, handle.T, 0);
				}
			}

			if (drawGridToogle) DrawGrid(pDrawBitmap, width, height);

			// Prevent readout to go out of window borders
			//liveSelectedRECT = RestrictToWindow(liveSelectedRECT, pGraphics->Width(), pGraphics->Height());

			// Make mode rect
			IRECT modeRECT(0, 0, 0, 0);
			if (*liveMode == 0 && (liveSelectedRECT == liveSelectedTargetRECT)) modeRECT = liveSelectedRECT;
			else if (*liveMode == 1) modeRECT = liveSelectedRECT;
			else if (*liveMode == 2) modeRECT = liveSelectedTargetRECT;

			// Check if gui resize is active, if so scale out rect
			IRECT printRECT;
			if (pPlug->GetGUIResize())
			{
				printRECT.L = int((double)modeRECT.L * guiScaleRatio);
				printRECT.T = int((double)modeRECT.T * guiScaleRatio);
				printRECT.R = int((double)modeRECT.R * guiScaleRatio);
				printRECT.B = int((double)modeRECT.B * guiScaleRatio);

			}
			else
			{
				printRECT = modeRECT;
			}

			// Print if rect if valid
			if (printRECT.W() > 0 && printRECT.H() > 0)
			{
				// Print selected control
				int textSize = 15;
				WDL_String controlNumber;

				// Different outputs if control is hidden
				if (liveControlNumber > 0 && !pGraphics->GetControl(liveControlNumber)->IsHidden())
					controlNumber.SetFormatted(100, "N:%i IRECT(%i,%i,%i,%i)", liveControlNumber, printRECT.L, printRECT.T, printRECT.R, printRECT.B);
				if (liveControlNumber > 0 && pGraphics->GetControl(liveControlNumber)->IsHidden())
					controlNumber.SetFormatted(100, "N:%i IRECT(%i,%i,%i,%i) (HIDDEN)", liveControlNumber, printRECT.L, printRECT.T, printRECT.R, printRECT.B);

				IText txtControlNumber(textSize, &EDIT_COLOR_BOTH, defaultFont, IText::kStyleNormal, IText::kAlignNear, 0, IText::kQualityClearType);
				IRECT textRect;
				if (liveControlNumber > 0) pGraphics->MeasureIText(&txtControlNumber, controlNumber.Get(), &textRect);
				IRECT rectControlNumber(modeRECT.L, modeRECT.T - textSize, modeRECT.L + textRect.R, modeRECT.T);

				if (rectControlNumber.T < 0)
				{
					rectControlNumber.T = modeRECT.B;
					rectControlNumber.B = modeRECT.B + textSize;
				}

				if (liveControlNumber > 0) pGraphics->FillIRect(&controlTextBackgroundColor, &rectControlNumber);
				if (liveControlNumber > 0) pGraphics->DrawIText(&txtControlNumber, controlNumber.Get(), &rectControlNumber);
			}

			// Draw mouse coordinates
			if (drawMouseCoordinats)
			{
				IColor color;
				if (*liveMode == 0) color = EDIT_COLOR_BOTH;
				else if (*liveMode == 1) color = EDIT_COLOR_DRAW;
				else if (*liveMode == 2) color = EDIT_COLOR_TARGET;

				WDL_String str;
				str.SetFormatted(32, "x: %i, y: %i", *mMouseX, *mMouseY);
				IText txt(20, &color, defaultFont);
				IRECT rect(width - 150, height - 20, width, height);
				pGraphics->DrawIText(&txt, str.Get(), &rect);
			}

			
			if (liveEditingMod->R)
				DoPopupMenu(pPlug, pGraphics, *mMouseX, *mMouseY, liveMode, guiScaleRatio);

			// Write to file
			CreateLayoutCode(pPlug, pGraphics, guiScaleRatio, currentViewMode, false);
		}
	}

	void MoveControl(IPlugBase* pPlug, IGraphics* pGraphics, LICE_IBitmap* pDrawBitmap, WDL_PtrList<IControl>* pControls,
		IMouseMod* liveEditingMod, int* liveSnap, int* liveMouseCapture, int controlSize, bool overControlHandle, int liveHandleSize,
		bool* liveMouseDragging, int* liveMode, int* mMouseX, int* mMouseY)
	{

		IControl* pControl = pControls->Get(*liveMouseCapture);

		liveSelectedRECT = *pControl->GetRECT();
		liveSelectedTargetRECT = *pControl->GetTargetRECT();
		liveControlNumber = *liveMouseCapture;

		// Find where mouse was clicked
		if (*liveMouseCapture != lastliveMouseCapture)
		{
			liveClickedX = *mMouseX;
			liveClickedY = *mMouseY;
			liveClickedRECT = liveSelectedRECT;
			liveClickedTargetRECT = liveSelectedTargetRECT;

			IRECT tmpRECT(0, 0, 0, 0);
			if (*liveMode == 0 && (*pControl->GetRECT() == *pControl->GetTargetRECT())) tmpRECT = *pControl->GetRECT();
			else if (*liveMode == 1) tmpRECT = *pControl->GetRECT();
			else if (*liveMode == 2) tmpRECT = *pControl->GetTargetRECT();

			IRECT handle = IRECT(tmpRECT.R - liveHandleSize, tmpRECT.B - liveHandleSize, tmpRECT.R, tmpRECT.B);
			liveClickedOnHandle = handle.Contains(liveClickedX, liveClickedY);
		}

		// Change cursor when clicked on handle
		if (liveClickedOnHandle || overControlHandle) SetCursor(LoadCursor(NULL, IDC_SIZENWSE));
		else SetCursor(LoadCursor(NULL, IDC_ARROW));

		// Prevent editing
		if (*liveMouseDragging || liveEditingMod->S)
		{
			IRECT drawArea;
			if (!liveClickedOnHandle)
			{
				drawArea.L = liveClickedRECT.L + (*mMouseX - liveClickedX);
				drawArea.T = liveClickedRECT.T + (*mMouseY - liveClickedY);
				drawArea.R = liveClickedRECT.R + (*mMouseX - liveClickedX);
				drawArea.B = liveClickedRECT.B + (*mMouseY - liveClickedY);
			}
			else
			{
				drawArea.L = liveClickedRECT.L;
				drawArea.T = liveClickedRECT.T;
				drawArea.R = liveClickedRECT.R + (*mMouseX - liveClickedX);
				drawArea.B = liveClickedRECT.B + (*mMouseY - liveClickedY);
			}

			IRECT targetArea;
			if (!liveClickedOnHandle)
			{
				targetArea.L = liveClickedTargetRECT.L + (*mMouseX - liveClickedX);
				targetArea.T = liveClickedTargetRECT.T + (*mMouseY - liveClickedY);
				targetArea.R = liveClickedTargetRECT.R + (*mMouseX - liveClickedX);
				targetArea.B = liveClickedTargetRECT.B + (*mMouseY - liveClickedY);
			}
			else
			{
				targetArea.L = liveClickedTargetRECT.L;
				targetArea.T = liveClickedTargetRECT.T;
				targetArea.R = liveClickedTargetRECT.R + (*mMouseX - liveClickedX);
				targetArea.B = liveClickedTargetRECT.B + (*mMouseY - liveClickedY);
			}

			// Snap to grid
			if (!liveEditingMod->A)
			{
				if (!liveClickedOnHandle)
				{
					int gridL = (drawArea.L / liveScaledGridSize) * liveScaledGridSize;
					int gridT = (drawArea.T / liveScaledGridSize) * liveScaledGridSize;

					int diffL = gridL - drawArea.L;
					int diffT = gridT - drawArea.T;

					drawArea.L += diffL;
					drawArea.T += diffT;
					drawArea.R += diffL;
					drawArea.B += diffT;

					targetArea.L += diffL;
					targetArea.T += diffT;
					targetArea.R += diffL;
					targetArea.B += diffT;
				}
				else
				{
					int gridR = (drawArea.R / liveScaledGridSize) * liveScaledGridSize;
					int gridB = (drawArea.B / liveScaledGridSize) * liveScaledGridSize;

					int diffR = gridR - drawArea.R;
					int diffB = gridB - drawArea.B;

					drawArea.R += diffR;
					drawArea.B += diffB;

					targetArea.R += diffR;
					targetArea.B += diffB;
				}
			}

			// Snap to control Function
			SnapToControl(pDrawBitmap, pControls, liveEditingMod, liveSnap, liveMouseCapture, &drawArea, &targetArea, liveMode, controlSize);

			// Prevent moving background
			if (liveControlNumber > 0)
			{
				// Add undo
				if ((pControl->GetRECT() != &drawArea || pControl->GetTargetRECT() != &targetArea) && undoMove)
				{
					AddUndo(pPlug, pGraphics);
					undoMove = false;
				}

				if (*liveMode == 0 || *liveMode == 1) pControl->SetDrawRECT(drawArea);
				if (*liveMode == 0 || *liveMode == 2) pControl->SetTargetRECT(targetArea);

				//pControl->SetDrawRECT(RestrictToWindow(drawArea, pGraphics->Width(), pGraphics->Height()));
				//pControl->SetTargetRECT(RestrictToWindow(targetArea, pGraphics->Width(), pGraphics->Height()));

				// Add current undo
				if (!undoMove)
				{
					AddCurrentUndo(pPlug, pGraphics);
				}

				DeleteLastSameUndo(pPlug, pGraphics);
			}

			if (*liveMode == 0 || *liveMode == 1) liveSelectedRECT = drawArea;
			if (*liveMode == 0 || *liveMode == 2) liveSelectedRECT = targetArea;
		}
	}

	void MoveSelectionOfControls(IPlugBase* pPlug, IGraphics* pGraphics, LICE_IBitmap* pDrawBitmap, WDL_PtrList<IControl>* pControls,
		IMouseMod* liveEditingMod, int* liveSnap, int* liveMouseCapture, int controlSize, bool overControlHandle, int liveHandleSize,
		bool* liveMouseDragging, int* liveMode, int* mMouseX, int* mMouseY)
	{
		liveControlNumber = -1;
		liveClickedRECT = IRECT(0, 0, 0, 0);
		liveClickedTargetRECT = IRECT(0, 0, 0, 0);

		IRECT dragSelectionRECT = IRECT(0, 0, 0, 0);
		
		// Find where mouse was clicked
		if (liveEditingMod->L != prevSelectingControls)
		{
			liveSelectionClickedX = *mMouseX;
			liveSelectionClickedY = *mMouseY;

			prevSelectingControls = liveEditingMod->L;
		}

		// Select only if ctrl is pressed
		if (liveEditingMod->C)
		{
			// Get selection rect
			if (liveEditingMod->L)
			{
				if (*mMouseX < liveSelectionClickedX)
				{
					dragSelectionRECT.R = liveSelectionClickedX;
					dragSelectionRECT.L = *mMouseX;
				}
				else
				{
					dragSelectionRECT.L = liveSelectionClickedX;
					dragSelectionRECT.R = *mMouseX;
				}

				if (*mMouseY < liveSelectionClickedY)
				{
					dragSelectionRECT.B = liveSelectionClickedY;
					dragSelectionRECT.T = *mMouseY;
				}
				else
				{
					dragSelectionRECT.T = liveSelectionClickedY;
					dragSelectionRECT.B = *mMouseY;
				}
			}
		}

		// Draw selection rect

		// Draw only if mouse is down
		if (liveEditingMod->C && liveEditingMod->L)
		{
			IColor color;
			if (snapMode == 0) color = EDIT_COLOR_BOTH;
			else if (snapMode == 1) color = EDIT_COLOR_DRAW;
			else if (snapMode == 2) color = EDIT_COLOR_TARGET;

			// T
			LICE_DashedLine(pDrawBitmap, dragSelectionRECT.L, dragSelectionRECT.T, dragSelectionRECT.R, dragSelectionRECT.T, 2, 2,
				LICE_RGBA(color.R, color.G, color.B, color.A));
			//B
			LICE_DashedLine(pDrawBitmap, dragSelectionRECT.L, dragSelectionRECT.B, dragSelectionRECT.R, dragSelectionRECT.B, 2, 2,
				LICE_RGBA(color.R, color.G, color.B, color.A));
			//L
			LICE_DashedLine(pDrawBitmap, dragSelectionRECT.L, dragSelectionRECT.T, dragSelectionRECT.L, dragSelectionRECT.B, 2, 2,
				LICE_RGBA(color.R, color.G, color.B, color.A));
			//R
			LICE_DashedLine(pDrawBitmap, dragSelectionRECT.R, dragSelectionRECT.T, dragSelectionRECT.R, dragSelectionRECT.B, 2, 2,
				LICE_RGBA(color.R, color.G, color.B, color.A));
		}

		// Find selectedControlsRECT only while selecting
		if (liveEditingMod->C)
		{
			// Find selected controls and selectedControlsRECT
			selected_controls.resize(0);
			selected_draw_rect.resize(0);
			selected_target_rect.reserve(0);
			selectedControlsRECT = IRECT(999999999, 999999999, 0, 0);

			for (int j = 1; j < controlSize; j++)
			{
				IControl* pControl = pControls->Get(j);

				if (!IsControlDeleted(pControl))
				{
					IRECT drawRECT = *pControl->GetRECT();
					IRECT targetRECT = *pControl->GetTargetRECT();

					IRECT tmpRECT(0, 0, 0, 0);
					if (*liveMode == 0 && (*pControl->GetRECT() == *pControl->GetTargetRECT())) tmpRECT = *pControl->GetRECT();
					else if (*liveMode == 1) tmpRECT = *pControl->GetRECT();
					else if (*liveMode == 2) tmpRECT = *pControl->GetTargetRECT();

					if (dragSelectionRECT.Contains(&tmpRECT))
					{
						selected_controls.push_back(j);

						selectedControlsRECT.L = IPMIN(selectedControlsRECT.L, tmpRECT.L);
						selectedControlsRECT.T = IPMIN(selectedControlsRECT.T, tmpRECT.T);
						selectedControlsRECT.R = IPMAX(selectedControlsRECT.R, tmpRECT.R);
						selectedControlsRECT.B = IPMAX(selectedControlsRECT.B, tmpRECT.B);

						selected_draw_rect.push_back(drawRECT);
						selected_target_rect.push_back(targetRECT);
					}
				}
			}

			// Get rect before move
			selectionRECT = selectedControlsRECT;
		}

		// Check if mouse has clicked on selection and get mouse coordinates
		// Get only if mouse has clicked
		if (liveEditingMod->L != prevClickedOnSelection)
		{
			clickedSelectionX = *mMouseX;
			clickedSelectionY = *mMouseY;

			for (int i = 0; i < selected_controls.size(); i++)
			{
				IControl* pControl = pGraphics->GetControl(selected_controls[i]);
				IRECT drawRECT = *pControl->GetRECT();
				IRECT targetRECT = *pControl->GetTargetRECT();

				selected_draw_rect[i] = drawRECT;
				selected_target_rect[i] = targetRECT;
			}

			// Get rect before move
			selectionRECT = selectedControlsRECT;

			prevClickedOnSelection = liveEditingMod->L;

			if (liveEditingMod->L)
				AddUndo(pPlug, pGraphics);
		}


		if (liveEditingMod->L)
		{
			if (!liveEditingMod->C)
			{
				selectedControlsRECT.L = selectionRECT.L + (*mMouseX - clickedSelectionX);
				selectedControlsRECT.T = selectionRECT.T + (*mMouseY - clickedSelectionY);
				selectedControlsRECT.R = selectionRECT.R + (*mMouseX - clickedSelectionX);
				selectedControlsRECT.B = selectionRECT.B + (*mMouseY - clickedSelectionY);
				
				// Snap to grid
				if (!liveEditingMod->A)
				{
						int gridL = (selectedControlsRECT.L / liveScaledGridSize) * liveScaledGridSize;
						int gridT = (selectedControlsRECT.T / liveScaledGridSize) * liveScaledGridSize;

						int diffL = gridL - selectedControlsRECT.L;
						int diffT = gridT - selectedControlsRECT.T;

						selectedControlsRECT.L += diffL;
						selectedControlsRECT.T += diffT;
						selectedControlsRECT.R += diffL;
						selectedControlsRECT.B += diffT;
					
				}

				// Snap to control Function
				//IRECT dummy = IRECT(0,0,0,0);
				SnapToControl(pDrawBitmap, pControls, liveEditingMod, liveSnap, liveMouseCapture, &selectedControlsRECT, &selectedControlsRECT, liveMode, controlSize, true);

				// Prevent selection to go out the window
				//selectedControlsRECT = RestrictToWindow(selectedControlsRECT, pGraphics->Width(), pGraphics->Height());

				for (int i = 0; i < selected_controls.size(); i++)
				{
					// Prevent editing
					if (*liveMouseDragging)
					{
						IControl* pControl = pGraphics->GetControl(selected_controls[i]);
						int diffL = selectedControlsRECT.L - selectionRECT.L;
						int diffT = selectedControlsRECT.T - selectionRECT.T;
						IRECT drawArea = *pControl->GetRECT();
						IRECT targetArea = *pControl->GetTargetRECT();

						if (*liveMode == 0 || *liveMode == 1)
						{
							drawArea.L = selected_draw_rect[i].L + diffL;
							drawArea.T = selected_draw_rect[i].T + diffT;
							drawArea.R = selected_draw_rect[i].R + diffL;
							drawArea.B = selected_draw_rect[i].B + diffT;
						}

						if (*liveMode == 0 || *liveMode == 2)
						{
							targetArea.L = selected_target_rect[i].L + diffL;
							targetArea.T = selected_target_rect[i].T + diffT;
							targetArea.R = selected_target_rect[i].R + diffL;
							targetArea.B = selected_target_rect[i].B + diffT;
						}

						if (*liveMode == 0 || *liveMode == 1) pControl->SetDrawRECT(drawArea);
						if (*liveMode == 0 || *liveMode == 2) pControl->SetTargetRECT(targetArea);
					}
				}

				AddCurrentUndo(pPlug, pGraphics);
				DeleteLastSameUndo(pPlug, pGraphics);
			}
		}

		// Draw selected controls selection
		if (selectedControlsRECT != IRECT(999999999, 999999999, 0, 0))
		{
			// Draw selected Controls
			// T
			LICE_DashedLine(pDrawBitmap, selectedControlsRECT.L, selectedControlsRECT.T, selectedControlsRECT.R, selectedControlsRECT.T, 2, 2,
				LICE_RGBA(SELECTION_COLOR.R, SELECTION_COLOR.G, SELECTION_COLOR.B, SELECTION_COLOR.A));
			//B
			LICE_DashedLine(pDrawBitmap, selectedControlsRECT.L, selectedControlsRECT.B, selectedControlsRECT.R, selectedControlsRECT.B, 2, 2,
				LICE_RGBA(SELECTION_COLOR.R, SELECTION_COLOR.G, SELECTION_COLOR.B, SELECTION_COLOR.A));
			//L
			LICE_DashedLine(pDrawBitmap, selectedControlsRECT.L, selectedControlsRECT.T, selectedControlsRECT.L, selectedControlsRECT.B, 2, 2,
				LICE_RGBA(SELECTION_COLOR.R, SELECTION_COLOR.G, SELECTION_COLOR.B, SELECTION_COLOR.A));
			//R
			LICE_DashedLine(pDrawBitmap, selectedControlsRECT.R, selectedControlsRECT.T, selectedControlsRECT.R, selectedControlsRECT.B, 2, 2,
				LICE_RGBA(SELECTION_COLOR.R, SELECTION_COLOR.G, SELECTION_COLOR.B, SELECTION_COLOR.A));

			// Outline selected rects
			for (int i = 0; i < selected_controls.size(); i++)
			{
				IRECT tmpRECT(0, 0, 0, 0);
				if (*liveMode == 0 && (*pControls->Get(selected_controls[i])->GetRECT() == *pControls->Get(selected_controls[i])->GetTargetRECT()))
					tmpRECT = *pControls->Get(selected_controls[i])->GetRECT();
				else if (*liveMode == 1) tmpRECT = *pControls->Get(selected_controls[i])->GetRECT();
				else if (*liveMode == 2) tmpRECT = *pControls->Get(selected_controls[i])->GetTargetRECT();

				IColor color;
				if (*liveMode == 0) color = EDIT_COLOR_BOTH;
				else if (*liveMode == 1) color = EDIT_COLOR_DRAW;
				else if (*liveMode == 2) color = EDIT_COLOR_TARGET;

				pGraphics->DrawRect(&color, &tmpRECT);
			}

		}
	}

	void SnapToControl(LICE_IBitmap* pDrawBitmap, WDL_PtrList<IControl>* pControls, 
		IMouseMod* liveEditingMod,int* liveSnap, int* liveMouseCapture, IRECT* drawArea, IRECT* targetArea, int* liveMode, int controlSize, bool snapSelection = false)
	{
		// Snap to other control
		if (liveEditingMod->S)
		{
			int snapSize = *liveSnap + 1;

			int snapL = 0;
			int snapMinL = 999999999;
			int snapMaxL = -999999999;
			int prevsnapMinL = 999999999;
			int prevsnapMaxL = -999999999;

			int snapT = 0;
			int snapMinT = 999999999;
			int snapMaxT = -999999999;
			int prevsnapMinT = 999999999;
			int prevsnapMaxT = -999999999;

			bool didSnappedT = false;
			bool didSnappedL = false;

			IRECT lineMinL, lineMinT;
			IRECT lineMaxL, lineMaxT;
			IRECT lineL, lineT;


			for (int index = 0; index < controlSize; index++)
			{
				if (!snapSelection && (index == *liveMouseCapture)) continue;

				IControl* pSnapControl = pControls->Get(index);

				IRECT tmpArea(0, 0, 0, 0);
				if (snapMode == 0 && (*pSnapControl->GetRECT() == *pSnapControl->GetTargetRECT())) tmpArea = *pSnapControl->GetRECT();
				else if (snapMode == 1) tmpArea = *pSnapControl->GetRECT();
				else if (snapMode == 2) tmpArea = *pSnapControl->GetTargetRECT();
				
				if (snapSelection && IsControlSelected(pControls, pSnapControl)) continue;
				
				int tmpSnapL;
				IRECT tmpRECTL;

				int tmpArea_L = tmpArea.L;
				int tmpArea_LM = tmpArea.L + tmpArea.W() / 2;
				int tmpArea_R = tmpArea.R;
				int tmpArea_TM = tmpArea.T + tmpArea.H() / 2;

				IRECT tmpRECT(0, 0, 0, 0);
				if (snapMode == 0 && (*drawArea == *targetArea)) tmpRECT = *drawArea;
				else if (snapMode == 1) tmpRECT = *drawArea;
				else if (snapMode == 2) tmpRECT = *targetArea;

				int Area_L = tmpRECT.L;
				int Area_LM = tmpRECT.L + tmpRECT.W() / 2;
				int Area_R = tmpRECT.R;
				int Area_TM = tmpRECT.T + tmpRECT.H() / 2;

				// Find snap to L
				for (int j = 0; j < 9; j++)
				{
					if (liveClickedOnHandle)
					{
						if ((j / 3) * 3 == j) continue;
					}

					if (j == 0) // L to L
					{
						tmpSnapL = tmpArea_L - Area_L;
						tmpRECTL = IRECT(tmpArea_L, tmpArea_TM, Area_L, Area_TM);
					}
					if (j == 1) // L to LM
					{
						tmpSnapL = tmpArea_L - Area_LM;
						tmpRECTL = IRECT(tmpArea_L, tmpArea_TM, Area_LM, Area_TM);
					}
					if (j == 2) // L to R
					{
						tmpSnapL = tmpArea_L - Area_R;
						tmpRECTL = IRECT(tmpArea_L, tmpArea_TM, Area_R, Area_TM);
					}
					if (j == 3) // LM to L
					{
						tmpSnapL = tmpArea_LM - Area_L;
						tmpRECTL = IRECT(tmpArea_LM, tmpArea_TM, Area_L, Area_TM);
					}
					if (j == 4) // LM to LM
					{
						tmpSnapL = tmpArea_LM - Area_LM;
						tmpRECTL = IRECT(tmpArea_LM, tmpArea_TM, Area_LM, Area_TM);
					}
					if (j == 5) // LM to R
					{
						tmpSnapL = tmpArea_LM - Area_R;
						tmpRECTL = IRECT(tmpArea_LM, tmpArea_TM, Area_R, Area_TM);
					}
					if (j == 6) // R to L
					{
						tmpSnapL = tmpArea_R - Area_L;
						tmpRECTL = IRECT(tmpArea_R, tmpArea_TM, Area_L, Area_TM);
					}
					if (j == 7) // R to LM
					{
						tmpSnapL = tmpArea_R - Area_LM;
						tmpRECTL = IRECT(tmpArea_R, tmpArea_TM, Area_LM, Area_TM);
					}
					if (j == 8) // R to R
					{
						tmpSnapL = tmpArea_R - Area_R;
						tmpRECTL = IRECT(tmpArea_R, tmpArea_TM, Area_R, Area_TM);
					}

					// ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
					if (tmpSnapL < snapSize && tmpSnapL >= 0)
					{
						snapMinL = IPMIN(snapMinL, tmpSnapL);

						if (snapMinL != prevsnapMinL)
						{
							lineMinL = tmpRECTL;
						}
						prevsnapMinL = snapMinL;

						didSnappedL = true;
					}
					if (tmpSnapL > -snapSize && tmpSnapL <= 0)
					{
						snapMaxL = IPMAX(snapMaxL, tmpSnapL);

						if (snapMaxL != prevsnapMaxL)
						{
							lineMaxL = tmpRECTL;
						}
						prevsnapMaxL = snapMaxL;

						didSnappedL = true;
					}
				}
			}

			if (didSnappedL)
			{
				if (snapMinL <= abs(snapMaxL))
				{
					lineL = lineMinL;
					snapL = snapMinL;
				}
				else
				{
					lineL = lineMaxL;
					snapL = snapMaxL;
				}

				// Snap control
				if (snapL != 0)
				{
					if (*liveMode == 0 || *liveMode == 1)
					{
						if (!liveClickedOnHandle)
						{
							drawArea->L = drawArea->L + snapL;
							drawArea->R = drawArea->R + snapL;
						}
						else drawArea->R = drawArea->R + snapL;
					}

					if (*liveMode == 0 || *liveMode == 2)
					{
						if (!liveClickedOnHandle)
						{
							targetArea->L = targetArea->L + snapL;
							targetArea->R = targetArea->R + snapL;
						}
						else targetArea->R = targetArea->R + snapL;
					}
				}

				IColor color;
				if (snapMode == 0) color = EDIT_COLOR_BOTH;
				else if (snapMode == 1) color = EDIT_COLOR_DRAW;
				else if (snapMode == 2) color = EDIT_COLOR_TARGET;

				// Draw snap line
				LICE_DashedLine(pDrawBitmap, lineL.L, lineL.T, lineL.R + snapL, lineL.B, 2, 2,
					LICE_RGBA(color.R, color.G, color.B, color.A));
			}

			for (int index = 0; index < controlSize; index++)
			{
				if (!snapSelection && (index == *liveMouseCapture)) continue;

				IControl* pSnapControl = pControls->Get(index);

				IRECT tmpArea(0, 0, 0, 0);
				if (snapMode == 0 && (*pSnapControl->GetRECT() == *pSnapControl->GetTargetRECT())) tmpArea = *pSnapControl->GetRECT();
				else if (snapMode == 1) tmpArea = *pSnapControl->GetRECT();
				else if (snapMode == 2) tmpArea = *pSnapControl->GetTargetRECT();

				int tmpSnapT;
				IRECT tmpRECTT;

				if (snapSelection && IsControlSelected(pControls, pSnapControl)) continue;

				int tmpArea_LM = tmpArea.L + tmpArea.W() / 2;
				int tmpArea_T = tmpArea.T;
				int tmpArea_TM = tmpArea.T + tmpArea.H() / 2;
				int tmpArea_B = tmpArea.B;

				IRECT tmpRECT(0, 0, 0, 0);
				if (snapMode == 0 && (*drawArea == *targetArea)) tmpRECT = *drawArea;
				else if (snapMode == 1) tmpRECT = *drawArea;
				else if (snapMode == 2) tmpRECT = *targetArea;

				int Area_LM = tmpRECT.L + tmpRECT.W() / 2;
				int Area_T = tmpRECT.T;
				int Area_TM = tmpRECT.T + tmpRECT.H() / 2;
				int Area_B = tmpRECT.B;

				// Find snap to T
				for (int j = 0; j < 9; j++)
				{
					if (liveClickedOnHandle)
					{
						if ((j / 3) * 3 == j) continue;
					}

					if (j == 0) // T to T
					{
						tmpSnapT = tmpArea_T - Area_T;
						tmpRECTT = IRECT(tmpArea_LM, tmpArea_T, Area_LM, Area_T);
					}
					if (j == 1) // T to TM
					{
						tmpSnapT = tmpArea_T - Area_TM;
						tmpRECTT = IRECT(tmpArea_LM, tmpArea_T, Area_LM, Area_TM);
					}
					if (j == 2) // T to B
					{
						tmpSnapT = tmpArea_T - Area_B;
						tmpRECTT = IRECT(tmpArea_LM, tmpArea_T, Area_LM, Area_B);
					}
					if (j == 3) // TM to T
					{
						tmpSnapT = tmpArea_TM - Area_T;
						tmpRECTT = IRECT(tmpArea_LM, tmpArea_TM, Area_LM, Area_T);
					}
					if (j == 4) // TM to TM
					{
						tmpSnapT = tmpArea_TM - Area_TM;
						tmpRECTT = IRECT(tmpArea_LM, tmpArea_TM, Area_LM, Area_TM);
					}
					if (j == 5) // TM to B
					{
						tmpSnapT = tmpArea_TM - Area_B;
						tmpRECTT = IRECT(tmpArea_LM, tmpArea_TM, Area_LM, Area_B);
					}
					if (j == 6) // B to T
					{
						tmpSnapT = tmpArea_B - Area_T;
						tmpRECTT = IRECT(tmpArea_LM, tmpArea_B, Area_LM, Area_T);
					}
					if (j == 7) // B to TM
					{
						tmpSnapT = tmpArea_B - Area_TM;
						tmpRECTT = IRECT(tmpArea_LM, tmpArea_B, Area_LM, Area_TM);
					}
					if (j == 8) // B to B
					{
						tmpSnapT = tmpArea_B - Area_B;
						tmpRECTT = IRECT(tmpArea_LM, tmpArea_B, Area_LM, Area_B);
					}

					// ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
					if (tmpSnapT < snapSize && tmpSnapT >= 0)
					{
						snapMinT = IPMIN(snapMinT, tmpSnapT);

						if (snapMinT != prevsnapMinT)
						{
							lineMinT = tmpRECTT;
						}
						prevsnapMinT = snapMinT;

						didSnappedT = true;
					}
					if (tmpSnapT > -snapSize && tmpSnapT <= 0)
					{
						snapMaxT = IPMAX(snapMaxT, tmpSnapT);

						if (snapMaxT != prevsnapMaxT)
						{
							lineMaxT = tmpRECTT;
						}
						prevsnapMaxT = snapMaxL;

						didSnappedT = true;
					}
				}
			}

			if (didSnappedT)
			{
				if (snapMinT <= abs(snapMaxT))
				{
					lineT = lineMinT;
					snapT = snapMinT;
				}
				else
				{
					lineT = lineMaxT;
					snapT = snapMaxT;
				}

				// Snap control
				if (snapT != 0)
				{
					if (*liveMode == 0 || *liveMode == 1)
					{
						if (!liveClickedOnHandle)
						{
							drawArea->T = drawArea->T + snapT;
							drawArea->B = drawArea->B + snapT;
						}
						else drawArea->B = drawArea->B + snapT;
					}

					if (*liveMode == 0 || *liveMode == 2)
					{
						if (!liveClickedOnHandle)
						{
							targetArea->T = targetArea->T + snapT;
							targetArea->B = targetArea->B + snapT;
						}
						else targetArea->B = targetArea->B + snapT;
					}
				}

				IColor color;
				if (snapMode == 0) color = EDIT_COLOR_BOTH;
				else if (snapMode == 1) color = EDIT_COLOR_DRAW;
				else if (snapMode == 2) color = EDIT_COLOR_TARGET;

				// Draw snap line
				LICE_DashedLine(pDrawBitmap, lineT.L, lineT.T, lineT.R, lineT.B + snapT, 2, 2,
					LICE_RGBA(color.R, color.G, color.B, color.A));
			}
		}
	}

	void WriteToTextFile(const char* data, const char* filePath)
	{
		ofstream myfile;
		myfile.open(filePath);
		if (myfile.is_open())
		{
			myfile << data;
			myfile.close();
		}
	}

	string ReadTextFile(const char* filePath)
	{
		string output;
		string line;
		ifstream myfile(filePath);
		if (myfile.is_open())
		{
			while (getline(myfile, line))
			{
				output.append(line);
				output.append("\n");
			}
			myfile.close();
		}
		return output;
	}

	void WriteLayoutToTextFile(const char* data)
	{
		ofstream myfile;
		myfile.open("LiveEditLayout.h");
		if (myfile.is_open())
		{
			myfile << data;
			myfile.close();
		}
	}

	void RetrieveOldLayoutChanges()
	{
		string oldCode;
		string line;
		ifstream myfile("LiveEditLayout.h");
		if (myfile.is_open())
		{
			while (getline(myfile, line))
			{
				oldCode.append(line);
				oldCode.append("\n");
			}
			myfile.close();
		}

		for (int i = 0; i < viewModeSize; i++)
		{
			WDL_String findStart, findEnd;
			findStart.SetFormatted(128, "		// View Mode: (%i)", i);
			findEnd.SetFormatted(128, "		// End (%i)", i);

			unsigned start_index = oldCode.find(findStart.Get());
			unsigned next_start_index = oldCode.find("        // class", start_index);
			unsigned end_index = oldCode.find(findEnd.Get());
			
			if (oldCode.npos > next_start_index && oldCode.npos > end_index)
			{
				code_view_mode[i] = oldCode.substr(next_start_index, end_index - next_start_index);
			}
		}
	}

	void CreateLayoutCode(IPlugBase* pPlug, IGraphics* pGraphics, double guiScaleRatio, int viewMode, bool reset)
	{
		string code;

		code = 
			"// Do not edit. All of this is generated automatically \n"
			"// Copyright Youlean 2016 \n\n"
            "#ifndef _LIVEEDITLAYOUT_\n"
            "#define _LIVEEDITLAYOUT_\n\n"
            "#include <vector>\n"
			"#include \"IGraphics.h\" \n"
			"#include \"IPlugGUIResize.h\" \n\n"
			"class LiveEditLayout \n"
			"{ \n"
			"public: \n"
			"	LiveEditLayout() {} \n \n"
			"	~LiveEditLayout() {} \n \n"
			"	void SetDefaultViewLayout(IGraphics* pGraphics) \n"
			"	{ \n"
			;

		
		// Write all controls positions
		int controlSize = default_layers.size();
		if (pPlug->GetGUIResize()) controlSize -= 3;

		if (!pPlug->GetGUIResize())
		{
			code.append
			(
				"	    // Backup original control pointers\n"
				"		for (int i = 0; i < pGraphics->GetNControls(); i++) \n"
				"			originalPointers.push_back(pGraphics->GetControl(i));\n"
				"\n"
				"	    // --------------------------------------------------------------------\n\n"
			);

			for (int i = 1; i < controlSize; i++)
			{
				IControl* pControl = default_layers[i];
				IRECT drawRECT = *pControl->GetRECT();
				IRECT targetRECT = *pControl->GetTargetRECT();

				WDL_String drawValue, targetValue, hiddenValue;

				// Get derived class name
				WDL_String derivedName;
				derivedName.SetFormatted(128, "        // %s %i\n", pControl->GetDerivedClassName(), i);
				code.append(derivedName.Get());

				hiddenValue.SetFormatted(128, "		pGraphics->GetControl(%i)->Hide(", i);
				if (pControl->IsHidden()) hiddenValue.Append("true");
				else hiddenValue.Append("false");
				hiddenValue.Append("); \n");
				code.append(hiddenValue.Get());

				drawValue.SetFormatted(128, "		pGraphics->GetControl(%i)->SetDrawRECT(IRECT(%i, %i, %i, %i)); \n", i, drawRECT.L, drawRECT.T, drawRECT.R, drawRECT.B);
				code.append(drawValue.Get());

				targetValue.SetFormatted(128, "		pGraphics->GetControl(%i)->SetTargetRECT(IRECT(%i, %i, %i, %i)); \n", i, targetRECT.L, targetRECT.T, targetRECT.R, targetRECT.B);
				code.append(targetValue.Get());

				code.append("\n");
			}


			// Reordering control layers
			code.append
			(
				"	    // --------------------------------------------------------------------\n\n"
				"	    // Reordering control layers\n"
			);
		}
			WDL_String layoutMove;

			// Backup current control layers
			for (int i = 0; i < pGraphics->GetNControls(); i++)
				current_layers[i] = pGraphics->GetControl(i);

			if (!pPlug->GetGUIResize())
			{
				for (int i = 0; i < controlSize; i++)
				{
					IControl* pControl = default_layers[i];
					layoutMove.SetFormatted(128, "		pGraphics->ReplaceControl(%i, originalPointers[%i]); \n", FindPointerPosition(pControl, current_layers), i);
					code.append(layoutMove.Get());
				}
			}

		code.append("	}\n\n");

		// If GUI resize is enabled
		code.append
		(
			"	void SetGUIResizeLayout(IGraphics* pGraphics, IPlugGUIResize* pGUIResize)\n"
			"	{\n"
		);


		// Set GUI Resize code -----------------------------------------------------------------------------------------------------------
		if (pPlug->GetGUIResize())
		{
			code_view_mode[viewMode].clear();
			WDL_String viewCode;
			int deletedFix = 0;
			for (int i = 0; i < controlSize; i++)
			{
				bool skipWriting = false;

				IControl* pControl = default_layers[i];
				int drawPointerPosition = FindPointerPosition(pControl, current_layers);

				// Check if some control is needed to be deleted
				if (deleted_control_default_index.size() > 0)
				{
					bool find = false;
					for (int j = 0; j < deleted_control_default_index.size(); j++)
					{
						if (deleted_control_default_index[j] == i)
						{
							find = true;
							break;
						}
					}

					int subtractPos = 0;
					for (int j = 0; j < deleted_control_default_index.size(); j++)
					{
						if (deleted_control_default_index[j] <= drawPointerPosition)
						{
							subtractPos++;
						}
					}

					drawPointerPosition -= subtractPos;

					// If index is the same as deleted control, skip this 
					if (find)
					{
						deletedFix++;
						skipWriting = true;
					}
					else
						skipWriting = false;
				}

				IRECT drawRECT = *pControl->GetRECT();
				IRECT targetRECT = *pControl->GetTargetRECT();

				if (pPlug->GetGUIResize() && !reset)
				{
					drawRECT.L = int(((double)drawRECT.L + 0.4999) * (1.0 / guiScaleRatio));
					drawRECT.T = int(((double)drawRECT.T + 0.4999) * (1.0 / guiScaleRatio));
					drawRECT.R = int(((double)drawRECT.R + 0.4999) * (1.0 / guiScaleRatio));
					drawRECT.B = int(((double)drawRECT.B + 0.4999) * (1.0 / guiScaleRatio));

					targetRECT.L = int(((double)targetRECT.L + 0.4999) * (1.0 / guiScaleRatio));
					targetRECT.T = int(((double)targetRECT.T + 0.4999) * (1.0 / guiScaleRatio));
					targetRECT.R = int(((double)targetRECT.R + 0.4999) * (1.0 / guiScaleRatio));
					targetRECT.B = int(((double)targetRECT.B + 0.4999) * (1.0 / guiScaleRatio));
				}
				else
				{
					drawRECT.L = drawRECT.L;
				}

				// Prevent writing if this is deleted control
				if (!skipWriting)
				{
					WDL_String derivedName;
					derivedName.SetFormatted(128, "        // %s %i\n", pControl->GetDerivedClassName(), i - deletedFix);

					viewCode.SetFormatted(128, "		pGUIResize->LiveEditSetLayout(%i, %i, %i, IRECT(%i, %i, %i, %i), IRECT(%i, %i, %i, %i)",
						viewMode, i - deletedFix, drawPointerPosition, drawRECT.L, drawRECT.T, drawRECT.R, drawRECT.B, targetRECT.L, targetRECT.T, targetRECT.R, targetRECT.B);
					if (pControl->IsHidden()) viewCode.Append(", true);\n");
					else  viewCode.Append(", false);\n");

					code_view_mode[viewMode].append(derivedName.Get());
					code_view_mode[viewMode].append(viewCode.Get());
				}

				// Update current view mode
				IControl* tmpControl = pGraphics->GetControl(FindPointerPosition(pControl, current_layers));
				if (pPlug->GetGUIResize() && reset)
				{
					drawRECT = *pControl->GetRECT();
					targetRECT = *pControl->GetTargetRECT();
				}
				pPlug->GetGUIResize()->LiveEditSetLayout(
					viewMode, 
					i,
					FindPointerPosition(pControl, current_layers),
					IRECT(drawRECT.L, drawRECT.T, drawRECT.R, drawRECT.B), 
					IRECT(targetRECT.L, targetRECT.T, targetRECT.R, targetRECT.B), 
					pControl->IsHidden());
			}

			// Append all view code code
			for (int i = 0; i < code_view_mode.size(); i++)
			{
				code.append("\n");
				WDL_String mode;
				mode.SetFormatted(128, "		// View Mode: (%i) ------------------------------------------------------------------------------------------------\n", i);
				code.append(mode.Get());
				code.append(code_view_mode[i]);
				WDL_String endMode;
				endMode.SetFormatted(128, "		// End (%i) -------------------------------------------------------------------------------------------------------\n", i);
				code.append(endMode.Get());
				code.append("\n");
			}
		}


		code.append("    }\n\n");
		// End
		code.append
		(
			"private:\n"
			"	std::vector <IControl*> originalPointers;\n"

		);

		code.append("};\n #endif");
		WriteLayoutToTextFile(code.c_str());
	}

	void DrawGrid(LICE_IBitmap* pDrawBitmap, int width, int height)
	{
		if (liveScaledGridSize > 1)
		{
			// Vertical Lines grid
			for (int i = 0; i < width; i += liveScaledGridSize)
			{
				LICE_Line(pDrawBitmap, i, 0, i, height,
					LICE_RGBA(EDIT_COLOR_BOTH.R, EDIT_COLOR_BOTH.G, EDIT_COLOR_BOTH.B, EDIT_COLOR_BOTH.A), 0.17f);
			}

			// Horisontal Lines grid
			for (int i = 0; i < height; i += liveScaledGridSize)
			{
				LICE_Line(pDrawBitmap, 0, i, width, i,
					LICE_RGBA(EDIT_COLOR_BOTH.R, EDIT_COLOR_BOTH.G, EDIT_COLOR_BOTH.B, EDIT_COLOR_BOTH.A), 0.17f);
			}
		}
		else
		{
			LICE_FillRect(pDrawBitmap, 0, 0, width, height,
				LICE_RGBA(EDIT_COLOR_BOTH.R, EDIT_COLOR_BOTH.G, EDIT_COLOR_BOTH.B, EDIT_COLOR_BOTH.A), 0.11f);
		}
	}

	void DoPopupMenu(IPlugBase* pPlug, IGraphics* pGraphics, int x, int y, int* liveMode, double guiScaleRatio)
	{
		IPopupMenu menu;

		// Item 0
		menu.AddItem("Undo");

		// Item 1
		menu.AddItem("Redo");

		// Item 2
		menu.AddSeparator();

		// Item 3
		if (liveControlNumber <= 0 || multipleControlsSelected)
		{
			menu.AddItem("Reset Control Position");
			menu.SetItemState(3, IPopupMenuItem::kDisabled, true);
		}
		else
		{
			menu.AddItem("Reset Control Position");
		}

		// Item 4
		if (liveControlNumber <= 0 || multipleControlsSelected)
		{
			menu.AddItem("Reset Control Size");
			menu.SetItemState(4, IPopupMenuItem::kDisabled, true);
		}
		else
		{
			menu.AddItem("Reset Control Size");
		}

		// Item 5
		if (liveControlNumber <= 0 || multipleControlsSelected || 
			*pGraphics->GetControl(liveControlNumber)->GetRECT() == *pGraphics->GetControl(liveControlNumber)->GetTargetRECT())
		{
			menu.AddItem("Merge Draw and Target IRECT");
			menu.SetItemState(5, IPopupMenuItem::kDisabled, true);
		}
		else
		{
			menu.AddItem("Merge Draw and Target Rect");
		}

		// Item 6
		menu.AddSeparator();

		// Item 7
		if (liveControlNumber <= 0 || multipleControlsSelected)
		{
			menu.AddItem("Show Control");
			menu.SetItemState(7, IPopupMenuItem::kDisabled, true);
		}
		else
		{
			if (pGraphics->GetControl(liveControlNumber)->IsHidden()) menu.AddItem("Show Control");
			else menu.AddItem("Hide Control");
		}

		// Item 8
		menu.AddSeparator();

		// Item 9
		if (liveControlNumber <= 0 || multipleControlsSelected)
		{
			menu.AddItem("Bring to Front");
			menu.SetItemState(9, IPopupMenuItem::kDisabled, true);
		}
		else
		{
			menu.AddItem("Bring to Front");
		}

		// Item 10
		if (liveControlNumber <= 0 || multipleControlsSelected)
		{
			menu.AddItem("Send to Back");
			menu.SetItemState(10, IPopupMenuItem::kDisabled, true);
		}
		else
		{
			menu.AddItem("Send to Back");
		}

		// Item 11
		menu.AddSeparator();

		// Item 12
		if (liveControlNumber <= 0 || multipleControlsSelected)
		{
			menu.AddItem("Bring Forward");
			menu.SetItemState(12, IPopupMenuItem::kDisabled, true);
		}
		else
		{
			menu.AddItem("Bring Forward");
		}

		// Item 13
		if (liveControlNumber <= 0 || multipleControlsSelected)
		{
			menu.AddItem("Send Backward");
			menu.SetItemState(13, IPopupMenuItem::kDisabled, true);
		}
		else
		{
			menu.AddItem("Send Backward");
		}

		// Item 14
		menu.AddSeparator();

		// Item 15
		if (multipleControlsSelected)
		{
			menu.AddItem("Deselect");
		}
		else
		{
			menu.AddItem("Deselect");
			menu.SetItemState(15, IPopupMenuItem::kDisabled, true);
		}

		// Item 16
		if (drawGridToogle) menu.AddItem("Hide Grid");
		else menu.AddItem("Show Grid");

		// Item 17
		if (drawMouseCoordinats) menu.AddItem("Hide Mouse Coordinates");
		else menu.AddItem("Show Mouse Coordinates");

		// Item 18
		menu.AddSeparator();

		// Item 19
		menu.AddItem("Reset View to Default");

		// Item 20
		menu.AddSeparator();

		// Item 21
		if (liveControlNumber <= 0 || multipleControlsSelected)
		{
			menu.AddItem("Delete Control !!!");
			menu.SetItemState(21, IPopupMenuItem::kDisabled, true);
		}
		else
		{
			menu.AddItem("Delete Control !!!");
		}

		if (pGraphics->CreateIPopupMenu(&menu, x, y))
		{
			int itemChosen = menu.GetChosenItemIdx();

			// Undo
			if (itemChosen == 0)
			{
				GetUndo(pPlug, pGraphics);
				selectedControlsRECT = IRECT(999999999, 999999999, 0, 0);
			}

			// Redo
			if (itemChosen == 1)
			{
				GetRedo(pPlug, pGraphics);
				selectedControlsRECT = IRECT(999999999, 999999999, 0, 0);
			}

			// Reset Control Position
			if (itemChosen == 3)
			{
				AddUndo(pPlug, pGraphics);

				IRECT drawRECT;
				IRECT targetRECT;

				if (*liveMode == 0 || *liveMode == 1)
				{
					drawRECT.L = int((double)default_draw_rect[liveControlNumber].L * guiScaleRatio);
					drawRECT.T = int((double)default_draw_rect[liveControlNumber].T * guiScaleRatio);
					drawRECT.R = drawRECT.L + liveSelectedRECT.W();
					drawRECT.B = drawRECT.T + liveSelectedRECT.H();
				}

				if (*liveMode == 0 || *liveMode == 2)
				{
					targetRECT.L = int((double)default_terget_rect[liveControlNumber].L * guiScaleRatio);
					targetRECT.T = int((double)default_terget_rect[liveControlNumber].T * guiScaleRatio);
					targetRECT.R = targetRECT.L + liveSelectedTargetRECT.W();
					targetRECT.B = targetRECT.T + liveSelectedTargetRECT.H();
				}

				if (*liveMode == 0 || *liveMode == 1) liveSelectedRECT = drawRECT;
				if (*liveMode == 0 || *liveMode == 2) liveSelectedTargetRECT = targetRECT;

				if (*liveMode == 0 || *liveMode == 1) pGraphics->GetControl(liveControlNumber)->SetDrawRECT(drawRECT);
				if (*liveMode == 0 || *liveMode == 2) pGraphics->GetControl(liveControlNumber)->SetTargetRECT(targetRECT);

				// Write to file
				CreateLayoutCode(pPlug, pGraphics, guiScaleRatio, currentViewMode, false);

				AddCurrentUndo(pPlug, pGraphics);
			}

			// Reset Control Size
			if (itemChosen == 4)
			{
				AddUndo(pPlug, pGraphics);

				IRECT drawRECT;
				IRECT targetRECT;

				if (*liveMode == 0 || *liveMode == 1)
				{
					drawRECT.L = liveSelectedRECT.L;
					drawRECT.T = liveSelectedRECT.T;
					drawRECT.R = drawRECT.L + int((double)default_draw_rect[liveControlNumber].W() * guiScaleRatio);
					drawRECT.B = drawRECT.T + int((double)default_draw_rect[liveControlNumber].H() * guiScaleRatio);
				}

				if (*liveMode == 0 || *liveMode == 2)
				{
					targetRECT.L = liveSelectedTargetRECT.L;
					targetRECT.T = liveSelectedTargetRECT.T;
					targetRECT.R = targetRECT.L + int((double)default_terget_rect[liveControlNumber].W() * guiScaleRatio);
					targetRECT.B = targetRECT.T + int((double)default_terget_rect[liveControlNumber].H() * guiScaleRatio);
				}

				if (*liveMode == 0 || *liveMode == 1) liveSelectedRECT = drawRECT;
				if (*liveMode == 0 || *liveMode == 2) liveSelectedTargetRECT = targetRECT;

				if (*liveMode == 0 || *liveMode == 1) pGraphics->GetControl(liveControlNumber)->SetDrawRECT(drawRECT);
				if (*liveMode == 0 || *liveMode == 2) pGraphics->GetControl(liveControlNumber)->SetTargetRECT(targetRECT);

				// Write to file
				CreateLayoutCode(pPlug, pGraphics, guiScaleRatio, currentViewMode, false);

				AddCurrentUndo(pPlug, pGraphics);
			}

			// Merge Draw and Target IRECT
			if (itemChosen == 5)
			{
				AddUndo(pPlug, pGraphics);

				if (*liveMode == 1)
				{
					pGraphics->GetControl(liveControlNumber)->SetDrawRECT(liveSelectedTargetRECT);
					liveSelectedRECT = liveSelectedTargetRECT;
				}

				if (*liveMode == 2)
				{
					pGraphics->GetControl(liveControlNumber)->SetTargetRECT(liveSelectedRECT);
					liveSelectedTargetRECT = liveSelectedRECT;
				}

				// Write to file
				CreateLayoutCode(pPlug, pGraphics, guiScaleRatio, currentViewMode, false);

				AddCurrentUndo(pPlug, pGraphics);
			}

			// Show/Hide Control
			if (itemChosen == 7)
			{
				AddUndo(pPlug, pGraphics);

				if (pGraphics->GetControl(liveControlNumber)->IsHidden()) pGraphics->GetControl(liveControlNumber)->Hide(false);
				else pGraphics->GetControl(liveControlNumber)->Hide(true);

				// Write to file
				CreateLayoutCode(pPlug, pGraphics, guiScaleRatio, currentViewMode, false);

				AddCurrentUndo(pPlug, pGraphics);
			}

			// Bring to Front
			if (itemChosen == 9)
			{
				AddUndo(pPlug, pGraphics);

				int controlSize = pGraphics->GetNControls();
				if (pPlug->GetGUIResize()) controlSize -= 3;

				control_move_from.push_back(liveControlNumber);
				control_move_to.push_back(controlSize - 1);
				liveControlNumber = control_move_to.back();

				pGraphics->MoveControlLayers(control_move_from.back(), control_move_to.back());

				// Write to file
				CreateLayoutCode(pPlug, pGraphics, guiScaleRatio, currentViewMode, false);

				AddCurrentUndo(pPlug, pGraphics);
			}

			// Send to Back
			if (itemChosen == 10)
			{
				AddUndo(pPlug, pGraphics);

				int controlSize = pGraphics->GetNControls();
				if (pPlug->GetGUIResize()) controlSize -= 3;

				control_move_from.push_back(liveControlNumber);
				control_move_to.push_back(1);
				liveControlNumber = control_move_to.back();

				pGraphics->MoveControlLayers(control_move_from.back(), control_move_to.back());

				// Write to file
				CreateLayoutCode(pPlug, pGraphics, guiScaleRatio, currentViewMode, false);

				AddCurrentUndo(pPlug, pGraphics);
			}

			// Bring Forward
			if (itemChosen == 12)
			{
				AddUndo(pPlug, pGraphics);

				int controlSize = pGraphics->GetNControls();
				if (pPlug->GetGUIResize()) controlSize -= 3;

				if (liveControlNumber + 1 < controlSize)
				{
					control_move_from.push_back(liveControlNumber);
					control_move_to.push_back(liveControlNumber + 1);
					liveControlNumber = control_move_to.back();

					pGraphics->SwapControlLayers(control_move_from.back(), control_move_to.back());
				}

				// Write to file
				CreateLayoutCode(pPlug, pGraphics, guiScaleRatio, currentViewMode, false);

				AddCurrentUndo(pPlug, pGraphics);
			}

			// Send Backward
			if (itemChosen == 13)
			{
				AddUndo(pPlug, pGraphics);

				int controlSize = pGraphics->GetNControls();
				if (pPlug->GetGUIResize()) controlSize -= 3;

				if (liveControlNumber - 1 > 0)
				{
					control_move_from.push_back(liveControlNumber);
					control_move_to.push_back(liveControlNumber - 1);
					liveControlNumber = control_move_to.back();

					pGraphics->SwapControlLayers(control_move_from.back(), control_move_to.back());
				}

				// Write to file
				CreateLayoutCode(pPlug, pGraphics, guiScaleRatio, currentViewMode, false);

				AddCurrentUndo(pPlug, pGraphics);
			}

			// Deselect
			if (itemChosen == 15)
			{
				selectedControlsRECT = IRECT(999999999, 999999999, 0, 0);
			}

			// Show/Hide Grid
			if (itemChosen == 16)
			{
				if (drawGridToogle) drawGridToogle = false;
				else drawGridToogle = true;
			}

			// Show/Hide Mouse Coordinates
			if (itemChosen == 17)
			{
				if (drawMouseCoordinats) drawMouseCoordinats = false;
				else drawMouseCoordinats = true;
			}

			// Reset View to Default
			if (itemChosen == 19)
			{
				AddUndo(pPlug, pGraphics);

				for (int i = 0; i < pGraphics->GetNControls(); i++)
				{
					pGraphics->ReplaceControl(i, default_layers[i]);
					IControl* pControl = pGraphics->GetControl(i);

					IRECT drawRECT = default_draw_rect[i];
					IRECT targetRECT = default_terget_rect[i];

					if (pPlug->GetGUIResize())
					{
						drawRECT.L = int(((double)drawRECT.L + 0.4999) * guiScaleRatio);
						drawRECT.T = int(((double)drawRECT.T + 0.4999) * guiScaleRatio);
						drawRECT.R = int(((double)drawRECT.R + 0.4999) * guiScaleRatio);
						drawRECT.B = int(((double)drawRECT.B + 0.4999) * guiScaleRatio);

						targetRECT.L = int(((double)targetRECT.L + 0.4999) * guiScaleRatio);
						targetRECT.T = int(((double)targetRECT.T + 0.4999) * guiScaleRatio);
						targetRECT.R = int(((double)targetRECT.R + 0.4999) * guiScaleRatio);
						targetRECT.B = int(((double)targetRECT.B + 0.4999) * guiScaleRatio);
					}

					pControl->SetDrawRECT(drawRECT);
					pControl->SetTargetRECT(targetRECT);

					// Prevent drawing deleted control
					if (!IsControlDeleted(pControl))
						pControl->Hide(!default_is_hidden[i]);
				}

				liveControlNumber = -1;
				liveClickedRECT = IRECT(0, 0, 0, 0);
				liveClickedTargetRECT = IRECT(0, 0, 0, 0);

				// Write to file
				CreateLayoutCode(pPlug, pGraphics, guiScaleRatio, currentViewMode, true);

				AddCurrentUndo(pPlug, pGraphics);
			}

			// Remove Control
			if (itemChosen == 21)
			{
				AddUndo(pPlug, pGraphics);

				IControl* pControl = pGraphics->GetControl(liveControlNumber);
				int position = FindPointerPosition(pControl, default_layers);

				WDL_String warningText, number;

				warningText.Append("If you confirm this dialog, you need to delete control number ");
				number.SetFormatted(32, "%i!", position);
				warningText.Append(number.Get());
				warningText.Append("\n\n");
				warningText.Append("Delete control from plugin constructor immediately, otherwise your GUI layout will be messed up after next recompile...\n\n");
				warningText.Append("If you want to add new control, just place it after last attached control in plugin constructor.");

				int dialog = pGraphics->ShowMessageBox(warningText.Get(), "Warning!!!", MB_OKCANCEL);

				if (dialog == IDOK)
				{
					// We are not actually removing controls, but we are hidding it and removing from the code
					deleted_control_default_index.push_back(position);

					if (pPlug->GetGUIResize())
					{
						int currentViewMode = pPlug->GetGUIResize()->GetViewMode();
						int viewSize = pPlug->GetGUIResize()->GetViewModeSize();

						for (int i = 0; i < viewSize; i++)
						{
							if (i == currentViewMode) continue;

							pPlug->GetGUIResize()->SelectViewMode(i);
							pPlug->GetGUIResize()->RearrangeLayers();
							pPlug->GetGUIResize()->ResizeControlRects();

							// Hide removed control
							for (int j = 1; j < pGraphics->GetNControls(); j++)
							{
								IControl* tmp = pGraphics->GetControl(j);
								if (tmp == pControl) pControl->Hide(true);
							}

							// Write to file
							CreateLayoutCode(pPlug, pGraphics, guiScaleRatio, i, false);
						}

						pPlug->GetGUIResize()->SelectViewMode(currentViewMode);
						pPlug->GetGUIResize()->RearrangeLayers();
						pPlug->GetGUIResize()->ResizeControlRects();
					}

					pControl->Hide(true);

					liveControlNumber = -1;
					liveClickedRECT = IRECT(0, 0, 0, 0);
					liveClickedTargetRECT = IRECT(0, 0, 0, 0);
				}

				// Write to file
				CreateLayoutCode(pPlug, pGraphics, guiScaleRatio, currentViewMode, false);

				AddCurrentUndo(pPlug, pGraphics);
			}
		}
	}

	void StoreDefaults(IGraphics* pGraphics)
	{
		for (int i = 0; i < pGraphics->GetNControls(); i++)
		{
			IControl* pControl = pGraphics->GetControl(i);
			IRECT drawRECT = *pControl->GetRECT();
			IRECT targetRECT = *pControl->GetTargetRECT();

			default_draw_rect.push_back(drawRECT);
			default_terget_rect.push_back(targetRECT);
			default_is_hidden.push_back(!pControl->IsHidden());
			default_layers.push_back(pControl);
			current_layers.push_back(pControl);
		}
	}

	void UndoMovingControlLayers(IGraphics* pGraphics)
	{
		for (int i = control_move_to.size() - 1; i > 0; i--)
		{
			pGraphics->MoveControlLayers(control_move_to[i], control_move_from[i]);
		}
	}

	void DoMovingControlLayers(IGraphics* pGraphics)
	{
		for (int i = 0; i < control_move_to.size(); i++)
		{
			pGraphics->MoveControlLayers(control_move_to[i], control_move_from[i]);
		}
	}

	void GetMouseOverControl(IPlugBase* pPlug, IGraphics* pGraphics, int mouseX, int mouseY)
	{
		int controlSize = pGraphics->GetNControls();
		if (pPlug->GetGUIResize()) controlSize -= 3;

		for (int i = controlSize; i >= 0; i--)
		{
			IControl* pControl = pGraphics->GetControl(i);
			pControl->IsHit(mouseX, mouseY);

			mouseOverControl = i;
			return;
		}
		mouseOverControl = -1;
	}

	int FindPointerPosition(IControl* pControl, vector <IControl*> vControl)
	{
		for (int i = 0; i < vControl.size(); i++)
		{
			if (pControl == vControl[i]) return i;
		}
		return -1;
	}

	void RoundIRECT(IRECT* pRECT)
	{
		pRECT->L = int((double)pRECT->L + 0.49999);
		pRECT->T = int((double)pRECT->T + 0.49999);
		pRECT->R = int((double)pRECT->R + 0.49999);
		pRECT->B = int((double)pRECT->B + 0.49999);
	}

	unsigned FindNumberOf(string* in, const char* find)
	{
		unsigned tmpIndex = 0;
		unsigned count = 0;
		while (true)
		{
			unsigned tmp = in->find(find, tmpIndex + 1);

			if (in->npos > tmp)
			{
				count++;
				tmpIndex = tmp;
			}
			else break;

		}
		return count;
	}

	unsigned FindIndexOfOccurrence(string* in, const char* find, unsigned occurrence)
	{
		unsigned tmpIndex = 0;
		unsigned count = 0;
		while (true)
		{
			unsigned tmp = in->find(find, tmpIndex + 1);

			if (in->npos > tmp)
			{
				count++;
				tmpIndex = tmp;

				if (count == occurrence) break;
			}
			else break;

		}
		return tmpIndex;
	}

	bool IsControlDeleted(IControl* pControl)
	{
		int position = FindPointerPosition(pControl, default_layers);

		for (int i = 0; i < deleted_control_default_index.size(); i++)
			if (deleted_control_default_index[i] == position) return true;

		return false;
	}

	bool IsControlSelected(WDL_PtrList<IControl>* pControls, IControl* pControl)
	{
		for (int i = 0; i < selected_controls.size(); i++)
			if (pControls->Get(selected_controls[i])== pControl) return true;

		return false;
	}

	void PrintDeletedControls(IGraphics* pGraphics)
	{
		if (deleted_control_default_index.size() > 0)
		{
			int textSize = 15;
			IText txt(textSize, &EDIT_COLOR_BOTH, defaultFont);
			txt.mAlign = IText::kAlignNear;

			IRECT rect(0, 0, 0, textSize);
			IRECT measure(0, 0, 0, textSize);
			IRECT introRect(0, 0, 0, textSize);

			// Intro Text
			WDL_String intro;
			intro.Set(" Delete this controls: ");
			pGraphics->MeasureIText(&txt, intro.Get(), &introRect);
			pGraphics->FillIRect(&controlTextBackgroundColor, &introRect);
			pGraphics->DrawIText(&txt, intro.Get(), &introRect);

			rect.R = introRect.R;

			// Draw controls that need to be deleted
			for (int j = 0; j < deleted_control_default_index.size(); j++)
			{
				WDL_String text;

				text.SetFormatted(128, "%i (%s), ", deleted_control_default_index[j], default_layers[deleted_control_default_index[j]]->GetDerivedClassName());

				pGraphics->MeasureIText(&txt, text.Get(), &measure);

				// Make text multiline
				if (rect.R + measure.W() <= pGraphics->Width())
				{
					rect.L = rect.R;
					rect.R = rect.R + measure.W();
				}
				else
				{
					rect.L = introRect.R;
					rect.R = rect.L + measure.W();

					int tmpH = rect.H();
					rect.T += tmpH;
					rect.B += tmpH;
				}
				pGraphics->FillIRect(&controlTextBackgroundColor, &rect);
				pGraphics->DrawIText(&txt, text.Get(), &rect);
			}
		}
	}

	void AddUndo(IPlugBase* pPlug, IGraphics* pGraphics)
	{
		int viewMode = 0;
		int viewSize = 1;

		if (pPlug->GetGUIResize())
		{
			viewMode = pPlug->GetGUIResize()->GetViewMode();
			viewSize = pPlug->GetGUIResize()->GetViewModeSize();
		}
		
		// Resize to fit viewSIze
		if (undo_viewMode.size() != viewSize)
		{
			undo_viewMode.resize(viewSize);

			for (int j = 0; j < viewSize; j++)
			{
				undo_viewMode[j].undo_pos = -1;
			}
		}

		// Add new undo to stack
		int undoPos = ++undo_viewMode[viewMode].undo_pos;

		// Resize containers
		undo_viewMode[viewMode].undo_stack.resize(undoPos + 1);
		undo_viewMode[viewMode].undo_stack[undoPos].pointers.resize(pGraphics->GetNControls());
		undo_viewMode[viewMode].undo_stack[undoPos].draw_rect.resize(pGraphics->GetNControls());
		undo_viewMode[viewMode].undo_stack[undoPos].target_rect.resize(pGraphics->GetNControls());
		undo_viewMode[viewMode].undo_stack[undoPos].is_hidden.resize(pGraphics->GetNControls());

		// Add values to the container
		for (int j = 0; j < pGraphics->GetNControls(); j++)
		{
			IControl* pControl = pGraphics->GetControl(j);
			IControl* tmp = default_layers[j];

			undo_viewMode[viewMode].undo_stack[undoPos].pointers[j] = pControl;
			undo_viewMode[viewMode].undo_stack[undoPos].draw_rect[j] = *tmp->GetRECT();
			undo_viewMode[viewMode].undo_stack[undoPos].target_rect[j] = *tmp->GetTargetRECT();
			undo_viewMode[viewMode].undo_stack[undoPos].is_hidden[j] = tmp->IsHidden();
		}

		// Add deleted controls
		undo_viewMode[viewMode].undo_stack[undoPos].deleted_controls = deleted_control_default_index;
	}

	void AddCurrentUndo(IPlugBase* pPlug, IGraphics* pGraphics)
	{
		int viewMode = 0;
		int viewSize = 1;
		lastWasUndo = false;
		lastWasRedo = false;

		if (pPlug->GetGUIResize())
		{
			viewMode = pPlug->GetGUIResize()->GetViewMode();
			viewSize = pPlug->GetGUIResize()->GetViewModeSize();
		}

		// Resize vector
		undo_current_stack.resize(viewSize);
		undo_current_stack[viewMode].pointers.resize(pGraphics->GetNControls());
		undo_current_stack[viewMode].draw_rect.resize(pGraphics->GetNControls());
		undo_current_stack[viewMode].target_rect.resize(pGraphics->GetNControls());
		undo_current_stack[viewMode].is_hidden.resize(pGraphics->GetNControls());

		// Add values to the container
		for (int j = 0; j < pGraphics->GetNControls(); j++)
		{
			IControl* pControl = pGraphics->GetControl(j);
			IControl* tmp = default_layers[j];

			undo_current_stack[viewMode].pointers[j] = pControl;
			undo_current_stack[viewMode].draw_rect[j] = *tmp->GetRECT();
			undo_current_stack[viewMode].target_rect[j] = *tmp->GetTargetRECT();
			undo_current_stack[viewMode].is_hidden[j] = tmp->IsHidden();
		}

		// Add deleted controls
		undo_current_stack[viewMode].deleted_controls = deleted_control_default_index;
	}

	void GetUndo(IPlugBase* pPlug, IGraphics* pGraphics)
	{
		int viewMode = 0;
		int viewSize = 1;
		double guiScaleRatio = 1.0;
		lastWasUndo = true;

		if (pPlug->GetGUIResize())
		{
			viewMode = pPlug->GetGUIResize()->GetViewMode();
			viewSize = pPlug->GetGUIResize()->GetViewModeSize();
			guiScaleRatio = pPlug->GetGUIResize()->GetGUIScaleRatio();
		}

		if (lastWasRedo)
		{
			if (undo_viewMode[viewMode].undo_pos + 1 < undo_viewMode[viewMode].undo_stack.size())
			undo_viewMode[viewMode].undo_pos--;

			lastWasRedo = false;
		}

		int undoPos = undo_viewMode[viewMode].undo_pos;

		if (undoPos >= 0)
		{
			// Set current view
			for (int j = 0; j < pGraphics->GetNControls(); j++)
			{
				IControl* tmp = pGraphics->GetControl(j);
				int defaultPos = FindPointerPosition(tmp, default_layers);
				int undoPointerPos = FindPointerPosition(tmp, undo_viewMode[viewMode].undo_stack[undoPos].pointers);

				pGraphics->ReplaceControl(j, undo_viewMode[viewMode].undo_stack[undoPos].pointers[j]);
				tmp->SetDrawRECT(undo_viewMode[viewMode].undo_stack[undoPos].draw_rect[defaultPos]);
				tmp->SetTargetRECT(undo_viewMode[viewMode].undo_stack[undoPos].target_rect[defaultPos]);
				tmp->Hide(undo_viewMode[viewMode].undo_stack[undoPos].is_hidden[defaultPos]);
			}

			// Get deleted controls
			// If deleted controls is changed, unhide all deleted controls and then hide controls from new list
			if (!VectorEquals(deleted_control_default_index, undo_viewMode[viewMode].undo_stack[undoPos].deleted_controls))
			{
				// Do if GUI resize is active
				if (pPlug->GetGUIResize())
				{
					int currentViewMode = pPlug->GetGUIResize()->GetViewMode();
					int viewSize = pPlug->GetGUIResize()->GetViewModeSize();

					// Unhide all deleted controls
					for (int i = 0; i < viewSize; i++)
					{
						pPlug->GetGUIResize()->SelectViewMode(i);
						pPlug->GetGUIResize()->RearrangeLayers();
						pPlug->GetGUIResize()->ResizeControlRects();

						// Hide removed control
						for (int j = 1; j < pGraphics->GetNControls(); j++)
						{
							IControl* tmp = pGraphics->GetControl(j);
							if (ControlIsDeleted(tmp, deleted_control_default_index)) tmp->Hide(false);
						}

						// Write to file
						CreateLayoutCode(pPlug, pGraphics, guiScaleRatio, i, false);
					}

					deleted_control_default_index = undo_viewMode[viewMode].undo_stack[undoPos].deleted_controls;

					// Hide all deleted controls
					for (int i = 0; i < viewSize; i++)
					{
						pPlug->GetGUIResize()->SelectViewMode(i);
						pPlug->GetGUIResize()->RearrangeLayers();
						pPlug->GetGUIResize()->ResizeControlRects();

						// Hide removed control
						for (int j = 1; j < pGraphics->GetNControls(); j++)
						{
							IControl* tmp = pGraphics->GetControl(j);
							//if (ControlIsDeleted(tmp, deleted_control_default_index)) tmp->Hide(true);
						}

						// Write to file
						CreateLayoutCode(pPlug, pGraphics, guiScaleRatio, i, false);
					}

					pPlug->GetGUIResize()->SelectViewMode(currentViewMode);
					pPlug->GetGUIResize()->RearrangeLayers();
					pPlug->GetGUIResize()->ResizeControlRects();
				}
				else // If GUI resize is not active
				{
					deleted_control_default_index = undo_viewMode[viewMode].undo_stack[undoPos].deleted_controls;

					// Hide removed control
					for (int j = 1; j < pGraphics->GetNControls(); j++)
					{
						IControl* tmp = pGraphics->GetControl(j);
						if (ControlIsDeleted(tmp, deleted_control_default_index)) tmp->Hide(true);
					}

					// Write to file
					CreateLayoutCode(pPlug, pGraphics, guiScaleRatio, viewMode, false);
				}
			}
			else deleted_control_default_index = undo_viewMode[viewMode].undo_stack[undoPos].deleted_controls;

			CreateLayoutCode(pPlug, pGraphics, guiScaleRatio, viewMode, false);
			undo_viewMode[viewMode].undo_pos--;
		}

		liveControlNumber = -1;
		liveClickedRECT = IRECT(0, 0, 0, 0);
		liveClickedTargetRECT = IRECT(0, 0, 0, 0);
	}

	void GetRedo(IPlugBase* pPlug, IGraphics* pGraphics)
	{
		int viewMode = 0;
		int viewSize = 1;
		double guiScaleRatio = 1.0;
		lastWasRedo = true;

		if (pPlug->GetGUIResize())
		{
			viewMode = pPlug->GetGUIResize()->GetViewMode();
			viewSize = pPlug->GetGUIResize()->GetViewModeSize();
			guiScaleRatio = pPlug->GetGUIResize()->GetGUIScaleRatio();
		}

		if (lastWasUndo)
		{
			undo_viewMode[viewMode].undo_pos++;
			lastWasUndo = false;
		}

		int undoPos = undo_viewMode[viewMode].undo_pos;

		if (undoPos + 1 < undo_viewMode[viewMode].undo_stack.size())
		{
			undo_viewMode[viewMode].undo_pos++;
			undoPos++;
			
			if (undoPos >= 0)
			{
				// Set current view
				for (int j = 0; j < pGraphics->GetNControls(); j++)
				{
					IControl* tmp = pGraphics->GetControl(j);
					int defaultPos = FindPointerPosition(tmp, default_layers);
					int undoPointerPos = FindPointerPosition(tmp, undo_viewMode[viewMode].undo_stack[undoPos].pointers);

					pGraphics->ReplaceControl(j, undo_viewMode[viewMode].undo_stack[undoPos].pointers[j]);
					tmp->SetDrawRECT(undo_viewMode[viewMode].undo_stack[undoPos].draw_rect[defaultPos]);
					tmp->SetTargetRECT(undo_viewMode[viewMode].undo_stack[undoPos].target_rect[defaultPos]);
					tmp->Hide(undo_viewMode[viewMode].undo_stack[undoPos].is_hidden[defaultPos]);
				}

				// Get deleted controls
				deleted_control_default_index = undo_viewMode[viewMode].undo_stack[undoPos].deleted_controls;

				CreateLayoutCode(pPlug, pGraphics, guiScaleRatio, viewMode, false);
			}
		}
		else // Need to get current state
		{
			if (undoPos >= 0)
			{
				// Set current view
				for (int j = 0; j < pGraphics->GetNControls(); j++)
				{
					IControl* tmp = pGraphics->GetControl(j);
					int defaultPos = FindPointerPosition(tmp, default_layers);
					int undoPointerPos = FindPointerPosition(tmp, undo_current_stack[viewMode].pointers);

					pGraphics->ReplaceControl(j, undo_current_stack[viewMode].pointers[j]);
					tmp->SetDrawRECT(undo_current_stack[viewMode].draw_rect[defaultPos]);
					tmp->SetTargetRECT(undo_current_stack[viewMode].target_rect[defaultPos]);
					tmp->Hide(undo_current_stack[viewMode].is_hidden[defaultPos]);
				}

				// Get deleted controls
				deleted_control_default_index = undo_current_stack[viewMode].deleted_controls;

				CreateLayoutCode(pPlug, pGraphics, guiScaleRatio, viewMode, false);
			}
		}

		liveControlNumber = -1;
		liveClickedRECT = IRECT(0, 0, 0, 0);
		liveClickedTargetRECT = IRECT(0, 0, 0, 0);
	}

	void DeleteLastSameUndo(IPlugBase* pPlug, IGraphics* pGraphics)
	{
		// If the new undo is same as previous, delete the new one

		int viewMode = 0;
		int viewSize = 1;

		if (pPlug->GetGUIResize())
		{
			viewMode = pPlug->GetGUIResize()->GetViewMode();
			viewSize = pPlug->GetGUIResize()->GetViewModeSize();
		}

		// Check if previous undo is same as current
		bool a = false, b = false, c = false, d = false, e = false;

		int undoSize = undo_viewMode[viewMode].undo_stack.size();

		if (undo_viewMode[viewMode].undo_stack.size() > 1)
		{
			a = VectorEquals(undo_viewMode[viewMode].undo_stack[undoSize - 1].pointers, 
				undo_viewMode[viewMode].undo_stack[undoSize - 2].pointers);
			b = VectorEquals(undo_viewMode[viewMode].undo_stack[undoSize - 1].draw_rect, 
				undo_viewMode[viewMode].undo_stack[undoSize - 2].draw_rect);
			c = VectorEquals(undo_viewMode[viewMode].undo_stack[undoSize - 1].target_rect,
				undo_viewMode[viewMode].undo_stack[undoSize - 2].target_rect);
			d = VectorEquals(undo_viewMode[viewMode].undo_stack[undoSize - 1].is_hidden, 
				undo_viewMode[viewMode].undo_stack[undoSize - 2].is_hidden);
			e = VectorEquals(undo_viewMode[viewMode].undo_stack[undoSize - 1].deleted_controls, 
				undo_viewMode[viewMode].undo_stack[undoSize - 2].deleted_controls);
		}

		if (a && b && c && d && e)
		{
			undo_viewMode[viewMode].undo_stack.pop_back();
			undo_viewMode[viewMode].undo_pos--;
		}
	}

	bool ControlIsDeleted(IControl* pControl, vector <int> indexes)
	{
		for (int i = 0; i < indexes.size(); i++)
		{
			if (default_layers[indexes[i]] == pControl) return true;
		}
		return false;
	}

	IRECT RestrictToWindow(IRECT in, int windowWidth, int windowHeight)
	{
		int moveX = 0, moveY = 0;

		if ((in.L < 0 && in.R > windowWidth) || (in.T < 0 && in.B > windowHeight))
			assert("Window is too small for this control!");

		if (in.L < 0) moveX = 0 - in.L;
		if (in.T < 0) moveY = 0 - in.T;
		if (in.R > windowWidth) moveX = windowWidth - in.R;
		if (in.B > windowHeight) moveY = windowHeight - in.B;

		in.L += moveX;
		in.T += moveY;
		in.R += moveX;
		in.B += moveY;

		return in;
	}

	template <typename vectorType>
	bool VectorEquals(vector <vectorType> vec1, vector <vectorType> vec2)
	{
		if (vec1.size() == vec2.size())
		{
			for (int i = 0; i < vec1.size(); i++)
			{
				if (vec1[i] != vec2[i]) return false;
			}
		}
		else return false;

		return true;
	}

	
private:
	// Live editing stuff
	char* defaultFont = "Tahoma";
	IColor EDIT_COLOR_BOTH = IColor(255, 255, 255, 255);
	IColor EDIT_COLOR_DRAW = IColor(255, 50, 255, 50);
	IColor EDIT_COLOR_TARGET = IColor(255, 255, 255, 50);
	IColor SELECTING_COLOR = IColor(255, 255, 255, 255);
	IColor SELECTION_COLOR = IColor(255, 255, 100, 100);
	IColor SNAP_COLOR = IColor(255, 255, 255, 255);
	IColor controlTextBackgroundColor = IColor(122, 0, 0, 0);
	IRECT liveSelectedRECT = IRECT(0, 0, 0, 0);
	IRECT liveSelectedTargetRECT = IRECT(0, 0, 0, 0);
	IRECT liveClickedRECT = IRECT(0, 0, 0, 0);
	IRECT liveClickedTargetRECT = IRECT(0, 0, 0, 0);
	IRECT selectionRECT = IRECT(999999999, 999999999, 0, 0);
	IRECT selectedControlsRECT = IRECT(999999999, 999999999, 0, 0);
	int liveControlNumber = -1;
	int lastliveMouseCapture = -1;
	int liveClickedX = 0, liveClickedY = 0;
	int liveSelectionClickedX = 0, liveSelectionClickedY = 0;
	int liveScaledGridSize = 1;
	bool liveClickedOnHandle = false;
	bool liveLastMouseDownL = false;
	int liveMouseXLock = 0;
	int liveMouseYLock = 0;
	bool drawGridToogle = false;
	int mouseOverControl = -1;
	vector <IRECT> default_draw_rect;
	vector <IRECT> default_terget_rect;
	vector <bool> default_is_hidden;
	vector <IControl*> default_layers;
	vector <bool> control_visibility;
	vector <int> control_move_from;
	vector <int> control_move_to;
	vector <IControl*> current_layers;
	vector <string> code_view_mode;
	vector <int> deleted_control_default_index;
	vector <int> selected_controls;
	vector <IRECT> selected_draw_rect;
	vector <IRECT> selected_target_rect;
	int currentViewMode = 0;
	int viewModeSize = 1;
	bool retrieveOldLayoutChanges = false;
	bool undoMove = true;
	bool lastWasUndo = false;
	bool lastWasRedo = false;
	bool drawMouseCoordinats = false;
	bool multipleControlsSelected = false;
	bool clickedOnSelection = false;
	bool prevClickedOnSelection = false;
	bool selectingControls = false;
	bool prevSelectingControls = false;
	int clickedX, clickedY;
	bool lastLDown = false;
	bool lastRDown = false;
	int clickedSelectionX = -1, clickedSelectionY = -1;
	int liveHandleSize;
	int editMode = 0;
	int snapMode = 0;

	
	struct undo
	{
		vector <IRECT> draw_rect;
		vector <IRECT> target_rect;
		vector <bool> is_hidden;
		vector <IControl*> pointers;
		vector <int> deleted_controls;
	};

	struct undoStack
	{
		vector <undo> undo_stack;
		int undo_pos;
	};

	vector <undo> undo_current_stack;
	vector <undoStack> undo_viewMode;
};
#endif