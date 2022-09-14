#ifndef BINARYFUSEFILTER_H
#define BINARYFUSEFILTER_H

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"

#ifndef XOR_MAX_ITERATIONS
#define XOR_MAX_ITERATIONS                                                     \
  100 // probabillity of success should always be > 0.5 so 100 iterations is
      // highly unlikely
#endif

#define MAGIC_FILTER "$binaryfuse8-filter-1.0\n"

/**
 * We start with a few utilities.
 ***/
static inline uint64_t binary_fuse_murmur64(uint64_t h) {
  h ^= h >> 33;
  h *= UINT64_C(0xff51afd7ed558ccd);
  h ^= h >> 33;
  h *= UINT64_C(0xc4ceb9fe1a85ec53);
  h ^= h >> 33;
  return h;
}
static inline uint64_t binary_fuse_mix_split(uint64_t key, uint64_t seed) {
  return binary_fuse_murmur64(key + seed);
}
static inline uint64_t binary_fuse_rotl64(uint64_t n, unsigned int c) {
  return (n << (c & 63)) | (n >> ((-c) & 63));
}
static inline uint32_t binary_fuse_reduce(uint32_t hash, uint32_t n) {
  // http://lemire.me/blog/2016/06/27/a-fast-alternative-to-the-modulo-reduction/
  return (uint32_t)(((uint64_t)hash * n) >> 32);
}
static inline uint64_t binary_fuse8_fingerprint(uint64_t hash) {
  return hash ^ (hash >> 32);
}

/**
 * We need a decent random number generator.
 **/

// returns random number, modifies the seed
static inline uint64_t binary_fuse_rng_splitmix64(uint64_t *seed) {
  uint64_t z = (*seed += UINT64_C(0x9E3779B97F4A7C15));
  z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
  z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
  return z ^ (z >> 31);
}

typedef struct binary_fuse8_s {
  uint64_t Seed;
  uint32_t SegmentLength;
  uint32_t SegmentLengthMask;
  uint32_t SegmentCount;
  uint32_t SegmentCountLength;
  uint32_t ArrayLength;
  uint8_t *Fingerprints;
} binary_fuse8_t;

static binary_fuse8_t filter = {0};

#ifdef _MSC_VER
// Windows programmers who target 32-bit platform may need help:
uint64_t binary_fuse_mulhi(uint64_t a, uint64_t b) { return __umulh(a, b); }
#else
uint64_t binary_fuse_mulhi(uint64_t a, uint64_t b) {
  return ((__uint128_t)a * b) >> 64;
}
#endif

typedef struct binary_hashes_s {
  uint32_t h0;
  uint32_t h1;
  uint32_t h2;
} binary_hashes_t;

static inline binary_hashes_t binary_fuse_hash_batch(uint64_t hash)
{
  uint64_t hi = binary_fuse_mulhi(hash, filter.SegmentCountLength);
  binary_hashes_t ans;
  ans.h0 = (uint32_t)hi;
  ans.h1 = ans.h0 + filter.SegmentLength;
  ans.h2 = ans.h1 + filter.SegmentLength;
  ans.h1 ^= (uint32_t)(hash >> 18) & filter.SegmentLengthMask;
  ans.h2 ^= (uint32_t)(hash)&filter.SegmentLengthMask;
  return ans;
}
static inline uint32_t binary_fuse_hash(int index, uint64_t hash)
{
    uint64_t h = binary_fuse_mulhi(hash, filter.SegmentCountLength);
    h += index * filter.SegmentLength;
    // keep the lower 36 bits
    uint64_t hh = hash & ((1UL << 36) - 1);
    // index 0: right shift by 36; index 1: right shift by 18; index 2: no shift
    h ^= (size_t)((hh >> (36 - 18 * index)) & filter.SegmentLengthMask);
    return h;
}

// Report if the key is in the set, with false positive rate.
static inline bool binary_fuse8_contain(uint64_t key)
{
  uint64_t hash = binary_fuse_mix_split(key, filter.Seed);
  uint8_t f = binary_fuse8_fingerprint(hash);
  binary_hashes_t hashes = binary_fuse_hash_batch(hash);
  f ^= filter.Fingerprints[hashes.h0] ^ filter.Fingerprints[hashes.h1] ^
       filter.Fingerprints[hashes.h2];
  return f == 0;
}

static inline uint32_t binary_fuse8_calculate_segment_length(uint32_t arity,
                                                             uint32_t size) {
  // These parameters are very sensitive. Replacing 'floor' by 'round' can
  // substantially affect the construction time. 
  if (arity == 3) {
    return ((uint32_t)1) << (int)(floor(log((double)(size)) / log(3.33) + 2.25));
  } else if (arity == 4) {
    return ((uint32_t)1) << (int)(floor(log((double)(size)) / log(2.91) - 0.5));
  } else {
    return 65536;
  }
}

