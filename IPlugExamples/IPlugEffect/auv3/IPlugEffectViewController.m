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
  return self.audioUnit;
}
@end


