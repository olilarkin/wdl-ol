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

#include "IControl.h"
#include "IGraphics.h"

class IPlugGUILiveEdit
{
public:
	IPlugGUILiveEdit() {}
	~IPlugGUILiveEdit() {}

	void EditGUI(IPlugBase* pPlug, IGraphics* pGraphics, WDL_PtrList<IControl>* pControls, LICE_IBitmap* pDrawBitmap, 
		IMouseMod* liveEditingMod, int* liveGridSize, int* liveSnap, int* liveKeyDown, bool* liveToogleEditing, int* liveMouseCapture,
		int* mMouseX, int* mMouseY, int width, int height, double guiScaleRatio)
	{
		// Moving controls --------------------------------------------------------------------
		pGraphics->SetAllControlsDirty();

		if (pPlug->GetGUIResize() != NULL) liveScaledGridSize = int((double)*liveGridSize * guiScaleRatio);
		else liveScaledGridSize = *liveGridSize;


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
			// Draw control rects
			int controlSize = pControls->GetSize();

			if (pPlug->GetGUIResize() != NULL)
			{
				controlSize -= 3;
				if (*liveMouseCapture > controlSize) *liveMouseCapture = -1;
			}

			for (int j = 1; j < controlSize; j++)
			{
				IControl* pControl = pControls->Get(j);

				// If control is hidden cross it with lines
				if (pControl->IsHidden())
				{
					IRECT drawRECT = *pControl->GetRECT();
					WDL_String strHidden;
					strHidden.Set("Hidden");
					IText txtHidden(IPMIN(drawRECT.W() / 4, drawRECT.H()), &EDIT_COLOR);
					pGraphics->DrawIText(&txtHidden, strHidden.Get(), &drawRECT);

					pGraphics->DrawRect(&EDIT_COLOR, &drawRECT);
				}
				else
					pGraphics->DrawRect(&EDIT_COLOR, pControl->GetRECT());
			}

			WDL_String str;
			str.SetFormatted(32, "x: %i, y: %i", *mMouseX, *mMouseY);
			IText txt(20, &EDIT_COLOR);
			IRECT rect(width - 150, height - 20, width, height);
			pGraphics->DrawIText(&txt, str.Get(), &rect);

			// Draw resizing handles
			int liveHandleSize = 8;

			for (int j = 1; j < controlSize; j++)
			{
				IControl* pControl = pControls->Get(j);

				IRECT drawRECT = *pControl->GetRECT();
				IRECT handle = IRECT(drawRECT.R - liveHandleSize, drawRECT.B - liveHandleSize, drawRECT.R, drawRECT.B);
				pGraphics->FillTriangle(&EDIT_COLOR, handle.L, handle.B, handle.R, handle.B, handle.R, handle.T, 0);
			}

			bool overControlHandle = false;
			// Find if over control handle
			for (int j = 1; j < controlSize; j++)
			{
				IControl* pControl = pControls->Get(j);

				IRECT drawRECT = *pControl->GetRECT();
				IRECT handle = IRECT(drawRECT.R - liveHandleSize, drawRECT.B - liveHandleSize, drawRECT.R, drawRECT.B);

				if (drawRECT.Contains(*mMouseX, *mMouseY))
				{
					overControlHandle = handle.Contains(*mMouseX, *mMouseY);
				}
			}

			if (!liveEditingMod->S)
			{
				// Change cursor when over handle
				if (overControlHandle) SetCursor(LoadCursor(NULL, IDC_SIZENWSE));
				else SetCursor(LoadCursor(NULL, IDC_ARROW));
			}

			if (*liveMouseCapture == 0) liveEditingMod->L = false;

			if (liveEditingMod->L)
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


				// Prevent editing
				if (!liveEditingMod->S)
				{
					// Change cursor when clicked on handle
					if (liveClickedOnHandle || overControlHandle) SetCursor(LoadCursor(NULL, IDC_SIZENWSE));
					else SetCursor(LoadCursor(NULL, IDC_ARROW));


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
					if (!liveEditingMod->A && !liveEditingMod->S)
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

					pControl->SetDrawArea(drawArea);
					pControl->SetTargetArea(targetArea);

					liveSelectedRECT = drawArea;
				}
			}

