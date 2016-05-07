#include "IPlugBetterGUIResize.h"
#include "IPlug_include_in_plug_src.h"
#include "IControl.h"
#include "resource.h"

const int kNumPrograms = 2;

enum EParams
{
	// This is reserved for GUI resize
	viewMode = 0,
	windowWidth = 1,
	windowHeight = 2,
	// -------------------------------

	// Your custom parameters:
	kGain = 3,
	kGain1 = 4,

	kNumParams
};

// Adding names for views. Default view is needed, place it on top
enum viewSets
{
	defaultView, // Default view will always be at 0

	// Add here your custom views
	miniView,
	hugeView
};

IPlugBetterGUIResize::IPlugBetterGUIResize(IPlugInstanceInfo instanceInfo)
  :	IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo)
{
  TRACE;

  pGraphics = MakeGraphics(this, GUI_WIDTH, GUI_HEIGHT);

  // Here we are creating our GUI resize control ------------------------------------------------------------------------------
  // It is important to create on top of all controls because we might use its pointer to call different sizes from other controls
  pGUIResize = new IPlugGUIResize(this, pGraphics, GUI_WIDTH, GUI_HEIGHT, BUNDLE_NAME, 16, 16);

  // Use fast resizing or slow. You must call this if you are using bitmaps
  pGUIResize->UsingBitmaps(false);

  // Adding new view. Default view will always be 0.
  pGUIResize->AddNewView(miniView, 300, 300);
  pGUIResize->AddNewView(hugeView, 800, 800);

  pGUIResize->SelectViewMode(miniView);
  // --------------------------------------------------------------------------------------------------------------------------


  // You can now use bitmaps with higher resolution, so that when you resize interface up, everything will be nice
  // This must be called before LoadPointerToBitmap
  pGraphics->SetBitmapOversample(1);


  
  // Your custom controls------------------------------------------------------------------------------------------------------
  pGraphics->AttachPanelBackground(&COLOR_BLACK);

  IBitmap *tube = pGraphics->LoadPointerToBitmap(BACKGROUND_ID, BACKGROUND_FN);
  background = pGraphics->AttachControl(new IBitmapControl(this, 0, 0, tube));
  
  //arguments are: name, defaultVal, minVal, maxVal, step, label
  GetParam(kGain)->InitDouble("Gain", 50., 0., 100.0, 0.01, "%");
  GetParam(kGain)->SetShape(2.);
  IBitmap *knob = pGraphics->LoadPointerToBitmap(KNOB_ID, KNOB_FN, 60);
  redKnob = pGraphics->AttachControl(new IKnobMultiControl(this, 50, 50, kGain, knob));

  //arguments are: name, defaultVal, minVal, maxVal, step, label
  GetParam(kGain1)->InitDouble("Gain1", 50., 0., 100.0, 0.01, "%");
  GetParam(kGain1)->SetShape(2.);
  IBitmap *knob1 = pGraphics->LoadPointerToBitmap(KNOB1_ID, KNOB1_FN, 60, true);
  grayKnob = pGraphics->AttachControl(new IKnobMultiControl(this, 600, 200, kGain1, knob1));

  IRECT tmpRect3(20, 760, 800, 800);
  IText textProps3(24, &COLOR_WHITE, "Arial", IText::kStyleItalic, IText::kAlignNear, 0, IText::kQualityDefault);
  infoText = pGraphics->AttachControl(new ITextControl(this, tmpRect3, &textProps3, "This is IPlugGUIResize example by Youlean..."));

  customControl = pGraphics->AttachControl(new CustomControl(this, IRECT(625,500,750,700), IColor(255,0,0,100)));

  pGraphics->AttachControl(new viewSelector(this, IRECT(0, 0 + 200, 150, 30 + 200), "miniView", pGUIResize, miniView));
  pGraphics->AttachControl(new viewSelector(this, IRECT(0, 50 + 200, 150, 80 + 200), "defaultView", pGUIResize, defaultView));
  pGraphics->AttachControl(new viewSelector(this, IRECT(0, 100 + 200, 150, 130 + 200), "hugeView", pGUIResize, hugeView));


  // Here we are attaching our GUI resize control. This must be the last control ----------------------------------------------
  pGraphics->AttachControl(pGUIResize->Attach(pGraphics));
  // --------------------------------------------------------------------------------------------------------------------------
  

  AttachGraphics(pGraphics);
  pGraphics->ShowControlBounds(true);

  //MakePreset("preset 1", ... );
  MakeDefaultPreset((char *) "-", kNumPrograms);
}

IPlugBetterGUIResize::~IPlugBetterGUIResize() {}

void IPlugBetterGUIResize::OnGUIOpen()
{ 
	TRACE; 
	pGUIResize->ResizeAtGUIOpen();

}

void IPlugBetterGUIResize::SetGUILayout(int viewMode, double windowWidth, double windowHeight)
{
	// You can use switch instead, but in this way it is easier to visualize the changes
	// Use constructor to initialize all controls and then hide or move controls that you don't need for specific viewMode
	// If you want to change specific control layout, you need to implement it for every viewMode

	// Use this function to move, hide, show and resize controls. Don't worry about GUI Scale, this will be handled automatically

	if (viewMode == defaultView)
	{
		pGUIResize->ShowControl(grayKnob);
		pGUIResize->MoveControl(redKnob, 50.0, 50.0);
	}

	if (viewMode == miniView)
	{
		pGUIResize->HideControl(grayKnob);
		pGUIResize->MoveControl(redKnob, 0.0, 0.0);
	}

	if (viewMode == hugeView)
	{
		pGUIResize->ShowControl(grayKnob);
		pGUIResize->MoveControl(redKnob, windowWidth - 100.0, 0.0);
	}

}


void IPlugBetterGUIResize::ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames)
{
  // Mutex is already locked for us.

  double* in1 = inputs[0];
  double* in2 = inputs[1];
  double* out1 = outputs[0];
  double* out2 = outputs[1];

  for (int s = 0; s < nFrames; ++s, ++in1, ++in2, ++out1, ++out2)
  {
    *out1 = *in1;
    *out2 = *in2;
  }
}

void IPlugBetterGUIResize::Reset()
{
  TRACE;
  IMutexLock lock(this);

}

void IPlugBetterGUIResize::OnParamChange(int paramIdx)
{
  IMutexLock lock(this);

  switch (paramIdx)
  {
    case kGain:
      //GetParam(kGain)->Value() / 100.;
      break;

	case kGain1:
	  //GetParam(kGain1)->Value() / 100.;
	  break;

    default:
      break;
  }
}


