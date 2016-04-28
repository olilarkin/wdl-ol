#include "IPlugBetterGUIResize.h"
#include "IPlug_include_in_plug_src.h"
#include "IControl.h"
#include "resource.h"

const int kNumPrograms = 2;

enum EParams
{
  kGain = 0,
  kGain1 = 1,
  kNumParams
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
  pGraphics->AttachControl(new IBitmapControl(this, 0, 0, tube));
  
  //arguments are: name, defaultVal, minVal, maxVal, step, label
  GetParam(kGain)->InitDouble("Gain", 50., 0., 100.0, 0.01, "%");
  GetParam(kGain)->SetShape(2.);
  IBitmap *knob = pGraphics->LoadPointerToBitmap(KNOB_ID, KNOB_FN, 60);
  pGraphics->AttachControl(new IKnobMultiControl(this, 50, 50, kGain, knob));

  //arguments are: name, defaultVal, minVal, maxVal, step, label
  GetParam(kGain1)->InitDouble("Gain1", 50., 0., 100.0, 0.01, "%");
  GetParam(kGain1)->SetShape(2.);
  IBitmap *knob1 = pGraphics->LoadPointerToBitmap(KNOB1_ID, KNOB1_FN, 60, true);
  pGraphics->AttachControl(new IKnobMultiControl(this, 600, 200, kGain1, knob1));

  IRECT tmpRect3(20, 760, 800, 800);
  IText textProps3(24, &COLOR_WHITE, "Arial", IText::kStyleItalic, IText::kAlignNear, 0, IText::kQualityDefault);
  pGraphics->AttachControl(new ITextControl(this, tmpRect3, &textProps3, "This is IPlugGUIResize example by Youlean..."));

  pGraphics->AttachControl(new CustomControl(this, IRECT(625,500,750,700), IColor(255,0,0,100)));

  // GUI resize control must be the last one ----------------------------------------------------------------------------------
  pGraphics->AttachControl(pGUIResize = new IPlugGUIResize(this, pGraphics, GUI_WIDTH, GUI_HEIGHT, BUNDLE_NAME, 16, 16));
  pGUIResize->UsingBitmaps(true); // Use fast resizing or slow. You must call this if you are using bitmaps
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


