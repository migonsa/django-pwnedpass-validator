#ifndef SBBLOOMFILTER_H
#define SBBLOOMFILTER_H

#include <immintrin.h>
#include <stdint.h>

#include "utils.h"

#define MAGIC_FILTER "$splitblockbloom-filter-1.0\n"


typedef struct splitblockbloom {
    uint32_t num_buckets;
    __m256i* fingerprints;
} splitblockbloom_t;

static splitblockbloom_t filter = {0};

// Take a hash value and get the block to access within a filter with
// num_buckets buckets.
static inline uint64_t block_index(const uint64_t hash) {
    return ((hash >> 32) * filter.num_buckets) >> 32;
}

// Takes a hash value and creates a mask with one bit set in each 32-bit lane.
// These are the bits to set or check when accessing the block.
static inline __m256i make_mask(uint32_t hash) {
    const __m256i ones = _mm256_set1_epi32(1);
    // Set eight odd constants for multiply-shift hashing
    const __m256i rehash = {INT64_C(0x47b6137b) << 32 | 0x44974d91,
    INT64_C(0x8824ad5b) << 32 | 0xa2b7289d,
    INT64_C(0x705495c7) << 32 | 0x2df1424b,
    INT64_C(0x9efc4947) << 32 | 0x5c6bfb31};
    __m256i hash_data = _mm256_set1_epi32(hash);
    hash_data = _mm256_mullo_epi32(rehash, hash_data);
    // Shift all data right, reducing the hash values from 32 bits to five bits.
    // Those five bits represent an index in [0, 31)
    hash_data = _mm256_srli_epi32(hash_data, 32 - 5);
    // Set a bit in each lane based on using the [0, 32) data as shift values.
    return _mm256_sllv_epi32(ones, hash_data);
}

static inline void add_hash(uint64_t hash) {
    const uint64_t bucket_idx = block_index(hash);
    const __m256i mask = make_mask(hash);
    __m256i *bucket = &filter.fingerprints[bucket_idx];
    // or the mask into the existing bucket
    _mm256_store_si256(bucket, _mm256_or_si256(*bucket, mask));
}

static inline bool find_hash(uint64_t hash) {
    const uint64_t bucket_idx = block_index(hash);
    const __m256i mask = make_mask(hash);
    const __m256i *bucket = &filter.fingerprints[bucket_idx];
    // checks if all the bits in mask are also set in *bucket. Scalar
    // equivalent: (~bucket & mask) == 0
    return _mm256_testc_si256(*bucket, mask);
}


//-------------------------------------------------------------------------------------------------------

void splitblockbloom_destroy()
{
  free(filter.fingerprints);
  filter.num_buckets = 0;
  filter.fingerprints = NULL;
}

bool splitblockbloom_create(char* filename, uint32_t maxkeys, double oversize)
{
  ribbon128_key_t* key;
  if (!open_keys_file(filename, &maxkeys))
    return false;
  splitblockbloom_destroy();

	filter.num_buckets = (uint32_t)(maxkeys * (oversize/32.));
  filter.fingerprints = (__m256i*)aligned_alloc(sizeof(__m256i), filter.num_buckets*sizeof(__m256i));
  bzero(filter.fingerprints, filter.num_buckets*sizeof(__m256i));

  while((key = read_key()) != NULL)
  {
    add_hash((uint64_t) key->ribbon);
  } 
  close_keys_file();
  return true;
}

bool splitblockbloom_exist()
{
  return filter.fingerprints != NULL;
}

bool splitblockbloom_sanity(char* filename, uint32_t maxkeys)
{
    assert(splitblockbloom_exist());
    ribbon128_key_t* key;
    if (!open_keys_file(filename, &maxkeys))
        return false;
    while((key = read_key()) != NULL)
    {
        if (!find_hash((uint64_t)key->ribbon))
        {
            printf("Sanity check failed.\n");
            return false;
        }
    }
    close_keys_file();
    return true;
}

uint32_t splitblockbloom_fp(uint32_t n)
{
    assert(splitblockbloom_exist());
    uint32_t matches = 0;
    init_shishua(clock());
    ribbon128_key_t randomkey; 

    while(n--)
    {
      random_ribbon_key(&randomkey);
      if(find_hash((uint64_t)randomkey.ribbon))
          matches++;
    }
    return matches;
}

bool splitblockbloom_query(char* pass, bool hashed)
{
  assert(splitblockbloom_exist());
  ribbon128_key_t key;
  if((hashed && hash2key(pass, &key)) || password2key(pass, &key))
    return find_hash((uint64_t)key.ribbon);
  else
    return false;
}


bool splitblockbloom_save(char* filename)
{
  assert(splitblockbloom_exist());
  FILE* fp = fopen(filename, "wb");
  if (fp == NULL)
  {
    printf("Cannot open the output file %s.", filename);
    return false;
  }
  if (!fwrite(MAGIC_FILTER, sizeof(MAGIC_FILTER), 1, fp)
      || !fwrite(&filter.num_buckets, sizeof(filter.num_buckets), 1, fp)
      || !fwrite(filter.fingerprints, sizeof(__m256i)*filter.num_buckets, 1, fp))
  {
    perror("Error when writing into file");
    fclose(fp);
    return false;
  }
  fclose(fp);
  return true;
}

bool splitblockbloom_load(char* filename, uint32_t maxkeys, double oversize)
{
  FILE* fp = fopen(filename, "rb");
  if (fp == NULL)
  {
    printf("Cannot open the input file %s.", filename);
    return false;
  }
  if (filter.fingerprints != NULL)
    splitblockbloom_destroy();
  
  char magic[sizeof(MAGIC_FILTER)];
  uint32_t expected_num_buckets = (uint32_t)(maxkeys * (oversize/32.));
  if (!fread(magic, sizeof(MAGIC_FILTER), 1, fp) 
      || strcmp(magic, MAGIC_FILTER)
      || !fread(&filter.num_buckets, sizeof(filter.num_buckets), 1, fp)
      || (maxkeys != 0 && filter.num_buckets != expected_num_buckets))
  {
    perror("Error when reading file");
    fclose(fp);
    return false;
  }

  filter.fingerprints = (__m256i*)aligned_alloc(sizeof(__m256i), filter.num_buckets*sizeof(__m256i));
  bzero(filter.fingerprints, filter.num_buckets*sizeof(__m256i));
  if (!fread(filter.fingerprints, sizeof(__m256i)*filter.num_buckets, 1, fp))
  {
    perror("Error when reading file");
    fclose(fp);
    return false;
  }
  fclose(fp);
  return true;
}

#endif