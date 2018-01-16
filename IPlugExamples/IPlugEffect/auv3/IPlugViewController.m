#import <CoreAudioKit/AUViewController.h>
#import "IPlugAUv3.h"

@interface IPlugViewController : AUViewController
@property (nonatomic)IPlugAUv3 *audioUnit;
@end

@implementation IPlugViewController
@end

@interface IPlugViewController (AUAudioUnitFactory) <AUAudioUnitFactory>
@end

@implementation IPlugViewController (AUAudioUnitFactory)

- (IPlugAUv3 *) createAudioUnitWithComponentDescription:(AudioComponentDescription) desc error:(NSError **)error {
  self.audioUnit = [[IPlugAUv3 alloc] initWithComponentDescription:desc error:error];

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


