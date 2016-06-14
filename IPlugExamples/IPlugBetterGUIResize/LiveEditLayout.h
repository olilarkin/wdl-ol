// Do not edit. All of this is generated automatically 
// Copyright Youlean 2016 
 
#include <vector>
#include "IGraphics.h" 
 
class LiveEditLayout 
{ 
public: 
	LiveEditLayout() {} 
 
	~LiveEditLayout() {} 
 
	void SetControlPositions(IGraphics* pGraphics) 
	{ 
        // class IBitmapControl
		pGraphics->GetControl(1)->Hide(false); 
		pGraphics->GetControl(1)->SetDrawRECT(IRECT(0, 0, 800, 800)); 
		pGraphics->GetControl(1)->SetTargetRECT(IRECT(0, 0, 800, 800)); 

        // class CustomControl
		pGraphics->GetControl(2)->Hide(false); 
		pGraphics->GetControl(2)->SetDrawRECT(IRECT(625, 0, 800, 800)); 
		pGraphics->GetControl(2)->SetTargetRECT(IRECT(625, 0, 800, 800)); 

        // class IKnobMultiControl
		pGraphics->GetControl(3)->Hide(false); 
		pGraphics->GetControl(3)->SetDrawRECT(IRECT(126, 72, 226, 172)); 
		pGraphics->GetControl(3)->SetTargetRECT(IRECT(126, 72, 226, 172)); 

        // class IKnobMultiControl
		pGraphics->GetControl(4)->Hide(false); 
		pGraphics->GetControl(4)->SetDrawRECT(IRECT(234, 72, 334, 172)); 
		pGraphics->GetControl(4)->SetTargetRECT(IRECT(234, 72, 334, 172)); 

        // class ITextControl
		pGraphics->GetControl(5)->Hide(false); 
		pGraphics->GetControl(5)->SetDrawRECT(IRECT(90, 540, 558, 576)); 
		pGraphics->GetControl(5)->SetTargetRECT(IRECT(90, 540, 558, 576)); 

        // class viewSelector
		pGraphics->GetControl(6)->Hide(false); 
		pGraphics->GetControl(6)->SetDrawRECT(IRECT(126, 198, 334, 231)); 
		pGraphics->GetControl(6)->SetTargetRECT(IRECT(126, 198, 334, 231)); 

        // class viewSelector
		pGraphics->GetControl(7)->Hide(false); 
		pGraphics->GetControl(7)->SetDrawRECT(IRECT(159, 252, 309, 282)); 
		pGraphics->GetControl(7)->SetTargetRECT(IRECT(159, 252, 309, 282)); 

        // class viewSelector
		pGraphics->GetControl(8)->Hide(false); 
		pGraphics->GetControl(8)->SetDrawRECT(IRECT(159, 306, 309, 336)); 
		pGraphics->GetControl(8)->SetTargetRECT(IRECT(159, 306, 309, 336)); 

        // class handleSelector
		pGraphics->GetControl(9)->Hide(false); 
		pGraphics->GetControl(9)->SetDrawRECT(IRECT(18, 378, 612, 720)); 
		pGraphics->GetControl(9)->SetTargetRECT(IRECT(18, 378, 612, 720)); 

	    // --------------------------------------------------------------------

	    // Reordering control layers
		std::vector <IControl*> pControl;
		for (int i = 0; i < pGraphics->GetNControls(); i++) 
			pControl.push_back(pGraphics->GetControl(i));

		pGraphics->ReplaceControl(0,pControl[0]); 
		pGraphics->ReplaceControl(1,pControl[1]); 
		pGraphics->ReplaceControl(2,pControl[2]); 
		pGraphics->ReplaceControl(3,pControl[3]); 
		pGraphics->ReplaceControl(4,pControl[4]); 
		pGraphics->ReplaceControl(9,pControl[5]); 
		pGraphics->ReplaceControl(5,pControl[6]); 
		pGraphics->ReplaceControl(6,pControl[7]); 
		pGraphics->ReplaceControl(7,pControl[8]); 
		pGraphics->ReplaceControl(8,pControl[9]); 
	}
}; 