// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
// Author: 2014, Tim Janik, see http://testbit.org/keccak
#include "randomhash.hh"
#include "entropy.hh"
#include "bse/internal.hh"

namespace Bse {

// == Lib::KeccakF1600 ==
Lib::KeccakF1600::KeccakF1600()
{
  reset();
}

void
Lib::KeccakF1600::reset ()
{
  memset4 ((uint32*) bytes, 0, sizeof (bytes) / 4);
}

static constexpr const uint8_t KECCAK_RHO_OFFSETS[25] = { 0, 1, 62, 28, 27, 36, 44, 6, 55, 20, 3, 10, 43,
                                                          25, 39, 41, 45, 15, 21, 8, 18, 2, 61, 56, 14 };
static constexpr const uint64_t KECCAK_ROUND_CONSTANTS[255] = {
  1, 32898, 0x800000000000808a, 0x8000000080008000, 32907, 0x80000001, 0x8000000080008081, 0x8000000000008009, 138, 136, 0x80008009,
  0x8000000a, 0x8000808b, 0x800000000000008b, 0x8000000000008089, 0x8000000000008003, 0x8000000000008002, 0x8000000000000080, 32778,
  0x800000008000000a, 0x8000000080008081, 0x8000000000008080, 0x80000001, 0x8000000080008008, 0x8000000080008082, 0x800000008000800a,
  0x8000000000000003, 0x8000000080000009, 0x8000000000008082, 32777, 0x8000000000000080, 32899, 0x8000000000000081, 1, 32779,
  0x8000000080008001, 128, 0x8000000000008000, 0x8000000080008001, 9, 0x800000008000808b, 129, 0x8000000000000082, 0x8000008b,
  0x8000000080008009, 0x8000000080000000, 0x80000080, 0x80008003, 0x8000000080008082, 0x8000000080008083, 0x8000000080000088, 32905,
  32777, 0x8000000000000009, 0x80008008, 0x80008001, 0x800000000000008a, 0x800000000000000b, 137, 0x80000002, 0x800000000000800b,
  0x8000800b, 32907, 0x80000088, 0x800000000000800a, 0x80000089, 0x8000000000000001, 0x8000000000008088, 0x8000000000000081, 136,
  0x80008080, 129, 0x800000000000000b, 0, 137, 0x8000008b, 0x8000000080008080, 0x800000000000008b, 0x8000000000008000,
  0x8000000080008088, 0x80000082, 11, 0x800000000000000a, 32898, 0x8000000000008003, 0x800000000000808b, 0x800000008000000b,
  0x800000008000008a, 0x80000081, 0x80000081, 0x80000008, 131, 0x8000000080008003, 0x80008088, 0x8000000080000088, 32768, 0x80008082,
  0x80008089, 0x8000000080008083, 0x8000000080000001, 0x80008002, 0x8000000080000089, 130, 0x8000000080000008, 0x8000000000000089,
  0x8000000080000008, 0x8000000000000000, 0x8000000000000083, 0x80008080, 8, 0x8000000080000080, 0x8000000080008080,
  0x8000000000000002, 0x800000008000808b, 8, 0x8000000080000009, 0x800000000000800b, 0x80008082, 0x80008000, 0x8000000000008008, 32897,
  0x8000000080008089, 0x80008089, 0x800000008000800a, 0x800000000000008a, 0x8000000000000082, 0x80000002, 0x8000000000008082, 32896,
  0x800000008000000b, 0x8000000080000003, 10, 0x8000000000008001, 0x8000000080000083, 0x8000000000008083, 139, 32778,
  0x8000000080000083, 0x800000000000800a, 0x80000000, 0x800000008000008a, 0x80000008, 10, 0x8000000000008088, 0x8000000000000008,
  0x80000003, 0x8000000000000000, 0x800000000000000a, 32779, 0x8000000080008088, 0x8000000b, 0x80000080, 0x8000808a,
  0x8000000000008009, 3, 0x80000003, 0x8000000000000089, 0x8000000080000081, 0x800000008000008b, 0x80008003, 0x800000008000800b,
  0x8000000000008008, 32776, 0x8000000000008002, 0x8000000000000009, 0x80008081, 32906, 0x8000800a, 128, 0x8000000000008089,
  0x800000000000808a, 0x8000000080008089, 0x80008000, 0x8000000000008081, 0x8000800a, 9, 0x8000000080008002, 0x8000000a, 0x80008002,
  0x8000000080000000, 0x80000009, 32904, 2, 0x80008008, 0x80008088, 0x8000000080000001, 0x8000808b, 0x8000000000000002,
  0x8000000080008002, 0x80000083, 32905, 32896, 0x8000000080000082, 0x8000000000000088, 0x800000008000808a, 32906, 0x80008083,
  0x8000000b, 0x80000009, 32769, 0x80000089, 0x8000000000000088, 0x8000000080008003, 0x80008001, 0x8000000000000003,
  0x8000000080000080, 0x8000000080008009, 0x8000000080000089, 11, 0x8000000000000083, 0x80008009, 0x80000083, 32768, 0x8000800b, 32770,
  3, 0x8000008a, 0x8000000080000002, 32769, 0x80000000, 0x8000000080000003, 131, 0x800000008000808a, 32771, 32776, 0x800000000000808b,
  0x8000000080000082, 0x8000000000000001, 0x8000000000008001, 0x800000008000000a, 0x8000000080008008, 0x800000008000800b,
  0x8000000000008081, 0x80008083, 0x80000082, 130, 0x8000000080000081, 0x8000000080000002, 32904, 139, 32899, 0x8000000000000008,
  0x8000008a, 0x800000008000008b, 0x8000808a, 0x8000000000008080, 0x80000088, 0x8000000000008083, 2, 0x80008081, 32771, 32897,
  0x8000000080008000, 32770, 138,
};

/// Rotate left for uint64_t.
static inline uint64_t
bit_rotate64 (uint64_t bits, unsigned int offset)
{
  // bitwise rotate-left pattern recognized by gcc & clang iff 64==sizeof (bits)
#if defined (__x86_64__) && 0           // rolq tends to be slower than C++ with g++ and clang++
  __asm__ ("rolq %%cl, %0"
           : "=r" (bits)                // out
           : "0" (bits), "c" (offset)   // in
           :                            // clobber
           );
#else
  bits = UNLIKELY (offset == 0) ? bits : (bits << offset) | (bits >> (64 - offset));
#endif
  return bits;
}

/// The Keccak-f[1600] permutation for up to 254 rounds, see Keccak11 @cite Keccak11 .
void
Lib::KeccakF1600::permute (const uint32_t n_rounds)
{
  BSE_ASSERT_RETURN (n_rounds < 255); // adjust the KECCAK_ROUND_CONSTANTS access to lift this assertion
  // Keccak forward rounds
  for (size_t round_index = 0; round_index < n_rounds; round_index++)
    {
      // theta
      uint64_t C[5];
      for (size_t x = 0; x < 5; x++)
        {
          C[x] = A[x];
          for (size_t y = 1; y < 5; y++)
            C[x] ^= A[x + 5 * y];
        }
      for (size_t x = 0; x < 5; x++)
        {
          const uint64_t D = C[(5 + x - 1) % 5] ^ bit_rotate64 (C[(x + 1) % 5], 1);
          for (size_t y = 0; y < 5; y++)
            A[x + 5 * y] ^= D;
        }
      // rho
      for (size_t y = 0; y < 25; y += 5)
        {
          uint64_t *const plane = &A[y];
          for (size_t x = 0; x < 5; x++)
            plane[x] = bit_rotate64 (plane[x], KECCAK_RHO_OFFSETS[x + y]);
        }
      // pi
      const uint64_t a[25] = { A[0], A[1], A[2], A[3], A[4], A[5], A[6], A[7], A[8], A[9], A[10], A[11], A[12],
                               A[13], A[14], A[15], A[16], A[17], A[18], A[19], A[20], A[21], A[22], A[23], A[24] };
      for (size_t y = 0; y < 5; y++)
        for (size_t x = 0; x < 5; x++)
          {
            const size_t X = (0 * x + 1 * y) % 5;
            const size_t Y = (2 * x + 3 * y) % 5;
            A[X + 5 * Y] = a[x + 5 * y];
          }
      // chi
      for (size_t y = 0; y < 25; y += 5)
        {
          uint64_t *const plane = &A[y];
          for (size_t x = 0; x < 5; x++)
            C[x] = plane[x] ^ (~plane[(x + 1) % 5] & plane[(x + 2) % 5]);
          for (size_t x = 0; x < 5; x++)
            plane[x] = C[x];
        }
      // iota
      A[0 + 5 * 0] ^= KECCAK_ROUND_CONSTANTS[round_index]; // round_index needs %255 for n_rounds>=255
    }
}

// == KeccakRng ==
/// Keccak permutation for 1600 bits, see Keccak11 @cite Keccak11 .
void
KeccakRng::permute1600()
{
  state_.permute (n_rounds_);
  opos_ = 0;    // fresh outputs available
}

/** Discard 2^256 bits of the current generator state.
 * This makes it practically infeasible to guess previous generator states or
 * deduce generated values from the past.
 * Use this for forward security @cite Security03 of generated security tokens like session keys.
 */
void
KeccakRng::forget()
{
  std::fill (&state_[0], &state_[256 / 64], 0);
  permute1600();
}

/** Discard @a count consecutive random values.
 * This function is slightly faster than calling operator()() exactly @a count times.
 */
void
KeccakRng::discard (unsigned long long count)
{
  while (count)
    {
      if (opos_ >= n_nums())
        permute1600();
      const unsigned long long ull = std::min ((unsigned long long) n_nums() - opos_, count);
      opos_ += ull;
      count -= ull;
    }
}

/** Incorporate @a seed_values into the current generator state.
 * A block permutation to advance the generator state is carried out per n_nums() seed values.
 * After calling this function, generating the next n_nums() random values will not need to
 * block for a new permutation.
 */
void
KeccakRng::xor_seed (const uint64_t *seeds, size_t n_seeds)
{
  // printerr ("xor_seed(%p): %s\n", this, String ((const char*) seeds, n_seeds * 8));
  size_t i = 0;
  while (n_seeds)
    {
      if (i > 0)        // not first seed block
        permute1600();  // prepare for next seed block
      const size_t count = std::min (n_nums(), n_seeds);
      for (i = 0; i < count; i++)
        state_[i] ^= seeds[i];
      seeds += count;
      n_seeds -= count;
    }
  if (i < 25)           // apply Simple padding: pad10*
    state_[i] ^= 0x1;   // 1: payload boundary bit for Keccak Sponge compliant padding
  permute1600();        // integrate seed
}

void
KeccakRng::auto_seed()
{
  AutoSeeder seeder;
  seed (seeder);
}

/// The destructor resets the generator state to avoid leaving memory trails.
KeccakRng::~KeccakRng()
{
  // forget all state and leave no trails
  std::fill (&state_[0], &state_[25], 0xaffeaffeaffeaffe);
  forget();
  opos_ = n_nums();
}

// == SHAKE_Base - primitive for SHA3 hashing ==
template<size_t HASHBITS, uint8_t DOMAINBITS>
class SHAKE_Base {
  Lib::KeccakF1600      state_;
  const size_t          rate_;
  size_t                iopos_;
  bool                  feeding_mode_;
protected:
  /// Add stream data up to block size into Keccak state via XOR.
  size_t
  xor_state (size_t offset, const uint8_t *input, size_t n_in)
  {
    BSE_ASSERT_RETURN (offset + n_in <= byte_rate(), 0);
    size_t i;
    for (i = offset; i < offset + n_in; i++)
      state_.byte (i) ^= input[i - offset];
    return i - offset;
  }
  /** Pad stream from offset to block boundary into Keccak state via XOR.
   * The @a trail argument must contain the termination bit, optionally preceeded by
   * additional (LSB) bits for domain separation. A permutation is carried out if the
   * trailing padding bits do not fit into the remaining block length.
   */
  void
  absorb_padding (size_t offset, uint8_t trail = 0x01)
  {
    BSE_ASSERT_RETURN (offset < byte_rate());   // offset is first byte following payload
    BSE_ASSERT_RETURN (trail != 0x00);
    // MultiRatePadding
    state_.byte (offset) ^= trail;              // 1: payload boundary bit for MultiRatePadding (and SimplePadding)
    // state_.bits[i..last] ^= 0x0;             // 0*: bitrate filler for MultiRatePadding (and SimplePadding)
    const size_t lastbyte = byte_rate() - 1;    // last bitrate byte, may coincide with offset
    if (offset == lastbyte && trail >= 0x80)
      state_.permute (24);                      // prepare new block to append last bit
    state_.byte (lastbyte) ^= 0x80;             // 1: last bitrate bit; required by MultiRatePadding
  }
  SHAKE_Base (size_t rate) :
    rate_ (rate), iopos_ (0), feeding_mode_ (true)
  {}
public:
  size_t
  byte_rate() const
  {
    return rate_ / 8;
  }
  void
  reset()
  {
    state_.reset();
    iopos_ = 0;
    feeding_mode_ = true;
  }
  void
  update (const uint8_t *data, size_t length)
  {
    BSE_ASSERT_RETURN (feeding_mode_ == true);
    while (length)
      {
        const size_t count = std::min (byte_rate() - iopos_, length);
        xor_state (iopos_, data, count);
        iopos_ += count;
        data += count;
        length -= count;
        if (iopos_ >= byte_rate())
          {
            state_.permute (24);
            iopos_ = 0;
          }
      }
  }
  /// Switch from absorbing into squeezing mode and return digest.
  size_t
  get_hash (uint8_t hashvalue[HASHBITS / 8])
  {
    if (feeding_mode_)
      {
        const uint8_t shake_delimiter = DOMAINBITS;
        absorb_padding (iopos_, shake_delimiter);
        feeding_mode_ = false;
        state_.permute (24);
        iopos_ = 0;
      }
    const size_t count = HASHBITS / 8;
    for (size_t i = 0; i < count; i++)
      hashvalue[i] = state_.byte (i);
    return count;
  }
  /// Read out the current Keccak state and permute as needed.
  void
  squeeze_digest (uint8_t *output, size_t n_out)
  {
    BSE_ASSERT_RETURN (HASHBITS == 0);
    get_hash (output);                  // absorb_padding and leave feeding_mode_
    BSE_ASSERT_RETURN (feeding_mode_ == false);
    while (n_out)
      {
        const size_t count = std::min (n_out, byte_rate() - iopos_);
        for (size_t i = 0; i < count; i++)
          output[i] = state_.byte (iopos_ + i);
        iopos_ += count;
        output += count;
        n_out -= count;
        if (iopos_ >= byte_rate())
          {
            state_.permute (24);
            iopos_ = 0;
          }
      }
  }
};

// == SHA3_224 ==
struct SHA3_224::State : SHAKE_Base<224, 0x06> {
  State() : SHAKE_Base (1152) {}
};

SHA3_224::SHA3_224 () :
  state_ (new (&mem_) State())
{
  static_assert (sizeof (mem_) >= sizeof (*state_), "");
}

SHA3_224::~SHA3_224 ()
{
  state_->~State();
}

void
SHA3_224::update (const uint8_t *data, size_t length)
{
  state_->update (data, length);
}

void
SHA3_224::digest (uint8_t hashvalue[28])
{
  state_->get_hash (hashvalue);
}

void
SHA3_224::reset ()
{
  state_->reset();
}

void
sha3_224_hash (const void *data, size_t data_length, uint8_t hashvalue[28])
{
  SHA3_224 context;
  context.update ((const uint8_t*) data, data_length);
  context.digest (hashvalue);
}

// == SHA3_256 ==
struct SHA3_256::State : SHAKE_Base<256, 0x06> {
  State() : SHAKE_Base (1088) {}
};

SHA3_256::SHA3_256 () :
  state_ (new (&mem_) State())
{
  static_assert (sizeof (mem_) >= sizeof (*state_), "");
}

SHA3_256::~SHA3_256 ()
{
  state_->~State();
}

void
SHA3_256::update (const uint8_t *data, size_t length)
{
  state_->update (data, length);
}

void
SHA3_256::digest (uint8_t hashvalue[32])
{
  state_->get_hash (hashvalue);
}

void
SHA3_256::reset ()
{
  state_->reset();
}

void
sha3_256_hash (const void *data, size_t data_length, uint8_t hashvalue[32])
{
  SHA3_256 context;
  context.update ((const uint8_t*) data, data_length);
  context.digest (hashvalue);
}

// == SHA3_384 ==
struct SHA3_384::State : SHAKE_Base<384, 0x06> {
  State() : SHAKE_Base (832) {}
};

SHA3_384::SHA3_384 () :
  state_ (new (&mem_) State())
{
  static_assert (sizeof (mem_) >= sizeof (*state_), "");
}

SHA3_384::~SHA3_384 ()
{
  state_->~State();
}

void
SHA3_384::update (const uint8_t *data, size_t length)
{
  state_->update (data, length);
}

void
SHA3_384::digest (uint8_t hashvalue[48])
{
  state_->get_hash (hashvalue);
}

void
SHA3_384::reset ()
{
  state_->reset();
}

void
sha3_384_hash (const void *data, size_t data_length, uint8_t hashvalue[48])
{
  SHA3_384 context;
  context.update ((const uint8_t*) data, data_length);
  context.digest (hashvalue);
}

// == SHA3_512 ==
struct SHA3_512::State : SHAKE_Base<512, 0x06> {
  State() : SHAKE_Base (576) {}
};

SHA3_512::SHA3_512 () :
  state_ (new (&mem_) State())
{
  static_assert (sizeof (mem_) >= sizeof (*state_), "");
}

SHA3_512::~SHA3_512 ()
{
  state_->~State();
}

void
SHA3_512::update (const uint8_t *data, size_t length)
{
  state_->update (data, length);
}

void
SHA3_512::digest (uint8_t hashvalue[64])
{
  state_->get_hash (hashvalue);
}

void
SHA3_512::reset ()
{
  state_->reset();
}

void
sha3_512_hash (const void *data, size_t data_length, uint8_t hashvalue[64])
{
  SHA3_512 context;
  context.update ((const uint8_t*) data, data_length);
  context.digest (hashvalue);
}

// == SHAKE128 ==
struct SHAKE128::State : SHAKE_Base<0, 0x1f> {
  State() : SHAKE_Base (1344) {}
};

SHAKE128::SHAKE128 () :
  state_ (new (&mem_) State())
{
  static_assert (sizeof (mem_) >= sizeof (*state_), "");
}

SHAKE128::~SHAKE128 ()
{
  state_->~State();
}

void
SHAKE128::update (const uint8_t *data, size_t length)
{
  state_->update (data, length);
}

void
SHAKE128::squeeze_digest (uint8_t *hashvalues, size_t n)
{
  state_->squeeze_digest (hashvalues, n);
}

void
SHAKE128::reset ()
{
  state_->reset();
}

void
shake128_hash (const void *data, size_t data_length, uint8_t *hashvalues, size_t n)
{
  SHAKE128 context;
  context.update ((const uint8_t*) data, data_length);
  context.squeeze_digest (hashvalues, n);
}

// == SHAKE256 ==
struct SHAKE256::State : SHAKE_Base<0, 0x1f> {
  State() : SHAKE_Base (1088) {}
};

SHAKE256::SHAKE256 () :
  state_ (new (&mem_) State())
{
  static_assert (sizeof (mem_) >= sizeof (*state_), "");
}

SHAKE256::~SHAKE256 ()
{
  state_->~State();
}

void
SHAKE256::update (const uint8_t *data, size_t length)
{
  state_->update (data, length);
}

void
SHAKE256::squeeze_digest (uint8_t *hashvalues, size_t n)
{
  state_->squeeze_digest (hashvalues, n);
}

void
SHAKE256::reset ()
{
  state_->reset();
}

void
shake256_hash (const void *data, size_t data_length, uint8_t *hashvalues, size_t n)
{
  SHAKE256 context;
  context.update ((const uint8_t*) data, data_length);
  context.squeeze_digest (hashvalues, n);
}

// == Pcg32Rng ==
Pcg32Rng::Pcg32Rng () :
  increment_ (0), accu_ (0)
{
  auto_seed();
}

Pcg32Rng::Pcg32Rng (uint64_t offset, uint64_t sequence) :
  increment_ (0), accu_ (0)
{
  seed (offset, sequence);
}

void
Pcg32Rng::auto_seed ()
{
  AutoSeeder seeder;
  seed (seeder);
}

void
Pcg32Rng::seed (uint64_t offset, uint64_t sequence)
{
  accu_ = sequence;
  increment_ = (sequence << 1) | 1;    // force increment_ to be odd
  accu_ += offset;
  accu_ = A * accu_ + increment_;
}

// == Random Numbers ==
static uint64_t
global_random64()
{
  static KeccakRng *global_rng = NULL;
  static std::mutex mtx;
  std::unique_lock<std::mutex> lock (mtx);
  if (UNLIKELY (!global_rng))
    {
      uint64 entropy[32];
      collect_runtime_entropy (entropy, ARRAY_SIZE (entropy));
      static std::aligned_storage<sizeof (KeccakRng), alignof (KeccakRng)>::type mem;
      // 8 rounds provide good statistical shuffling, and
      // 256 hidden bits make the generator state unguessable
      global_rng = new (&mem) KeccakRng (256, 8);
      global_rng->seed (entropy, ARRAY_SIZE (entropy));
    }
  return global_rng->random();
}

/** Generate a non-deterministic, uniformly distributed 64 bit pseudo-random number.
 * This function generates pseudo-random numbers using the system state as entropy
 * and class KeccakRng for the mixing. No seeding is required.
 */
uint64_t
random_int64 ()
{
  return global_random64();
}

/** Generate uniformly distributed pseudo-random integer within range.
 * This function generates a pseudo-random number like random_int64(),
 * constrained to the range: @a begin <= number < @a end.
 */
int64_t
random_irange (int64_t begin, int64_t end)
{
  return_unless (begin < end, begin);
  const uint64_t range    = end - begin;
  const uint64_t quotient = 0xffffffffffffffffULL / range;
  const uint64_t bound    = quotient * range;
  uint64_t r = global_random64();
  while (BSE_UNLIKELY (r >= bound))        // repeats with <50% probability
    r = global_random64();
  return begin + r / quotient;
}

/** Generate uniformly distributed pseudo-random floating point number.
 * This function generates a pseudo-random number like random_int64(),
 * constrained to the range: 0.0 <= number < 1.0.
 */
double
random_float ()
{
  double r01;
  do
    r01 = global_random64() * 5.42101086242752217003726400434970855712890625e-20; // 1.0 / 2^64
  while (BSE_UNLIKELY (r01 >= 1.0));       // retry if arithmetic exceeds boundary
  return r01;
}

/** Generate uniformly distributed pseudo-random floating point number within a range.
 * This function generates a pseudo-random number like random_float(),
 * constrained to the range: @a begin <= number < @a end.
 */
double
random_frange (double begin, double end)
{
  return_unless (begin < end, begin + 0 * end); // catch and propagate NaNs
  const double r01 = global_random64() * 5.42101086242752217003726400434970855712890625e-20; // 1.0 / 2^64
  return end * r01 + (1.0 - r01) * begin;
}

/// Provide a unique 64 bit identifier that is not 0, see also random_int64().
uint64_t
random_nonce ()
{
  static uint64_t cached_nonce = 0;
  if (BSE_UNLIKELY (cached_nonce == 0))
    random_secret (&cached_nonce);
  return cached_nonce;
}

/// Generate a secret non-zero nonce in secret_var, unless it has already been assigned.
void
random_secret (uint64_t *secret_var)
{
  static std::mutex mtx;
  std::unique_lock<std::mutex> lock (mtx);
  if (!*secret_var)
    {
      uint64_t d;
      do
        d = global_random64();
      while (d == 0); // very unlikely
      *secret_var = d;
    }
}

uint64_t cached_hash_secret = 0;

} // Bse
