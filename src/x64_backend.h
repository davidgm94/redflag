#pragma once

typedef s32 get_constant_s32(s32);
typedef s64 identity_s64(s64);
typedef s64 increment_s64(s64);
void ptest(const char* text, bool expr);
get_constant_s32* make_constant_s32(s32 value);

identity_s64* make_identity_s64(void);

increment_s64* make_increment_s64();