			lastliveMouseCapture = *liveMouseCapture;
		}

		if (*liveToogleEditing)
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


			// Check if gui resize is active, if so scale out rect

			IRECT printRECT;
			if (pPlug->GetGUIResize() != NULL)
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
			WDL_String controlNumber;
			controlNumber.SetFormatted(100, "Control Pos: %i", liveControlNumber);
			IText txtControlNumber(17, &EDIT_COLOR);
			txtControlNumber.mAlign = IText::kAlignNear;
			IRECT rectControlNumber(4, 2, 150, 17);
			pGraphics->DrawIText(&txtControlNumber, controlNumber.Get(), &rectControlNumber);

			WDL_String controlPositionL;
			controlPositionL.SetFormatted(100, "L: %i, T: %i", printRECT.L, printRECT.T);
			IText txtControlPositionL(17, &EDIT_COLOR);
			txtControlPositionL.mAlign = IText::kAlignNear;
			IRECT rectControlPositionL(4, 20, 150, 37);
			pGraphics->DrawIText(&txtControlPositionL, controlPositionL.Get(), &rectControlPositionL);

			WDL_String controlPositionR;
			controlPositionR.SetFormatted(100, "R: %i, B: %i", printRECT.R, printRECT.B);
			IText txtControlPositionR(17, &EDIT_COLOR);
			txtControlPositionR.mAlign = IText::kAlignNear;
			IRECT rectControlPositionR(4, 38, 150, 55);
			pGraphics->DrawIText(&txtControlPositionR, controlPositionR.Get(), &rectControlPositionR);


			// Write to file all controls IRECTs
			BackupIRECTs(pPlug, pGraphics, "edited", guiScaleRatio);
		}
	}

	void BackupIRECTs(IPlugBase* pPlug, IGraphics* pGraphics, const char* category, double guiScaleRatio)
	{
		int controlSize = pGraphics->GetNControls();
		if (pPlug->GetGUIResize() != NULL) controlSize -= 3;
		
		for (int i = 1; i < controlSize; i++)
		{
			IControl* pControl = pGraphics->GetControl(i);
			IRECT drawRECT = *pControl->GetRECT();
			IRECT targetRECT = *pControl->GetTargetRECT();

			if (pPlug->GetGUIResize() != NULL)
			{
				drawRECT.L = int((double)drawRECT.L * guiScaleRatio);
				drawRECT.T = int((double)drawRECT.T * guiScaleRatio);
				drawRECT.R = int((double)drawRECT.R * guiScaleRatio);
				drawRECT.B = int((double)drawRECT.B * guiScaleRatio);

				targetRECT.L = int((double)targetRECT.L * guiScaleRatio);
				targetRECT.T = int((double)targetRECT.T * guiScaleRatio);
				targetRECT.R = int((double)targetRECT.R * guiScaleRatio);
				targetRECT.B = int((double)targetRECT.B * guiScaleRatio);
			}

			WDL_String IRECT_Position;
			WDL_String IRECT_Name;
			IRECT_Name.SetFormatted(128, "IRECT%i", i);
			
			// Set L
			WDL_String IRECT_NameDL;
			IRECT_NameDL.Set(IRECT_Name.Get(), 128);
			IRECT_NameDL.Append("_drawL");
			IRECT_Position.SetFormatted(128, "%i", drawRECT.L);
			SetIntToFile(category, IRECT_NameDL.Get(), IRECT_Position.Get());

			WDL_String IRECT_NameTL;
			IRECT_NameTL.Set(IRECT_Name.Get(), 128);
			IRECT_NameTL.Append("_targetL");
			IRECT_Position.SetFormatted(128, "%i", targetRECT.L);
			SetIntToFile(category, IRECT_NameTL.Get(), IRECT_Position.Get());

			// Set T
			WDL_String IRECT_NameDT;
			IRECT_NameDT.Set(IRECT_Name.Get(), 128);
			IRECT_NameDT.Append("_drawT");
			IRECT_Position.SetFormatted(128, "%i", drawRECT.T);
			SetIntToFile(category, IRECT_NameDT.Get(), IRECT_Position.Get());

			WDL_String IRECT_NameTT;
			IRECT_NameTT.Set(IRECT_Name.Get(), 128);
			IRECT_NameTT.Append("_targetT");
			IRECT_Position.SetFormatted(128, "%i", targetRECT.T);
			SetIntToFile(category, IRECT_NameTT.Get(), IRECT_Position.Get());

			// Set R
			WDL_String IRECT_NameDR;
			IRECT_NameDR.Set(IRECT_Name.Get(), 128);
			IRECT_NameDR.Append("_drawR");
			IRECT_Position.SetFormatted(128, "%i", drawRECT.R);
			SetIntToFile(category, IRECT_NameDR.Get(), IRECT_Position.Get());

			WDL_String IRECT_NameTR;
			IRECT_NameTR.Set(IRECT_Name.Get(), 128);
			IRECT_NameTR.Append("_targetR");
			IRECT_Position.SetFormatted(128, "%i", targetRECT.R);
			SetIntToFile(category, IRECT_NameTR.Get(), IRECT_Position.Get());

			// Set B
			WDL_String IRECT_NameDB;
			IRECT_NameDB.Set(IRECT_Name.Get(), 128);
			IRECT_NameDB.Append("_drawB");
			IRECT_Position.SetFormatted(128, "%i", drawRECT.B);
			SetIntToFile(category, IRECT_NameDB.Get(), IRECT_Position.Get());

			WDL_String IRECT_NameTB;
			IRECT_NameTB.Set(IRECT_Name.Get(), 128);
			IRECT_NameTB.Append("_targetB");
			IRECT_Position.SetFormatted(128, "%i", targetRECT.B);
			SetIntToFile(category, IRECT_NameTB.Get(), IRECT_Position.Get());
		}
	}

	void LoadIRECTsFromFile(IPlugBase* pPlug, IGraphics* pGraphics, const char* category)

	{
		int controlSize = pGraphics->GetNControls();
		if (pPlug->GetGUIResize() != NULL) controlSize -= 3;

		for (int i = 1; i < controlSize; i++)
		{
			IControl* pControl = pGraphics->GetControl(i);
			IRECT drawRECT;
			IRECT targetRECT;

			WDL_String IRECT_Name;
			IRECT_Name.SetFormatted(128, "IRECT%i", i);

			// Set L
			WDL_String IRECT_NameDL;
			IRECT_NameDL.Set(IRECT_Name.Get(), 128);
			IRECT_NameDL.Append("_drawL");
			drawRECT.L = GetIntFromFile(category, IRECT_NameDL.Get());

			WDL_String IRECT_NameTL;
			IRECT_NameTL.Set(IRECT_Name.Get(), 128);
			IRECT_NameTL.Append("_targetL");
			targetRECT.L = GetIntFromFile(category, IRECT_NameTL.Get());

			// Set T
			WDL_String IRECT_NameDT;
			IRECT_NameDT.Set(IRECT_Name.Get(), 128);
			IRECT_NameDT.Append("_drawT");
			drawRECT.T = GetIntFromFile(category, IRECT_NameDT.Get());

			WDL_String IRECT_NameTT;
			IRECT_NameTT.Set(IRECT_Name.Get(), 128);
			IRECT_NameTT.Append("_targetT");
			targetRECT.T = GetIntFromFile(category, IRECT_NameTT.Get());

			// Set R
			WDL_String IRECT_NameDR;
			IRECT_NameDR.Set(IRECT_Name.Get(), 128);
			IRECT_NameDR.Append("_drawR");
			drawRECT.R = GetIntFromFile(category, IRECT_NameDR.Get());

			WDL_String IRECT_NameTR;
			IRECT_NameTR.Set(IRECT_Name.Get(), 128);
			IRECT_NameTR.Append("_targetR");
			targetRECT.R = GetIntFromFile(category, IRECT_NameTR.Get());

			// Set B
			WDL_String IRECT_NameDB;
			IRECT_NameDB.Set(IRECT_Name.Get(), 128);
			IRECT_NameDB.Append("_drawB");
			drawRECT.B = GetIntFromFile(category, IRECT_NameDB.Get());

			WDL_String IRECT_NameTB;
			IRECT_NameTB.Set(IRECT_Name.Get(), 128);
			IRECT_NameTB.Append("_targetB");
			targetRECT.B = GetIntFromFile(category, IRECT_NameTB.Get());

			if (drawRECT.L >= 0 && drawRECT.T >= 0 && drawRECT.R >= 0 && drawRECT.B >= 0)
			{
				if (targetRECT.L >= 0 && targetRECT.T >= 0 && targetRECT.R >= 0 && targetRECT.B >= 0)
				{
					pControl->SetDrawArea(drawRECT);
					pControl->SetTargetArea(targetRECT);
				}
			}
		}
	}

	void SetIntToFile(const char * category, const char * variable_name, const char * variable_value)
	{
		WritePrivateProfileString(category, variable_name, variable_value, "C:/LiveOut.txt");
	}

	int GetIntFromFile(const char * category, const char * variable_name)
	{
		return GetPrivateProfileInt(category, variable_name, -1, "C:/LiveOut.txt");
	}


private:
	// Live editing stuff
	char buf[512]; // temp buffer for writing integers to profile strings
	IColor EDIT_COLOR = IColor(255, 0, 255, 0);
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
};