#ifndef HEXAVX2_H
#define HEXAVX2_H

#include <immintrin.h>

#if !defined(__AVX2__)
#error AVX2 required: Compile with "-mavx2" option.
#endif


static inline bool ascii2hex(__m256i input, __m256i* result, bool verify)
{
    const __m256i shufflemask = _mm256_set_epi64x(
                0x0E0C0A0806040200, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0x0E0C0A0806040200);
    const __m256i msb = _mm256_set1_epi8(0x80);
    __m256i numbers = _mm256_sub_epi8(input, _mm256_set1_epi8('0'));
    __m256i mask = _mm256_cmpgt_epi8(numbers, _mm256_set1_epi8(9));
    __m256i letters = _mm256_sub_epi8(
                        _mm256_andnot_si256(
                            _mm256_set1_epi8('a'-'A'), input),
                            _mm256_set1_epi8('A'-10));
    *result = _mm256_blendv_epi8(numbers, letters, mask);
    if (verify)
    {
        __m256i errors = _mm256_or_si256(
                    _mm256_or_si256(
                        _mm256_or_si256(
                            _mm256_and_si256(input, msb), 
                            _mm256_and_si256(*result, msb)), 
                        _mm256_and_si256(
                            mask,
                            _mm256_cmpgt_epi8(_mm256_set1_epi8(10), *result))), 
                    _mm256_and_si256(
                        mask,
                        _mm256_cmpgt_epi8(*result, _mm256_set1_epi8(15))));

        if (!_mm256_testz_si256(errors, errors))
            return false;
    }
    *result = _mm256_shuffle_epi8(
                _mm256_or_si256(_mm256_srli_si256(*result, 1), 
                    _mm256_slli_epi16(*result, 4)), shufflemask);
    *result = _mm256_or_si256(*result, _mm256_permute4x64_epi64(*result, 0xCC));
    return true;
}

static inline void hex2ascii(__m256i input, __m256i *out)
{	
	input =	_mm256_shuffle_epi8(input,
				_mm256_set_epi64x(0x0F0F0E0E0D0D0C0C, 0x0B0B0A0A09090808, 0x0707060605050404, 0x0303020201010000));
				
	*out =	_mm256_shuffle_epi8(
			_mm256_setr_epi8('0', '1', '2', '3', '4', '5', '6', '7','8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
							'0', '1', '2', '3', '4', '5', '6', '7','8', '9', 'A', 'B', 'C', 'D', 'E', 'F'),
			_mm256_or_si256(
				_mm256_and_si256(input,
					_mm256_set1_epi16(0x0f00)),
				_mm256_and_si256(
					_mm256_srli_epi16(input, 4),
					_mm256_set1_epi16(0x0000f))));	
}


#endif