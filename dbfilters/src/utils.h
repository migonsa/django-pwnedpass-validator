#ifndef UTILS_H
#define UTILS_H

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
#include <sys/stat.h>

#include "shishua.h"
#include "sha1.h"
#include "hex_avx2.h"

#if !defined(__AVX2__)
#error AVX2 required: Compile with "-mavx2" option.
#endif

#define MAGIC_KEYS "$ribbon128-keys-1.0\n"
#define SHISHUA_BUF (128)
#define AIO_MAXKEYS (4096)
#define AIO_BUF (AIO_MAXKEYS*sizeof(ribbon128_key_t))


typedef struct __attribute__((__packed__))
{
    __uint128_t ribbon;
    uint32_t index;
} ribbon128_key_t;

typedef struct
{
    prng_state s;
    uint8_t buf[SHISHUA_BUF];
    uint8_t index;
} shishua_t;

typedef struct
{
    struct aiocb aiocb;
    ribbon128_key_t* bufs[2];
    uint32_t bindex;
    uint32_t kindex;
    uint64_t offset;
    int32_t nblocks;
    
} aio_t;

static shishua_t shishua = {0};
static aio_t aio = {0};


void init_shishua(uint64_t s)
{
    uint64_t seed[4];
    seed[0] = seed[1] = seed[2] = seed[3] = s;
    prng_init(&shishua.s, seed);
    prng_gen(&shishua.s, shishua.buf, sizeof(shishua.buf));
    shishua.index = 0;
}

static inline uint8_t get8_shishua()
{
    uint8_t res = shishua.buf[shishua.index++];
    if (shishua.index >= SHISHUA_BUF)
    {
        prng_gen(&shishua.s, shishua.buf, sizeof(shishua.buf));
        shishua.index = 0;
    }
    return res;
}

static inline uint16_t get16_shishua()
{
    uint16_t res;
    if (SHISHUA_BUF - shishua.index < 2)
    {
        prng_gen(&shishua.s, shishua.buf, sizeof(shishua.buf));
        shishua.index = 0;
    }
    res = *(uint16_t*)&shishua.buf[shishua.index];
    shishua.index += 2;
    return res;
}

static inline void fill_shishua(uint8_t* buf, uint8_t size)
{
    uint8_t left = SHISHUA_BUF - shishua.index;
    if (size > left)
    {
        memcpy(buf, &shishua.buf[shishua.index], left);
        prng_gen(&shishua.s, shishua.buf, sizeof(shishua.buf));
        memcpy(&buf[left], shishua.buf, size-left);
        shishua.index = size-left;
    }
    else
    {
        memcpy(buf, &shishua.buf[shishua.index], size);
        shishua.index += size;
    }
    if (shishua.index >= SHISHUA_BUF)
    {
        prng_gen(&shishua.s, shishua.buf, sizeof(shishua.buf));
        shishua.index = 0;
    }
}

static inline void random_ribbon_key(ribbon128_key_t* key)
{
    fill_shishua((uint8_t*) key, (int8_t) sizeof(ribbon128_key_t));
}

bool password2key(char* pass, ribbon128_key_t* key)
{
    sha1_hash_buffer(key, pass, strlen(pass));
    return true;
}

bool password2hash(char* pass, char* hash)
{
    ribbon128_key_t key;
    password2key(pass, &key);
    int i;
    for(i = 0; i<sizeof(key); i++)
    {
        sprintf(&hash[2*i], "%02x", ((uint8_t*)&key)[i]);
    }
    //hash[2*i] = 0;
    //sprintf(hash, "%016lx%016lx%08x", (uint64_t)(key.ribbon>>64), (uint64_t)key.ribbon, (uint32_t)key.index);
    return true;
}

bool hash2key(char* hash, ribbon128_key_t* key)
{
    __m256i res1, res2;

    if (!ascii2hex(_mm256_loadu_si256((__m256i*) hash), &res1, true)
            || !ascii2hex(_mm256_set1_epi64x(*(uint64_t*) (hash + sizeof(__m256i))), &res2, true))
    {
        return false;
    }
    key->ribbon = (__uint128_t) _mm256_extracti128_si256(res1, 1);
    key->index = (uint32_t) _mm256_extract_epi64(res2, 3);
}

