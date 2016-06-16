// Do not edit. All of this is generated automatically 
// Copyright Youlean 2016 
 
#include <vector>
#include "IGraphics.h" 
#include "IPlugGUIResize.h" 

class LiveEditLayout 
{ 
public: 
	LiveEditLayout() {} 
 
	~LiveEditLayout() {} 
 
	void SetControlPositions(IGraphics* pGraphics) 
	{ 
	    // Backup original control pointers
		for (int i = 0; i < pGraphics->GetNControls(); i++) 
			originalPointers.push_back(pGraphics->GetControl(i));

	    // --------------------------------------------------------------------

        // class IBitmapControl
		pGraphics->GetControl(1)->Hide(false); 
		pGraphics->GetControl(1)->SetDrawRECT(IRECT(0, 0, 635, 635)); 
		pGraphics->GetControl(1)->SetTargetRECT(IRECT(0, 0, 635, 635)); 

        // class CustomControl
		pGraphics->GetControl(2)->Hide(false); 
		pGraphics->GetControl(2)->SetDrawRECT(IRECT(496, 0, 635, 635)); 
		pGraphics->GetControl(2)->SetTargetRECT(IRECT(496, 0, 635, 635)); 

        // class IKnobMultiControl
		pGraphics->GetControl(3)->Hide(false); 
		pGraphics->GetControl(3)->SetDrawRECT(IRECT(40, 40, 119, 119)); 
		pGraphics->GetControl(3)->SetTargetRECT(IRECT(40, 40, 119, 119)); 

        // class IKnobMultiControl
		pGraphics->GetControl(4)->Hide(false); 
		pGraphics->GetControl(4)->SetDrawRECT(IRECT(39, 357, 118, 437)); 
		pGraphics->GetControl(4)->SetTargetRECT(IRECT(39, 357, 118, 437)); 

        // class ITextControl
		pGraphics->GetControl(5)->Hide(false); 
		pGraphics->GetControl(5)->SetDrawRECT(IRECT(16, 604, 635, 635)); 
		pGraphics->GetControl(5)->SetTargetRECT(IRECT(16, 604, 635, 635)); 

        // class viewSelector
		pGraphics->GetControl(6)->Hide(false); 
		pGraphics->GetControl(6)->SetDrawRECT(IRECT(19, 158, 139, 182)); 
		pGraphics->GetControl(6)->SetTargetRECT(IRECT(19, 158, 139, 182)); 

        // class viewSelector
		pGraphics->GetControl(7)->Hide(false); 
		pGraphics->GetControl(7)->SetDrawRECT(IRECT(19, 198, 139, 222)); 
		pGraphics->GetControl(7)->SetTargetRECT(IRECT(19, 198, 139, 222)); 

        // class viewSelector
		pGraphics->GetControl(8)->Hide(false); 
		pGraphics->GetControl(8)->SetDrawRECT(IRECT(19, 239, 139, 262)); 
		pGraphics->GetControl(8)->SetTargetRECT(IRECT(19, 239, 139, 262)); 

        // class handleSelector
		pGraphics->GetControl(9)->Hide(false); 
		pGraphics->GetControl(9)->SetDrawRECT(IRECT(156, 128, 465, 327)); 
		pGraphics->GetControl(9)->SetTargetRECT(IRECT(156, 128, 465, 327)); 

	    // --------------------------------------------------------------------

	    // Reordering control layers
		pGraphics->ReplaceControl(0, originalPointers[0]); 
		pGraphics->ReplaceControl(1, originalPointers[1]); 
		pGraphics->ReplaceControl(2, originalPointers[2]); 
		pGraphics->ReplaceControl(3, originalPointers[3]); 
		pGraphics->ReplaceControl(4, originalPointers[4]); 
		pGraphics->ReplaceControl(5, originalPointers[5]); 
		pGraphics->ReplaceControl(6, originalPointers[6]); 
		pGraphics->ReplaceControl(7, originalPointers[7]); 
		pGraphics->ReplaceControl(8, originalPointers[8]); 
		pGraphics->ReplaceControl(9, originalPointers[9]); 
	}

	void SetGUIResizeLayout(IGraphics* pGraphics, IPlugGUIResize* pGUIResize)
	{
		IControl* pControl;

		// View Mode: (0) ------------------------------------------------------------------------------------------------
		pControl = pGraphics->GetControl(0); 
		pGUIResize->LiveEditSetLayout(0, pControl, IRECT(0, 0, 799, 799), IRECT(0, 0, 799, 799), false);
		pControl = pGraphics->GetControl(1); 
		pGUIResize->LiveEditSetLayout(0, pControl, IRECT(0, 0, 799, 799), IRECT(0, 0, 799, 799), false);
		pControl = pGraphics->GetControl(2); 
		pGUIResize->LiveEditSetLayout(0, pControl, IRECT(625, 0, 799, 799), IRECT(625, 0, 799, 799), false);
		pControl = pGraphics->GetControl(3); 
		pGUIResize->LiveEditSetLayout(0, pControl, IRECT(51, 51, 150, 150), IRECT(51, 51, 150, 150), false);
		pControl = pGraphics->GetControl(4); 
		pGUIResize->LiveEditSetLayout(0, pControl, IRECT(49, 450, 149, 551), IRECT(49, 450, 149, 551), false);
		pControl = pGraphics->GetControl(5); 
		pGUIResize->LiveEditSetLayout(0, pControl, IRECT(20, 760, 799, 799), IRECT(20, 760, 799, 799), false);
		pControl = pGraphics->GetControl(6); 
		pGUIResize->LiveEditSetLayout(0, pControl, IRECT(25, 200, 175, 230), IRECT(25, 200, 175, 230), false);
		pControl = pGraphics->GetControl(7); 
		pGUIResize->LiveEditSetLayout(0, pControl, IRECT(25, 250, 175, 280), IRECT(25, 250, 175, 280), false);
		pControl = pGraphics->GetControl(8); 
		pGUIResize->LiveEditSetLayout(0, pControl, IRECT(25, 301, 175, 330), IRECT(25, 301, 175, 330), false);
		pControl = pGraphics->GetControl(9); 
		pGUIResize->LiveEditSetLayout(0, pControl, IRECT(197, 162, 585, 412), IRECT(197, 162, 585, 412), false);
		// End (0) -------------------------------------------------------------------------------------------------------


		// View Mode: (1) ------------------------------------------------------------------------------------------------
		// End (1) -------------------------------------------------------------------------------------------------------


		// View Mode: (2) ------------------------------------------------------------------------------------------------
		// End (2) -------------------------------------------------------------------------------------------------------

    }

private:
	std::vector <IControl*> originalPointers;
}; 