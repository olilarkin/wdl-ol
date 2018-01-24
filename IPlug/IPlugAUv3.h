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
  WDL_String mOSXBundleID;
};

union AURenderEvent;
struct AUMIDIEvent;

class IPlugAUv3 : public IPLUG_BASE_CLASS
{
public:
  IPlugAUv3(IPlugInstanceInfo instanceInfo, IPlugConfig config);

  void process(uint32_t frameCount, uint32_t bufferOffset);
  void startRamp(uint64_t address, float value, uint32_t duration);
  void handleMIDIEvent(AUMIDIEvent const& midiEvent) {};
  void processWithEvents(AudioTimeStamp const* timestamp, uint32_t frameCount, AURenderEvent const* events);

  void setParameter(uint64_t address, float value);
  float getParameter(uint64_t address);
  void setBuffers(AudioBufferList* inBufferList, AudioBufferList* outBufferList);
  
  //IPlug
  void BeginInformHostOfParamChange(int idx) override {};
  void InformHostOfParamChange(int idx, double normalizedValue) override {};
  void EndInformHostOfParamChange(int idx) override {};
  void InformHostOfProgramChange() override {};
  
  int GetSamplePos() override { return 0; }
  double GetTempo() override { return DEFAULT_TEMPO; }
  void GetTimeSig(int& numerator, int& denominator) override { return; }
  void GetTime(ITimeInfo& timeInfo) override { return; }
  
  void ResizeGraphics(int w, int h, double scale) override {}
  
protected:
  bool SendMidiMsg(IMidiMsg& msg) override { return false; }
  bool SendSysEx(ISysEx& msg) override { return false; }
  
  
private:
  void handleOneEvent(AURenderEvent const* event);
  void performAllSimultaneousEvents(int64_t now, AURenderEvent const*& event);
  
  AudioBufferList* mInBufferList = nullptr;
  AudioBufferList* mOutBufferList = nullptr;
};

IPlugAUv3* MakePlug();

#endif //_IPLUGAPI_