double binary_fuse8_max(double a, double b) {
  if (a < b) {
    return b;
  }
  return a;
}

static inline double binary_fuse8_calculate_size_factor(uint32_t arity,
                                                        uint32_t size) {
  if (arity == 3) {
    return binary_fuse8_max(1.125, 0.875 + 0.25 * log(1000000.0) / log((double)size));
  } else if (arity == 4) {
    return binary_fuse8_max(1.075, 0.77 + 0.305 * log(600000.0) / log((double)size));
  } else {
    return 2.0;
  }
}

// allocate enough capacity for a set containing up to 'size' elements
// caller is responsible to call binary_fuse8_free(filter)
static inline bool binary_fuse8_allocate(uint32_t size)
{
  uint32_t arity = 3;
  filter.SegmentLength = binary_fuse8_calculate_segment_length(arity, size);
  if (filter.SegmentLength > 262144) {
    filter.SegmentLength = 262144;
  }
  filter.SegmentLengthMask = filter.SegmentLength - 1;
  double sizeFactor = binary_fuse8_calculate_size_factor(arity, size);
  uint32_t capacity = (uint32_t)(round((double)size * sizeFactor));
  uint32_t initSegmentCount =
      (capacity + filter.SegmentLength - 1) / filter.SegmentLength -
      (arity - 1);
  filter.ArrayLength = (initSegmentCount + arity - 1) * filter.SegmentLength;
  filter.SegmentCount =
      (filter.ArrayLength + filter.SegmentLength - 1) / filter.SegmentLength;
  if (filter.SegmentCount <= arity - 1) {
    filter.SegmentCount = 1;
  } else {
    filter.SegmentCount = filter.SegmentCount - (arity - 1);
  }
  filter.ArrayLength =
      (filter.SegmentCount + arity - 1) * filter.SegmentLength;
  filter.SegmentCountLength = filter.SegmentCount * filter.SegmentLength;
  filter.Fingerprints = (uint8_t*)malloc(filter.ArrayLength);
  return filter.Fingerprints != NULL;
}

// report memory usage
static inline size_t binary_fuse8_size_in_bytes()
{
  return filter.ArrayLength * sizeof(uint8_t) + sizeof(binary_fuse8_t);
}

// release memory
static inline void binary_fuse8_free()
{
  free(filter.Fingerprints);
  filter.Fingerprints = NULL;
  filter.Seed = 0;
  filter.SegmentLength = 0;
  filter.SegmentLengthMask = 0;
  filter.SegmentCount = 0;
  filter.SegmentCountLength = 0;
  filter.ArrayLength = 0;
}

static inline uint8_t binary_fuse8_mod3(uint8_t x) {
    return x > 2 ? x - 3 : x;
}


//-------------------------------------------------------------------------------------------------------

void binaryfuse8_destroy()
{
  binary_fuse8_free();
}

