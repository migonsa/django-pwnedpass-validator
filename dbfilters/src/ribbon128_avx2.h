#ifndef RIBBON128_H
#define RIBBON128_H

#include <immintrin.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <aio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "utils.h"


#if !defined(__AVX2__)
#error AVX2 required: Compile with "-mavx2" option.
#endif

#define RIBBON128_OVERHEAD_FACTOR (1.045)
#define RIBBON128_EXTRA (128)
#define MAGIC_FILTER "$ribbon128-filter-1.0\n"


typedef struct
{
    uint8_t r;
    uint32_t m;
    uint8_t* f;
} ribbon128_t;


static ribbon128_t filter = {0};


static inline void solve_avx2_r8(__m128i* coeff)
{
    const __m256i zero = _mm256_setzero_si256();
	const __m256i shufmask = _mm256_set_epi64x(
        0x0000000000000000, 0x0101010101010101,
        0x0202020202020202, 0x0303030303030303);
	const __m256i andmask = _mm256_set1_epi64x(0x0102040810204080);

    init_shishua(0x5555555555555555);

    uint8_t *pre_ptr;

	for(int32_t i = filter.m-1; i >= 0; i--)
	{
        pre_ptr = (uint8_t*)(coeff+i-1);
        __builtin_prefetch(pre_ptr);
        __builtin_prefetch(pre_ptr+8);

        //pre_ptr = (uint8_t*)(filter.f+i-1);
        //__builtin_prefetch(pre_ptr);

		__m256i* ptr = (__m256i *)(filter.f+i);
		__m128i v = _mm_load_si128(coeff+i);
        _mm_store_si128(coeff+i, _mm_setzero_si128());
		if(_mm_testz_si128(v, v))
			filter.f[i] = get8_shishua();
		else
		{
		    __m256i vv = _mm256_setr_m128i(v,v);
		    __m256i accum256 = 
				_mm256_and_si256(
					_mm256_loadu_si256(ptr++),
					_mm256_cmpeq_epi8(
                        _mm256_andnot_si256(
                            _mm256_shuffle_epi8(
                                _mm256_shuffle_epi32(vv, 0xFF) , shufmask), andmask), zero));
			
			accum256 = _mm256_xor_si256(accum256,
				_mm256_and_si256(
                    _mm256_loadu_si256(ptr++),
                    _mm256_cmpeq_epi8(
                        _mm256_andnot_si256(
                            _mm256_shuffle_epi8(
                                _mm256_shuffle_epi32(vv, 0xAA) , shufmask), andmask), zero)));
								
			accum256 = _mm256_xor_si256(accum256,
				_mm256_and_si256(
				    _mm256_loadu_si256(ptr++),
                    _mm256_cmpeq_epi8(
                        _mm256_andnot_si256(
                            _mm256_shuffle_epi8(
                                _mm256_shuffle_epi32(vv, 0x55) , shufmask), andmask), zero)));	
								
			accum256 = _mm256_xor_si256(accum256,
				_mm256_and_si256(
				    _mm256_loadu_si256(ptr),				
					_mm256_cmpeq_epi8(
						_mm256_andnot_si256(
							_mm256_shuffle_epi8(
								_mm256_shuffle_epi32(vv, 0x00) , shufmask), andmask), zero)));

			accum256 = _mm256_xor_si256(accum256, _mm256_permute4x64_epi64(accum256, 0x4E));
			accum256 = _mm256_xor_si256(accum256, _mm256_srli_si256(accum256, 8));
			accum256 = _mm256_xor_si256(accum256, _mm256_srli_si256(accum256, 4));
			accum256 = _mm256_xor_si256(accum256, _mm256_srli_si256(accum256, 2));
			accum256 = _mm256_xor_si256(accum256, _mm256_srli_si256(accum256, 1));
			
			filter.f[i] = _mm256_extract_epi8(accum256,0);
		}
	}	
}

