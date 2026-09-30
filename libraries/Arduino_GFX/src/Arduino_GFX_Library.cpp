#include "Arduino_GFX_Library.h"

// Minimal stub — this vendored subset only ships the QSPI + RM67162 driver.
// These default-factory functions are unused by Bowser; they exist only to
// satisfy arduino-cli's requirement to compile all library source files.
Arduino_DataBus *create_default_Arduino_DataBus()
{
  return new Arduino_HWSPI(-1, -1);
}

Arduino_GFX *create_default_Arduino_GFX()
{
  return nullptr;
}
