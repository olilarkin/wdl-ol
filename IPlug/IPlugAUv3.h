#ifndef _IPLUGAPI_
#define _IPLUGAPI_
// Only load one API class!

#include <cstring>

#include <CoreAudio/CoreAudioTypes.h>

#include "wdlstring.h"

#include "IPlugBase_select.h"

/** Used to pass various instance info to the API class */
struct IPlugInstanceInfo
{
  WDL_String mBundleID;
};

union AURenderEvent;
struct AUMIDIEvent;

class IPlugAUv3 : public IPLUG_BASE_CLASS
{
public:
  IPlugAUv3(IPlugInstanceInfo instanceInfo, IPlugConfig config);

//  void startRamp(uint64_t address, float value, uint32_t duration);
//  void HandleMIDIEvent(AUMIDIEvent const& midiEvent) {};
  void ProcessWithEvents(AudioTimeStamp const* timestamp, uint32_t frameCount, AURenderEvent const* events);

  void SetParameter(uint64_t address, float value);
  float GetParameter(uint64_t address);
  const char* GetParamDisplayForHost(uint64_t address, float value);
  
  void SetBuffers(AudioBufferList* inBufferList, AudioBufferList* outBufferList);
  void SetTimeInfo(ITimeInfo& timeInfo);

  //IPlug
  void BeginInformHostOfParamChange(int idx) override {};
  void InformHostOfParamChange(int idx, double normalizedValue) override {};
  void EndInformHostOfParamChange(int idx) override {};
  void InformHostOfProgramChange() override {};
  
  void ResizeGraphics(int w, int h, double scale) override {}
  
protected:
  bool SendMidiMsg(IMidiMsg& msg) override { return false; }
  bool SendSysEx(ISysEx& msg) override { return false; }
  
private:
  void HandleOneEvent(AURenderEvent const* event);
  void PerformAllSimultaneousEvents(int64_t now, AURenderEvent const*& event);
};

IPlugAUv3* MakePlug();

#endif //_IPLUGAPI_