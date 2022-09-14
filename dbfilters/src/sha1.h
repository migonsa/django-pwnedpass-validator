#ifndef SHA1_H
#define SHA1_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>

void _gcry_sha1_transform_amd64_avx_bmi2 (void *state, const unsigned char *data,
                                     uint64_t nblks);
						
typedef struct
{
  uint32_t           h0,h1,h2,h3,h4;
} SHA1_CONTEXT;

void sha1_hash_buffer(void *outbuf, const void *buffer, uint64_t length)
{
	SHA1_CONTEXT hd;
	const uint8_t *inbuf = buffer;

	hd.h0 = 0x67452301;
	hd.h1 = 0xefcdab89;
	hd.h2 = 0x98badcfe;
	hd.h3 = 0x10325476;
	hd.h4 = 0xc3d2e1f0;

    uint8_t buf[64]  __attribute__((aligned(32)));
    uint64_t inblocks = 0;
	uint64_t nb = 0;
    uint32_t count = 0;
	uint32_t blocksize = 64;
  
	if (length >= blocksize)
	{
		inblocks = length >> 6;
		_gcry_sha1_transform_amd64_avx_bmi2(&hd, inbuf, inblocks);
		count = 0;
		nb = inblocks << 6;
		length -= nb;
		inbuf += nb;
	}
	memcpy(&buf[count],inbuf,length);
	count += length;

	/* multiply by 64 to make a byte count */
	/* add the count */
	/* multiply by 8 to make a bit count */
	
	nb = (nb + count) << 3;

	if( count < 56 )  /* enough room */
	{
		buf[count++] = 0x80; /* pad */
		memset(&buf[count], 0, 56 - count);
    }
	else  /* need one extra block */
	{
		buf[count++] = 0x80; /* pad character */
		memset(&buf[count], 0, 64 - count);
		_gcry_sha1_transform_amd64_avx_bmi2( &hd, buf, 1 );
		memset(buf, 0, 64 ); /* fill next block with zeroes */
    }
	/* append the 64 bit count */
	*(uint64_t *)&buf[56] = __builtin_bswap64(nb);
	_gcry_sha1_transform_amd64_avx_bmi2( &hd, buf, 1 );

	uint8_t *p = outbuf;
	*(uint32_t*)p = __builtin_bswap32(hd.h0) ; p += 4;
	*(uint32_t*)p = __builtin_bswap32(hd.h1) ; p += 4;
	*(uint32_t*)p = __builtin_bswap32(hd.h2) ; p += 4;
	*(uint32_t*)p = __builtin_bswap32(hd.h3) ; p += 4;
	*(uint32_t*)p = __builtin_bswap32(hd.h4) ;
}


#endif