static inline void solve_avx2_r16(__m128i* coeff)
{
    const __m256i zero = _mm256_setzero_si256();
    const __m256i shufmask1 = _mm256_set_epi64x(
        0x0202020202020202, 0x0202020202020202,
        0x0303030303030303, 0x0303030303030303);
	const __m256i shufmask2 = _mm256_set_epi64x(
        0x0000000000000000, 0x0000000000000000,
        0x0101010101010101, 0x0101010101010101);
	const __m256i andmask = _mm256_set_epi64x(
        0x0101020204040808, 0x1010202040408080,
        0x0101020204040808, 0x1010202040408080);

    init_shishua(0x5555555555555555);
    uint8_t *pre_ptr;
    uint16_t *fptr = (uint16_t*) filter.f;

	for(int32_t i = filter.m-1; i >= 0; i--)
	{
        pre_ptr = (uint8_t*)(coeff+i-1);
        __builtin_prefetch(pre_ptr);
        __builtin_prefetch(pre_ptr+8);

		__m256i* ptr = (__m256i *)(fptr+i);
		__m128i v = _mm_load_si128(coeff+i);
        _mm_store_si128(coeff+i, _mm_setzero_si128());
		if(_mm_testz_si128(v, v))
			fptr[i] = get16_shishua();
		else
		{
		    __m256i vv = _mm256_setr_m128i(v,v);
		    __m256i accum256 = 
				_mm256_and_si256(
					_mm256_loadu_si256(ptr++),
					_mm256_cmpeq_epi16(
                        _mm256_andnot_si256(
                            _mm256_shuffle_epi8(
                                _mm256_shuffle_epi32(vv, 0xFF) , shufmask1), andmask), zero));
            
            accum256 = _mm256_xor_si256(accum256,
				_mm256_and_si256(
                    _mm256_loadu_si256(ptr++),
                    _mm256_cmpeq_epi16(
                        _mm256_andnot_si256(
                            _mm256_shuffle_epi8(
                                _mm256_shuffle_epi32(vv, 0xFF) , shufmask2), andmask), zero)));
			
			accum256 = _mm256_xor_si256(accum256,
				_mm256_and_si256(
                    _mm256_loadu_si256(ptr++),
                    _mm256_cmpeq_epi16(
                        _mm256_andnot_si256(
                            _mm256_shuffle_epi8(
                                _mm256_shuffle_epi32(vv, 0xAA) , shufmask1), andmask), zero)));
            
            accum256 = _mm256_xor_si256(accum256,
				_mm256_and_si256(
                    _mm256_loadu_si256(ptr++),
                    _mm256_cmpeq_epi16(
                        _mm256_andnot_si256(
                            _mm256_shuffle_epi8(
                                _mm256_shuffle_epi32(vv, 0xAA) , shufmask2), andmask), zero)));
								
			accum256 = _mm256_xor_si256(accum256,
				_mm256_and_si256(
				    _mm256_loadu_si256(ptr++),
                    _mm256_cmpeq_epi16(
                        _mm256_andnot_si256(
                            _mm256_shuffle_epi8(
                                _mm256_shuffle_epi32(vv, 0x55) , shufmask1), andmask), zero)));	
            
            accum256 = _mm256_xor_si256(accum256,
				_mm256_and_si256(
				    _mm256_loadu_si256(ptr++),
                    _mm256_cmpeq_epi16(
                        _mm256_andnot_si256(
                            _mm256_shuffle_epi8(
                                _mm256_shuffle_epi32(vv, 0x55) , shufmask2), andmask), zero)));
								
			accum256 = _mm256_xor_si256(accum256,
				_mm256_and_si256(
				    _mm256_loadu_si256(ptr++),				
					_mm256_cmpeq_epi16(
						_mm256_andnot_si256(
							_mm256_shuffle_epi8(
								_mm256_shuffle_epi32(vv, 0x00) , shufmask1), andmask), zero)));
            
            accum256 = _mm256_xor_si256(accum256,
				_mm256_and_si256(
				    _mm256_loadu_si256(ptr),				
					_mm256_cmpeq_epi16(
						_mm256_andnot_si256(
							_mm256_shuffle_epi8(
								_mm256_shuffle_epi32(vv, 0x00) , shufmask2), andmask), zero)));

			accum256 = _mm256_xor_si256(accum256, _mm256_permute4x64_epi64(accum256, 0x4E));
			accum256 = _mm256_xor_si256(accum256, _mm256_srli_si256(accum256, 8));
			accum256 = _mm256_xor_si256(accum256, _mm256_srli_si256(accum256, 4));
			accum256 = _mm256_xor_si256(accum256, _mm256_srli_si256(accum256, 2));
			
			fptr[i] = _mm256_extract_epi16(accum256,0);
		}
	}	
}

void destroy_filter()
{
    free(filter.f);
    bzero(&filter, sizeof(ribbon128_t));
}

