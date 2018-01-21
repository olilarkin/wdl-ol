#include "IPlugAUv3.h"
#import <AudioToolbox/AudioToolbox.h>

IPlugAUv3::IPlugAUv3(IPlugInstanceInfo instanceInfo, IPlugConfig c)
: IPLUG_BASE_CLASS(c, kAPIAUv3)
{
}

void IPlugAUv3::handleOneEvent(AURenderEvent const *event) {
  switch (event->head.eventType) {
    case AURenderEventParameter:
    case AURenderEventParameterRamp: {
      AUParameterEvent const& paramEvent = event->parameter;
      
      startRamp(paramEvent.parameterAddress, paramEvent.value, paramEvent.rampDurationSampleFrames);
      break;
    }
      
    case AURenderEventMIDI:
      handleMIDIEvent(event->MIDI);
      break;
    
    default:
      break;
  }
}

void IPlugAUv3::performAllSimultaneousEvents(AUEventSampleTime now, AURenderEvent const *&event) {
  do {
    handleOneEvent(event);

    // Go to next event.
    event = event->head.next;
    
    // While event is not null and is simultaneous (or late).
  } while (event && event->head.eventSampleTime <= now);
}

void IPlugAUv3::processWithEvents(AudioTimeStamp const *timestamp, uint32_t frameCount, AURenderEvent const *events) {

  AUEventSampleTime now = AUEventSampleTime(timestamp->mSampleTime);
  uint32_t framesRemaining = frameCount;
  AURenderEvent const *event = events;
  
  while (framesRemaining > 0) {
    // If there are no more events, we can process the entire remaining segment and exit.
    if (event == nullptr) {
      uint32_t const bufferOffset = frameCount - framesRemaining;
      process(framesRemaining, bufferOffset);
      return;
    }

    // **** start late events late.
    auto timeZero = AUEventSampleTime(0);
    auto headEventTime = event->head.eventSampleTime;
    uint32_t const framesThisSegment = uint32_t(std::max(timeZero, headEventTime - now));
    
    // Compute everything before the next event.
    if (framesThisSegment > 0) {
      uint32_t const bufferOffset = frameCount - framesRemaining;
      process(framesThisSegment, bufferOffset);
              
      // Advance frames.
      framesRemaining -= framesThisSegment;

      // Advance time.
      now += AUEventSampleTime(framesThisSegment);
    }
    
    performAllSimultaneousEvents(now, event);
  }
}


void IPlugAUv3::setParameter(uint64_t address, float value) {
}

float IPlugAUv3::getParameter(uint64_t address) {
  return 0.0;
}

void IPlugAUv3::startRamp(uint64_t address, float value, uint32_t duration) {
}

void IPlugAUv3::setBuffers(AudioBufferList* inBufferList, AudioBufferList* outBufferList) {
  mInBufferList = inBufferList;
  mOutBufferList = outBufferList;
}

void IPlugAUv3::process(uint32_t frameCount, uint32_t bufferOffset) {
//  int channelCount = mNumChannels;
//
//  for (int frameIndex = 0; frameIndex < frameCount; ++frameIndex) {
//
//    int frameOffset = int(frameIndex + bufferOffset);
//
//    for (int channel = 0; channel < channelCount; ++channel) {
//      float* input  = (float*) mInBufferList->mBuffers[channel].mData  + frameOffset;
//      float* output = (float*) mOutBufferList->mBuffers[channel].mData + frameOffset;
//
//      *output = *input * 0.5f;
//    }
//  }
}


