// Shared Mac/iOS face-button adapter. GameController exposes Switch Pro buttons by Nintendo letter;
// Xbox and PlayStation profiles use the conventional bottom-A/right-B diamond. Preserve the OS's user remapping.
#pragma once
#import <GameController/GameController.h>
#include "dm_input.h"

static inline uint64_t dm_pad_apple_face_buttons(GCExtendedGamepad *g) {
    BOOL nintendo = [g.controller.productCategory isEqualToString:@"Switch Pro Controller"];
    uint64_t m = 0;
    if (g.buttonA.isPressed) m |= nintendo ? DM_BTN_A : DM_BTN_B;
    if (g.buttonB.isPressed) m |= nintendo ? DM_BTN_B : DM_BTN_A;
    if (g.buttonX.isPressed) m |= nintendo ? DM_BTN_X : DM_BTN_Y;
    if (g.buttonY.isPressed) m |= nintendo ? DM_BTN_Y : DM_BTN_X;
    return m;
}
