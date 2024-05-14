
#ifndef UNITY_BUILD

#include <stdint.h>

#endif

typedef uint8_t Flag;

#define U8ForFlag(count) (((uint32_t)((count) / 8)) == 0 ? 1 : (count) / 8)
#define TestFlag(flags, flag) ((flags)[(flag) / 8] & (1 << (((flag) % 8) - 1)))
#define SetFlag(flags, flag) ((flags)[(flag) / 8] |= (1 << (((flag) % 8) - 1)))
#define UnsetFlag(flags, flag) ((flags)[(flag) / 8] &= (0 << (((flag) % 8) - 1)))