bool binaryfuse8_create(char* filename, uint32_t size)
{
  ribbon128_key_t* key;
  uint32_t maxkeys = calculate_nkeys(filename);
  if(!maxkeys)
    return false;
  if(!size)
    size = maxkeys;
  else
    size = size < maxkeys ? size : maxkeys;
  
  binaryfuse8_destroy();
  binary_fuse8_allocate(size);

  uint64_t rng_counter = 0x726b2b9d438b9d4d;
  filter.Seed = binary_fuse_rng_splitmix64(&rng_counter);
  uint64_t *reverseOrder = (uint64_t *)calloc((size + 1), sizeof(uint64_t));
  uint32_t capacity = filter.ArrayLength;
  uint32_t *alone = (uint32_t *)malloc(capacity * sizeof(uint32_t));
  uint8_t *t2count = (uint8_t *)calloc(capacity, sizeof(uint8_t));
  uint8_t *reverseH = (uint8_t *)malloc(size * sizeof(uint8_t));
  uint64_t *t2hash = (uint64_t *)calloc(capacity, sizeof(uint64_t));

  uint32_t blockBits = 1;
  while (((uint32_t)1 << blockBits) < filter.SegmentCount) {
    blockBits += 1;
  }
  uint32_t block = ((uint32_t)1 << blockBits);
  uint32_t *startPos = (uint32_t *)malloc((1 << blockBits) * sizeof(uint32_t));
  uint32_t h012[5];

  if ((alone == NULL) || (t2count == NULL) || (reverseH == NULL) ||
      (t2hash == NULL) || (reverseOrder == NULL) || (startPos == NULL)) {
    free(alone);
    free(t2count);
    free(reverseH);
    free(t2hash);
    free(reverseOrder);
    free(startPos);
    return false;
  }
  reverseOrder[size] = 1;
  for (int loop = 0; true; ++loop) {
    if (loop + 1 > XOR_MAX_ITERATIONS) {
      fprintf(stderr, "Too many iterations. Are all your keys unique?");
      free(alone);
      free(t2count);
      free(reverseH);
      free(t2hash);
      free(reverseOrder);
      free(startPos);
      return false;
    }

    for (uint32_t i = 0; i < block; i++) {
      // important : i * size would overflow as a 32-bit number in some
      // cases.
      startPos[i] = ((uint64_t)i * size) >> blockBits;
    }

    uint64_t maskblock = block - 1; 
    if (!open_keys_file(filename, &size))
      return false;
    while((key = read_key()) != NULL) {
      uint64_t hash = binary_fuse_murmur64((uint64_t)key->ribbon + filter.Seed);
      uint64_t segment_index = hash >> (64 - blockBits);
      while (reverseOrder[startPos[segment_index]] != 0) {
        segment_index++;
        segment_index &= maskblock;
      }
      reverseOrder[startPos[segment_index]] = hash;
      startPos[segment_index]++;
    }
    close_keys_file();

    int error = 0;
    for (uint32_t i = 0; i < size; i++) {
      uint64_t hash = reverseOrder[i];
      uint32_t h0 = binary_fuse_hash(0, hash);
      t2count[h0] += 4;
      t2hash[h0] ^= hash;
      uint32_t h1= binary_fuse_hash(1, hash);
      t2count[h1] += 4;
      t2count[h1] ^= 1;
      t2hash[h1] ^= hash;
      uint32_t h2 = binary_fuse_hash(2, hash);
      t2count[h2] += 4;
      t2hash[h2] ^= hash;
      t2count[h2] ^= 2;
      error = (t2count[h0] < 4) ? 1 : error;
      error = (t2count[h1] < 4) ? 1 : error;
      error = (t2count[h2] < 4) ? 1 : error;
    }
    if(error) { continue; }

    // End of key addition
    uint32_t Qsize = 0;
    // Add sets with one key to the queue.
    for (uint32_t i = 0; i < capacity; i++) {
      alone[Qsize] = i;
      Qsize += ((t2count[i] >> 2) == 1) ? 1 : 0;
    }
    uint32_t stacksize = 0;
    while (Qsize > 0) {
      Qsize--;
      uint32_t index = alone[Qsize];
      if ((t2count[index] >> 2) == 1) {
        uint64_t hash = t2hash[index];

        //h012[0] = binary_fuse_hash(0, hash, filter);
        h012[1] = binary_fuse_hash(1, hash);
        h012[2] = binary_fuse_hash(2, hash);
        h012[3] = binary_fuse_hash(0, hash); // == h012[0];
        h012[4] = h012[1];
        uint8_t found = t2count[index] & 3;
        reverseH[stacksize] = found;
        reverseOrder[stacksize] = hash;
        stacksize++;
        uint32_t other_index1 = h012[found + 1];
        alone[Qsize] = other_index1;
        Qsize += ((t2count[other_index1] >> 2) == 2 ? 1 : 0);

        t2count[other_index1] -= 4;
        t2count[other_index1] ^= binary_fuse8_mod3(found + 1); 
        t2hash[other_index1] ^= hash;

        uint32_t other_index2 = h012[found + 2];
        alone[Qsize] = other_index2;
        Qsize += ((t2count[other_index2] >> 2) == 2 ? 1 : 0);
        t2count[other_index2] -= 4;
        t2count[other_index2] ^= binary_fuse8_mod3(found + 2);
        t2hash[other_index2] ^= hash;
      }
    }
    if (stacksize == size) {
      // success
      break;
    }
    memset(reverseOrder, 0, sizeof(uint64_t[size]));
    memset(t2count, 0, sizeof(uint8_t[capacity]));
    memset(t2hash, 0, sizeof(uint64_t[capacity]));
    filter.Seed = binary_fuse_rng_splitmix64(&rng_counter);
  }

  for (uint32_t i = size - 1; i < size; i--) {
    // the hash of the key we insert next
    uint64_t hash = reverseOrder[i];
    uint8_t xor2 = binary_fuse8_fingerprint(hash);
    uint8_t found = reverseH[i];
    h012[0] = binary_fuse_hash(0, hash);
    h012[1] = binary_fuse_hash(1, hash);
    h012[2] = binary_fuse_hash(2, hash);
    h012[3] = h012[0];
    h012[4] = h012[1];
    filter.Fingerprints[h012[found]] = xor2 ^
                                        filter.Fingerprints[h012[found + 1]] ^
                                        filter.Fingerprints[h012[found + 2]];
  }
  free(alone);
  free(t2count);
  free(reverseH);
  free(t2hash);
  free(reverseOrder);
  free(startPos);
  return true;
}

