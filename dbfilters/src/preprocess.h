#ifndef PREPROCESS_H
#define PREPROCESS_H

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
#include <string.h>

#include "hex_avx2.h"

#if !defined(__AVX2__)
#error AVX2 required: Compile with "-mavx2" option.
#endif

#define MAGIC_KEYS "$ribbon128-keys-1.0\n"
#define AIO_R_BUF (1024*1024)
#define AIO_MAXKEYS (64*1024)
#define AIO_W_BUF (AIO_MAXKEYS*sizeof(ribbon128_key_t))
#define EXTRA_BUF (32) 


typedef struct __attribute__((__packed__))
{
    __uint128_t ribbon;
    uint32_t index;
} ribbon128_key_t;

typedef struct
{
    struct aiocb aiocb;
    uint8_t* bufs[2];
    uint32_t bindex;
    int32_t index;
    uint64_t offset;
    int32_t nblocks;   
} aio_r_t;

typedef struct
{
    struct aiocb aiocb;
    ribbon128_key_t* bufs[2];
    uint32_t bindex;
    uint32_t kindex;
    uint64_t offset;
    
} aio_w_t;

static aio_r_t aio_r = {0};
static aio_w_t aio_w = {0};

void close_passwd_file()
{
	if(aio_r.aiocb.aio_fildes <= 0)
		close(aio_r.aiocb.aio_fildes);
	if(aio_r.bufs[0])
        free(aio_r.bufs[0] - EXTRA_BUF);
	if(aio_r.bufs[1])
		free(aio_r.bufs[1] - EXTRA_BUF);
	bzero(&aio_r, sizeof(aio_r_t));
}

bool open_passwd_file(char* filename)
{
	bzero(&aio_r, sizeof(aio_r_t));
    aio_r.aiocb.aio_fildes = open(filename, O_RDONLY);
    if (aio_r.aiocb.aio_fildes < 0)
    {
        printf("Cannot open the input file %s", filename);
        perror("");
        return false;
    }
    
    uint64_t filesize = lseek(aio_r.aiocb.aio_fildes, 0L, SEEK_END);
    if (!filesize)
    {
        printf("Empty input file %s", filename);
        return false;
    }
    lseek(aio_r.aiocb.aio_fildes, 0L, SEEK_SET);
    aio_r.bufs[0] = (uint8_t*)malloc(AIO_R_BUF + EXTRA_BUF) + EXTRA_BUF;
    aio_r.bufs[1] = (uint8_t*)malloc(AIO_R_BUF + EXTRA_BUF) + EXTRA_BUF;
    aio_r.nblocks = filesize/AIO_R_BUF;	
    aio_r.index = AIO_R_BUF - (filesize - aio_r.nblocks*AIO_R_BUF);
    if (aio_r.index != AIO_R_BUF)
    {		
        aio_r.aiocb.aio_buf = &aio_r.bufs[0][aio_r.index];
        aio_r.aiocb.aio_nbytes = (AIO_R_BUF-aio_r.index);
    }
    else
    {
        aio_r.nblocks--;
        aio_r.aiocb.aio_buf = aio_r.bufs[0];
        aio_r.aiocb.aio_nbytes = AIO_R_BUF;
    }
    aio_r.offset = 0;
    if (read(aio_r.aiocb.aio_fildes, (void*) aio_r.aiocb.aio_buf, aio_r.aiocb.aio_nbytes) != aio_r.aiocb.aio_nbytes)
    {
        perror("read");
        close_passwd_file();
        return false;
    }   
    if (aio_r.nblocks)
    {
        aio_r.aiocb.aio_buf = aio_r.bufs[1];
        aio_r.offset += aio_r.aiocb.aio_nbytes;
        aio_r.aiocb.aio_nbytes = AIO_R_BUF;
        aio_r.aiocb.aio_offset = aio_r.offset;
        if (aio_read(&aio_r.aiocb) < 0)
        {
            perror("aio_read");
            close_passwd_file();
            return false;
        }
    }
    return true;
}

static inline uint8_t* read_passwd_data(uint32_t size)
{
	uint8_t* ptr;
    if (aio_r.nblocks < 0)
        return NULL;
	uint32_t left = AIO_R_BUF - aio_r.index;
	if(left > size)
	{
		ptr = aio_r.bufs[aio_r.bindex] + aio_r.index;
		aio_r.index += size;
		left -= size;
		size = 0;
	}
	else
	{
		ptr = aio_r.bufs[aio_r.bindex^1]-left;
		memcpy(ptr, &aio_r.bufs[aio_r.bindex][aio_r.index], left);
		size -= left;
		left = 0;
	}
    if(!left)
    {
        if (aio_r.nblocks--)
        {
            while(aio_error(&aio_r.aiocb) == EINPROGRESS);
            if ((aio_return(&aio_r.aiocb)) < 0)
            {
                perror("aio_return2");
				return NULL;
            }
            if (aio_r.nblocks)
            {
                aio_r.offset += AIO_R_BUF;
                aio_r.aiocb.aio_offset = aio_r.offset;
                aio_r.aiocb.aio_buf = aio_r.bufs[aio_r.bindex];
                aio_r.aiocb.aio_nbytes = AIO_R_BUF;
                if (aio_read(&aio_r.aiocb) < 0)
                {
                    perror("aio_read2");
					return NULL;
                }
            }
        }
		else if(size)
			return NULL;
		aio_r.index = size;
        aio_r.bindex ^= 1;
    }
    uint8_t* pre_ptr = &aio_r.bufs[aio_r.bindex][aio_r.index];
    __builtin_prefetch(pre_ptr);
    __builtin_prefetch(pre_ptr+8);
    __builtin_prefetch(pre_ptr+16);
    __builtin_prefetch(pre_ptr+24);
    return ptr;
}

