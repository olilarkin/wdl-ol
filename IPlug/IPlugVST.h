#ifndef _IPLUGAPI_
#define _IPLUGAPI_
// Only load one API class!

/**
 * @file
 * @copydoc IPlugVST
 */

#include "aeffectx.h"

#include "IPlugBase_select.h"

/** Used to pass various instance info to the API class */
struct IPlugInstanceInfo
{
  audioMasterCallback mVSTHostCallback;
};

/**  VST2.4 API base class for an IPlug plug-in, inherits from IPlugBase or IPlugBaseGraphics
*   @ingroup APIClasses
*/
class IPlugVST : public IPLUG_BASE_CLASS
{
public:
  IPlugVST(IPlugInstanceInfo instanceInfo, IPlugConfig config);

  void BeginInformHostOfParamChange(int idx) override;
  void InformHostOfParamChange(int idx, double normalizedValue) override;
  void EndInformHostOfParamChange(int idx) override;

  void InformHostOfProgramChange() override;

  int GetSamplePos() override;
  double GetTempo() override;
  void GetTimeSig(int& numerator, int& denominator) override;
  void GetTime(ITimeInfo& timeInfo) override;
  EHost GetHost() override;

  void ResizeGraphics(int w, int h, double scale) override;
  bool IsRenderingOffline() override;

  void OnGUICreated() override;

  audioMasterCallback mHostCallback;

protected:
  void HostSpecificInit() override;
  void SetLatency(int samples) override;
  bool SendMidiMsg(IMidiMsg& msg) override;
  bool SendSysEx(ISysEx& msg) override;
  audioMasterCallback GetHostCallback();

private:
  template <class SAMPLETYPE>
  void VSTPrepProcess(SAMPLETYPE** inputs, SAMPLETYPE** outputs, VstInt32 nFrames);

  ERect mEditRect;

  bool SendVSTEvent(VstEvent& event);
  bool SendVSTEvents(WDL_TypedBuf<VstEvent>* pEvents);

  VstSpeakerArrangement mInputSpkrArr, mOutputSpkrArr;

  bool mHostSpecificInitDone = false;

  enum { VSTEXT_NONE=0, VSTEXT_COCKOS, VSTEXT_COCOA }; // list of VST extensions supported by host
  int mHasVSTExtensions;

  IByteChunk mState;     // Persistent storage if the host asks for plugin state.
  IByteChunk mBankState; // Persistent storage if the host asks for bank state.

public:
  static VstIntPtr VSTCALLBACK VSTDispatcher(AEffect *pEffect, VstInt32 opCode, VstInt32 idx, VstIntPtr value, void *ptr, float opt);
  static void VSTCALLBACK VSTProcess(AEffect *pEffect, float **inputs, float **outputs, VstInt32 nFrames);  // Deprecated.
  static void VSTCALLBACK VSTProcessReplacing(AEffect *pEffect, float **inputs, float **outputs, VstInt32 nFrames);
  static void VSTCALLBACK VSTProcessDoubleReplacing(AEffect *pEffect, double **inputs, double **outputs, VstInt32 nFrames);
  static float VSTCALLBACK VSTGetParameter(AEffect *pEffect, VstInt32 idx);
  static void VSTCALLBACK VSTSetParameter(AEffect *pEffect, VstInt32 idx, float value);
  AEffect mAEffect;
};

IPlugVST* MakePlug();

#endif
