// Test the shared Mac/iOS adapter with independent controller identities and every face-button combination.
#import "../pad_apple.h"
#include <assert.h>
#include <stdio.h>

@interface TestButton : NSObject
@property(getter=isPressed) BOOL pressed;
@end
@implementation TestButton
@end
@interface TestController : NSObject
@property(copy) NSString *productCategory;
@end
@implementation TestController
@end
@interface TestPad : NSObject
@property TestController *controller;
@property TestButton *buttonA, *buttonB, *buttonX, *buttonY;
@end
@implementation TestPad
@end

static TestPad *pad(NSString *category) {
    TestPad *g = [TestPad new]; g.controller = [TestController new];
    g.controller.productCategory = category;
    g.buttonA = [TestButton new]; g.buttonB = [TestButton new];
    g.buttonX = [TestButton new]; g.buttonY = [TestButton new];
    return g;
}
static void press(TestPad *g, unsigned bits) {
    g.buttonA.pressed = (bits & 1) != 0; g.buttonB.pressed = (bits & 2) != 0;
    g.buttonX.pressed = (bits & 4) != 0; g.buttonY.pressed = (bits & 8) != 0;
}
int main(void) { @autoreleasepool {
    TestPad *pro = pad(@"Switch Pro Controller");
    for (id category in @[@"Xbox One", @"DualShock 4", @"DualSense", @"Generic Controller", [NSNull null]]) {
        TestPad *other = pad(category == [NSNull null] ? nil : category);
        for (unsigned bits = 0; bits < 16; bits++) {
            press(pro, bits); press(other, bits);
            uint64_t nintendo = dm_pad_apple_face_buttons((id)pro);
            uint64_t conventional = dm_pad_apple_face_buttons((id)other);
            // Switch Pro A/B/X/Y must reach the same guest letters, including chords and releases.
            assert(nintendo == bits);
            // Conventional profiles retain Nintendo positions: right B -> A, bottom A -> B, top Y -> X, left X -> Y.
            assert(!!(conventional & DM_BTN_A) == !!(bits & 2));
            assert(!!(conventional & DM_BTN_B) == !!(bits & 1));
            assert(!!(conventional & DM_BTN_X) == !!(bits & 8));
            assert(!!(conventional & DM_BTN_Y) == !!(bits & 4));
            assert(!(conventional & ~15ull));
        }
    }
    assert(dm_pad_apple_face_buttons(nil) == 0);
    puts("t_pad_apple: passed (Switch Pro, Xbox, DualShock, DualSense, generic; all face-button combinations)");
    return 0;
} }
