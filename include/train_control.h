#pragma once

// Shared state — written by handlePoti(), read/written by handleButtons() (stop handler)
extern int gSpeed;
extern bool potBlocked;
extern int speedAtBlock;

void applyStopMode();
void handlePoti();