bool binaryfuse8_exist()
{
  return filter.Fingerprints != NULL;
}

bool binaryfuse8_sanity(char* filename, uint32_t maxkeys)
{
    assert(binaryfuse8_exist());
    ribbon128_key_t* key;
    if (!open_keys_file(filename, &maxkeys))
        return false;
    while((key = read_key()) != NULL)
    {
        if (!binary_fuse8_contain((uint64_t)key->ribbon))
        {
            printf("Sanity check failed.\n");
            return false;
        }
    }
    close_keys_file();
    return true;
}

uint32_t binaryfuse8_fp(uint32_t n)
{
    assert(binaryfuse8_exist());
    uint32_t matches = 0;
    init_shishua(clock());
    ribbon128_key_t randomkey; 

    while(n--)
    {
        random_ribbon_key(&randomkey);
        if(binary_fuse8_contain((uint64_t)randomkey.ribbon))
            matches++;
    }
    return matches;
}

bool binaryfuse8_query(char* pass, bool hashed)
{
  assert(binaryfuse8_exist());
  ribbon128_key_t key;
  if((hashed && hash2key(pass, &key)) || password2key(pass, &key))
    return binary_fuse8_contain((uint64_t)key.ribbon);
  else
    return false;
}

bool binaryfuse8_save(char* filename)
{
  assert(binaryfuse8_exist());
  FILE* fp = fopen(filename, "wb");
  if (fp == NULL)
  {
    printf("Cannot open the output file %s.", filename);
    return false;
  }
  if (!fwrite(MAGIC_FILTER, sizeof(MAGIC_FILTER), 1, fp)
      || !fwrite(&filter.Seed, sizeof(filter.Seed), 1, fp)
      || !fwrite(&filter.SegmentLength, sizeof(filter.SegmentLength), 1, fp)
      || !fwrite(&filter.SegmentLengthMask, sizeof(filter.SegmentLengthMask), 1, fp)
      || !fwrite(&filter.SegmentCount, sizeof(filter.SegmentCount), 1, fp)
      || !fwrite(&filter.SegmentCountLength, sizeof(filter.SegmentCountLength), 1, fp)
      || !fwrite(&filter.ArrayLength, sizeof(filter.ArrayLength), 1, fp)
      || !fwrite(filter.Fingerprints, sizeof(uint8_t)*filter.ArrayLength, 1, fp))
  {
    perror("Error when writing into file");
    fclose(fp);
    return false;
  }
  fclose(fp);
  return true;
}

bool binaryfuse8_load(char* filename, uint32_t size)
{
  FILE* fp = fopen(filename, "rb");
  if (fp == NULL)
  {
    printf("Cannot open the input file %s.", filename);
    return false;
  }
  if (filter.Fingerprints != NULL)
      binaryfuse8_destroy();

  char magic[sizeof(MAGIC_FILTER)];
  binary_fuse8_allocate(size);
  uint32_t expected_ArrayLength;
  if (!fread(magic, sizeof(MAGIC_FILTER), 1, fp) 
      || strcmp(magic, MAGIC_FILTER)
      || !fread(&filter.Seed, sizeof(filter.Seed), 1, fp)
      || !fread(&filter.SegmentLength, sizeof(filter.SegmentLength), 1, fp)
      || !fread(&filter.SegmentLengthMask, sizeof(filter.SegmentLengthMask), 1, fp)
      || !fread(&filter.SegmentCount, sizeof(filter.SegmentCount), 1, fp)
      || !fread(&filter.SegmentCountLength, sizeof(filter.SegmentCountLength), 1, fp)
      || !fread(&expected_ArrayLength, sizeof(filter.ArrayLength), 1, fp)
      || (size != 0 && filter.ArrayLength != expected_ArrayLength))
  {
    perror("Error when reading file");
    fclose(fp);
    return false;
  }
  
  filter.ArrayLength = expected_ArrayLength;
  filter.Fingerprints = (uint8_t*)malloc(filter.ArrayLength);
  if (!fread(filter.Fingerprints, sizeof(uint8_t)*filter.ArrayLength, 1, fp))
  {
    perror("Error when reading file");
    fclose(fp);
    return false;
  }
  fclose(fp);
  return true;
}


#endif