bool synthetic(char* destfile, uint32_t nkeys)
{
    FILE* fp = fopen(destfile, "wb");
    if (fp == NULL)
    {
        printf("Cannot open the output file %s.", destfile);
        return false;
    }
    ribbon128_key_t key;
    fwrite(MAGIC_KEYS, sizeof(MAGIC_KEYS),1, fp);
    init_shishua(clock());
    while(nkeys--)
    {
        random_ribbon_key(&key);
        fwrite(&key, sizeof(ribbon128_key_t), 1, fp);
    }
    fclose(fp);
    return true;
}

uint32_t calculate_nkeys(char* filename)
{
    struct stat st;
    if (stat(filename, &st) < 0)
    {
        printf("Invalid input file %s", filename);
        perror("");
        return 0;
    }
    return (st.st_size-sizeof(MAGIC_KEYS))/sizeof(ribbon128_key_t);
}

bool open_keys_file(char* filename, uint32_t* maxkeys)
{
    char magic[sizeof(MAGIC_KEYS)];
    int fd = open(filename, O_RDONLY);
    if (fd < 0)
    {
        printf("Cannot open the input file %s", filename);
        perror("");
        return false;
    }
    if((read(fd, magic, sizeof(MAGIC_KEYS)) != sizeof(MAGIC_KEYS)) || strcmp(magic, MAGIC_KEYS))
    {
        printf("Invalid input file %s", filename);
        perror("");
        close(fd);
        return false;
    }
    
    uint32_t filekeys = (lseek(fd, 0L, SEEK_END)-sizeof(MAGIC_KEYS))/sizeof(ribbon128_key_t);
    if (!*maxkeys || *maxkeys > filekeys)
        *maxkeys = filekeys;
    if (!*maxkeys)
    {
        printf("Empty input file %s", filename);
        close(fd);
        return false;
    }
    
    bzero(&aio, sizeof(aio_t));
    aio.bufs[0] = malloc(AIO_BUF);
    aio.bufs[1] = malloc(AIO_BUF);
    aio.nblocks = *maxkeys/AIO_MAXKEYS;
    aio.kindex = AIO_MAXKEYS - (*maxkeys - aio.nblocks*AIO_MAXKEYS);
    if (aio.kindex != AIO_MAXKEYS)
    {
        aio.aiocb.aio_buf = &aio.bufs[0][aio.kindex];
        aio.aiocb.aio_nbytes = (AIO_MAXKEYS-aio.kindex)*sizeof(ribbon128_key_t);
    }
    else
    {
        aio.nblocks--;
        aio.aiocb.aio_buf = aio.bufs[0];
        aio.aiocb.aio_nbytes = AIO_BUF;
    }
    aio.offset = sizeof(MAGIC_KEYS);
    aio.aiocb.aio_offset = aio.offset;
    aio.aiocb.aio_fildes = fd;
    if (aio_read(&aio.aiocb) < 0)
    {
        perror("aio_read");
        close(fd);
        return false;
    }
    while (aio_error(&aio.aiocb) == EINPROGRESS);
    if ((aio_return(&aio.aiocb)) < 0)
    {
        perror("aio_return");
        close(fd);
        return false;
    }
    if (aio.nblocks)
    {
        aio.aiocb.aio_buf = aio.bufs[1];
        aio.offset += aio.aiocb.aio_nbytes;
        aio.aiocb.aio_nbytes = AIO_BUF;
        aio.aiocb.aio_offset = aio.offset;
        if (aio_read(&aio.aiocb) < 0)
        {
            perror("aio_read");
            close(fd);
            return false;
        }
    }
    return true;
}

void close_keys_file()
{
    close(aio.aiocb.aio_fildes);
    free(aio.bufs[0]);
    free(aio.bufs[1]);
}

ribbon128_key_t* read_key()
{
    if (aio.nblocks < 0)
        return NULL;
    ribbon128_key_t* key = &aio.bufs[aio.bindex][aio.kindex++];
    if(aio.kindex >= AIO_MAXKEYS)
    {
        aio.kindex = 0;
        if (aio.nblocks--)
        {
            while(aio_error(&aio.aiocb) == EINPROGRESS);
            if ((aio_return(&aio.aiocb)) < 0)
            {
                perror("aio_return2");
            }
            if (aio.nblocks)
            {
                aio.offset += AIO_BUF;
                aio.aiocb.aio_offset = aio.offset;
                aio.aiocb.aio_buf = aio.bufs[aio.bindex];
                aio.aiocb.aio_nbytes = AIO_BUF;
                if (aio_read(&aio.aiocb) < 0)
                {
                    perror("aio_read2");
                }
            }
            aio.bindex ^= 1;
        }
    }
    return key;
}


#endif