#include "SFConfig.h"

void SFConfig::begin(bool readOnly) {
    prefs.begin("flapmod", readOnly);
}

void SFConfig::setZeroOffset(int32_t value) {
    prefs.putInt("zero_offset", value);
}

int32_t SFConfig::getZeroOffset(int32_t defaultVal) {
    if (!prefs.isKey("zero_offset")) {
        prefs.putInt("zero_offset", defaultVal);  // Write it once
        return defaultVal;
    }
    return prefs.getInt("zero_offset", defaultVal);
}


void SFConfig::setStepOffset(int32_t value) {
    prefs.putInt("step_offset", value);
}

int32_t SFConfig::getStepOffset(int32_t defaultVal) {
    return prefs.getInt("step_offset", defaultVal);
}

void SFConfig::setReserved(const String& value) {
    prefs.putString("reserved", value);
}

String SFConfig::getReserved() {
    return prefs.getString("reserved", "");
}

void SFConfig::print(Stream& out) {
    out.println("-------- SFConfig --------");
    out.printf("Zero Offset (degrees): %d\n", getZeroOffset());
    out.printf("Step Offset (steps): %d\n", getStepOffset());
    out.printf("Reserved: %s\n", getReserved().c_str());
}
