
#ifndef UNITY_BUILD

#include <stdint.h>

#endif

typedef uint8_t Flag;

#define U8ForFlag(count) (((uint32_t)((count) / 8)) == 0 ? 1 : (count) / 8)
#define TestByteFlag(flags, flag) ((flags)[(flag) / 8] & (1 << (((flag) % 8) - 1)))
#define SetByteFlag(flags, flag) ((flags)[(flag) / 8] |= (1 << (((flag) % 8) - 1)))
#define UnsetByteFlag(flags, flag) ((flags)[(flag) / 8] &= (0 << (((flag) % 8) - 1)))
#define ByteField(flag) ((flag))

#define TestBitFlag(flags, flag) ((flags) & (flag))
#define SetBitFlag(flags, flag) ((flags) |= (flag))
#define UnsetBitFlag(flags, flag) ((flags) &= ~(flag))
#define BitField(flag) (2 << (flag))
