#pragma once
#include <Preferences.h>

class SFConfig {
public:
    void begin(bool readOnly = false);

    void setZeroOffset(int32_t value);
    int32_t getZeroOffset(int32_t defaultVal = 91);

    void setStepOffset(int32_t value);
    int32_t getStepOffset(int32_t defaultVal = 0);

    String getReserved();
    void setReserved(const String& value);

    void print(Stream& out = Serial);

private:
    Preferences prefs;
};