static inline uint8_t* unread_passwd_data(uint32_t size)
{
	aio_r.index -= size;
}

static inline bool skip2line()
{
	const __m256i nl = _mm256_set1_epi8('\n');
	const __m256i shuffle_mask = _mm256_set_epi8(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
	__m256i line;
	__m256i found;
	uint8_t* ptr;
	
	do
	{
		ptr = read_passwd_data(sizeof(__m256i));
		if(!ptr)
			return false;
		line = _mm256_loadu_si256((__m256i *) ptr);
		found = _mm256_cmpeq_epi8(line, nl);
	} while(_mm256_testz_si256(found, found));
	
	uint32_t cnt = sizeof(__m256i) - 1 - _lzcnt_u32(
		_mm256_movemask_epi8(
			_mm256_permute4x64_epi64(
				_mm256_shuffle_epi8(found, shuffle_mask), 0x4E)));
	unread_passwd_data(cnt);
	return true;
}

bool open_keys_file(char* filename)
{
	bzero(&aio_w, sizeof(aio_w_t));
    aio_w.aiocb.aio_fildes = open(filename, O_CREAT|O_WRONLY|O_TRUNC, 0666);
    if (aio_w.aiocb.aio_fildes < 0)
    {
        printf("Cannot open the output file %s", filename);
        perror("");
        return false;
    }
    
    aio_w.bufs[0] = malloc(AIO_W_BUF);
    aio_w.bufs[1] = malloc(AIO_W_BUF);
    aio_w.kindex = 0;
	aio_w.offset = sizeof(MAGIC_KEYS);
	aio_w.bindex = 0;
	
	aio_w.aiocb.aio_buf = MAGIC_KEYS;	
    aio_w.aiocb.aio_offset = 0;
	aio_w.aiocb.aio_nbytes = sizeof(MAGIC_KEYS);
	if (aio_write(&aio_w.aiocb) < 0)
	{
		printf("Cannot write to the output file %s - ", filename);
		perror("aio_write");
        close_passwd_file();
        return false;
    }
    return true;
}

static inline bool write_keys_file(__m256i ribbon, __m256i index)
{
	uint8_t* ptr = (uint8_t*)&aio_w.bufs[aio_w.bindex][aio_w.kindex++];
	*(__uint128_t*)ptr = (__uint128_t) _mm256_extracti128_si256(ribbon, 1);
	ptr += sizeof(__uint128_t);
    *(uint32_t*)ptr = (uint32_t) _mm256_extract_epi64(index, 3);
	if(aio_w.kindex >= AIO_MAXKEYS)
	{
		while(aio_error(&aio_w.aiocb) == EINPROGRESS);
        if ((aio_return(&aio_w.aiocb)) < 0)
        {
            perror("aio_return2");
			return false;
        }
        aio_w.aiocb.aio_offset = aio_w.offset;
        aio_w.offset += AIO_W_BUF;
        aio_w.aiocb.aio_buf = aio_w.bufs[aio_w.bindex];
        aio_w.aiocb.aio_nbytes = AIO_W_BUF;
        if (aio_write(&aio_w.aiocb) < 0)
        {
            perror("aio_read2");
			return false;
        }
		aio_w.kindex = 0;
		aio_w.bindex ^= 1;
	}
	return true;
}

bool flush_keys_file()
{
	while(aio_error(&aio_w.aiocb) == EINPROGRESS);
    if ((aio_return(&aio_w.aiocb)) < 0)
	{
		perror("aio_return2");
		return false;
	}
    if (aio_w.kindex)
    {
        lseek(aio_w.aiocb.aio_fildes, aio_w.offset, SEEK_SET);
	    int res = write(aio_w.aiocb.aio_fildes, aio_w.bufs[aio_w.bindex], aio_w.kindex*sizeof(ribbon128_key_t));
    }
    return true;
}

void close_keys_file()
{
	if(aio_w.aiocb.aio_fildes <= 0)
		close(aio_w.aiocb.aio_fildes);
	if(aio_w.bufs[0])
		free(aio_w.bufs[0]);
	if(aio_w.bufs[1])
		free(aio_w.bufs[1]);
	bzero(&aio_w, sizeof(aio_w_t));
}

bool preprocess_password_file(char* sourcefile, char* destfile, uint32_t maxlines, bool verify)
{	
	if(!open_passwd_file(sourcefile) || !open_keys_file(destfile))
	{
		close_passwd_file();
		close_keys_file();
		return false;
	}
    uint32_t ignored = 0;
    __m256i ribbon, index;
	while(maxlines)
	{
		uint8_t* ptr = read_passwd_data(sizeof(__m256i));
		if(!ptr)
			break;
		if(!ascii2hex(_mm256_loadu_si256((__m256i *) ptr), &ribbon, verify))
		{
			if(!skip2line())
				break;
            ignored++;
			continue;
		}
		ptr = read_passwd_data(sizeof(uint64_t));
		if(!ptr)
			break;
		if(!ascii2hex(_mm256_set1_epi64x(*(uint64_t*) ptr), &index, verify))
		{
			if(!skip2line())
				break;
            ignored++;
			continue;
		}
		maxlines--;
		write_keys_file(ribbon, index);
		if(!skip2line())
			break;
	}
	flush_keys_file();
	close_passwd_file();
	close_keys_file();
    if (ignored)
        printf("Ignored %d lines due to bad format.\n", ignored);
    return true;
}


#endif