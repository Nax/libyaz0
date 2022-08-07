#include "libyaz0.h"

uint32_t swap32(uint32_t in)
{
    return ((in & 0xFF) << 24) | ((in & 0xFF00) << 8) | ((in & 0xFF0000) >> 8) | ((in & 0xFF000000) >> 24);
}
