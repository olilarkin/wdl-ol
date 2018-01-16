#pragma once

#import <AudioToolbox/AudioToolbox.h>
#import <algorithm>

class IPlugDSPKernel
{
public:
  void process(AUAudioFrameCount frameCount, AUAudioFrameCount bufferOffset);
  void startRamp(AUParameterAddress address, AUValue value, AUAudioFrameCount duration);
  void handleMIDIEvent(AUMIDIEvent const& midiEvent) {};
  void processWithEvents(AudioTimeStamp const* timestamp, AUAudioFrameCount frameCount, AURenderEvent const* events);

  void init(int channelCount, double inSampleRate);
  void reset();
  void setParameter(AUParameterAddress address, AUValue value);
  AUValue getParameter(AUParameterAddress address);
  void setBuffers(AudioBufferList* inBufferList, AudioBufferList* outBufferList);
  
private:
  void handleOneEvent(AURenderEvent const* event);
  void performAllSimultaneousEvents(AUEventSampleTime now, AURenderEvent const*& event);
  
  AudioBufferList* inBufferListPtr = nullptr;
  AudioBufferList* outBufferListPtr = nullptr;
};

