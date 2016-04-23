#ifndef __IPLUGBETTERGUIRESIZE__
#define __IPLUGBETTERGUIRESIZE__

#include "IPlug_include_in_plug_hdr.h"
#include "IPlugGUIResize.h"

class IPlugBetterGUIResize : public IPlug
{
public:
  IPlugBetterGUIResize(IPlugInstanceInfo instanceInfo);
  ~IPlugBetterGUIResize();

  void Reset();
  void OnParamChange(int paramIdx);
  void ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames);
  void OnGUIOpen();

private:
  IGraphics* pGraphics;
  IPlugGUIResize* pGUIResize;
};

#endif
