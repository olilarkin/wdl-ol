#include "IPlugAUv3.h"
#import <AudioToolbox/AudioToolbox.h>

IPlugAUv3::IPlugAUv3(IPlugInstanceInfo instanceInfo, IPlugConfig c)
: IPLUG_BASE_CLASS(c, kAPIAUv3)
{
  Trace(TRACELOC, "%s", c.effectName);
}

void IPlugAUv3::HandleOneEvent(AURenderEvent const *event)
{
  TRACE_PROCESS;
  
  switch (event->head.eventType)
  {
//      TODO: audiounit parameter automation
    case AURenderEventParameter:
    case AURenderEventParameterRamp: {
//      AUParameterEvent const& paramEvent = event->parameter;
//
//      startRamp(paramEvent.parameterAddress, paramEvent.value, paramEvent.rampDurationSampleFrames);
      break;
    }
      
    case AURenderEventMIDI:
    {
      IMidiMsg msg;
      msg.mStatus = event->MIDI.data[0];
      msg.mData1 = event->MIDI.data[1];
      msg.mData2 = event->MIDI.data[2];
      msg.mOffset = (int) event->MIDI.eventSampleTime;
      ProcessMidiMsg(msg);
      break;
    }
    default:
      break;
  }
}

void IPlugAUv3::PerformAllSimultaneousEvents(AUEventSampleTime now, AURenderEvent const *&event)
{
  do {
    HandleOneEvent(event);

    // Go to next event.
    event = event->head.next;
    
    // While event is not null and is simultaneous (or late).
  } while (event && event->head.eventSampleTime <= now);
}

void IPlugAUv3::ProcessWithEvents(AudioTimeStamp const *timestamp, uint32_t frameCount, AURenderEvent const *events)
{
  AUEventSampleTime now = AUEventSampleTime(timestamp->mSampleTime);
  uint32_t framesRemaining = frameCount;
  AURenderEvent const *event = events;
  
  while (framesRemaining > 0) {
    // If there are no more events, we can process the entire remaining segment and exit.
    if (event == nullptr) {
//      uint32_t const bufferOffset = frameCount - framesRemaining;
      ProcessBuffers(0.f, framesRemaining); // what about bufferOffset

      return;
    }

    // **** start late events late.
    auto timeZero = AUEventSampleTime(0);
    auto headEventTime = event->head.eventSampleTime;
    uint32_t const framesThisSegment = uint32_t(std::max(timeZero, headEventTime - now));
    
    // Compute everything before the next event.
    if (framesThisSegment > 0)
    {
//      uint32_t const bufferOffset = frameCount - framesRemaining;
      ProcessBuffers(0.f, framesThisSegment); // what about bufferOffset

      // Advance frames.
      framesRemaining -= framesThisSegment;

      // Advance time.
      now += AUEventSampleTime(framesThisSegment);
    }
    
    PerformAllSimultaneousEvents(now, event);
  }
}

void IPlugAUv3::SetParameter(uint64_t address, float value)
{
  const int paramIdx = (int) address;
  
  WDL_MutexLock lock(&mParams_mutex);
  IParam* pParam = GetParam(paramIdx);
  pParam->Set((double) value);
  SetParameterInUIFromAPI(paramIdx, value, false);
  OnParamChange(paramIdx);
}

float IPlugAUv3::GetParameter(uint64_t address)
{
  const int paramIdx = (int) address;

  WDL_MutexLock lock(&mParams_mutex);
  return (float) GetParam(paramIdx)->Value();
}

const char* IPlugAUv3::GetParamDisplayForHost(uint64_t address, float value)
{
  const int paramIdx = (int) address;
  
  WDL_MutexLock lock(&mParams_mutex);
  GetParam(paramIdx)->GetDisplayForHost(value, false, mParamDisplayStr);
  return (const char*) mParamDisplayStr.Get();
}

//void IPlugAUv3::startRamp(uint64_t address, float value, uint32_t duration) {
//}

void IPlugAUv3::SetBuffers(AudioBufferList* pInBufList, AudioBufferList* pOutBufList)
{
  TRACE_PROCESS;

  SetInputChannelConnections(0, NInChannels(), false);
  SetOutputChannelConnections(0, NOutChannels(), false);

  int chanIdx = 0;
  for(int i = 0; i < pInBufList->mNumberBuffers; i++)
  {
    int nConnected = pInBufList->mBuffers[i].mNumberChannels;
    SetInputChannelConnections(chanIdx, nConnected, true);
    AttachInputBuffers(chanIdx, nConnected, (float**) &(pInBufList->mBuffers[i].mData), GetBlockSize());
    chanIdx += nConnected;
  }
  
  chanIdx = 0;
  for(int i = 0; i < pOutBufList->mNumberBuffers; i++)
  {
    int nConnected = pOutBufList->mBuffers[i].mNumberChannels;
    SetOutputChannelConnections(chanIdx, nConnected, true);
    AttachOutputBuffers(chanIdx, nConnected, (float**) &(pOutBufList->mBuffers[i].mData));
    chanIdx += nConnected;
  }
}

void IPlugAUv3::SetTimeInfo(ITimeInfo& timeInfo)
{
  mTimeInfo = timeInfo;
}
