// Do not edit. All of this is generated automatically 
// Copyright Youlean 2016 
 
#include <vector>
#include "IGraphics.h" 
#include "IPlugGUIResize.h" 

class LiveEditSetLayout 
{ 
public: 
	LiveEditSetLayout() {} 
 
	~LiveEditSetLayout() {} 
 
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
		pGraphics->GetControl(3)->SetDrawRECT(IRECT(50, 50, 150, 150)); 
		pGraphics->GetControl(3)->SetTargetRECT(IRECT(50, 50, 150, 150)); 

        // class IKnobMultiControl
		pGraphics->GetControl(4)->Hide(false); 
		pGraphics->GetControl(4)->SetDrawRECT(IRECT(600, 200, 700, 300)); 
		pGraphics->GetControl(4)->SetTargetRECT(IRECT(600, 200, 700, 300)); 

        // class ITextControl
		pGraphics->GetControl(5)->Hide(false); 
		pGraphics->GetControl(5)->SetDrawRECT(IRECT(20, 760, 800, 800)); 
		pGraphics->GetControl(5)->SetTargetRECT(IRECT(20, 760, 800, 800)); 

        // class viewSelector
		pGraphics->GetControl(6)->Hide(false); 
		pGraphics->GetControl(6)->SetDrawRECT(IRECT(25, 200, 175, 230)); 
		pGraphics->GetControl(6)->SetTargetRECT(IRECT(25, 200, 175, 230)); 

        // class viewSelector
		pGraphics->GetControl(7)->Hide(false); 
		pGraphics->GetControl(7)->SetDrawRECT(IRECT(25, 250, 175, 280)); 
		pGraphics->GetControl(7)->SetTargetRECT(IRECT(25, 250, 175, 280)); 

        // class viewSelector
		pGraphics->GetControl(8)->Hide(false); 
		pGraphics->GetControl(8)->SetDrawRECT(IRECT(25, 300, 175, 330)); 
		pGraphics->GetControl(8)->SetTargetRECT(IRECT(25, 300, 175, 330)); 

        // class handleSelector
		pGraphics->GetControl(9)->Hide(false); 
		pGraphics->GetControl(9)->SetDrawRECT(IRECT(12, 350, 400, 600)); 
		pGraphics->GetControl(9)->SetTargetRECT(IRECT(12, 350, 400, 600)); 

	    // --------------------------------------------------------------------

