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
		bool* liveMouseDragging, int* mMouseX, int* mMouseY, int width, int height, double guiScaleRatio)
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

			for (int j = 1; j < controlSize; j++)
			{
				IControl* pControl = pControls->Get(j);

				IRECT drawRECT = *pControl->GetRECT();

				// Dont outline deleted control
				if (!IsControlDeleted(pControl))
				{
					// T
					LICE_DashedLine(pDrawBitmap, drawRECT.L, drawRECT.T, drawRECT.R, drawRECT.T, 2, 2,
						LICE_RGBA(EDIT_COLOR.R, EDIT_COLOR.G, EDIT_COLOR.B, EDIT_COLOR.A));
					//B
					LICE_DashedLine(pDrawBitmap, drawRECT.L, drawRECT.B, drawRECT.R, drawRECT.B, 2, 2,
						LICE_RGBA(EDIT_COLOR.R, EDIT_COLOR.G, EDIT_COLOR.B, EDIT_COLOR.A));
					//L
					LICE_DashedLine(pDrawBitmap, drawRECT.L, drawRECT.T, drawRECT.L, drawRECT.B, 2, 2,
						LICE_RGBA(EDIT_COLOR.R, EDIT_COLOR.G, EDIT_COLOR.B, EDIT_COLOR.A));
					//R
					LICE_DashedLine(pDrawBitmap, drawRECT.R, drawRECT.T, drawRECT.R, drawRECT.B, 2, 2,
						LICE_RGBA(EDIT_COLOR.R, EDIT_COLOR.G, EDIT_COLOR.B, EDIT_COLOR.A));
				}
			}

			WDL_String str;
			str.SetFormatted(32, "x: %i, y: %i", *mMouseX, *mMouseY);
			IText txt(20, &EDIT_COLOR, defaultFont);
			IRECT rect(width - 150, height - 20, width, height);
			pGraphics->DrawIText(&txt, str.Get(), &rect);

			// Draw resizing handles
			int liveHandleSize = int(8.0 * guiScaleRatio);

			for (int j = 1; j < controlSize; j++)
			{
				IControl* pControl = pControls->Get(j);

				if (!IsControlDeleted(pControl))
				{
					IRECT drawRECT = *pControl->GetRECT();
					IRECT handle = IRECT(drawRECT.R - liveHandleSize, drawRECT.B - liveHandleSize, drawRECT.R, drawRECT.B);
					pGraphics->FillTriangle(&EDIT_COLOR, handle.L, handle.B, handle.R, handle.B, handle.R, handle.T, 0);
				}
			}

			bool overControlHandle = false;
			// Find if over control handle
			for (int j = 1; j < controlSize; j++)
			{
				IControl* pControl = pControls->Get(j);

				if (!IsControlDeleted(pControl))
				{
					IRECT drawRECT = *pControl->GetRECT();
					IRECT handle = IRECT(drawRECT.R - liveHandleSize, drawRECT.B - liveHandleSize, drawRECT.R, drawRECT.B);

					if (drawRECT.Contains(*mMouseX, *mMouseY))
					{
						overControlHandle = handle.Contains(*mMouseX, *mMouseY);
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

					IRECT handle = IRECT(liveClickedRECT.R - liveHandleSize, liveClickedRECT.B - liveHandleSize, liveClickedRECT.R, liveClickedRECT.B);
					liveClickedOnHandle = handle.Contains(liveClickedX, liveClickedY);
				}
			}

			// Prepare undo to be executed on next mose click if nedeed
			if (!liveEditingMod->L) undoMove = true;

			if (liveEditingMod->L && !IsControlDeleted(pControls->Get(*liveMouseCapture)))
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

					IRECT handle = IRECT(liveClickedRECT.R - liveHandleSize, liveClickedRECT.B - liveHandleSize, liveClickedRECT.R, liveClickedRECT.B);
					liveClickedOnHandle = handle.Contains(liveClickedX, liveClickedY);
				}


				// Change cursor when clicked on handle
				if (liveClickedOnHandle || overControlHandle) SetCursor(LoadCursor(NULL, IDC_SIZENWSE));
				else SetCursor(LoadCursor(NULL, IDC_ARROW));

				// Prevent editing
				if (*liveMouseDragging || liveEditingMod->C)
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
						targetArea.L = liveClickedRECT.L + (*mMouseX - liveClickedX);
						targetArea.T = liveClickedRECT.T + (*mMouseY - liveClickedY);
						targetArea.R = liveClickedRECT.R + (*mMouseX - liveClickedX);
						targetArea.B = liveClickedRECT.B + (*mMouseY - liveClickedY);
					}
					else
					{
						targetArea.L = liveClickedRECT.L;
						targetArea.T = liveClickedRECT.T;
						targetArea.R = liveClickedRECT.R + (*mMouseX - liveClickedX);
						targetArea.B = liveClickedRECT.B + (*mMouseY - liveClickedY);
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

					// Snap to other control
					if (liveEditingMod->C)
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
							if (index == *liveMouseCapture) continue;

							IControl* pSnapControl = pControls->Get(index);
							IRECT tmpDrawArea = *pSnapControl->GetRECT();
							int tmpSnapL;
							IRECT tmpRECTL;

							int tmpDrawArea_L = tmpDrawArea.L;
							int tmpDrawArea_LM = tmpDrawArea.L + tmpDrawArea.W() / 2;
							int tmpDrawArea_R = tmpDrawArea.R;
							int tmpDrawArea_T = tmpDrawArea.T;
							int tmpDrawArea_TM = tmpDrawArea.T + tmpDrawArea.H() / 2;
							int tmpDrawArea_B = tmpDrawArea.B;

							int drawArea_L = drawArea.L;
							int drawArea_LM = drawArea.L + drawArea.W() / 2;
							int drawArea_R = drawArea.R;
							int drawArea_T = drawArea.T;
							int drawArea_TM = drawArea.T + drawArea.H() / 2;
							int drawArea_B = drawArea.B;

							// Find snap to L
							for (int j = 0; j < 9; j++)
							{
								if (liveClickedOnHandle)
								{
									if ((j / 3) * 3 == j) continue;
								}

								if (j == 0) // L to L
								{
									tmpSnapL = tmpDrawArea_L - drawArea_L;
									tmpRECTL = IRECT(tmpDrawArea_L, tmpDrawArea_TM, drawArea_L, drawArea_TM);
								}
								if (j == 1) // L to LM
								{
									tmpSnapL = tmpDrawArea_L - drawArea_LM;
									tmpRECTL = IRECT(tmpDrawArea_L, tmpDrawArea_TM, drawArea_LM, drawArea_TM);
								}
								if (j == 2) // L to R
								{
									tmpSnapL = tmpDrawArea_L - drawArea_R;
									tmpRECTL = IRECT(tmpDrawArea_L, tmpDrawArea_TM, drawArea_R, drawArea_TM);
								}
								if (j == 3) // LM to L
								{
									tmpSnapL = tmpDrawArea_LM - drawArea_L;
									tmpRECTL = IRECT(tmpDrawArea_LM, tmpDrawArea_TM, drawArea_L, drawArea_TM);
								}
								if (j == 4) // LM to LM
								{
									tmpSnapL = tmpDrawArea_LM - drawArea_LM;
									tmpRECTL = IRECT(tmpDrawArea_LM, tmpDrawArea_TM, drawArea_LM, drawArea_TM);
								}
								if (j == 5) // LM to R
								{
									tmpSnapL = tmpDrawArea_LM - drawArea_R;
									tmpRECTL = IRECT(tmpDrawArea_LM, tmpDrawArea_TM, drawArea_R, drawArea_TM);
								}
								if (j == 6) // R to L
								{
									tmpSnapL = tmpDrawArea_R - drawArea_L;
									tmpRECTL = IRECT(tmpDrawArea_R, tmpDrawArea_TM, drawArea_L, drawArea_TM);
								}
								if (j == 7) // R to LM
								{
									tmpSnapL = tmpDrawArea_R - drawArea_LM;
									tmpRECTL = IRECT(tmpDrawArea_R, tmpDrawArea_TM, drawArea_LM, drawArea_TM);
								}
								if (j == 8) // R to R
								{
									tmpSnapL = tmpDrawArea_R - drawArea_R;
									tmpRECTL = IRECT(tmpDrawArea_R, tmpDrawArea_TM, drawArea_R, drawArea_TM);
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
								if (!liveClickedOnHandle)
								{
									drawArea.L = drawArea.L + snapL;
									drawArea.R = drawArea.R + snapL;
								}
								else drawArea.R = drawArea.R + snapL;

								if (!liveClickedOnHandle)
								{
									targetArea.L = targetArea.L + snapL;
									targetArea.R = targetArea.R + snapL;
								}
								else targetArea.R = targetArea.R + snapL;
							}

							// Draw snap line
							LICE_DashedLine(pDrawBitmap, lineL.L, lineL.T, lineL.R + snapL, lineL.B, 2, 2,
								LICE_RGBA(EDIT_COLOR.R, EDIT_COLOR.G, EDIT_COLOR.B, EDIT_COLOR.A));
						}

						for (int index = 0; index < controlSize; index++)
						{
							if (index == *liveMouseCapture) continue;

							IControl* pSnapControl = pControls->Get(index);
							IRECT tmpDrawArea = *pSnapControl->GetRECT();
							int tmpSnapT;
							IRECT tmpRECTT;

							int tmpDrawArea_L = tmpDrawArea.L;
							int tmpDrawArea_LM = tmpDrawArea.L + tmpDrawArea.W() / 2;
							int tmpDrawArea_R = tmpDrawArea.R;
							int tmpDrawArea_T = tmpDrawArea.T;
							int tmpDrawArea_TM = tmpDrawArea.T + tmpDrawArea.H() / 2;
							int tmpDrawArea_B = tmpDrawArea.B;

							int drawArea_L = drawArea.L;
							int drawArea_LM = drawArea.L + drawArea.W() / 2;
							int drawArea_R = drawArea.R;
							int drawArea_T = drawArea.T;
							int drawArea_TM = drawArea.T + drawArea.H() / 2;
							int drawArea_B = drawArea.B;

							// Find snap to T
							for (int j = 0; j < 9; j++)
							{
								if (liveClickedOnHandle)
								{
									if ((j / 3) * 3 == j) continue;
								}

								if (j == 0) // T to T
								{
									tmpSnapT = tmpDrawArea_T - drawArea_T;
									tmpRECTT = IRECT(tmpDrawArea_LM, tmpDrawArea_T, drawArea_LM, drawArea_T);
								}
								if (j == 1) // T to TM
								{
									tmpSnapT = tmpDrawArea_T - drawArea_TM;
									tmpRECTT = IRECT(tmpDrawArea_LM, tmpDrawArea_T, drawArea_LM, drawArea_TM);
								}
								if (j == 2) // T to B
								{
									tmpSnapT = tmpDrawArea_T - drawArea_B;
									tmpRECTT = IRECT(tmpDrawArea_LM, tmpDrawArea_T, drawArea_LM, drawArea_B);
								}
								if (j == 3) // TM to T
								{
									tmpSnapT = tmpDrawArea_TM - drawArea_T;
									tmpRECTT = IRECT(tmpDrawArea_LM, tmpDrawArea_TM, drawArea_LM, drawArea_T);
								}
								if (j == 4) // TM to TM
								{
									tmpSnapT = tmpDrawArea_TM - drawArea_TM;
									tmpRECTT = IRECT(tmpDrawArea_LM, tmpDrawArea_TM, drawArea_LM, drawArea_TM);
								}
								if (j == 5) // TM to B
								{
									tmpSnapT = tmpDrawArea_TM - drawArea_B;
									tmpRECTT = IRECT(tmpDrawArea_LM, tmpDrawArea_TM, drawArea_LM, drawArea_B);
								}
								if (j == 6) // B to T
								{
									tmpSnapT = tmpDrawArea_B - drawArea_T;
									tmpRECTT = IRECT(tmpDrawArea_LM, tmpDrawArea_B, drawArea_LM, drawArea_T);
								}
								if (j == 7) // B to TM
								{
									tmpSnapT = tmpDrawArea_B - drawArea_TM;
									tmpRECTT = IRECT(tmpDrawArea_LM, tmpDrawArea_B, drawArea_LM, drawArea_TM);
								}
								if (j == 8) // B to B
								{
									tmpSnapT = tmpDrawArea_B - drawArea_B;
									tmpRECTT = IRECT(tmpDrawArea_LM, tmpDrawArea_B, drawArea_LM, drawArea_B);
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
								if (!liveClickedOnHandle)
								{
									drawArea.T = drawArea.T + snapT;
									drawArea.B = drawArea.B + snapT;
								}
								else drawArea.B = drawArea.B + snapT;


								if (!liveClickedOnHandle)
								{
									targetArea.T = targetArea.T + snapT;
									targetArea.B = targetArea.B + snapT;
								}
								else targetArea.B = targetArea.B + snapT;
							}

							// Draw snap line
							LICE_DashedLine(pDrawBitmap, lineT.L, lineT.T, lineT.R, lineT.B + snapT, 2, 2,
								LICE_RGBA(EDIT_COLOR.R, EDIT_COLOR.G, EDIT_COLOR.B, EDIT_COLOR.A));
						}
					}

					// Prevent moving background
					if (liveControlNumber > 0)
					{
						// Add undo
						if ((pControl->GetRECT() != &drawArea || pControl->GetTargetRECT() != &targetArea) && undoMove)
						{
							AddUndo(pPlug, pGraphics);
							undoMove = false;
						}

						pControl->SetDrawRECT(drawArea);
						pControl->SetTargetRECT(targetArea);

						// Add current undo
						if (!undoMove)
						{
							AddCurrentUndo(pPlug, pGraphics);
						}
					}

					liveSelectedRECT = drawArea;
				}
			}

			*liveMouseDragging = false;
			lastliveMouseCapture = *liveMouseCapture;
		}

		if (*liveToogleEditing)
		{
			// Outline selected rect
			if (liveControlNumber > 0) pGraphics->DrawRect(&EDIT_COLOR, pControls->Get(liveControlNumber)->GetRECT());

			if (drawGridToogle) DrawGrid(pDrawBitmap, width, height);

			// Check if gui resize is active, if so scale out rect
			IRECT printRECT;
			if (pPlug->GetGUIResize())
			{
				printRECT.L = int((double)liveSelectedRECT.L * guiScaleRatio);
				printRECT.T = int((double)liveSelectedRECT.T * guiScaleRatio);
				printRECT.R = int((double)liveSelectedRECT.R * guiScaleRatio);
				printRECT.B = int((double)liveSelectedRECT.B * guiScaleRatio);

			}
			else
			{
				printRECT = liveSelectedRECT;
			}

			// Print selected control
			int textSize = 15;
			WDL_String controlNumber;

			// Different outputs if control is hidden
			if (liveControlNumber > 0 && !pGraphics->GetControl(liveControlNumber)->IsHidden())
				controlNumber.SetFormatted(100, "N:%i IRECT(%i,%i,%i,%i)", liveControlNumber, printRECT.L, printRECT.T, printRECT.R, printRECT.B);
			if (liveControlNumber > 0 && pGraphics->GetControl(liveControlNumber)->IsHidden())
				controlNumber.SetFormatted(100, "N:%i IRECT(%i,%i,%i,%i) (HIDDEN)", liveControlNumber, printRECT.L, printRECT.T, printRECT.R, printRECT.B);

			IText txtControlNumber(textSize, &EDIT_COLOR, defaultFont, IText::kStyleNormal, IText::kAlignNear, 0, IText::kQualityClearType);
			IRECT textRect;
			if (liveControlNumber > 0) pGraphics->MeasureIText(&txtControlNumber, controlNumber.Get(), &textRect);
			IRECT rectControlNumber(liveSelectedRECT.L, liveSelectedRECT.T - textSize, liveSelectedRECT.L + textRect.R, liveSelectedRECT.T);

			if (rectControlNumber.T < 0)
			{
				rectControlNumber.T = liveSelectedRECT.B;
				rectControlNumber.B = liveSelectedRECT.B + textSize;
			}

			if (liveControlNumber > 0) pGraphics->FillIRect(&controlTextBackgroundColor, &rectControlNumber);
			if (liveControlNumber > 0) pGraphics->DrawIText(&txtControlNumber, controlNumber.Get(), &rectControlNumber);
			
			if (liveEditingMod->R) DoPopupMenu(pPlug, pGraphics, *mMouseX, *mMouseY, guiScaleRatio);

			// Write to file
			CreateLayoutCode(pPlug, pGraphics, guiScaleRatio, currentViewMode, false);
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
			"// Copyright Youlean 2016 \n \n"
            "#include <vector>\n"
			"#include \"IGraphics.h\" \n"
			"#include \"IPlugGUIResize.h\" \n\n"
			"class LiveEditLayout \n"
			"{ \n"
			"public: \n"
			"	LiveEditLayout() {} \n \n"
			"	~LiveEditLayout() {} \n \n"
			"	void SetControlPositions(IGraphics* pGraphics) \n"
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

		code.append("}; ");
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
					LICE_RGBA(EDIT_COLOR.R, EDIT_COLOR.G, EDIT_COLOR.B, EDIT_COLOR.A), 0.17f);
			}

			// Horisontal Lines grid
			for (int i = 0; i < height; i += liveScaledGridSize)
			{
				LICE_Line(pDrawBitmap, 0, i, width, i,
					LICE_RGBA(EDIT_COLOR.R, EDIT_COLOR.G, EDIT_COLOR.B, EDIT_COLOR.A), 0.17f);
			}
		}
		else
		{
			LICE_FillRect(pDrawBitmap, 0, 0, width, height,
				LICE_RGBA(EDIT_COLOR.R, EDIT_COLOR.G, EDIT_COLOR.B, EDIT_COLOR.A), 0.11f);
		}
	}

	void DoPopupMenu(IPlugBase* pPlug, IGraphics* pGraphics, int x, int y, double guiScaleRatio)
	{
		IPopupMenu menu;

		// Item 0
		menu.AddItem("Undo");

		// Item 1
		menu.AddItem("Redo");

		// Item 2
		menu.AddSeparator();

		// Item 3
		menu.AddItem("Reset Control Position");
		if (liveControlNumber <= 0) menu.SetItemState(3, IPopupMenuItem::kDisabled, true);

		// Item 4
		menu.AddItem("Reset Control Size");
		if (liveControlNumber <= 0) menu.SetItemState(4, IPopupMenuItem::kDisabled, true);

		// Item 5
		menu.AddSeparator();

		// Item 6
		if (pGraphics->GetControl(liveControlNumber)->IsHidden()) menu.AddItem("Show Control");
		else menu.AddItem("Hide Control");
		if (liveControlNumber <= 0) menu.SetItemState(6, IPopupMenuItem::kDisabled, true);

		// Item 7
		menu.AddSeparator();

		// Item 8
		menu.AddItem("Bring to Front");
		if (liveControlNumber <= 0) menu.SetItemState(8, IPopupMenuItem::kDisabled, true);

		// Item 9
		menu.AddItem("Send to Back");
		if (liveControlNumber <= 0) menu.SetItemState(9, IPopupMenuItem::kDisabled, true);

		// Item 10
		menu.AddSeparator();

		// Item 11
		menu.AddItem("Bring Forward");
		if (liveControlNumber <= 0) menu.SetItemState(11, IPopupMenuItem::kDisabled, true);

		// Item 12
		menu.AddItem("Send Backward");
		if (liveControlNumber <= 0) menu.SetItemState(12, IPopupMenuItem::kDisabled, true);

		// Item 13
		menu.AddSeparator();

		// Item 14
		if (drawGridToogle) menu.AddItem("Hide Grid");
		else menu.AddItem("Show Grid");

		// Item 15
		menu.AddSeparator();

		// Item 16
		menu.AddItem("Reset View");

		// Item 17
		menu.AddSeparator();

		// Item 18
		menu.AddItem("Delete Control");

		if (pGraphics->CreateIPopupMenu(&menu, x, y))
		{
			int itemChosen = menu.GetChosenItemIdx();

			// Undo
			if (itemChosen == 0)
			{
				GetUndo(pPlug, pGraphics);
			}

			// Redo
			if (itemChosen == 1)
			{
				GetRedo(pPlug, pGraphics);
			}

			// Reset Control Position
			if (itemChosen == 3)
			{
				AddUndo(pPlug, pGraphics);

				IRECT drawRECT;
				IRECT targetRECT;

				drawRECT.L = int((double)default_draw_rect[liveControlNumber].L * guiScaleRatio);
				drawRECT.T = int((double)default_draw_rect[liveControlNumber].T * guiScaleRatio);
				drawRECT.R = drawRECT.L + liveSelectedRECT.W();
				drawRECT.B = drawRECT.T + liveSelectedRECT.H();

				targetRECT.L = int((double)default_terget_rect[liveControlNumber].L * guiScaleRatio);
				targetRECT.T = int((double)default_terget_rect[liveControlNumber].T * guiScaleRatio);
				targetRECT.R = targetRECT.L + liveSelectedTargetRECT.W();
				targetRECT.B = targetRECT.T + liveSelectedTargetRECT.H();

				liveSelectedRECT = drawRECT;
				liveSelectedTargetRECT = targetRECT;

				pGraphics->GetControl(liveControlNumber)->SetDrawRECT(drawRECT);
				pGraphics->GetControl(liveControlNumber)->SetTargetRECT(targetRECT);

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

				drawRECT.L = liveSelectedRECT.L;
				drawRECT.T = liveSelectedRECT.T;
				drawRECT.R = drawRECT.L + int((double)default_draw_rect[liveControlNumber].W() * guiScaleRatio);
				drawRECT.B = drawRECT.T + int((double)default_draw_rect[liveControlNumber].H() * guiScaleRatio);

				targetRECT.L = liveSelectedTargetRECT.L;
				targetRECT.T = liveSelectedTargetRECT.T;
				targetRECT.R = targetRECT.L + int((double)default_terget_rect[liveControlNumber].W() * guiScaleRatio);
				targetRECT.B = targetRECT.T + int((double)default_terget_rect[liveControlNumber].H() * guiScaleRatio);

				liveSelectedRECT = drawRECT;
				liveSelectedTargetRECT = targetRECT;

				pGraphics->GetControl(liveControlNumber)->SetDrawRECT(drawRECT);
				pGraphics->GetControl(liveControlNumber)->SetTargetRECT(targetRECT);

				// Write to file
				CreateLayoutCode(pPlug, pGraphics, guiScaleRatio, currentViewMode, false);

				AddCurrentUndo(pPlug, pGraphics);
			}

			// Show/Hide Control
			if (itemChosen == 6)
			{
				AddUndo(pPlug, pGraphics);

				if (pGraphics->GetControl(liveControlNumber)->IsHidden()) pGraphics->GetControl(liveControlNumber)->Hide(false);
				else pGraphics->GetControl(liveControlNumber)->Hide(true);

				// Write to file
				CreateLayoutCode(pPlug, pGraphics, guiScaleRatio, currentViewMode, false);

				AddCurrentUndo(pPlug, pGraphics);
			}

			// Bring to Front
			if (itemChosen == 8)
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
			if (itemChosen == 9)
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
			if (itemChosen == 11)
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
			if (itemChosen == 12)
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

			// Show/Hide Grid
			if (itemChosen == 14)
			{
				if (drawGridToogle) drawGridToogle = false;
				else drawGridToogle = true;
			}

			// Reset View to Default
			if (itemChosen == 16)
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
			if (itemChosen == 18)
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
						pControl->Hide(true);
					}

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

	void PrintDeletedControls(IGraphics* pGraphics)
	{
		if (deleted_control_default_index.size() > 0)
		{
			int textSize = 15;
			IText txt(textSize, &EDIT_COLOR, defaultFont);
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

				text.SetFormatted(128, "%i (%s), ", j, default_layers[deleted_control_default_index[j]]->GetDerivedClassName());

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
		undo_viewMode.resize(viewSize);
		
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
		bool skipIfSame = true;

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
		
		// Check if previous undo is same as current
		for (int j = 0; j < pGraphics->GetNControls(); j++)
		{

			IControl* pControl = pGraphics->GetControl(j);
			IControl* tmp = default_layers[j];

			bool a, b, c, d, e;

			if (undo_viewMode[viewMode].undo_stack.size() > 0)
			{
				a = undo_viewMode[viewMode].undo_stack.back().pointers[j] == undo_current_stack[viewMode].pointers[j];
				b = undo_viewMode[viewMode].undo_stack.back().draw_rect[j] == undo_current_stack[viewMode].draw_rect[j];
				c = undo_viewMode[viewMode].undo_stack.back().target_rect[j] == undo_current_stack[viewMode].target_rect[j];
				d = undo_viewMode[viewMode].undo_stack.back().is_hidden[j] == undo_current_stack[viewMode].is_hidden[j];
				e = undo_viewMode[viewMode].undo_stack.back().deleted_controls.size() == undo_current_stack[viewMode].deleted_controls.size();

				if (!a || !b || !c || !d || !e)
				{
					skipIfSame = false;
					break;
				}
			}
		}

		if (skipIfSame && undo_viewMode[viewMode].undo_stack.size() > 0)
		{
			undo_viewMode[viewMode].undo_stack.pop_back();
			undo_viewMode[viewMode].undo_pos--;
		}
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
			deleted_control_default_index = undo_viewMode[viewMode].undo_stack[undoPos].deleted_controls;

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

private:
	// Live editing stuff
	char* defaultFont = "Tahoma";
	IColor EDIT_COLOR = IColor(255, 255, 255, 255);
	IColor controlTextBackgroundColor = IColor(122, 0, 0, 0);
	IRECT liveSelectedRECT = IRECT(0, 0, 0, 0);
	IRECT liveSelectedTargetRECT = IRECT(0, 0, 0, 0);
	IRECT liveClickedRECT = IRECT(0, 0, 0, 0);
	IRECT liveClickedTargetRECT = IRECT(0, 0, 0, 0);
	int liveControlNumber = -1;
	int lastliveMouseCapture = -1;
	int liveClickedX = 0, liveClickedY = 0;
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
	int currentViewMode = 0;
	int viewModeSize = 1;
	bool retrieveOldLayoutChanges = false;
	bool undoMove = true;
	bool lastWasUndo = false;
	bool lastWasRedo = false;
	
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
		int undo_pos = -1;
	};

	vector <undo> undo_current_stack;
	vector <undoStack> undo_viewMode;
};
#endif