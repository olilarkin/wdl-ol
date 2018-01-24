#import <CoreAudioKit/AUViewController.h>
#import "IPlugAUAudioUnit.h"

@interface IPlugViewController : AUViewController
@property (nonatomic)IPlugAUAudioUnit* audioUnit; // TODO: what is @property (nonatomic)
@end

@implementation IPlugViewController
  AUParameterObserverToken parameterObserverToken;
@end

@interface IPlugViewController (AUAudioUnitFactory) <AUAudioUnitFactory>
@end

@implementation IPlugViewController (AUAudioUnitFactory)

- (IPlugAUAudioUnit *) createAudioUnitWithComponentDescription:(AudioComponentDescription) desc error:(NSError **)error
{
  self.audioUnit = [[[IPlugAUAudioUnit alloc] initWithComponentDescription:desc error:error] retain];

  // Check if the UI has been loaded
  if(self.isViewLoaded)
  {
    [self connectUIToAudioUnit];
  }

  return self.audioUnit;
}

- (void) viewDidLoad
{
  [super viewDidLoad];
  
  // Check if the Audio Unit has been loaded
  if(self.audioUnit)
  {
    [self connectUIToAudioUnit];
  }
}

- (void)connectUIToAudioUnit
{
  // Get the parameter tree and add observers for any parameters that the UI needs to keep in sync with the Audio Unit
}

- (IPlugAUAudioUnit *)getAudioUnit
{
  return _audioUnit;
}

- (void)setAudioUnit:(IPlugAUAudioUnit*) audioUnit
{
  _audioUnit = audioUnit;
  dispatch_async(dispatch_get_main_queue(), ^
  {
    if ([self isViewLoaded])
    {
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
  
  dispatch_async(dispatch_get_main_queue(), ^
  {
    //TODO:
  });
}

- (void)connectViewWithAU
{
  AUParameterTree *paramTree = _audioUnit.parameterTree;
  
  if (paramTree)
  {
    [_audioUnit addObserver:self forKeyPath:@"allParameterValues"
                    options:NSKeyValueObservingOptionNew
                    context:parameterObserverToken];
  }
  else
  {
    NSLog(@"paramTree is NULL!\n");
  }
}

- (void)disconnectViewWithAU
{
  if (parameterObserverToken)
  {
    [_audioUnit.parameterTree removeParameterObserver:parameterObserverToken];
    [_audioUnit removeObserver:self forKeyPath:@"allParameterValues" context:parameterObserverToken];
    parameterObserverToken = 0;
  }
}


@end


