#include "IPlugAUv3.h"
#import <AudioToolbox/AudioToolbox.h>

IPlugAUv3::IPlugAUv3(IPlugInstanceInfo instanceInfo, IPlugConfig c)
: IPLUG_BASE_CLASS(c, kAPIAUv3)
{
  Trace(TRACELOC, "%s", c.effectName);
}

void IPlugAUv3::HandleOneEvent(AURenderEvent const *event)
{
  switch (event->head.eventType)
  {
//      TODO: audiounit parameter automation
//    case AURenderEventParameter:
//    case AURenderEventParameterRamp: {
//      AUParameterEvent const& paramEvent = event->parameter;
//
//      startRamp(paramEvent.parameterAddress, paramEvent.value, paramEvent.rampDurationSampleFrames);
//      break;
//    }
      
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

void IPlugAUv3::PerformAllSimultaneousEvents(AUEventSampleTime now, AURenderEvent const *&event) {
  do {
    HandleOneEvent(event);

    // Go to next event.
    event = event->head.next;
    
    // While event is not null and is simultaneous (or late).
  } while (event && event->head.eventSampleTime <= now);
}

void IPlugAUv3::ProcessWithEvents(AudioTimeStamp const *timestamp, uint32_t frameCount, AURenderEvent const *events) {

  AUEventSampleTime now = AUEventSampleTime(timestamp->mSampleTime);
  uint32_t framesRemaining = frameCount;
  AURenderEvent const *event = events;
  
  while (framesRemaining > 0) {
    // If there are no more events, we can process the entire remaining segment and exit.
    if (event == nullptr) {
      uint32_t const bufferOffset = frameCount - framesRemaining;
      Process(framesRemaining, bufferOffset);
      return;
    }

    // **** start late events late.
    auto timeZero = AUEventSampleTime(0);
    auto headEventTime = event->head.eventSampleTime;
    uint32_t const framesThisSegment = uint32_t(std::max(timeZero, headEventTime - now));
    
    // Compute everything before the next event.
    if (framesThisSegment > 0) {
      uint32_t const bufferOffset = frameCount - framesRemaining;
      Process(framesThisSegment, bufferOffset);
              
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

void IPlugAUv3::SetBuffers(AudioBufferList* pInBufList, AudioBufferList* pOutBufferList)
{
  SetInputChannelConnections(0, NInChannels(), false);
  SetOutputChannelConnections(0, NOutChannels(), false);

  //TODO: assumes 1 bus
//  int inputChanIdx = 0;
//  for(int i = 0; i < pInBufList->mNumberBuffers; i++)
//  {
//    AttachInputBuffers(inputChanIdx, pInBufList->mBuffers[i].mNumberChannels, (float**) &(pInBufList->mBuffers[i].mData), GetBlockSize());
//    inputChanIdx += pInBufList->mBuffers[i].mNumberChannels;
//  }
//
//  int outputChanIdx = 0;
//  for(int i = 0; i < pOutBufferList->mNumberBuffers; i++)
//  {
//    AttachOutputBuffers(outputChanIdx, pOutBufferList->mBuffers[i].mNumberChannels, (float**) &(pOutBufferList->mBuffers[i].mData));
//    outputChanIdx += pOutBufferList->mBuffers[i].mNumberChannels;
//  }
}

void IPlugAUv3::Process(uint32_t frameCount, uint32_t bufferOffset)
{
//  ProcessBuffers(0.f, frameCount);
}

void IPlugAUv3::SetTimeInfo(ITimeInfo& timeInfo)
{
  mTimeInfo = timeInfo;
}
