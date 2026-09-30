#ifndef _ARDUINO_GFX_LIBRARIES_H_
#define _ARDUINO_GFX_LIBRARIES_H_

// Minimal subset vendored for the Bowser Wallet T-Display S3 AMOLED target.
// Only the ESP32 QSPI bus and RM67162 display driver are included.

#include "Arduino_DataBus.h"
#include "databus/Arduino_SWSPI.h"
#include "databus/Arduino_HWSPI.h"
#include "databus/Arduino_Wire.h"
#include "databus/Arduino_ESP32QSPI.h"

#include "Arduino_GFX.h"

#include "display/Arduino_RM67162.h"

#endif // _ARDUINO_GFX_LIBRARIES_H_
