/*
 * Author:
 *   Andreas Färber <andreas.faerber@web.de>
 */
#ifndef BSWAP_H
#define BSWAP_H


#ifdef __APPLE__

#include <CoreFoundation/CoreFoundation.h>
#define cpu_to_le16(x) CFSwapInt16HostToLittle(x)
#define cpu_to_le32(x) CFSwapInt32HostToLittle(x)
#define cpu_to_be16(x) CFSwapInt16HostToBig(x)
#define cpu_to_be32(x) CFSwapInt32HostToBig(x)
#define le16_to_cpu(x) CFSwapInt16LittleToHost(x)
#define le32_to_cpu(x) CFSwapInt32LittleToHost(x)
#define be16_to_cpu(x) CFSwapInt16BigToHost(x)
#define be32_to_cpu(x) CFSwapInt32BigToHost(x)

#elif defined(__linux)

#include <endian.h>
#include <byteswap.h>
#if __BYTE_ORDER == __LITTLE_ENDIAN
static inline uint16_t cpu_to_le16(uint16_t x)
{
    return x;
}
static inline uint32_t cpu_to_le32(uint32_t x)
{
    return x;
}
static inline uint16_t le16_to_cpu(uint16_t x)
{
    return x;
}
static inline uint32_t le32_to_cpu(uint32_t x)
{
    return x;
}
#define cpu_to_be16 bswap_16
#define cpu_to_be32 bswap_32
#define be16_to_cpu bswap_16
#define be32_to_cpu bswap_32
#else
#define 
#endif

#else
#error Byte swapping not yet implemented for this platform
#endif


#endif
