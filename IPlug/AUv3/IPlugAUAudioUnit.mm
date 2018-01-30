#import <AVFoundation/AVFoundation.h>
#include "BufferedAudioBus.hpp"

#import "IPlugAUAudioUnit.h"
#include "IPlugAUv3.h"

@interface IPlugAUAudioUnit ()

//@property AUAudioUnitBus* mOutputBus;
@property AUAudioUnitBusArray* mInputBusArray;
@property AUAudioUnitBusArray* mOutputBusArray;

@end

static AUAudioUnitPreset* NewAUPreset(NSInteger number, NSString* pName)
{
  AUAudioUnitPreset* pPreset = [AUAudioUnitPreset new];
  pPreset.number = number;
  pPreset.name = pName;
  return pPreset;
}

@implementation IPlugAUAudioUnit
{
  IPlugAUv3* mPlug;
  BufferedInputBus mInputBus;
  BufferedOutputBus mOutputBus;
//  BufferedInputBus mSideChainInputBus;
  NSArray<NSNumber*>* mChannelCapabilitiesArray;
  AUHostMusicalContextBlock mMusicalContext;
  AUHostTransportStateBlock mTransportContext;
  AUAudioUnitPreset* mCurrentPreset;
  NSArray<AUAudioUnitPreset*>* mPresets;
}

@synthesize parameterTree = _mParameterTree;
@synthesize factoryPresets = mPresets;

