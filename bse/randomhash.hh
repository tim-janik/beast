// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
// Author: 2014, Tim Janik, see http://testbit.org/keccak
#ifndef __BSE_RANDOMHASH_HH__
#define __BSE_RANDOMHASH_HH__

#include <bse/bcore.hh>

namespace Bse {

/** Helper to provide memory for placement new
 * AlignedPOD<SIZE> is aligned like max_align_t or like malloc()-ed memory and
 * provides SIZE bytes. Idiomatic use is:
 * \code{.cc}
 *   static AlignedPOD<sizeof (std::string)> pod_mem;
 *   std::string *str = new (&pod_mem) std::string();
 * \endcode
 */
template<size_t SIZE>
struct alignas (16) AlignedPOD {
  typename std::aligned_storage<SIZE, 16>::type mem;
  /* malloc() aligns to 2 * sizeof (size_t), i.e. 16 on 64bit, max_align_t is
   * usually aligned to long double, i.e. 16, and most SIMD code also needs at
   * least 16 byte alignment.
   */
};

// == Random Numbers ==
uint64_t        random_nonce    ();
uint64_t        random_int64    ();
int64_t         random_irange   (int64_t begin, int64_t end);
double          random_float    ();
double          random_frange   (double begin, double end);
void            random_secret   (uint64_t *secret_var);


// == Hashing ==
/** SHA3_224 - 224 Bit digest generation.
 * This class implements the SHA3 hash funtion to create 224 Bit digests, see FIPS 202 @cite Fips202 .
 */
struct SHA3_224 {
  /*dtor*/ ~SHA3_224    ();
  /*ctor*/  SHA3_224    ();         ///< Create context to calculate a 224 bit SHA3 hash digest.
  void      reset       ();         ///< Reset state to feed and retrieve a new hash value.
  void      update      (const uint8_t *data, size_t length);   ///< Feed data to be hashed.
  void      digest      (uint8_t hashvalue[28]);                ///< Retrieve the resulting hash value.
private:
  AlignedPOD<232> mem_;
  struct    State;
  State    *state_;
};
/// Calculate 224 bit SHA3 digest from @a data, see also class SHA3_224.
void    sha3_224_hash   (const void *data, size_t data_length, uint8_t hashvalue[28]);

/** SHA3_256 - 256 Bit digest generation.
 * This class implements the SHA3 hash funtion to create 256 Bit digests, see FIPS 202 @cite Fips202 .
 */
struct SHA3_256 {
  /*dtor*/ ~SHA3_256    ();
  /*ctor*/  SHA3_256    ();         ///< Create context to calculate a 256 bit SHA3 hash digest.
  void      reset       ();         ///< Reset state to feed and retrieve a new hash value.
  void      update      (const uint8_t *data, size_t length);   ///< Feed data to be hashed.
  void      digest      (uint8_t hashvalue[32]);                ///< Retrieve the resulting hash value.
private:
  AlignedPOD<232> mem_;
  struct    State;
  State    *state_;
};
/// Calculate 256 bit SHA3 digest from @a data, see also class SHA3_256.
void    sha3_256_hash   (const void *data, size_t data_length, uint8_t hashvalue[32]);

/** SHA3_384 - 384 Bit digest generation.
 * This class implements the SHA3 hash funtion to create 384 Bit digests, see FIPS 202 @cite Fips202 .
 */
struct SHA3_384 {
  /*dtor*/ ~SHA3_384    ();
  /*ctor*/  SHA3_384    ();         ///< Create context to calculate a 384 bit SHA3 hash digest.
  void      reset       ();         ///< Reset state to feed and retrieve a new hash value.
  void      update      (const uint8_t *data, size_t length);   ///< Feed data to be hashed.
  void      digest      (uint8_t hashvalue[48]);                ///< Retrieve the resulting hash value.
private:
  AlignedPOD<232> mem_;
  struct    State;
  State    *state_;
};
/// Calculate 384 bit SHA3 digest from @a data, see also class SHA3_384.
void    sha3_384_hash   (const void *data, size_t data_length, uint8_t hashvalue[48]);

/** SHA3_512 - 512 Bit digest generation.
 * This class implements the SHA3 hash funtion to create 512 Bit digests, see FIPS 202 @cite Fips202 .
 */
struct SHA3_512 {
  /*dtor*/ ~SHA3_512    ();
  /*ctor*/  SHA3_512    ();         ///< Create context to calculate a 512 bit SHA3 hash digest.
  void      reset       ();         ///< Reset state to feed and retrieve a new hash value.
  void      update      (const uint8_t *data, size_t length);   ///< Feed data to be hashed.
  void      digest      (uint8_t hashvalue[64]);                ///< Retrieve the resulting hash value.
private:
  AlignedPOD<232> mem_;
  struct    State;
  State    *state_;
};
/// Calculate 512 bit SHA3 digest from @a data, see also class SHA3_512.
void    sha3_512_hash   (const void *data, size_t data_length, uint8_t hashvalue[64]);

/** SHAKE128 - 128 Bit extendable output digest generation.
 * This class implements the SHA3 extendable output hash funtion with 128 bit security strength, see FIPS 202 @cite Fips202 .
 */
struct SHAKE128 {
  /*dtor*/ ~SHAKE128        ();
  /*ctor*/  SHAKE128        ();         ///< Create context to calculate an unbounded SHAKE128 hash digest.
  void      reset           ();         ///< Reset state to feed and retrieve a new hash value.
  void      update          (const uint8_t *data, size_t length);   ///< Feed data to be hashed.
  void      squeeze_digest  (uint8_t *hashvalues, size_t n);        ///< Retrieve an arbitrary number of hash value bytes.
private:
  AlignedPOD<232> mem_;
  struct    State;
  State    *state_;
};
/// Calculate SHA3 extendable output digest for 128 bit security strength, see also class SHAKE128.
void    shake128_hash   (const void *data, size_t data_length, uint8_t *hashvalues, size_t n);

/** SHAKE256 - 256 Bit extendable output digest generation.
 * This class implements the SHA3 extendable output hash funtion with 256 bit security strength, see FIPS 202 @cite Fips202 .
 */
struct SHAKE256 {
  /*dtor*/ ~SHAKE256        ();
  /*ctor*/  SHAKE256        ();         ///< Create context to calculate an unbounded SHAKE256 hash digest.
  void      reset           ();         ///< Reset state to feed and retrieve a new hash value.
  void      update          (const uint8_t *data, size_t length);   ///< Feed data to be hashed.
  void      squeeze_digest  (uint8_t *hashvalues, size_t n);        ///< Retrieve an arbitrary number of hash value bytes.
private:
  AlignedPOD<232> mem_;
  struct    State;
  State    *state_;
};
/// Calculate SHA3 extendable output digest for 256 bit security strength, see also class SHAKE256.
void    shake256_hash   (const void *data, size_t data_length, uint8_t *hashvalues, size_t n);

namespace Lib { // Namespace for implementation internals

/// The Keccak-f[1600] Permutation, see the Keccak specification @cite Keccak11 .
class KeccakF1600 {
  union alignas (2 * sizeof (uint64_t))
  {
    uint64_t            A[25];
    uint8_t             bytes[200];
    // __MMX__: __m64   V[25];
  };
public:
  explicit      KeccakF1600 ();                         ///< Zero the state.
  void          reset       ();                         ///< Zero the state.
  uint64_t&     operator[]  (int      index)       { return A[index]; }
  uint64_t      operator[]  (int      index) const { return A[index]; }
  void          permute     (uint32_t n_rounds);        ///< Apply Keccak permutation with @a n_rounds.
  inline uint8_t&
  byte (size_t state_index)                             ///< Access byte 0..199 of the state.
  {
#if   __BYTE_ORDER == __LITTLE_ENDIAN
    return bytes[(state_index / 8) * 8 + (state_index % 8)];            // 8 == sizeof (uint64_t)
#elif __BYTE_ORDER == __BIG_ENDIAN
    return bytes[(state_index / 8) * 8 + (8 - 1 - (state_index % 8))];  // 8 == sizeof (uint64_t)
#else
#   error "Unknown __BYTE_ORDER"
#endif
  }
};

} // Lib

/// AutoSeeder provides non-deterministic seeding entropy.
class AutoSeeder {
public:
  /// Generate non-deterministic 64bit random value.
  static uint64   random     ()       { return random_int64(); }
  /// Generate non-deterministic 64bit random value.
  uint64          operator() () const { return this->random(); }
  /// Fill the range [begin, end) with random unsigned integer values.
  template<typename RandomAccessIterator> void
  generate (RandomAccessIterator begin, RandomAccessIterator end)
  {
    typedef typename std::iterator_traits<RandomAccessIterator>::value_type Value;
    while (begin != end)
      {
        const uint64_t rbits = operator()();
        *begin++ = Value (rbits);
        if (sizeof (Value) <= 4 && begin != end)
          *begin++ = Value (rbits >> 32);
      }
  }
};

/** KeccakRng - A KeccakF1600 based pseudo-random number generator.
 * The permutation steps are derived from the Keccak specification @cite Keccak11 .
 * For further details about this implementation, see also: http://testbit.org/keccak
 * This class is primarily used to implement more fine tuned generators, such as:
 * KeccakCryptoRng, KeccakGoodRng and KeccakFastRng.
 */
class KeccakRng {
  const uint16_t      bit_rate_, n_rounds_;
  uint32_t            opos_;
  Lib::KeccakF1600    state_;
  void                permute1600();
public:
  /*copy*/            KeccakRng  (const KeccakRng&) = default;
  /// Integral type of the KeccakRng generator results.
  typedef uint64_t    result_type;
  /// Amount of 64 bit random numbers per generated block.
  inline size_t       n_nums() const            { return bit_rate_ / 64; }
  /// Amount of bits used to store hidden random number generator state.
  inline size_t       bit_capacity() const      { return 1600 - bit_rate_; }
  /*dtor*/           ~KeccakRng  ();
  /// Create an unseeded Keccak PRNG with specific capacity and number of rounds, for experts only.
  explicit
  KeccakRng (uint16_t hidden_state_capacity, uint16_t n_rounds) :
    bit_rate_ (1600 - hidden_state_capacity), n_rounds_ (n_rounds), opos_ (n_nums())
  {
    BSE_ASSERT_RETURN (hidden_state_capacity > 0 && hidden_state_capacity <= 1600 - 64);
    BSE_ASSERT_RETURN (64 * (hidden_state_capacity / 64) == hidden_state_capacity); // capacity must be 64bit aligned
    BSE_ASSERT_RETURN (n_rounds > 0 && n_rounds < 255);                             // see KECCAK_ROUND_CONSTANTS access
  }
  void forget   ();
  void discard  (unsigned long long count);
  void xor_seed (const uint64_t *seeds, size_t n_seeds);
  /// Reinitialize the generator state using a 64 bit @a seed_value.
  void seed     (uint64_t seed_value = 1)               { seed (&seed_value, 1); }
  /// Reinitialize the generator state using a nuber of 64 bit @a seeds.
  void
  seed (const uint64_t *seeds, size_t n_seeds)
  {
    state_.reset();
    xor_seed (seeds, n_seeds);
  }
  /// Seed the generator state from a @a seed_sequence.
  template<class SeedSeq> void
  seed (SeedSeq &seed_sequence)
  {
    uint32_t u32[50];                   // fill 50 * 32 = 1600 state bits
    seed_sequence.generate (&u32[0], &u32[50]);
    uint64_t u64[25];
    for (size_t i = 0; i < 25; i++)     // Keccak bit order: 1) LSB 2) MSB
      u64[i] = u32[i * 2] | (uint64_t (u32[i * 2 + 1]) << 32);
    seed (u64, 25);
  }
  /// Seed the generator from a system specific nondeterministic random source.
  void auto_seed ();
  /// Generate uniformly distributed 64 bit pseudo random number.
  /// A new block permutation is carried out every n_nums() calls, see also xor_seed().
  uint64_t
  random ()
  {
    if (opos_ >= n_nums())
      permute1600();
    return state_[opos_++];
  }
  /// Generate uniformly distributed 32 bit pseudo random number.
  result_type   operator() ()   { return random(); }
  /// Fill the range [begin, end) with random unsigned integer values.
  template<typename RandomAccessIterator> void
  generate (RandomAccessIterator begin, RandomAccessIterator end)
  {
    typedef typename std::iterator_traits<RandomAccessIterator>::value_type Value;
    while (begin != end)
      {
        const uint64_t rbits = operator()();
        *begin++ = Value (rbits);
        if (sizeof (Value) <= 4 && begin != end)
          *begin++ = Value (rbits >> 32);
      }
  }
  /// Compare two generators for state equality.
  friend bool
  operator== (const KeccakRng &lhs, const KeccakRng &rhs)
  {
    for (size_t i = 0; i < 25; i++)
      if (lhs.state_[i] != rhs.state_[i])
        return false;
    return lhs.opos_ == rhs.opos_ && lhs.bit_rate_ == rhs.bit_rate_;
  }
  /// Compare two generators for state inequality.
  friend bool
  operator!= (const KeccakRng &lhs, const KeccakRng &rhs)
  {
    return !(lhs == rhs);
  }
  /// Minimum of the result type, for uint64_t that is 0.
  result_type
  min() const
  {
    return std::numeric_limits<result_type>::min(); // 0
  }
  /// Maximum of the result type, for uint64_t that is 18446744073709551615.
  result_type
  max() const
  {
    return std::numeric_limits<result_type>::max(); // 18446744073709551615
  }
  /// Serialize generator state into an OStream.
  template<typename CharT, typename Traits>
  friend std::basic_ostream<CharT, Traits>&
  operator<< (std::basic_ostream<CharT, Traits> &os, const KeccakRng &self)
  {
    typedef typename std::basic_ostream<CharT, Traits>::ios_base IOS;
    const typename IOS::fmtflags saved_flags = os.flags();
    os.flags (IOS::dec | IOS::fixed | IOS::left);
    const CharT space = os.widen (' ');
    const CharT saved_fill = os.fill();
    os.fill (space);
    os << self.opos_;
    for (size_t i = 0; i < 25; i++)
      os << space << self.state_[i];
    os.flags (saved_flags);
    os.fill (saved_fill);
    return os;
  }
  /// Deserialize generator state from an IStream.
  template<typename CharT, typename Traits>
  friend std::basic_istream<CharT, Traits>&
  operator>> (std::basic_istream<CharT, Traits> &is, KeccakRng &self)
  {
    typedef typename std::basic_istream<CharT, Traits>::ios_base IOS;
    const typename IOS::fmtflags saved_flags = is.flags();
    is.flags (IOS::dec | IOS::skipws);
    is >> self.opos_;
    self.opos_ = std::min (self.n_nums(), size_t (self.opos_));
    for (size_t i = 0; i < 25; i++)
      is >> self.state_[i];
    is.flags (saved_flags);
    return is;
  }
};

/** KeccakCryptoRng - A KeccakF1600 based cryptographic quality pseudo-random number generator.
 * The quality of the generated pseudo random numbers is comaparable to the hash output of SHAKE128.
 */
class KeccakCryptoRng : public KeccakRng {
public:
  /// Initialize and seed the generator from a system specific nondeterministic random source.
  explicit      KeccakCryptoRng ()                       : KeccakRng (256, 24)   { auto_seed(); }
  /// Initialize and seed the generator from @a seed_sequence.
  template<class SeedSeq>
  explicit      KeccakCryptoRng (SeedSeq &seed_sequence) : KeccakRng (256, 24)   { seed (seed_sequence); }
};

/** KeccakGoodRng - A KeccakF1600 based good quality pseudo-random number generator.
 * This class provides very good random numbers, using the KeccakF1600 algorithm without
 * the extra security margins applied for SHA3 hash generation. This improves performance
 * significantly without noticably trading random number quality.
 * For cryptography grade number generation KeccakCryptoRng should be used instead.
 */
class KeccakGoodRng : public KeccakRng {
public:
  /// Initialize and seed the generator from a system specific nondeterministic random source.
  explicit      KeccakGoodRng   ()                       : KeccakRng (192, 13)   { auto_seed(); }
  /// Initialize and seed the generator from @a seed_sequence.
  template<class SeedSeq>
  explicit      KeccakGoodRng   (SeedSeq &seed_sequence) : KeccakRng (192, 13)   { seed (seed_sequence); }
};

/** KeccakFastRng - A KeccakF1600 based fast pseudo-random number generator.
 * This class tunes the KeccakF1600 algorithm for best performance in pseudo random
 * number generation. Performance is improved while still retaining quality random
 * number generation, according to the findings in seciton "4.1.1 Statistical tests"
 * from http://keccak.noekeon.org/Keccak-reference-3.0.pdf.
 */
class KeccakFastRng : public KeccakRng {
public:
  /// Initialize and seed the generator from a system specific nondeterministic random source.
  explicit      KeccakFastRng   ()                       : KeccakRng (128, 8)    { auto_seed(); }
  /// Initialize and seed the generator from @a seed_sequence.
  template<class SeedSeq>
  explicit      KeccakFastRng   (SeedSeq &seed_sequence) : KeccakRng (128, 8)    { seed (seed_sequence); }
};

/** Pcg32Rng is a permutating linear congruential PRNG.
 * At the core, this pseudo random number generator uses the well known
 * linear congruential generator:
 * 6364136223846793005 * accumulator + 1442695040888963407 mod 2^64.
 * See also TAOCP by D. E. Knuth, section 3.3.4, table 1, line 26.
 * For good statistical performance, the output function of the permuted congruential
 * generator family is used as described on http://www.pcg-random.org/.
 * Period length for this generator is 2^64, the specified seed @a offset
 * chooses the position of the genrator and the seed @a sequence parameter
 * can be used to choose from 2^63 distinct random sequences.
 */
class Pcg32Rng {
  uint64_t increment_;          // must be odd, allows for 2^63 distinct random sequences
  uint64_t accu_;               // can contain all 2^64 possible values
  static constexpr const uint64_t A = 6364136223846793005ULL; // from C. E. Hayness, see TAOCP by D. E. Knuth, 3.3.4, table 1, line 26.
  static inline constexpr uint32_t
  ror32 (const uint32_t bits, const uint32_t offset)
  {
    // bitwise rotate-right pattern recognized by gcc & clang iff 32==sizeof (bits)
    return (bits >> offset) | (bits << (32 - offset));
  }
  static inline constexpr uint32_t
  pcg_xsh_rr (const uint64_t input)
  {
    // Section 6.3.1. 32-bit Output, 64-bit State: PCG-XSH-RR
    // http://www.pcg-random.org/pdf/toms-oneill-pcg-family-v1.02.pdf
    return ror32 ((input ^ (input >> 18)) >> 27, input >> 59);
  }
public:
  /// Initialize and seed from @a seed_sequence.
  template<class SeedSeq>
  explicit Pcg32Rng  (SeedSeq &seed_sequence) : increment_ (0), accu_ (0) { seed (seed_sequence); }
  /// Initialize and seed by seeking to position @a offset within stream @a sequence.
  explicit Pcg32Rng  (uint64_t offset, uint64_t sequence);
  /// Initialize and seed the generator from a system specific nondeterministic random source.
  explicit Pcg32Rng  ();
  /// Seed the generator from a system specific nondeterministic random source.
  void     auto_seed ();
  /// Seed by seeking to position @a offset within stream @a sequence.
  void     seed      (uint64_t offset, uint64_t sequence);
  /// Seed the generator state from a @a seed_sequence.
  template<class SeedSeq> void
  seed (SeedSeq &seed_sequence)
  {
    uint64_t seeds[2];
    seed_sequence.generate (&seeds[0], &seeds[2]);
    seed (seeds[0], seeds[1]);
  }
  /// Generate uniformly distributed 32 bit pseudo random number.
  uint32_t
  random ()
  {
    const uint64_t lcgout = accu_;      // using the *last* state as ouput helps with CPU pipelining
    accu_ = A * accu_ + increment_;
    return pcg_xsh_rr (lcgout);         // PCG XOR-shift + random rotation
  }
};

// == Hashing ==
/** Simple, very fast and well known hash function as constexpr with good dispersion.
 * This is the 64bit version of the well known
 * [FNV-1a](https://en.wikipedia.org/wiki/Fowler-Noll-Vo_hash_function)
 * hash function, implemented as a C++11 constexpr for zero-terminated
 * strings, so the hashes can be used e.g. as case labels in switch statements.
 */
template<class Num> static inline constexpr uint64_t
fnv1a_consthash64 (const Num *const ztdata, uint64_t hash = 0xcbf29ce484222325)
{
  static_assert (sizeof (Num) <= 1, "");
  return BSE_ISLIKELY (ztdata[0] != 0) ? fnv1a_consthash64 (ztdata + 1, 0x100000001b3 * (hash ^ uint8_t (ztdata[0]))) : hash;
}

/// Variant of fnv1a_consthash64() for memory blocks of arbitrary size.
template<class Num> static inline constexpr uint64_t
fnv1a_consthash64 (const Num *const data, size_t length, uint64_t hash)
{
  static_assert (sizeof (Num) <= 1, "");

  return length ? fnv1a_consthash64 (data + 1, length - 1, 0x100000001b3 * (hash ^ uint8_t (data[0]))) : hash;
}

/// Variant of fnv1a_consthash64() for std::string.
extern inline constexpr uint64_t
fnv1a_consthash64 (const std::string &str, uint64_t hash = 0xcbf29ce484222325)
{
  return fnv1a_consthash64 (str.data(), str.size(), hash);
}

/** Hash function based on the PCG family of random number generators (RNG).
 * This function is based on the paper [PCG: A Family of Simple Fast
 * Space-Efficient Statistically Good Algorithms for Random Number
 * Generation](http://www.pcg-random.org/paper.html),
 * because of its excellent avalange and distribution properties.
 * The hash data is integrated as octets in the inner LCG RNG loop.
 * Hash generation is very fast, because the inner loop consists only of a
 * multiplication and subtraction, while the ouput bit mixing consists of
 * just 5 simple operations (xor, shift and rotation).
 */
template<class Num> static BSE_CONST inline uint32_t
pcg_hash32 (const Num *data, size_t length, uint64_t seed)
{
  static_assert (sizeof (Num) <= 1, "");
  uint64_t h = seed;
  // Knuth LCG
  h ^= 0x14057b7ef767814fULL;
  for (size_t i = 0; BSE_ISLIKELY (i < length); i++)
    {
      h -= uint8_t (data[i]);
      h *= 6364136223846793005ULL;
    }
  // based on pcg_detail::xsh_rr_mixin
  const size_t   rsh = h >> 59;
  const uint32_t xsh = (h ^ (h >> 18)) >> 27;
  const uint32_t rot = (xsh >> rsh) | (xsh << (32 - rsh));
  return rot;
}

/** Hash function based on the PCG family of random number generators (RNG).
 * This function is similar to pcg_hash32() at its core, but because the
 * output is 64bit, the accumulated 64bit LCG state does not need to be
 * bit reduced. A fast but statistially good mixing function with
 * 5 xor/shifts and one multiplication is applied as output stage.
 * This function is allmost as fast as fnv1a_consthash64 due to the similar
 * structures of the inner loops, but it tends to score much better in
 * [Avalanche effect](https://en.wikipedia.org/wiki/Avalanche_effect)
 * tests, usch as [SMHasher](https://code.google.com/p/smhasher/).
 */
template<class Num> static BSE_CONST inline uint64_t
pcg_hash64 (const Num *data, size_t length, uint64_t seed)
{
  static_assert (sizeof (Num) <= 1, "");
  uint64_t h = seed;
  // Knuth LCG
  h ^= 0x14057b7ef767814fULL;
  for (size_t i = 0; BSE_ISLIKELY (i < length); i++)
    {
      h -= uint8_t (data[i]);
      h *= 6364136223846793005ULL;
    }
  // based on pcg_detail::rxs_m_xs_mixin
  const size_t   rsh = h >> 59;
  const uint64_t rxs = h ^ (h >> (5 + rsh));
  const uint64_t m = rxs * 12605985483714917081ULL;
  const uint64_t xs = m ^ (m >> 43);
  return xs;
}

/// pcg_hash64() variant for zero-terminated strings.
static BSE_CONST inline uint64_t
pcg_hash64 (const char *ztdata, uint64_t seed)
{
  uint64_t h = seed;
  // Knuth LCG
  h ^= 0x14057b7ef767814fULL;
  for (size_t i = 0; BSE_ISLIKELY (ztdata[i] != 0); i++)
    {
      h -= uint8_t (ztdata[i]);
      h *= 6364136223846793005ULL;
    }
  // based on pcg_detail::rxs_m_xs_mixin
  const size_t   rsh = h >> 59;
  const uint64_t rxs = h ^ (h >> (5 + rsh));
  const uint64_t m = rxs * 12605985483714917081ULL;
  const uint64_t xs = m ^ (m >> 43);
  return xs;
}

extern uint64_t cached_hash_secret; ///< Use hash_secret() for access.

/// Provide hashing nonce for reseeding hashes at program start to avoid collision attacks.
static BSE_PURE inline uint64_t
hash_secret ()
{
  if (BSE_UNLIKELY (cached_hash_secret == 0))
    random_secret (&cached_hash_secret);
  return cached_hash_secret;
}

/// Fast byte hashing with good dispersion and runtime randomization.
template<class Num> static BSE_CONST inline uint64_t
byte_hash64 (const Num *data, size_t length)
{
  static_assert (sizeof (Num) <= 1, "");
  return pcg_hash64 (data, length, hash_secret());
}

/// Fast string hashing with good dispersion for std::string and runtime randomization.
static BSE_CONST inline uint64_t
string_hash64 (const std::string &string)
{
  return pcg_hash64 (string.data(), string.size(), hash_secret());
}

/// pcg_hash64() variant for zero-terminated strings.
static BSE_CONST inline uint64_t
string_hash64 (const char *ztdata)
{
  return pcg_hash64 (ztdata, hash_secret());
}

} // Bse

#endif // __BSE_RANDOMHASH_HH__
