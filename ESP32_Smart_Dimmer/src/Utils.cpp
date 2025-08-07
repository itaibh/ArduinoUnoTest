#include "Utils.h"

// --- HELPER FUNCTION: Escapes a string for JSON output ---
// This is crucial to ensure valid JSON if your strings contain quotes or backslashes.
// For full JSON compatibility, you might need to escape more characters (e.g., newlines, tabs)
// but for typical names/IDs, this should suffice.
String escapeJsonString(const String &input)
{
    String escapedString = "";
    escapedString.reserve(input.length() * 2); // Pre-allocate memory to reduce reallocations
    for (char c : input)
    {
        if (c == '"')
        {
            escapedString += "\\\"";
        }
        else if (c == '\\')
        {
            escapedString += "\\\\";
        }
        // You might need to add more escapes for control characters like:
        // else if (c == '\b') escapedString += "\\b"; // Backspace
        // else if (c == '\f') escapedString += "\\f"; // Form feed
        // else if (c == '\n') escapedString += "\\n"; // Newline
        // else if (c == '\r') escapedString += "\\r"; // Carriage return
        // else if (c == '\t') escapedString += "\\t"; // Tab
        // For basic names, " and \ are the most common problematic characters.
        else
        {
            escapedString += c;
        }
    }
    return escapedString;
}


// --- LightMode Conversion Helpers ---
// Assuming LightMode is an enum, or enum class, defined where accessible
String lightModeToString(LightMode mode)
{
    if (mode == LightMode::MAIN_LIGHT)
        return "main";
    if (mode == LightMode::RGB_RING)
        return "rgb";
    return "unknown";
}

LightMode stringToLightMode(const String &modeStr)
{
    if (modeStr == "main")
        return LightMode::MAIN_LIGHT;
    if (modeStr == "rgb")
        return LightMode::RGB_RING;
    return LightMode::MAIN_LIGHT; // Default to MAIN_LIGHT if unrecognized
}

const String getDeviceNamespace(String mac_address)
{
    String macNoColons = mac_address;
    macNoColons.replace(":", "");
    // Use the full MAC address for namespace uniqueness
    String prefNS = "cfg" + macNoColons.substring(6);
    return prefNS;
}

String sanitizeString(const String& input) {
    String output = "";
    for (int i = 0; i < input.length(); i++) {
        if (input[i] >= 32) { // 32 is the ASCII code for a space, the first printable character
            output += input[i];
        }
    }
    return output;
}