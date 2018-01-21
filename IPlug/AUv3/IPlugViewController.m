#import <CoreAudioKit/AUViewController.h>
#import "IPlugAUAudioUnit.h"

@interface IPlugViewController : AUViewController
@property (nonatomic)IPlugAUAudioUnit* audioUnit;
@end

@implementation IPlugViewController
  AUParameterObserverToken parameterObserverToken;
@end

@interface IPlugViewController (AUAudioUnitFactory) <AUAudioUnitFactory>
@end

@implementation IPlugViewController (AUAudioUnitFactory)

- (IPlugAUAudioUnit *) createAudioUnitWithComponentDescription:(AudioComponentDescription) desc error:(NSError **)error {
  self.audioUnit = [[IPlugAUAudioUnit alloc] initWithComponentDescription:desc error:error];

  // Check if the UI has been loaded
  if(self.isViewLoaded) {
    [self connectUIToAudioUnit];
  }

  return self.audioUnit;
}

- (void) viewDidLoad {
  [super viewDidLoad];
  
  // Check if the Audio Unit has been loaded
  if(self.audioUnit) {
    [self connectUIToAudioUnit];
  }
}

- (void)connectUIToAudioUnit {
  // Get the parameter tree and add observers for any parameters that the UI needs to keep in sync with the Audio Unit
}

- (IPlugAUAudioUnit *)getAudioUnit {
  return _audioUnit;
}

- (void)setAudioUnit:(IPlugAUAudioUnit*) audioUnit {
  _audioUnit = audioUnit;
  dispatch_async(dispatch_get_main_queue(), ^{
    if ([self isViewLoaded]) {
      [self connectViewWithAU];
    }
  });
}

- (void)observeValueForKeyPath:(NSString *)keyPath
                      ofObject:(id)object
                        change:(NSDictionary<NSString *, id> *)change
                       context:(void *)context
{
  NSLog(@"IPlugViewController allParameterValues key path changed: %s\n", keyPath.UTF8String);
  
  dispatch_async(dispatch_get_main_queue(), ^{
    
//    filterView.frequency = cutoffParameter.value;
//    filterView.resonance = resonanceParameter.value;
//
//    frequencyLabel.stringValue = [cutoffParameter stringFromValue: nil];
//    resonanceLabel.stringValue = [resonanceParameter stringFromValue: nil];
//
//    [self updateFilterViewFrequencyAndMagnitudes];
  });
}

- (void)connectViewWithAU {
  AUParameterTree *paramTree = _audioUnit.parameterTree;
  
  if (paramTree) {
//    cutoffParameter = [paramTree valueForKey: @"cutoff"];
//    resonanceParameter = [paramTree valueForKey: @"resonance"];
//
//    // prevent retain cycle in parameter observer
//    __weak FilterDemoViewController *weakSelf = self;
//    __weak AUParameter *weakCutoffParameter = cutoffParameter;
//    __weak AUParameter *weakResonanceParameter = resonanceParameter;
//    parameterObserverToken = [paramTree tokenByAddingParameterObserver:^(AUParameterAddress address, AUValue value) {
//      __strong FilterDemoViewController *strongSelf = weakSelf;
//      __strong AUParameter *strongCutoffParameter = weakCutoffParameter;
//      __strong AUParameter *strongResonanceParameter = weakResonanceParameter;
//
//      dispatch_async(dispatch_get_main_queue(), ^{
//        if (address == strongCutoffParameter.address){
//          strongSelf->filterView.frequency = value;
//          strongSelf->frequencyLabel.stringValue = [strongCutoffParameter stringFromValue: nil];
//        } else if (address == strongResonanceParameter.address) {
//          strongSelf->filterView.resonance = value;
//          strongSelf->resonanceLabel.stringValue = [strongResonanceParameter stringFromValue: nil];
//        }
//
//        [strongSelf updateFilterViewFrequencyAndMagnitudes];
//      });
//    }];
//
//    filterView.frequency = cutoffParameter.value;
//    filterView.resonance = resonanceParameter.value;
//    frequencyLabel.stringValue = [cutoffParameter stringFromValue: nil];
//    resonanceLabel.stringValue = [resonanceParameter stringFromValue: nil];
    
    [_audioUnit addObserver:self forKeyPath:@"allParameterValues"
                    options:NSKeyValueObservingOptionNew
                    context:parameterObserverToken];
  } else {
    NSLog(@"paramTree is NULL!\n");
  }
}

- (void)disconnectViewWithAU {
  if (parameterObserverToken) {
    [_audioUnit.parameterTree removeParameterObserver:parameterObserverToken];
    [_audioUnit removeObserver:self forKeyPath:@"allParameterValues" context:parameterObserverToken];
    parameterObserverToken = 0;
  }
}

@end


