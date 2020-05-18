#pragma once

#define TRACE(Format, ...) \
{ \
    SET_WARN_COLOR(COLOR_BLUE);\
    const FString Msg = FString::Printf(TEXT(Format), ##__VA_ARGS__); \
    if (Msg == "") \
    { \
        UE_LOG(FlexLog, Log, TEXT("%s() : %s"), TEXT(__FUNCTION__), *GetNameSafe(this));\
    } \
    else \
    { \
        UE_LOG(FlexLog, Log, TEXT("%s() : %s"), TEXT(__FUNCTION__), *Msg);\
    } \
    CLEAR_WARN_COLOR(); \
} // Usage example: TRACE("Name: %s, Health %d", *Player.ToString(), Health);


// Macros for manipulating and testing bit-masks
#define TEST_BIT(Bitmask, Bit)  ( ( (Bitmask) & (1 << static_cast<int32>(Bit)) ) > 0)
#define SET_BIT(Bitmask, Bit)   (Bitmask |= 1 << static_cast<int32>(Bit))
#define CLEAR_BIT(Bitmask, Bit) (Bitmask &= ~(1 << static_cast<int32>(Bit)))