- (instancetype)initWithComponentDescription:(AudioComponentDescription)componentDescription
                                     options:(AudioComponentInstantiationOptions)options
                                       error:(NSError **)ppOutError {
  
  self = [super initWithComponentDescription:componentDescription
                                     options:options
                                       error:ppOutError];
  
  if (self == nil) { return nil; }
  
  // Create a DSP kernel to handle the signal processing.
  mPlug = MakePlug();
  
  assert(mPlug);
  
  NSMutableArray* pChannelCapabilities = [[NSMutableArray alloc] init];
  
  for (int i = 0; i < mPlug->NChannelIO(); i++)
  {
    int inputs, outputs;
    mPlug->GetChannelIO(i, inputs, outputs);
    [pChannelCapabilities addObject: [NSNumber numberWithInt:inputs]];
    [pChannelCapabilities addObject: [NSNumber numberWithInt:outputs]];
  }

  mChannelCapabilitiesArray = pChannelCapabilities;

  // Initialize a default format for the busses.
  AVAudioFormat* pInputBusFormat = nil;
  
  if(mPlug->NInChannels())
    pInputBusFormat = [[AVAudioFormat alloc] initStandardFormatWithSampleRate:DEFAULT_SAMPLE_RATE channels:mPlug->NInChannels()];
  
//  if(mPlug->HasSidechainInput())
//    AVAudioFormat* pSideChainInputBusFormat = nil;
  
  AVAudioFormat* pOutputBusFormat = [[AVAudioFormat alloc] initStandardFormatWithSampleRate:DEFAULT_SAMPLE_RATE channels:mPlug->NOutChannels()];
  
  NSMutableArray* pParametersToAddToTree = [[NSMutableArray alloc] init];
  
  for ( auto i = 0; i < mPlug->NParams(); i++)
  {
    IParam* pParam = mPlug->GetParam(i);
    
    AudioUnitParameterOptions options =
    //kAudioUnitParameterFlag_CFNameRelease
    //kAudioUnitParameterFlag_PlotHistory
    //kAudioUnitParameterFlag_MeterReadOnly
    //kAudioUnitParameterFlag_DisplayMask
    //kAudioUnitParameterFlag_DisplaySquareRoot
    //kAudioUnitParameterFlag_DisplaySquared
    //kAudioUnitParameterFlag_DisplayCubed
    //kAudioUnitParameterFlag_DisplayCubeRoot
    //kAudioUnitParameterFlag_DisplayExponential
    //kAudioUnitParameterFlag_HasClump
    //kAudioUnitParameterFlag_ValuesHaveStrings
    //kAudioUnitParameterFlag_DisplayLogarithmic
    kAudioUnitParameterFlag_IsHighResolution |
    //kAudioUnitParameterFlag_NonRealTime
    //kAudioUnitParameterFlag_CanRamp |
    //kAudioUnitParameterFlag_ExpertMode
    //kAudioUnitParameterFlag_HasCFNameString
    //kAudioUnitParameterFlag_IsGlobalMeta
    kAudioUnitParameterFlag_IsReadable ;
    
    if (pParam->GetCanAutomate())
    {
      options = options | kAudioUnitParameterFlag_IsWritable;
    }
    
    if (pParam->GetIsMeta())
    {
      options |= kAudioUnitParameterFlag_IsElementMeta;
    }
    
    AudioUnitParameterUnit unit;
    
    NSString* pUnitName = nil;
    
    switch (pParam->Type())
    {
      case IParam::kTypeBool:
        unit = kAudioUnitParameterUnit_Boolean;
        break;
      case IParam::kTypeEnum:
        //fall through
      case IParam::kTypeInt:
        unit = kAudioUnitParameterUnit_Indexed;
        break;
      default:
      {
        const char* label = pParam->GetLabelForHost();
        
        if (CSTR_NOT_EMPTY(label))
        {
          unit = kAudioUnitParameterUnit_CustomUnit;
          pUnitName = [NSString stringWithCString:label encoding:NSUTF8StringEncoding];
        }
        else
        {
          unit = kAudioUnitParameterUnit_Generic;
        }
      }
    }
    
    NSMutableArray* pValueStrings = nil;
    
    //TODO: pValueStrings

    //TODO:: make better identifier?
    AUParameter *pAUParam = [AUParameterTree createParameterWithIdentifier:    [NSString stringWithString:[NSString stringWithFormat:@"%d", i]]
                                                                         name: [NSString stringWithCString:pParam->GetNameForHost() encoding:NSUTF8StringEncoding]
                                                                      address: AUParameterAddress(i)
                                                                          min: pParam->GetMin()
                                                                          max: pParam->GetMax()
                                                                         unit: unit
                                                                     unitName: pUnitName
                                                                        flags: options
                                                                 valueStrings: pValueStrings
                                                          dependentParameters: nil];
    
    pAUParam.value = pParam->GetDefault();
    
    [pParametersToAddToTree addObject:pAUParam];

//    [pAUParam retain];
  }
  
  // Create factory preset array.
  NSMutableArray* pPresets = [[NSMutableArray alloc] init];

  for(auto i = 0; i < mPlug->NPresets(); i++)
  {
    [pPresets addObject:NewAUPreset(i, [NSString stringWithCString: mPlug->GetPresetName(i) encoding:NSUTF8StringEncoding])];
  }
  
  mPresets = pPresets;
  [mPresets retain];
//  [pPresets release];
  
  // Create the parameter tree.
  _mParameterTree = [[AUParameterTree createTreeWithChildren:pParametersToAddToTree] retain];

  // Create the input and output busses.
  
  if (!mPlug->IsInstrument())
    mInputBus.init(pInputBusFormat, mPlug->NInChannels());
  
  mOutputBus.init(pOutputBusFormat, mPlug->NOutChannels());
  
  if(pInputBusFormat)
    [pInputBusFormat release];
  
  [pOutputBusFormat release];
  
  // Create the input and output bus arrays.
  if(!mPlug->IsInstrument())
    _mInputBusArray  = [[[AUAudioUnitBusArray alloc] initWithAudioUnit:self busType:AUAudioUnitBusTypeInput busses: @[mInputBus.bus]] retain];
  
  _mOutputBusArray = [[[AUAudioUnitBusArray alloc] initWithAudioUnit:self busType:AUAudioUnitBusTypeOutput busses: @[mOutputBus.bus]] retain];
  
  // Make a local pointer to the kernel to avoid capturing self.
  __block IPlugAUv3* plug = mPlug;

  // implementorValueObserver is called when a parameter changes value.
  _mParameterTree.implementorValueObserver = ^(AUParameter *param, AUValue value) {
    plug->SetParameter(param.address, value);
  };

  // implementorValueProvider is called when the value needs to be refreshed.
  _mParameterTree.implementorValueProvider = ^(AUParameter *param) {
    return plug->GetParameter(param.address);
  };
  
  // A function to provide string representations of parameter values.
  _mParameterTree.implementorStringFromValueCallback = ^(AUParameter *param, const AUValue *__nullable valuePtr) {
    AUValue value = valuePtr == nil ? param.value : *valuePtr;
    return [NSString stringWithCString:plug->GetParamDisplayForHost(param.address, value) encoding:NSUTF8StringEncoding];
  };
  
  self.maximumFramesToRender = 512;
  
  self.currentPreset = mPresets.firstObject;
  
  return self;
}

