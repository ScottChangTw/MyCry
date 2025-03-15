#pragma once

#if defined (__cplusplus)
extern "C" {
#endif

#include <stdint.h>



uint32_t Simple_CRC_SlicingBy8(uint32_t StartCRC, const uint8_t* buf, unsigned long len);


#if defined (__cplusplus)
}
#endif


