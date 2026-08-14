#pragma once

// Each board selects a display backend. Application/UI code depends only on
// the small TFT_eSPI-compatible surface exposed as BowserDisplay.
#ifndef BOWSER_DISPLAY_BACKEND
#define BOWSER_DISPLAY_BACKEND display_backends/tft_espi.h
#endif

#include BOWSER_STRINGIFY(BOWSER_DISPLAY_BACKEND)