-(void)dealloc
{
  [mPresets release];
  
  if(!mPlug->IsInstrument())
    [_mInputBusArray release];
  
  [_mOutputBusArray release];
  
  delete mPlug;
  
  [super dealloc];
}

#pragma mark - AUAudioUnit (Overrides)

- (AUAudioUnitBusArray *)inputBusses {
  return _mInputBusArray;
}

- (AUAudioUnitBusArray *)outputBusses {
  return _mOutputBusArray;
}

- (BOOL)allocateRenderResourcesAndReturnError:(NSError **)ppOutError {
  if (![super allocateRenderResourcesAndReturnError:ppOutError]) {
    return NO;
  }
  
  uint32_t reqNumInputChannels = mInputBus.bus.format.channelCount;  //requested # input channels
  uint32_t reqNumOutputChannels = mOutputBus.bus.format.channelCount;//requested # output channels
  
  // TODO: legal io doesn't consider sidechain inputs
  if (!mPlug->LegalIO(reqNumInputChannels, reqNumOutputChannels))
  {
    if (ppOutError)
      *ppOutError = [NSError errorWithDomain:NSOSStatusErrorDomain code:kAudioUnitErr_FailedInitialization userInfo:nil];
  
    // Notify superclass that initialization was not successful
    self.renderResourcesAllocated = NO;
    
    return NO;
  }
  
  if (self.musicalContextBlock)
    mMusicalContext = self.musicalContextBlock;
  else
    mMusicalContext = nil;
  
  if (self.transportStateBlock)
    mTransportContext = self.transportStateBlock;
  else
    mTransportContext = nil;
  
  if(mPlug->NInChannels())
    mInputBus.allocateRenderResources(self.maximumFramesToRender);
  
//  if(mPlug->HasSidechainInput())
//    mSidchainInputBuss.allocateRenderResources(self.maximumFramesToRender);
  
//  mOutputBus.allocateRenderResources(self.maximumFramesToRender);
  
  mPlug->SetBlockSize(self.maximumFramesToRender);
  mPlug->SetSampleRate(mOutputBus.bus.format.sampleRate);
  mPlug->OnReset();
  
  return YES;
}

- (void)deallocateRenderResources {
  
  if(mPlug->NInChannels())
    mInputBus.deallocateRenderResources();
  
//  if(mPlug->HasSidechainInput())
//    mSidchainInputBus.deallocateRenderResources();
  
//  mOutputBus.deallocateRenderResources();
  
  mMusicalContext = nullptr;
  mTransportContext = nullptr;
  
  [super deallocateRenderResources];
}

#pragma mark - AUAudioUnit (AUAudioUnitImplementation)