bool create_ribbon128(char* filename, uint32_t maxkeys, uint8_t r, double oversize)
{
    const __uint128_t msbmask = ((__uint128_t)1<<127);
    ribbon128_key_t* key;
    if (!open_keys_file(filename, &maxkeys))
        return false;
    destroy_filter();

    filter.r = r;
	/*
    filter.m = filter.r == 1 
                    ? ((uint32_t)(maxkeys * oversize + RIBBON128_EXTRA + 31)) & ~0x1f
                    : ((uint32_t)(maxkeys * oversize + RIBBON128_EXTRA + 63)) & ~0x3f;*/
	filter.m = (uint32_t)(maxkeys * oversize + RIBBON128_EXTRA);
    __uint128_t* coeff = (__uint128_t*)aligned_alloc(sizeof(__uint128_t), filter.m*sizeof(__uint128_t)+RIBBON128_EXTRA*filter.r);
    bzero(coeff, filter.m*sizeof(__uint128_t)+RIBBON128_EXTRA*filter.r);
    
    uint32_t n = filter.m - RIBBON128_EXTRA;

    while((key = read_key()) != NULL)
    { 
        uint8_t *ptr;
        uint32_t index = key->index;
        index = ((uint64_t) index*n)>>32;
		ptr = (uint8_t*)(coeff+index);
		__builtin_prefetch(ptr);
		__builtin_prefetch(ptr+8);
        __uint128_t v = key->ribbon;
        
		//ptr = (uint8_t*)(key+1);
		//__builtin_prefetch(ptr,1,0);
		
        v |= msbmask;
        for(;;)
        {
			v ^= *(coeff+index);
			uint64_t vl = (uint64_t)(v);
			uint64_t vh = (uint64_t)(v>>64);
			uint64_t lzcnt;
			if(vh)
			{
				if(vh & ((uint64_t)1<<63))
				{
					*(coeff+index) = v;
					break;
				}
				lzcnt = _lzcnt_u64(vh);
			}
			else if(vl)
				lzcnt = 64+_lzcnt_u64(vl);
			else
				break;
			index += lzcnt;
			ptr = (uint8_t*)(coeff+index);
			__builtin_prefetch(ptr);
			__builtin_prefetch(ptr+8);
			v = v<<lzcnt;
        }
    }
    close_keys_file();
    
    uint32_t filtersize = filter.r*filter.m;
    filter.f = (uint8_t*)(coeff)+filter.m*(sizeof(__m128i)-filter.r);
    filter.r == 1 ? solve_avx2_r8((__m128i*)coeff) : solve_avx2_r16((__m128i*)coeff);
    memcpy(coeff, filter.f, filtersize);
    filter.f = realloc(coeff, filtersize);
    return filter.f != NULL;
}

bool query_ribbon128_r8(const ribbon128_key_t* key)
{
    const __m256i zero = _mm256_setzero_si256();
	const __m256i shufmask = _mm256_set_epi64x(
        0x0000000000000000, 0x0101010101010101,
        0x0202020202020202, 0x0303030303030303);
	const __m256i andmask = _mm256_set1_epi64x(0x0102040810204080);
	const __m128i msbmask = _mm_set_epi64x((uint64_t)0x8000000000000000LL,(uint64_t)0);

	__m128i v = _mm_loadu_si128((__m128i *)&key->ribbon);
    v = _mm_or_si128(v, msbmask);

    uint32_t index = ((uint64_t) key->index*(filter.m - RIBBON128_EXTRA))>>32;
    
    __m256i* ptr = (__m256i *)(filter.f + index);
    __m256i vv = _mm256_setr_m128i(v,v);

    __m256i accum256 = 
        _mm256_and_si256(
            _mm256_loadu_si256(ptr++),
            _mm256_cmpeq_epi8(
                _mm256_andnot_si256(
                    _mm256_shuffle_epi8(
                        _mm256_shuffle_epi32(vv, 0xFF) , shufmask), andmask), zero));
    
    accum256 = _mm256_xor_si256(accum256,
        _mm256_and_si256(
            _mm256_loadu_si256(ptr++),
            _mm256_cmpeq_epi8(
                _mm256_andnot_si256(
                    _mm256_shuffle_epi8(
                        _mm256_shuffle_epi32(vv, 0xAA) , shufmask), andmask), zero)));
                        
    accum256 = _mm256_xor_si256(accum256,
        _mm256_and_si256(
            _mm256_loadu_si256(ptr++),
            _mm256_cmpeq_epi8(
                _mm256_andnot_si256(
                    _mm256_shuffle_epi8(
                        _mm256_shuffle_epi32(vv, 0x55) , shufmask), andmask), zero)));	
                        
    accum256 = _mm256_xor_si256(accum256,
        _mm256_and_si256(
            _mm256_loadu_si256(ptr),				
            _mm256_cmpeq_epi8(
                _mm256_andnot_si256(
                    _mm256_shuffle_epi8(
                        _mm256_shuffle_epi32(vv, 0x00) , shufmask), andmask), zero)));

    accum256 = _mm256_xor_si256(accum256, _mm256_permute4x64_epi64(accum256, 0x4E));
    accum256 = _mm256_xor_si256(accum256, _mm256_srli_si256(accum256, 8));
    accum256 = _mm256_xor_si256(accum256, _mm256_srli_si256(accum256, 4));
    accum256 = _mm256_xor_si256(accum256, _mm256_srli_si256(accum256, 2));
    accum256 = _mm256_xor_si256(accum256, _mm256_srli_si256(accum256, 1));

	return !_mm256_extract_epi8(accum256, 0);
}

