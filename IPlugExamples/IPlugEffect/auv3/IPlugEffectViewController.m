#import <CoreAudioKit/AUViewController.h>
#import "IPlugEffectAUv3.h"

@interface IPlugEffectViewController : AUViewController
@property (nonatomic)IPlugEffectAUv3 *audioUnit;
@end

@implementation IPlugEffectViewController
@end

@interface IPlugEffectViewController (AUAudioUnitFactory) <AUAudioUnitFactory>
@end

@implementation IPlugEffectViewController (AUAudioUnitFactory)

- (IPlugEffectAUv3 *) createAudioUnitWithComponentDescription:(AudioComponentDescription) desc error:(NSError **)error {
  self.audioUnit = [[IPlugEffectAUv3 alloc] initWithComponentDescription:desc error:error];

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
@end