- (AUInternalRenderBlock)internalRenderBlock {

  __block IPlugAUv3* pPlug = mPlug;
  __block BufferedInputBus* input = &mInputBus;
//  __block BufferedOutputBus* output = &mOutputBus;

  return Block_copy(^AUAudioUnitStatus(AudioUnitRenderActionFlags *actionFlags,
                            const AudioTimeStamp       *timestamp,
                            AVAudioFrameCount           frameCount,
                            NSInteger                   outputBusNumber,
                            AudioBufferList            *outputData,
                            const AURenderEvent        *realtimeEventListHead,
                            AURenderPullInputBlock      pullInputBlock) {
    
    TRACE_PROCESS
    
    AudioUnitRenderActionFlags pullFlags = 0;

    AUAudioUnitStatus err = input->pullInput(&pullFlags, timestamp, frameCount, 0, pullInputBlock);

    if (err != 0) { return err; }
    
    AudioBufferList *inAudioBufferList = input->mutableAudioBufferList;
//    AudioBufferList *outAudioBufferList = output->mutableAudioBufferList;

//    output->prepareOutputBufferList(outputData, frameCount, true);
    
    AudioBufferList *outAudioBufferList = outputData;
    if (outAudioBufferList->mBuffers[0].mData == nullptr) {
      for (UInt32 i = 0; i < outAudioBufferList->mNumberBuffers; ++i) {
        outAudioBufferList->mBuffers[i].mData = inAudioBufferList->mBuffers[i].mData;
      }
    }
    
    pPlug->SetBuffers(inAudioBufferList, outAudioBufferList);
    
    ITimeInfo timeInfo;

    if(mMusicalContext)
    {
      Float64 tempo; Float64 ppqPos; double numerator; NSInteger denominator; double currentMeasureDownbeatPosition; NSInteger sampleOffsetToNextBeat;

      mMusicalContext(&tempo, &numerator, &denominator, &ppqPos, &sampleOffsetToNextBeat, &currentMeasureDownbeatPosition);
      
      timeInfo.mTempo = tempo;
      timeInfo.mPPQPos = ppqPos;
      timeInfo.mLastBar = currentMeasureDownbeatPosition; //TODO: is that correct?
      timeInfo.mNumerator = (int) numerator; //TODO: update ITimeInfo precision?
      timeInfo.mDenominator = (int) denominator; //TODO: update ITimeInfo precision?
    }
      
    if(mTransportContext)
    {
      double samplePos; double cycleStart; double cycleEnd; AUHostTransportStateFlags transportStateFlags;

      mTransportContext(&transportStateFlags, &samplePos, &cycleStart, &cycleEnd);
      
      timeInfo.mSamplePos = samplePos;
      timeInfo.mCycleStart = cycleStart;
      timeInfo.mCycleEnd = cycleEnd;
      timeInfo.mTransportIsRunning = transportStateFlags == AUHostTransportStateMoving || transportStateFlags == AUHostTransportStateRecording;
      timeInfo.mTransportLoopEnabled = transportStateFlags == AUHostTransportStateCycling;
    }

    pPlug->SetTimeInfo(timeInfo);
    pPlug->ProcessWithEvents(timestamp, frameCount, realtimeEventListHead);

    return noErr;
  }
  );
}

#pragma mark- AUAudioUnit (Optional Properties)

//- (AUAudioUnitPreset *)currentPreset
//{
//  if (mCurrentPreset.number >= 0)
//    return [mPresets objectAtIndex:mPlug->GetCurrentPresetIdx()];
//  else
//    return mCurrentPreset;
//}

//- (void)setCurrentPreset:(AUAudioUnitPreset *)currentPreset
//{
//  if (nil == currentPreset) { NSLog(@"nil passed to setCurrentPreset!"); return; }
//
//  if (currentPreset.number >= 0)
//  {
//    // factory preset
//    for (AUAudioUnitPreset* pFactoryPreset in mPresets)
//    {
//      if (currentPreset.number == pFactoryPreset.number)
//      {
//        mPlug->RestorePreset(int(pFactoryPreset.number));
//        mCurrentPreset = currentPreset;
//        break;
//      }
//    }
//  }
//  else if (currentPreset.name != nil)
//  {
//    mCurrentPreset = currentPreset;
//  }
//}

- (BOOL)canProcessInPlace {
  return NO;
}

- (NSArray<NSNumber*>*)channelCapabilities
{
  return mChannelCapabilitiesArray;
}

@end