bool query_ribbon128_r16(const ribbon128_key_t* key)
{
    const __m256i zero = _mm256_setzero_si256();
    const __m128i msbmask = _mm_set_epi64x((uint64_t)0x8000000000000000LL,(uint64_t)0);
    const __m256i shufmask1 = _mm256_set_epi64x(
        0x0202020202020202, 0x0202020202020202,
        0x0303030303030303, 0x0303030303030303);
	const __m256i shufmask2 = _mm256_set_epi64x(
        0x0000000000000000, 0x0000000000000000,
        0x0101010101010101, 0x0101010101010101);
	const __m256i andmask = _mm256_set_epi64x(
        0x0101020204040808, 0x1010202040408080,
        0x0101020204040808, 0x1010202040408080);
    
	__m128i v = _mm_loadu_si128((__m128i *)&key->ribbon);
    v = _mm_or_si128(v, msbmask);
	uint32_t index = key->index;

    index = ((uint64_t) index*(filter.m - RIBBON128_EXTRA))>>32;

    uint16_t *fptr = (uint16_t*) filter.f;
    __m256i* ptr = (__m256i *)(fptr + index);
    __m256i vv = _mm256_setr_m128i(v,v);

    __m256i accum256 = 
        _mm256_and_si256(
            _mm256_loadu_si256(ptr++),
            _mm256_cmpeq_epi16(
                _mm256_andnot_si256(
                    _mm256_shuffle_epi8(
                        _mm256_shuffle_epi32(vv, 0xFF) , shufmask1), andmask), zero));
    
    accum256 = _mm256_xor_si256(accum256,
        _mm256_and_si256(
            _mm256_loadu_si256(ptr++),
            _mm256_cmpeq_epi16(
                _mm256_andnot_si256(
                    _mm256_shuffle_epi8(
                        _mm256_shuffle_epi32(vv, 0xFF) , shufmask2), andmask), zero)));
    
    accum256 = _mm256_xor_si256(accum256,
        _mm256_and_si256(
            _mm256_loadu_si256(ptr++),
            _mm256_cmpeq_epi16(
                _mm256_andnot_si256(
                    _mm256_shuffle_epi8(
                        _mm256_shuffle_epi32(vv, 0xAA) , shufmask1), andmask), zero)));
    
    accum256 = _mm256_xor_si256(accum256,
        _mm256_and_si256(
            _mm256_loadu_si256(ptr++),
            _mm256_cmpeq_epi16(
                _mm256_andnot_si256(
                    _mm256_shuffle_epi8(
                        _mm256_shuffle_epi32(vv, 0xAA) , shufmask2), andmask), zero)));
                        
    accum256 = _mm256_xor_si256(accum256,
        _mm256_and_si256(
            _mm256_loadu_si256(ptr++),
            _mm256_cmpeq_epi16(
                _mm256_andnot_si256(
                    _mm256_shuffle_epi8(
                        _mm256_shuffle_epi32(vv, 0x55) , shufmask1), andmask), zero)));	
    
    accum256 = _mm256_xor_si256(accum256,
        _mm256_and_si256(
            _mm256_loadu_si256(ptr++),
            _mm256_cmpeq_epi16(
                _mm256_andnot_si256(
                    _mm256_shuffle_epi8(
                        _mm256_shuffle_epi32(vv, 0x55) , shufmask2), andmask), zero)));
                        
    accum256 = _mm256_xor_si256(accum256,
        _mm256_and_si256(
            _mm256_loadu_si256(ptr++),				
            _mm256_cmpeq_epi16(
                _mm256_andnot_si256(
                    _mm256_shuffle_epi8(
                        _mm256_shuffle_epi32(vv, 0x00) , shufmask1), andmask), zero)));
    
    accum256 = _mm256_xor_si256(accum256,
        _mm256_and_si256(
            _mm256_loadu_si256(ptr),				
            _mm256_cmpeq_epi16(
                _mm256_andnot_si256(
                    _mm256_shuffle_epi8(
                        _mm256_shuffle_epi32(vv, 0x00) , shufmask2), andmask), zero)));

    accum256 = _mm256_xor_si256(accum256, _mm256_permute4x64_epi64(accum256, 0x4E));
    accum256 = _mm256_xor_si256(accum256, _mm256_srli_si256(accum256, 8));
    accum256 = _mm256_xor_si256(accum256, _mm256_srli_si256(accum256, 4));
    accum256 = _mm256_xor_si256(accum256, _mm256_srli_si256(accum256, 2));

	return !_mm256_extract_epi16(accum256, 0);
}

