#include "IPlugRetina.h"
#include "IPlug_include_in_plug_src.h"
#include "IControl.h"
#include "resource.h"

const int kNumPrograms = 1;

enum EParams
{
  kGain = 0,
  kNumParams
};

enum ELayout
{
  kWidth = GUI_WIDTH,
  kHeight = GUI_HEIGHT,

  kGainX = 100,
  kGainY = 100,
  kKnobFrames = 60
};

IPlugRetina::IPlugRetina(IPlugInstanceInfo instanceInfo)
  :	IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo), mGain(1.)
{
  TRACE;

  //arguments are: name, defaultVal, minVal, maxVal, step, label
  GetParam(kGain)->InitDouble("Gain", 50., 0., 100.0, 0.01, "%");
  GetParam(kGain)->SetShape(2.);

  IGraphics* pGraphics = MakeGraphics(this, kWidth, kHeight);
  CreateControls(pGraphics);
  AttachGraphics(pGraphics);

  //MakePreset("preset 1", ... );
  MakeDefaultPreset((char *) "-", kNumPrograms);
}

IPlugRetina::~IPlugRetina() {}

void IPlugRetina::ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames)
{
  // Mutex is already locked for us.

  double* in1 = inputs[0];
  double* in2 = inputs[1];
  double* out1 = outputs[0];
  double* out2 = outputs[1];

  for (int s = 0; s < nFrames; ++s, ++in1, ++in2, ++out1, ++out2)
  {
    *out1 = *in1 * mGain;
    *out2 = *in2 * mGain;
  }
}

void IPlugRetina::Reset()
{
  TRACE;
  IMutexLock lock(this);
}

void IPlugRetina::OnParamChange(int paramIdx)
{
  IMutexLock lock(this);

  switch (paramIdx)
  {
    case kGain:
      mGain = GetParam(kGain)->Value() / 100.;
      break;

    default:
      break;
  }
}

void IPlugRetina::OnWindowResize()
{
  CreateControls(GetGUI());
}

void IPlugRetina::CreateControls(IGraphics* pGraphics)
{
  double sf = pGraphics->GetScalingFactor();
  
  pGraphics->AttachPanelBackground(&COLOR_RED);
  
  IBitmap knob;
  
  if (sf == 2.)
  {
    printf("retina\n");
    knob = pGraphics->LoadIBitmap(knob2x_ID, knob2x_FN, kKnobFrames);
  }
  else
  {
    printf("not retina\n");
    knob = pGraphics->LoadIBitmap(knob_ID, knob_FN, kKnobFrames);
  }
  
  pGraphics->AttachControl(new IKnobMultiControl(this, kGainX*sf, kGainY*sf, kGain, &knob));
  
  RedrawParamControls();
}
