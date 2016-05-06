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

enum viewSelector
{
	miniView,
	normalView
};

IPlugBetterGUIResize::IPlugBetterGUIResize(IPlugInstanceInfo instanceInfo)
  :	IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo)
{
  TRACE;

  pGraphics = MakeGraphics(this, GUI_WIDTH, GUI_HEIGHT);

  // You can now use bitmaps with higher resolution, so that when you resize interface up, everything will be nice
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

  pGraphics->AttachControl(new CustomControl(this, IRECT(625,500,750,700), IColor(255,0,0,100)));

  // GUI resize control must be the last one ----------------------------------------------------------------------------------
  customControl = pGraphics->AttachControl(pGUIResize = new IPlugGUIResize(this, pGraphics, GUI_WIDTH, GUI_HEIGHT, BUNDLE_NAME, 16, 16));
  pGUIResize->UsingBitmaps(true); // Use fast resizing or slow. You must call this if you are using bitmaps
  // --------------------------------------------------------------------------------------------------------------------------
  
  AttachGraphics(pGraphics);
  pGraphics->ShowControlBounds(true);

  //MakePreset("preset 1", ... );
  MakeDefaultPreset((char *) "-", kNumPrograms);

  SetGUILayout(0, 1.0, 1.0, 1.0);
}

IPlugBetterGUIResize::~IPlugBetterGUIResize() {}

void IPlugBetterGUIResize::OnGUIOpen()
{ 
	TRACE; 
	pGUIResize->ResizeAtGUIOpen();

}

void IPlugBetterGUIResize::SetGUILayout(int viewMode, double windowWidthRatio, double windowHeightRatio, double guiScaleRatio)
{
	// You can use switch instead, but in this way it is easier to visualize the changes
	// Use constructor to initialize all controls and then hide or move controls that you don't need for specific viewMode
	// If you want to change specific control layout, you need to implement it for every viewMode

	if (false) //(viewMode == 0)
	{
		pGUIResize->HideControl(grayKnob);
		pGUIResize->MoveControl(redKnob, 0, 0);
	}

	if (true) //(viewMode == 1)
	{
		pGUIResize->ShowControl(grayKnob);
		pGUIResize->MoveControl(redKnob, 200, 200);
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