	    // Reordering control layers
		for (int i = 0; i < pGraphics->GetNControls(); i++) 
			originalPointers.push_back(pGraphics->GetControl(i));

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
		IControl* pControl = pGraphics->GetControl(0); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(0, 0, 800, 800), IRECT(0, 0, 800, 800), false);

		IControl* pControl = pGraphics->GetControl(1); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(0, 0, 800, 800), IRECT(0, 0, 800, 800), false);

		IControl* pControl = pGraphics->GetControl(2); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(625, 0, 800, 800), IRECT(625, 0, 800, 800), false);

		IControl* pControl = pGraphics->GetControl(3); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(50, 50, 150, 150), IRECT(50, 50, 150, 150), false);

		IControl* pControl = pGraphics->GetControl(4); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(600, 200, 700, 300), IRECT(600, 200, 700, 300), false);

		IControl* pControl = pGraphics->GetControl(5); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(20, 760, 800, 800), IRECT(20, 760, 800, 800), false);

		IControl* pControl = pGraphics->GetControl(6); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(25, 200, 175, 230), IRECT(25, 200, 175, 230), false);

		IControl* pControl = pGraphics->GetControl(7); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(25, 250, 175, 280), IRECT(25, 250, 175, 280), false);

		IControl* pControl = pGraphics->GetControl(8); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(25, 300, 175, 330), IRECT(25, 300, 175, 330), false);

		IControl* pControl = pGraphics->GetControl(9); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(12, 350, 400, 600), IRECT(12, 350, 400, 600), false);

		IControl* pControl = pGraphics->GetControl(0); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(0, 0, 800, 800), IRECT(0, 0, 800, 800), false);

		IControl* pControl = pGraphics->GetControl(1); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(0, 0, 800, 800), IRECT(0, 0, 800, 800), false);

		IControl* pControl = pGraphics->GetControl(2); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(625, 0, 800, 800), IRECT(625, 0, 800, 800), false);

		IControl* pControl = pGraphics->GetControl(3); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(50, 50, 150, 150), IRECT(50, 50, 150, 150), false);

		IControl* pControl = pGraphics->GetControl(4); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(600, 200, 700, 300), IRECT(600, 200, 700, 300), false);

		IControl* pControl = pGraphics->GetControl(5); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(20, 760, 800, 800), IRECT(20, 760, 800, 800), false);

		IControl* pControl = pGraphics->GetControl(6); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(25, 200, 175, 230), IRECT(25, 200, 175, 230), false);

		IControl* pControl = pGraphics->GetControl(7); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(25, 250, 175, 280), IRECT(25, 250, 175, 280), false);

		IControl* pControl = pGraphics->GetControl(8); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(25, 300, 175, 330), IRECT(25, 300, 175, 330), false);

		IControl* pControl = pGraphics->GetControl(9); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(12, 350, 400, 600), IRECT(12, 350, 400, 600), false);

		IControl* pControl = pGraphics->GetControl(0); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(0, 0, 800, 800), IRECT(0, 0, 800, 800), false);

		IControl* pControl = pGraphics->GetControl(1); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(0, 0, 800, 800), IRECT(0, 0, 800, 800), false);

		IControl* pControl = pGraphics->GetControl(2); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(625, 0, 800, 800), IRECT(625, 0, 800, 800), false);

		IControl* pControl = pGraphics->GetControl(3); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(50, 50, 150, 150), IRECT(50, 50, 150, 150), false);

		IControl* pControl = pGraphics->GetControl(4); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(600, 200, 700, 300), IRECT(600, 200, 700, 300), false);

		IControl* pControl = pGraphics->GetControl(5); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(20, 760, 800, 800), IRECT(20, 760, 800, 800), false);

		IControl* pControl = pGraphics->GetControl(6); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(25, 200, 175, 230), IRECT(25, 200, 175, 230), false);

		IControl* pControl = pGraphics->GetControl(7); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(25, 250, 175, 280), IRECT(25, 250, 175, 280), false);

		IControl* pControl = pGraphics->GetControl(8); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(25, 300, 175, 330), IRECT(25, 300, 175, 330), false);

		IControl* pControl = pGraphics->GetControl(9); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(12, 350, 400, 600), IRECT(12, 350, 400, 600), false);

		IControl* pControl = pGraphics->GetControl(0); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(0, 0, 800, 800), IRECT(0, 0, 800, 800), false);

		IControl* pControl = pGraphics->GetControl(1); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(0, 0, 800, 800), IRECT(0, 0, 800, 800), false);

		IControl* pControl = pGraphics->GetControl(2); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(625, 0, 800, 800), IRECT(625, 0, 800, 800), false);

		IControl* pControl = pGraphics->GetControl(3); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(50, 50, 150, 150), IRECT(50, 50, 150, 150), false);

		IControl* pControl = pGraphics->GetControl(4); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(600, 200, 700, 300), IRECT(600, 200, 700, 300), false);

		IControl* pControl = pGraphics->GetControl(5); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(20, 760, 800, 800), IRECT(20, 760, 800, 800), false);

		IControl* pControl = pGraphics->GetControl(6); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(25, 200, 175, 230), IRECT(25, 200, 175, 230), false);

		IControl* pControl = pGraphics->GetControl(7); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(25, 250, 175, 280), IRECT(25, 250, 175, 280), false);

		IControl* pControl = pGraphics->GetControl(8); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(25, 300, 175, 330), IRECT(25, 300, 175, 330), false);

		IControl* pControl = pGraphics->GetControl(9); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(12, 350, 400, 600), IRECT(12, 350, 400, 600), false);

		IControl* pControl = pGraphics->GetControl(0); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(0, 0, 800, 800), IRECT(0, 0, 800, 800), false);

		IControl* pControl = pGraphics->GetControl(1); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(0, 0, 800, 800), IRECT(0, 0, 800, 800), false);

		IControl* pControl = pGraphics->GetControl(2); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(625, 0, 800, 800), IRECT(625, 0, 800, 800), false);

		IControl* pControl = pGraphics->GetControl(3); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(50, 50, 150, 150), IRECT(50, 50, 150, 150), false);

		IControl* pControl = pGraphics->GetControl(4); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(600, 200, 700, 300), IRECT(600, 200, 700, 300), false);

		IControl* pControl = pGraphics->GetControl(5); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(20, 760, 800, 800), IRECT(20, 760, 800, 800), false);

		IControl* pControl = pGraphics->GetControl(6); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(25, 200, 175, 230), IRECT(25, 200, 175, 230), false);

		IControl* pControl = pGraphics->GetControl(7); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(25, 250, 175, 280), IRECT(25, 250, 175, 280), false);

		IControl* pControl = pGraphics->GetControl(8); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(25, 300, 175, 330), IRECT(25, 300, 175, 330), false);

		IControl* pControl = pGraphics->GetControl(9); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(12, 350, 400, 600), IRECT(12, 350, 400, 600), false);

		IControl* pControl = pGraphics->GetControl(0); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(0, 0, 800, 800), IRECT(0, 0, 800, 800), false);

		IControl* pControl = pGraphics->GetControl(1); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(0, 0, 800, 800), IRECT(0, 0, 800, 800), false);

		IControl* pControl = pGraphics->GetControl(2); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(625, 0, 800, 800), IRECT(625, 0, 800, 800), false);

		IControl* pControl = pGraphics->GetControl(3); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(50, 50, 150, 150), IRECT(50, 50, 150, 150), false);

		IControl* pControl = pGraphics->GetControl(4); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(600, 200, 700, 300), IRECT(600, 200, 700, 300), false);

		IControl* pControl = pGraphics->GetControl(5); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(20, 760, 800, 800), IRECT(20, 760, 800, 800), false);

		IControl* pControl = pGraphics->GetControl(6); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(25, 200, 175, 230), IRECT(25, 200, 175, 230), false);

		IControl* pControl = pGraphics->GetControl(7); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(25, 250, 175, 280), IRECT(25, 250, 175, 280), false);

		IControl* pControl = pGraphics->GetControl(8); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(25, 300, 175, 330), IRECT(25, 300, 175, 330), false);

		IControl* pControl = pGraphics->GetControl(9); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(12, 350, 400, 600), IRECT(12, 350, 400, 600), false);

		IControl* pControl = pGraphics->GetControl(0); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(0, 0, 800, 800), IRECT(0, 0, 800, 800), false);

		IControl* pControl = pGraphics->GetControl(1); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(0, 0, 800, 800), IRECT(0, 0, 800, 800), false);

		IControl* pControl = pGraphics->GetControl(2); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(625, 0, 800, 800), IRECT(625, 0, 800, 800), false);

		IControl* pControl = pGraphics->GetControl(3); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(50, 50, 150, 150), IRECT(50, 50, 150, 150), false);

		IControl* pControl = pGraphics->GetControl(4); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(600, 200, 700, 300), IRECT(600, 200, 700, 300), false);

		IControl* pControl = pGraphics->GetControl(5); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(20, 760, 800, 800), IRECT(20, 760, 800, 800), false);

		IControl* pControl = pGraphics->GetControl(6); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(25, 200, 175, 230), IRECT(25, 200, 175, 230), false);

		IControl* pControl = pGraphics->GetControl(7); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(25, 250, 175, 280), IRECT(25, 250, 175, 280), false);

		IControl* pControl = pGraphics->GetControl(8); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(25, 300, 175, 330), IRECT(25, 300, 175, 330), false);

		IControl* pControl = pGraphics->GetControl(9); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(12, 350, 400, 600), IRECT(12, 350, 400, 600), false);

		IControl* pControl = pGraphics->GetControl(0); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(0, 0, 800, 800), IRECT(0, 0, 800, 800), false);

		IControl* pControl = pGraphics->GetControl(1); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(0, 0, 800, 800), IRECT(0, 0, 800, 800), false);

		IControl* pControl = pGraphics->GetControl(2); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(625, 0, 800, 800), IRECT(625, 0, 800, 800), false);

		IControl* pControl = pGraphics->GetControl(3); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(50, 50, 150, 150), IRECT(50, 50, 150, 150), false);

		IControl* pControl = pGraphics->GetControl(4); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(600, 200, 700, 300), IRECT(600, 200, 700, 300), false);

		IControl* pControl = pGraphics->GetControl(5); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(20, 760, 800, 800), IRECT(20, 760, 800, 800), false);

		IControl* pControl = pGraphics->GetControl(6); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(25, 200, 175, 230), IRECT(25, 200, 175, 230), false);

		IControl* pControl = pGraphics->GetControl(7); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(25, 250, 175, 280), IRECT(25, 250, 175, 280), false);

		IControl* pControl = pGraphics->GetControl(8); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(25, 300, 175, 330), IRECT(25, 300, 175, 330), false);

		IControl* pControl = pGraphics->GetControl(9); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(12, 350, 400, 600), IRECT(12, 350, 400, 600), false);

		IControl* pControl = pGraphics->GetControl(0); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(0, 0, 800, 800), IRECT(0, 0, 800, 800), false);

		IControl* pControl = pGraphics->GetControl(1); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(0, 0, 800, 800), IRECT(0, 0, 800, 800), false);

		IControl* pControl = pGraphics->GetControl(2); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(625, 0, 800, 800), IRECT(625, 0, 800, 800), false);

		IControl* pControl = pGraphics->GetControl(3); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(50, 50, 150, 150), IRECT(50, 50, 150, 150), false);

		IControl* pControl = pGraphics->GetControl(4); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(600, 200, 700, 300), IRECT(600, 200, 700, 300), false);

		IControl* pControl = pGraphics->GetControl(5); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(20, 760, 800, 800), IRECT(20, 760, 800, 800), false);

		IControl* pControl = pGraphics->GetControl(6); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(25, 200, 175, 230), IRECT(25, 200, 175, 230), false);

		IControl* pControl = pGraphics->GetControl(7); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(25, 250, 175, 280), IRECT(25, 250, 175, 280), false);

		IControl* pControl = pGraphics->GetControl(8); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(25, 300, 175, 330), IRECT(25, 300, 175, 330), false);

		IControl* pControl = pGraphics->GetControl(9); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(12, 350, 400, 600), IRECT(12, 350, 400, 600), false);

		IControl* pControl = pGraphics->GetControl(0); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(0, 0, 800, 800), IRECT(0, 0, 800, 800), false);

		IControl* pControl = pGraphics->GetControl(1); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(0, 0, 800, 800), IRECT(0, 0, 800, 800), false);

		IControl* pControl = pGraphics->GetControl(2); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(625, 0, 800, 800), IRECT(625, 0, 800, 800), false);

		IControl* pControl = pGraphics->GetControl(3); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(50, 50, 150, 150), IRECT(50, 50, 150, 150), false);

		IControl* pControl = pGraphics->GetControl(4); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(600, 200, 700, 300), IRECT(600, 200, 700, 300), false);

		IControl* pControl = pGraphics->GetControl(5); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(20, 760, 800, 800), IRECT(20, 760, 800, 800), false);

		IControl* pControl = pGraphics->GetControl(6); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(25, 200, 175, 230), IRECT(25, 200, 175, 230), false);

		IControl* pControl = pGraphics->GetControl(7); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(25, 250, 175, 280), IRECT(25, 250, 175, 280), false);

		IControl* pControl = pGraphics->GetControl(8); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(25, 300, 175, 330), IRECT(25, 300, 175, 330), false);

		IControl* pControl = pGraphics->GetControl(9); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(12, 350, 400, 600), IRECT(12, 350, 400, 600), false);

		IControl* pControl = pGraphics->GetControl(0); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(0, 0, 800, 800), IRECT(0, 0, 800, 800), false);

		IControl* pControl = pGraphics->GetControl(1); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(0, 0, 800, 800), IRECT(0, 0, 800, 800), false);

		IControl* pControl = pGraphics->GetControl(2); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(625, 0, 800, 800), IRECT(625, 0, 800, 800), false);

		IControl* pControl = pGraphics->GetControl(3); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(50, 50, 150, 150), IRECT(50, 50, 150, 150), false);

		IControl* pControl = pGraphics->GetControl(4); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(600, 200, 700, 300), IRECT(600, 200, 700, 300), false);

		IControl* pControl = pGraphics->GetControl(5); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(20, 760, 800, 800), IRECT(20, 760, 800, 800), false);

		IControl* pControl = pGraphics->GetControl(6); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(25, 200, 175, 230), IRECT(25, 200, 175, 230), false);

		IControl* pControl = pGraphics->GetControl(7); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(25, 250, 175, 280), IRECT(25, 250, 175, 280), false);

		IControl* pControl = pGraphics->GetControl(8); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(25, 300, 175, 330), IRECT(25, 300, 175, 330), false);

		IControl* pControl = pGraphics->GetControl(9); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(12, 350, 400, 600), IRECT(12, 350, 400, 600), false);

		IControl* pControl = pGraphics->GetControl(0); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(0, 0, 800, 800), IRECT(0, 0, 800, 800), false);

		IControl* pControl = pGraphics->GetControl(1); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(0, 0, 800, 800), IRECT(0, 0, 800, 800), false);

		IControl* pControl = pGraphics->GetControl(2); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(625, 0, 800, 800), IRECT(625, 0, 800, 800), false);

		IControl* pControl = pGraphics->GetControl(3); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(50, 50, 150, 150), IRECT(50, 50, 150, 150), false);

		IControl* pControl = pGraphics->GetControl(4); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(600, 200, 700, 300), IRECT(600, 200, 700, 300), false);

		IControl* pControl = pGraphics->GetControl(5); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(20, 760, 800, 800), IRECT(20, 760, 800, 800), false);

		IControl* pControl = pGraphics->GetControl(6); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(25, 200, 175, 230), IRECT(25, 200, 175, 230), false);

		IControl* pControl = pGraphics->GetControl(7); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(25, 250, 175, 280), IRECT(25, 250, 175, 280), false);

		IControl* pControl = pGraphics->GetControl(8); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(25, 300, 175, 330), IRECT(25, 300, 175, 330), false);

		IControl* pControl = pGraphics->GetControl(9); 
		pGUIResize->LiveEditSetLayout(1, pControl, IRECT(12, 350, 400, 600), IRECT(12, 350, 400, 600), false);

} private:
	std::vector <IControl*> originalPointers;
}; 