#ifndef UTILS_H
#define UTILS_H

#include <Arduino.h>
#include "LightMode.h"
#include "CommandType.h"

// --- HELPER FUNCTION: Escapes a string for JSON output ---
// This is crucial to ensure valid JSON if your strings contain quotes or backslashes.
// For full JSON compatibility, you might need to escape more characters (e.g., newlines, tabs)
// but for typical names/IDs, this should suffice.
String escapeJsonString(const String &input);

// --- LightMode Conversion Helpers ---
// Assuming LightMode is an enum, or enum class, defined where accessible
String lightModeToString(LightMode mode);
LightMode stringToLightMode(const String &modeStr);

String commandTypeToString(CommandType cmdType);

const String getDeviceNamespace(String mac_address);

String sanitizeString(const String& input);

#endif