bool filter_exist()
{
  return filter.f != NULL;
}

bool query_ribbon128(char* pass, bool hashed)
{
    assert(filter_exist());
    bool (*query)(const ribbon128_key_t*) = filter.r == 1 ? &query_ribbon128_r8 : &query_ribbon128_r16;
    ribbon128_key_t key;
    if((hashed && hash2key(pass, &key)) || password2key(pass, &key))
        return query(&key);
    else
        return false;
}

bool sanity_check(char* filename, uint32_t maxkeys)
{
    assert(filter_exist());
    ribbon128_key_t* key;
    if (!open_keys_file(filename, &maxkeys))
        return false;
    bool (*query)(const ribbon128_key_t*) = filter.r == 1 ? &query_ribbon128_r8 : &query_ribbon128_r16;
    while((key = read_key()) != NULL)
    {
        if (!query(key))
        {
            printf("Sanity check failed.\n");
            return false;
        }
    }
    close_keys_file();
    return true;
}

uint32_t fp_filter(uint32_t n)
{
    assert(filter_exist());
    uint32_t matches = 0;
    init_shishua(clock());
    ribbon128_key_t randomkey;
    bool (*query)(const ribbon128_key_t*) = filter.r == 1 ? &query_ribbon128_r8 : &query_ribbon128_r16;
    while(n--)
    {
        random_ribbon_key(&randomkey);
        if(query(&randomkey))
            matches++;
    }
    return matches;
}

bool save_filter(char* filename)
{
    assert(filter_exist());
    FILE* fp = fopen(filename, "wb");
    if (fp == NULL)
    {
        printf("Cannot open the output file %s.", filename);
        return false;
    }
    if (!fwrite(MAGIC_FILTER, sizeof(MAGIC_FILTER), 1, fp)
        || !fwrite(&filter.r, sizeof(uint8_t), 1, fp)
        || !fwrite(&filter.m, sizeof(uint32_t), 1, fp)
        || !fwrite(filter.f, filter.m*filter.r, 1, fp))
    {
        perror("Error when writing into file");
        fclose(fp);
        return false;
    }
    fclose(fp);
    return true;
}

bool load_filter(char* filename, uint32_t maxkeys, uint8_t r, double oversize)
{ 
    FILE* fp = fopen(filename, "rb");
    if (fp == NULL)
    {
        printf("Cannot open the input file %s.", filename);
        return false;
    }
    if (filter.f != NULL)
        destroy_filter();

    char magic[sizeof(MAGIC_FILTER)];
    uint32_t expected_m = (uint32_t)(maxkeys * oversize + RIBBON128_EXTRA);
    if (!fread(magic, sizeof(MAGIC_FILTER), 1, fp) 
        || strcmp(magic, MAGIC_FILTER)
        || !fread(&filter.r, sizeof(uint8_t), 1, fp)
        || filter.r != r
        || !fread(&filter.m, sizeof(uint32_t), 1, fp)
        || (maxkeys != 0 && filter.m != expected_m))
    {
        perror("Error when reading file");
        fclose(fp);
        return false;
    }

    filter.f = aligned_alloc(sizeof(__m256i), filter.m*filter.r);
    if (!fread(filter.f, filter.m*filter.r, 1, fp))
    {
        perror("Error when reading file");
        fclose(fp);
        return false;
    }
    fclose(fp);
    return true;
}


#endif