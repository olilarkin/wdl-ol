#ifndef __IPLUGRETINA__
#define __IPLUGRETINA__

#include "IPlug_include_in_plug_hdr.h"

class IPlugRetina : public IPlug
{
public:
  IPlugRetina(IPlugInstanceInfo instanceInfo);
  ~IPlugRetina();

  void Reset();
  void OnParamChange(int paramIdx);
  void ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames);

  void CreateControls(IGraphics* pGraphics);

  void OnWindowResize();

private:
  double mGain;
};

#endif
