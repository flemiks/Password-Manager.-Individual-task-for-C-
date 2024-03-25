#ifndef MRDCVLSC_AES_IMPLEMENTATION
#define MRDCVLSC_AES_IMPLEMENTATION

#include <iostream>

#if defined(USE_NEON_AES)
  #ifndef HARDWARE_ACCELERATION_ARM_NEON_AES
    #define HARDWARE_ACCELERATION_ARM_NEON_AES
  #endif
  #undef HARDWARE_ACCELERATION_INTEL_AESNI
  #undef PORTABLE_CPP_CODE

#elif defined(USE_INTEL_AESNI)
  #ifndef HARDWARE_ACCELERATION_INTEL_AESNI
    #define HARDWARE_ACCELERATION_INTEL_AESNI
  #endif
  #undef HARDWARE_ACCELERATION_ARM_NEON_AES
  #undef PORTABLE_CPP_CODE

#elif defined(USE_CXX_AES)
  #ifndef PORTABLE_CPP_CODE
    #define PORTABLE_CPP_CODE
  #endif
  #undef HARDWARE_ACCELERATION_INTEL_AESNI
  #undef HARDWARE_ACCELERATION_ARM_NEON_AES

#else

#endif

#if (defined(_WIN32) || defined(_WIN64) || defined(__x86_64) || defined(__x86_64__) || defined(__amd64) || defined(__amd64__)) && \
  !defined(USE_CXX_AES) && !defined(USE_NEON_AES)
  #ifdef _MSC_VER
    #include <intrin.h>
  #endif

  #include <emmintrin.h>
  #include <immintrin.h>
  #include <xmmintrin.h>

  #ifndef HARDWARE_ACCELERATION_INTEL_AESNI
    #define HARDWARE_ACCELERATION_INTEL_AESNI
  #endif
#elif (defined(__arm64__) || defined(__aarch64__) || defined(_M_ARM)) && !defined(USE_CXX_AES) &&                      \
  !defined(USE_INTEL_AESNI)
  #if defined(__GNUC__)
    #include <stdint.h>
  #endif

  #if defined(__ARM_NEON) || defined(_MSC_VER)
    #include <arm_neon.h>
  #endif

  /* GCC and LLVM Clang, but not Apple Clang */
  #if defined(__GNUC__) && !defined(__apple_build_version__)
    #if defined(__ARM_ACLE) || defined(__ARM_FEATURE_CRYPTO)
      #include <arm_acle.h>
    #endif
  #endif

  #ifndef HARDWARE_ACCELERATION_ARM_NEON_AES
    #define HARDWARE_ACCELERATION_ARM_NEON_AES
  #endif
#else
  #ifndef PORTABLE_CPP_CODE
    #define PORTABLE_CPP_CODE
  #endif
#endif

#include <cstring>
#include <exception>
#include <iostream>

namespace Cipher {
  template <size_t key_bits = 128>
  class Aes {
    static constexpr size_t AES_BLOCK = 16;
    static constexpr size_t Nb = 4;
    static constexpr size_t Nk = key_bits / 32;
    static constexpr size_t Nr = Nk + 6;
    static constexpr size_t round_keys_size = 4 * Nb * (Nr + 1);

    unsigned char round_keys[round_keys_size];

#ifdef HARDWARE_ACCELERATION_INTEL_AESNI
    inline __m128i AES_128_ASSIST(__m128i tmp1, __m128i tmp2) {
      __m128i tmp3;
      tmp2 = _mm_shuffle_epi32(tmp2, 0xff);
      tmp3 = _mm_slli_si128(tmp1, 0x4);
      tmp1 = _mm_xor_si128(tmp1, tmp3);
      tmp3 = _mm_slli_si128(tmp3, 0x4);
      tmp1 = _mm_xor_si128(tmp1, tmp3);
      tmp3 = _mm_slli_si128(tmp3, 0x4);
      tmp1 = _mm_xor_si128(tmp1, tmp3);
      tmp1 = _mm_xor_si128(tmp1, tmp2);
      return tmp1;
    }

    void AES_128_Key_Expansion(const unsigned char *user_key, unsigned char *key) {
      __m128i tmp1, tmp2;
      __m128i *key_sched = (__m128i *) key;

      tmp1 = _mm_loadu_si128((__m128i *) user_key);
      key_sched[0] = tmp1;
      tmp2 = _mm_aeskeygenassist_si128(tmp1, 0x1);
      tmp1 = AES_128_ASSIST(tmp1, tmp2);
      key_sched[1] = tmp1;
      tmp2 = _mm_aeskeygenassist_si128(tmp1, 0x2);
      tmp1 = AES_128_ASSIST(tmp1, tmp2);
      key_sched[2] = tmp1;
      tmp2 = _mm_aeskeygenassist_si128(tmp1, 0x4);
      tmp1 = AES_128_ASSIST(tmp1, tmp2);
      key_sched[3] = tmp1;
      tmp2 = _mm_aeskeygenassist_si128(tmp1, 0x8);
      tmp1 = AES_128_ASSIST(tmp1, tmp2);
      key_sched[4] = tmp1;
      tmp2 = _mm_aeskeygenassist_si128(tmp1, 0x10);
      tmp1 = AES_128_ASSIST(tmp1, tmp2);
      key_sched[5] = tmp1;
      tmp2 = _mm_aeskeygenassist_si128(tmp1, 0x20);
      tmp1 = AES_128_ASSIST(tmp1, tmp2);
      key_sched[6] = tmp1;
      tmp2 = _mm_aeskeygenassist_si128(tmp1, 0x40);
      tmp1 = AES_128_ASSIST(tmp1, tmp2);
      key_sched[7] = tmp1;
      tmp2 = _mm_aeskeygenassist_si128(tmp1, 0x80);
      tmp1 = AES_128_ASSIST(tmp1, tmp2);
      key_sched[8] = tmp1;
      tmp2 = _mm_aeskeygenassist_si128(tmp1, 0x1b);
      tmp1 = AES_128_ASSIST(tmp1, tmp2);
      key_sched[9] = tmp1;
      tmp2 = _mm_aeskeygenassist_si128(tmp1, 0x36);
      tmp1 = AES_128_ASSIST(tmp1, tmp2);
      key_sched[10] = tmp1;
    }

    inline void KEY_192_ASSIST(__m128i *tmp1, __m128i *tmp2, __m128i *tmp3) {
      __m128i tmp4;
      *tmp2 = _mm_shuffle_epi32(*tmp2, 0x55);
      tmp4 = _mm_slli_si128(*tmp1, 0x4);
      *tmp1 = _mm_xor_si128(*tmp1, tmp4);
      tmp4 = _mm_slli_si128(tmp4, 0x4);
      *tmp1 = _mm_xor_si128(*tmp1, tmp4);
      tmp4 = _mm_slli_si128(tmp4, 0x4);
      *tmp1 = _mm_xor_si128(*tmp1, tmp4);
      *tmp1 = _mm_xor_si128(*tmp1, *tmp2);
      *tmp2 = _mm_shuffle_epi32(*tmp1, 0xff);
      tmp4 = _mm_slli_si128(*tmp3, 0x4);
      *tmp3 = _mm_xor_si128(*tmp3, tmp4);
      *tmp3 = _mm_xor_si128(*tmp3, *tmp2);
    }

    void AES_192_Key_Expansion(const unsigned char *user_key, unsigned char *key) {
      __m128i tmp1, tmp2, tmp3;
      __m128i *key_sched = (__m128i *) key;
      tmp1 = _mm_loadu_si128((__m128i *) user_key);
      tmp3 = _mm_loadu_si128((__m128i *) (user_key + 16));
      key_sched[0] = tmp1;
      key_sched[1] = tmp3;
      tmp2 = _mm_aeskeygenassist_si128(tmp3, 0x1);
      KEY_192_ASSIST(&tmp1, &tmp2, &tmp3);
      key_sched[1] = _mm_castpd_si128(_mm_shuffle_pd(_mm_castsi128_pd(key_sched[1]), _mm_castsi128_pd(tmp1), 0));
      key_sched[2] = _mm_castpd_si128(_mm_shuffle_pd(_mm_castsi128_pd(tmp1), _mm_castsi128_pd(tmp3), 1));
      tmp2 = _mm_aeskeygenassist_si128(tmp3, 0x2);
      KEY_192_ASSIST(&tmp1, &tmp2, &tmp3);
      key_sched[3] = tmp1;
      key_sched[4] = tmp3;
      tmp2 = _mm_aeskeygenassist_si128(tmp3, 0x4);
      KEY_192_ASSIST(&tmp1, &tmp2, &tmp3);
      key_sched[4] = _mm_castpd_si128(_mm_shuffle_pd(_mm_castsi128_pd(key_sched[4]), _mm_castsi128_pd(tmp1), 0));
      key_sched[5] = _mm_castpd_si128(_mm_shuffle_pd(_mm_castsi128_pd(tmp1), _mm_castsi128_pd(tmp3), 1));
      tmp2 = _mm_aeskeygenassist_si128(tmp3, 0x8);
      KEY_192_ASSIST(&tmp1, &tmp2, &tmp3);
      key_sched[6] = tmp1;
      key_sched[7] = tmp3;
      tmp2 = _mm_aeskeygenassist_si128(tmp3, 0x10);
      KEY_192_ASSIST(&tmp1, &tmp2, &tmp3);
      key_sched[7] = _mm_castpd_si128(_mm_shuffle_pd(_mm_castsi128_pd(key_sched[7]), _mm_castsi128_pd(tmp1), 0));
      key_sched[8] = _mm_castpd_si128(_mm_shuffle_pd(_mm_castsi128_pd(tmp1), _mm_castsi128_pd(tmp3), 1));
      tmp2 = _mm_aeskeygenassist_si128(tmp3, 0x20);
      KEY_192_ASSIST(&tmp1, &tmp2, &tmp3);
      key_sched[9] = tmp1;
      key_sched[10] = tmp3;
      tmp2 = _mm_aeskeygenassist_si128(tmp3, 0x40);
      KEY_192_ASSIST(&tmp1, &tmp2, &tmp3);
      key_sched[10] = _mm_castpd_si128(_mm_shuffle_pd(_mm_castsi128_pd(key_sched[10]), _mm_castsi128_pd(tmp1), 0));
      key_sched[11] = _mm_castpd_si128(_mm_shuffle_pd(_mm_castsi128_pd(tmp1), _mm_castsi128_pd(tmp3), 1));
      tmp2 = _mm_aeskeygenassist_si128(tmp3, 0x80);
      KEY_192_ASSIST(&tmp1, &tmp2, &tmp3);
      key_sched[12] = tmp1;
    }

    inline void KEY_256_ASSIST_1(__m128i *tmp1, __m128i *tmp2) {
      __m128i tmp4;
      *tmp2 = _mm_shuffle_epi32(*tmp2, 0xff);
      tmp4 = _mm_slli_si128(*tmp1, 0x4);
      *tmp1 = _mm_xor_si128(*tmp1, tmp4);
      tmp4 = _mm_slli_si128(tmp4, 0x4);
      *tmp1 = _mm_xor_si128(*tmp1, tmp4);
      tmp4 = _mm_slli_si128(tmp4, 0x4);
      *tmp1 = _mm_xor_si128(*tmp1, tmp4);
      *tmp1 = _mm_xor_si128(*tmp1, *tmp2);
    }

    inline void KEY_256_ASSIST_2(__m128i *tmp1, __m128i *tmp3) {
      __m128i tmp2, tmp4;
      tmp4 = _mm_aeskeygenassist_si128(*tmp1, 0x0);
      tmp2 = _mm_shuffle_epi32(tmp4, 0xaa);
      tmp4 = _mm_slli_si128(*tmp3, 0x4);
      *tmp3 = _mm_xor_si128(*tmp3, tmp4);
      tmp4 = _mm_slli_si128(tmp4, 0x4);
      *tmp3 = _mm_xor_si128(*tmp3, tmp4);
      tmp4 = _mm_slli_si128(tmp4, 0x4);
      *tmp3 = _mm_xor_si128(*tmp3, tmp4);
      *tmp3 = _mm_xor_si128(*tmp3, tmp2);
    }

    void AES_256_Key_Expansion(const unsigned char *user_key, unsigned char *key) {
      __m128i tmp1, tmp2, tmp3;
      __m128i *key_sched = (__m128i *) key;
      tmp1 = _mm_loadu_si128((__m128i *) user_key);
      tmp3 = _mm_loadu_si128((__m128i *) (user_key + 16));
      key_sched[0] = tmp1;
      key_sched[1] = tmp3;
      tmp2 = _mm_aeskeygenassist_si128(tmp3, 0x01);
      KEY_256_ASSIST_1(&tmp1, &tmp2);
      key_sched[2] = tmp1;
      KEY_256_ASSIST_2(&tmp1, &tmp3);
      key_sched[3] = tmp3;
      tmp2 = _mm_aeskeygenassist_si128(tmp3, 0x02);
      KEY_256_ASSIST_1(&tmp1, &tmp2);
      key_sched[4] = tmp1;
      KEY_256_ASSIST_2(&tmp1, &tmp3);
      key_sched[5] = tmp3;
      tmp2 = _mm_aeskeygenassist_si128(tmp3, 0x04);
      KEY_256_ASSIST_1(&tmp1, &tmp2);
      key_sched[6] = tmp1;
      KEY_256_ASSIST_2(&tmp1, &tmp3);
      key_sched[7] = tmp3;
      tmp2 = _mm_aeskeygenassist_si128(tmp3, 0x08);
      KEY_256_ASSIST_1(&tmp1, &tmp2);
      key_sched[8] = tmp1;
      KEY_256_ASSIST_2(&tmp1, &tmp3);
      key_sched[9] = tmp3;
      tmp2 = _mm_aeskeygenassist_si128(tmp3, 0x10);
      KEY_256_ASSIST_1(&tmp1, &tmp2);
      key_sched[10] = tmp1;
      KEY_256_ASSIST_2(&tmp1, &tmp3);
      key_sched[11] = tmp3;
      tmp2 = _mm_aeskeygenassist_si128(tmp3, 0x20);
      KEY_256_ASSIST_1(&tmp1, &tmp2);
      key_sched[12] = tmp1;
      KEY_256_ASSIST_2(&tmp1, &tmp3);
      key_sched[13] = tmp3;
      tmp2 = _mm_aeskeygenassist_si128(tmp3, 0x40);
      KEY_256_ASSIST_1(&tmp1, &tmp2);
      key_sched[14] = tmp1;
    }
// #elif defined(HARDWARE_ACCELERATION_ARM_NEON_AES)
// space for arm neon variables in case needed in the future.
#else
    static constexpr unsigned char sbox[256] = {
      0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76, 0xca, 0x82, 0xc9,
      0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0, 0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f,
      0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15, 0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07,
      0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75, 0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3,
      0x29, 0xe3, 0x2f, 0x84, 0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58,
      0xcf, 0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8, 0x51, 0xa3,
      0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2, 0xcd, 0x0c, 0x13, 0xec, 0x5f,
      0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73, 0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88,
      0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb, 0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac,
      0x62, 0x91, 0x95, 0xe4, 0x79, 0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a,
      0xae, 0x08, 0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a, 0x70,
      0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e, 0xe1, 0xf8, 0x98, 0x11,
      0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf, 0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42,
      0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16,
    };

    static constexpr unsigned char inverse_sbox[256] = {
      0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb, 0x7c, 0xe3, 0x39,
      0x82, 0x9b, 0x2f, 0xff, 0x87, 0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb, 0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2,
      0x23, 0x3d, 0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e, 0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2, 0x76,
      0x5b, 0xa2, 0x49, 0x6d, 0x8b, 0xd1, 0x25, 0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xd4, 0xa4, 0x5c, 0xcc,
      0x5d, 0x65, 0xb6, 0x92, 0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda, 0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d,
      0x84, 0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a, 0xf7, 0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06, 0xd0, 0x2c,
      0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02, 0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b, 0x3a, 0x91, 0x11, 0x41, 0x4f,
      0x67, 0xdc, 0xea, 0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73, 0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85,
      0xe2, 0xf9, 0x37, 0xe8, 0x1c, 0x75, 0xdf, 0x6e, 0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89, 0x6f, 0xb7, 0x62,
      0x0e, 0xaa, 0x18, 0xbe, 0x1b, 0xfc, 0x56, 0x3e, 0x4b, 0xc6, 0xd2, 0x79, 0x20, 0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd,
      0x5a, 0xf4, 0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31, 0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 0x5f, 0x60,
      0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d, 0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef, 0xa0, 0xe0, 0x3b, 0x4d,
      0xae, 0x2a, 0xf5, 0xb0, 0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61, 0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6,
      0x26, 0xe1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0c, 0x7d,
    };

    /// Galois Multiplication lookup tables.
    static constexpr unsigned char gf[15][256] = {
      {},
      {},

      // mul 2
      {
        0x00, 0x02, 0x04, 0x06, 0x08, 0x0a, 0x0c, 0x0e, 0x10, 0x12, 0x14, 0x16, 0x18, 0x1a, 0x1c, 0x1e, 0x20, 0x22,
        0x24, 0x26, 0x28, 0x2a, 0x2c, 0x2e, 0x30, 0x32, 0x34, 0x36, 0x38, 0x3a, 0x3c, 0x3e, 0x40, 0x42, 0x44, 0x46,
        0x48, 0x4a, 0x4c, 0x4e, 0x50, 0x52, 0x54, 0x56, 0x58, 0x5a, 0x5c, 0x5e, 0x60, 0x62, 0x64, 0x66, 0x68, 0x6a,
        0x6c, 0x6e, 0x70, 0x72, 0x74, 0x76, 0x78, 0x7a, 0x7c, 0x7e, 0x80, 0x82, 0x84, 0x86, 0x88, 0x8a, 0x8c, 0x8e,
        0x90, 0x92, 0x94, 0x96, 0x98, 0x9a, 0x9c, 0x9e, 0xa0, 0xa2, 0xa4, 0xa6, 0xa8, 0xaa, 0xac, 0xae, 0xb0, 0xb2,
        0xb4, 0xb6, 0xb8, 0xba, 0xbc, 0xbe, 0xc0, 0xc2, 0xc4, 0xc6, 0xc8, 0xca, 0xcc, 0xce, 0xd0, 0xd2, 0xd4, 0xd6,
        0xd8, 0xda, 0xdc, 0xde, 0xe0, 0xe2, 0xe4, 0xe6, 0xe8, 0xea, 0xec, 0xee, 0xf0, 0xf2, 0xf4, 0xf6, 0xf8, 0xfa,
        0xfc, 0xfe, 0x1b, 0x19, 0x1f, 0x1d, 0x13, 0x11, 0x17, 0x15, 0x0b, 0x09, 0x0f, 0x0d, 0x03, 0x01, 0x07, 0x05,
        0x3b, 0x39, 0x3f, 0x3d, 0x33, 0x31, 0x37, 0x35, 0x2b, 0x29, 0x2f, 0x2d, 0x23, 0x21, 0x27, 0x25, 0x5b, 0x59,
        0x5f, 0x5d, 0x53, 0x51, 0x57, 0x55, 0x4b, 0x49, 0x4f, 0x4d, 0x43, 0x41, 0x47, 0x45, 0x7b, 0x79, 0x7f, 0x7d,
        0x73, 0x71, 0x77, 0x75, 0x6b, 0x69, 0x6f, 0x6d, 0x63, 0x61, 0x67, 0x65, 0x9b, 0x99, 0x9f, 0x9d, 0x93, 0x91,
        0x97, 0x95, 0x8b, 0x89, 0x8f, 0x8d, 0x83, 0x81, 0x87, 0x85, 0xbb, 0xb9, 0xbf, 0xbd, 0xb3, 0xb1, 0xb7, 0xb5,
        0xab, 0xa9, 0xaf, 0xad, 0xa3, 0xa1, 0xa7, 0xa5, 0xdb, 0xd9, 0xdf, 0xdd, 0xd3, 0xd1, 0xd7, 0xd5, 0xcb, 0xc9,
        0xcf, 0xcd, 0xc3, 0xc1, 0xc7, 0xc5, 0xfb, 0xf9, 0xff, 0xfd, 0xf3, 0xf1, 0xf7, 0xf5, 0xeb, 0xe9, 0xef, 0xed,
        0xe3, 0xe1, 0xe7, 0xe5,
      },

      // mul 3
      {
        0x00, 0x03, 0x06, 0x05, 0x0c, 0x0f, 0x0a, 0x09, 0x18, 0x1b, 0x1e, 0x1d, 0x14, 0x17, 0x12, 0x11, 0x30, 0x33,
        0x36, 0x35, 0x3c, 0x3f, 0x3a, 0x39, 0x28, 0x2b, 0x2e, 0x2d, 0x24, 0x27, 0x22, 0x21, 0x60, 0x63, 0x66, 0x65,
        0x6c, 0x6f, 0x6a, 0x69, 0x78, 0x7b, 0x7e, 0x7d, 0x74, 0x77, 0x72, 0x71, 0x50, 0x53, 0x56, 0x55, 0x5c, 0x5f,
        0x5a, 0x59, 0x48, 0x4b, 0x4e, 0x4d, 0x44, 0x47, 0x42, 0x41, 0xc0, 0xc3, 0xc6, 0xc5, 0xcc, 0xcf, 0xca, 0xc9,
        0xd8, 0xdb, 0xde, 0xdd, 0xd4, 0xd7, 0xd2, 0xd1, 0xf0, 0xf3, 0xf6, 0xf5, 0xfc, 0xff, 0xfa, 0xf9, 0xe8, 0xeb,
        0xee, 0xed, 0xe4, 0xe7, 0xe2, 0xe1, 0xa0, 0xa3, 0xa6, 0xa5, 0xac, 0xaf, 0xaa, 0xa9, 0xb8, 0xbb, 0xbe, 0xbd,
        0xb4, 0xb7, 0xb2, 0xb1, 0x90, 0x93, 0x96, 0x95, 0x9c, 0x9f, 0x9a, 0x99, 0x88, 0x8b, 0x8e, 0x8d, 0x84, 0x87,
        0x82, 0x81, 0x9b, 0x98, 0x9d, 0x9e, 0x97, 0x94, 0x91, 0x92, 0x83, 0x80, 0x85, 0x86, 0x8f, 0x8c, 0x89, 0x8a,
        0xab, 0xa8, 0xad, 0xae, 0xa7, 0xa4, 0xa1, 0xa2, 0xb3, 0xb0, 0xb5, 0xb6, 0xbf, 0xbc, 0xb9, 0xba, 0xfb, 0xf8,
        0xfd, 0xfe, 0xf7, 0xf4, 0xf1, 0xf2, 0xe3, 0xe0, 0xe5, 0xe6, 0xef, 0xec, 0xe9, 0xea, 0xcb, 0xc8, 0xcd, 0xce,
        0xc7, 0xc4, 0xc1, 0xc2, 0xd3, 0xd0, 0xd5, 0xd6, 0xdf, 0xdc, 0xd9, 0xda, 0x5b, 0x58, 0x5d, 0x5e, 0x57, 0x54,
        0x51, 0x52, 0x43, 0x40, 0x45, 0x46, 0x4f, 0x4c, 0x49, 0x4a, 0x6b, 0x68, 0x6d, 0x6e, 0x67, 0x64, 0x61, 0x62,
        0x73, 0x70, 0x75, 0x76, 0x7f, 0x7c, 0x79, 0x7a, 0x3b, 0x38, 0x3d, 0x3e, 0x37, 0x34, 0x31, 0x32, 0x23, 0x20,
        0x25, 0x26, 0x2f, 0x2c, 0x29, 0x2a, 0x0b, 0x08, 0x0d, 0x0e, 0x07, 0x04, 0x01, 0x02, 0x13, 0x10, 0x15, 0x16,
        0x1f, 0x1c, 0x19, 0x1a,
      },

      {},
      {},
      {},
      {},
      {},

      // mul 9
      {
        0x00, 0x09, 0x12, 0x1b, 0x24, 0x2d, 0x36, 0x3f, 0x48, 0x41, 0x5a, 0x53, 0x6c, 0x65, 0x7e, 0x77, 0x90, 0x99,
        0x82, 0x8b, 0xb4, 0xbd, 0xa6, 0xaf, 0xd8, 0xd1, 0xca, 0xc3, 0xfc, 0xf5, 0xee, 0xe7, 0x3b, 0x32, 0x29, 0x20,
        0x1f, 0x16, 0x0d, 0x04, 0x73, 0x7a, 0x61, 0x68, 0x57, 0x5e, 0x45, 0x4c, 0xab, 0xa2, 0xb9, 0xb0, 0x8f, 0x86,
        0x9d, 0x94, 0xe3, 0xea, 0xf1, 0xf8, 0xc7, 0xce, 0xd5, 0xdc, 0x76, 0x7f, 0x64, 0x6d, 0x52, 0x5b, 0x40, 0x49,
        0x3e, 0x37, 0x2c, 0x25, 0x1a, 0x13, 0x08, 0x01, 0xe6, 0xef, 0xf4, 0xfd, 0xc2, 0xcb, 0xd0, 0xd9, 0xae, 0xa7,
        0xbc, 0xb5, 0x8a, 0x83, 0x98, 0x91, 0x4d, 0x44, 0x5f, 0x56, 0x69, 0x60, 0x7b, 0x72, 0x05, 0x0c, 0x17, 0x1e,
        0x21, 0x28, 0x33, 0x3a, 0xdd, 0xd4, 0xcf, 0xc6, 0xf9, 0xf0, 0xeb, 0xe2, 0x95, 0x9c, 0x87, 0x8e, 0xb1, 0xb8,
        0xa3, 0xaa, 0xec, 0xe5, 0xfe, 0xf7, 0xc8, 0xc1, 0xda, 0xd3, 0xa4, 0xad, 0xb6, 0xbf, 0x80, 0x89, 0x92, 0x9b,
        0x7c, 0x75, 0x6e, 0x67, 0x58, 0x51, 0x4a, 0x43, 0x34, 0x3d, 0x26, 0x2f, 0x10, 0x19, 0x02, 0x0b, 0xd7, 0xde,
        0xc5, 0xcc, 0xf3, 0xfa, 0xe1, 0xe8, 0x9f, 0x96, 0x8d, 0x84, 0xbb, 0xb2, 0xa9, 0xa0, 0x47, 0x4e, 0x55, 0x5c,
        0x63, 0x6a, 0x71, 0x78, 0x0f, 0x06, 0x1d, 0x14, 0x2b, 0x22, 0x39, 0x30, 0x9a, 0x93, 0x88, 0x81, 0xbe, 0xb7,
        0xac, 0xa5, 0xd2, 0xdb, 0xc0, 0xc9, 0xf6, 0xff, 0xe4, 0xed, 0x0a, 0x03, 0x18, 0x11, 0x2e, 0x27, 0x3c, 0x35,
        0x42, 0x4b, 0x50, 0x59, 0x66, 0x6f, 0x74, 0x7d, 0xa1, 0xa8, 0xb3, 0xba, 0x85, 0x8c, 0x97, 0x9e, 0xe9, 0xe0,
        0xfb, 0xf2, 0xcd, 0xc4, 0xdf, 0xd6, 0x31, 0x38, 0x23, 0x2a, 0x15, 0x1c, 0x07, 0x0e, 0x79, 0x70, 0x6b, 0x62,
        0x5d, 0x54, 0x4f, 0x46,
      },

      {},

      // mul 11
      {
        0x00, 0x0b, 0x16, 0x1d, 0x2c, 0x27, 0x3a, 0x31, 0x58, 0x53, 0x4e, 0x45, 0x74, 0x7f, 0x62, 0x69, 0xb0, 0xbb,
        0xa6, 0xad, 0x9c, 0x97, 0x8a, 0x81, 0xe8, 0xe3, 0xfe, 0xf5, 0xc4, 0xcf, 0xd2, 0xd9, 0x7b, 0x70, 0x6d, 0x66,
        0x57, 0x5c, 0x41, 0x4a, 0x23, 0x28, 0x35, 0x3e, 0x0f, 0x04, 0x19, 0x12, 0xcb, 0xc0, 0xdd, 0xd6, 0xe7, 0xec,
        0xf1, 0xfa, 0x93, 0x98, 0x85, 0x8e, 0xbf, 0xb4, 0xa9, 0xa2, 0xf6, 0xfd, 0xe0, 0xeb, 0xda, 0xd1, 0xcc, 0xc7,
        0xae, 0xa5, 0xb8, 0xb3, 0x82, 0x89, 0x94, 0x9f, 0x46, 0x4d, 0x50, 0x5b, 0x6a, 0x61, 0x7c, 0x77, 0x1e, 0x15,
        0x08, 0x03, 0x32, 0x39, 0x24, 0x2f, 0x8d, 0x86, 0x9b, 0x90, 0xa1, 0xaa, 0xb7, 0xbc, 0xd5, 0xde, 0xc3, 0xc8,
        0xf9, 0xf2, 0xef, 0xe4, 0x3d, 0x36, 0x2b, 0x20, 0x11, 0x1a, 0x07, 0x0c, 0x65, 0x6e, 0x73, 0x78, 0x49, 0x42,
        0x5f, 0x54, 0xf7, 0xfc, 0xe1, 0xea, 0xdb, 0xd0, 0xcd, 0xc6, 0xaf, 0xa4, 0xb9, 0xb2, 0x83, 0x88, 0x95, 0x9e,
        0x47, 0x4c, 0x51, 0x5a, 0x6b, 0x60, 0x7d, 0x76, 0x1f, 0x14, 0x09, 0x02, 0x33, 0x38, 0x25, 0x2e, 0x8c, 0x87,
        0x9a, 0x91, 0xa0, 0xab, 0xb6, 0xbd, 0xd4, 0xdf, 0xc2, 0xc9, 0xf8, 0xf3, 0xee, 0xe5, 0x3c, 0x37, 0x2a, 0x21,
        0x10, 0x1b, 0x06, 0x0d, 0x64, 0x6f, 0x72, 0x79, 0x48, 0x43, 0x5e, 0x55, 0x01, 0x0a, 0x17, 0x1c, 0x2d, 0x26,
        0x3b, 0x30, 0x59, 0x52, 0x4f, 0x44, 0x75, 0x7e, 0x63, 0x68, 0xb1, 0xba, 0xa7, 0xac, 0x9d, 0x96, 0x8b, 0x80,
        0xe9, 0xe2, 0xff, 0xf4, 0xc5, 0xce, 0xd3, 0xd8, 0x7a, 0x71, 0x6c, 0x67, 0x56, 0x5d, 0x40, 0x4b, 0x22, 0x29,
        0x34, 0x3f, 0x0e, 0x05, 0x18, 0x13, 0xca, 0xc1, 0xdc, 0xd7, 0xe6, 0xed, 0xf0, 0xfb, 0x92, 0x99, 0x84, 0x8f,
        0xbe, 0xb5, 0xa8, 0xa3,
      },

      {},

      // mul 13
      {
        0x00, 0x0d, 0x1a, 0x17, 0x34, 0x39, 0x2e, 0x23, 0x68, 0x65, 0x72, 0x7f, 0x5c, 0x51, 0x46, 0x4b, 0xd0, 0xdd,
        0xca, 0xc7, 0xe4, 0xe9, 0xfe, 0xf3, 0xb8, 0xb5, 0xa2, 0xaf, 0x8c, 0x81, 0x96, 0x9b, 0xbb, 0xb6, 0xa1, 0xac,
        0x8f, 0x82, 0x95, 0x98, 0xd3, 0xde, 0xc9, 0xc4, 0xe7, 0xea, 0xfd, 0xf0, 0x6b, 0x66, 0x71, 0x7c, 0x5f, 0x52,
        0x45, 0x48, 0x03, 0x0e, 0x19, 0x14, 0x37, 0x3a, 0x2d, 0x20, 0x6d, 0x60, 0x77, 0x7a, 0x59, 0x54, 0x43, 0x4e,
        0x05, 0x08, 0x1f, 0x12, 0x31, 0x3c, 0x2b, 0x26, 0xbd, 0xb0, 0xa7, 0xaa, 0x89, 0x84, 0x93, 0x9e, 0xd5, 0xd8,
        0xcf, 0xc2, 0xe1, 0xec, 0xfb, 0xf6, 0xd6, 0xdb, 0xcc, 0xc1, 0xe2, 0xef, 0xf8, 0xf5, 0xbe, 0xb3, 0xa4, 0xa9,
        0x8a, 0x87, 0x90, 0x9d, 0x06, 0x0b, 0x1c, 0x11, 0x32, 0x3f, 0x28, 0x25, 0x6e, 0x63, 0x74, 0x79, 0x5a, 0x57,
        0x40, 0x4d, 0xda, 0xd7, 0xc0, 0xcd, 0xee, 0xe3, 0xf4, 0xf9, 0xb2, 0xbf, 0xa8, 0xa5, 0x86, 0x8b, 0x9c, 0x91,
        0x0a, 0x07, 0x10, 0x1d, 0x3e, 0x33, 0x24, 0x29, 0x62, 0x6f, 0x78, 0x75, 0x56, 0x5b, 0x4c, 0x41, 0x61, 0x6c,
        0x7b, 0x76, 0x55, 0x58, 0x4f, 0x42, 0x09, 0x04, 0x13, 0x1e, 0x3d, 0x30, 0x27, 0x2a, 0xb1, 0xbc, 0xab, 0xa6,
        0x85, 0x88, 0x9f, 0x92, 0xd9, 0xd4, 0xc3, 0xce, 0xed, 0xe0, 0xf7, 0xfa, 0xb7, 0xba, 0xad, 0xa0, 0x83, 0x8e,
        0x99, 0x94, 0xdf, 0xd2, 0xc5, 0xc8, 0xeb, 0xe6, 0xf1, 0xfc, 0x67, 0x6a, 0x7d, 0x70, 0x53, 0x5e, 0x49, 0x44,
        0x0f, 0x02, 0x15, 0x18, 0x3b, 0x36, 0x21, 0x2c, 0x0c, 0x01, 0x16, 0x1b, 0x38, 0x35, 0x22, 0x2f, 0x64, 0x69,
        0x7e, 0x73, 0x50, 0x5d, 0x4a, 0x47, 0xdc, 0xd1, 0xc6, 0xcb, 0xe8, 0xe5, 0xf2, 0xff, 0xb4, 0xb9, 0xae, 0xa3,
        0x80, 0x8d, 0x9a, 0x97,
      },

      // mul 14
      {
        0x00, 0x0e, 0x1c, 0x12, 0x38, 0x36, 0x24, 0x2a, 0x70, 0x7e, 0x6c, 0x62, 0x48, 0x46, 0x54, 0x5a, 0xe0, 0xee,
        0xfc, 0xf2, 0xd8, 0xd6, 0xc4, 0xca, 0x90, 0x9e, 0x8c, 0x82, 0xa8, 0xa6, 0xb4, 0xba, 0xdb, 0xd5, 0xc7, 0xc9,
        0xe3, 0xed, 0xff, 0xf1, 0xab, 0xa5, 0xb7, 0xb9, 0x93, 0x9d, 0x8f, 0x81, 0x3b, 0x35, 0x27, 0x29, 0x03, 0x0d,
        0x1f, 0x11, 0x4b, 0x45, 0x57, 0x59, 0x73, 0x7d, 0x6f, 0x61, 0xad, 0xa3, 0xb1, 0xbf, 0x95, 0x9b, 0x89, 0x87,
        0xdd, 0xd3, 0xc1, 0xcf, 0xe5, 0xeb, 0xf9, 0xf7, 0x4d, 0x43, 0x51, 0x5f, 0x75, 0x7b, 0x69, 0x67, 0x3d, 0x33,
        0x21, 0x2f, 0x05, 0x0b, 0x19, 0x17, 0x76, 0x78, 0x6a, 0x64, 0x4e, 0x40, 0x52, 0x5c, 0x06, 0x08, 0x1a, 0x14,
        0x3e, 0x30, 0x22, 0x2c, 0x96, 0x98, 0x8a, 0x84, 0xae, 0xa0, 0xb2, 0xbc, 0xe6, 0xe8, 0xfa, 0xf4, 0xde, 0xd0,
        0xc2, 0xcc, 0x41, 0x4f, 0x5d, 0x53, 0x79, 0x77, 0x65, 0x6b, 0x31, 0x3f, 0x2d, 0x23, 0x09, 0x07, 0x15, 0x1b,
        0xa1, 0xaf, 0xbd, 0xb3, 0x99, 0x97, 0x85, 0x8b, 0xd1, 0xdf, 0xcd, 0xc3, 0xe9, 0xe7, 0xf5, 0xfb, 0x9a, 0x94,
        0x86, 0x88, 0xa2, 0xac, 0xbe, 0xb0, 0xea, 0xe4, 0xf6, 0xf8, 0xd2, 0xdc, 0xce, 0xc0, 0x7a, 0x74, 0x66, 0x68,
        0x42, 0x4c, 0x5e, 0x50, 0x0a, 0x04, 0x16, 0x18, 0x32, 0x3c, 0x2e, 0x20, 0xec, 0xe2, 0xf0, 0xfe, 0xd4, 0xda,
        0xc8, 0xc6, 0x9c, 0x92, 0x80, 0x8e, 0xa4, 0xaa, 0xb8, 0xb6, 0x0c, 0x02, 0x10, 0x1e, 0x34, 0x3a, 0x28, 0x26,
        0x7c, 0x72, 0x60, 0x6e, 0x44, 0x4a, 0x58, 0x56, 0x37, 0x39, 0x2b, 0x25, 0x0f, 0x01, 0x13, 0x1d, 0x47, 0x49,
        0x5b, 0x55, 0x7f, 0x71, 0x63, 0x6d, 0xd7, 0xd9, 0xcb, 0xc5, 0xef, 0xe1, 0xf3, 0xfd, 0xa7, 0xa9, 0xbb, 0xb5,
        0x9f, 0x91, 0x83, 0x8d,
      },
    };

    /// circulant MDS matrix
    static constexpr unsigned char cmds[4][4] = {
      {2, 3, 1, 1},
      {1, 2, 3, 1},
      {1, 1, 2, 3},
      {3, 1, 1, 2},
    };

    /// Inverse circulant MDS matrix
    static constexpr unsigned char inverse_cmds[4][4] = {
      {14, 11, 13, 9},
      {9, 14, 11, 13},
      {13, 9, 14, 11},
      {11, 13, 9, 14},
    };

    void sub_bytes(unsigned char state[AES_BLOCK]) noexcept {
      for (size_t i = 0; i < AES_BLOCK; i += 4) {
        state[i] = sbox[state[i]];
        state[i + 1] = sbox[state[i + 1]];
        state[i + 2] = sbox[state[i + 2]];
        state[i + 3] = sbox[state[i + 3]];
      }
    }

    void inverse_sub_bytes(unsigned char state[AES_BLOCK]) noexcept {
      for (size_t i = 0; i < AES_BLOCK; i += 4) {
        state[i] = inverse_sbox[state[i]];
        state[i + 1] = inverse_sbox[state[i + 1]];
        state[i + 2] = inverse_sbox[state[i + 2]];
        state[i + 3] = inverse_sbox[state[i + 3]];
      }
    }

    void shift_rows(unsigned char state[AES_BLOCK]) noexcept {
      unsigned char temp = state[4];

      state[4] = state[5];
      state[5] = state[6];
      state[6] = state[7];
      state[7] = temp;

      temp = state[8];
      state[8] = state[10];
      state[10] = temp;
      temp = state[9];
      state[9] = state[11];
      state[11] = temp;

      temp = state[12];
      state[12] = state[15];
      state[15] = state[14];
      state[14] = state[13];
      state[13] = temp;
    }

    void inverse_shift_rows(unsigned char state[AES_BLOCK]) noexcept {
      unsigned char temp = state[4];

      state[4] = state[7];
      state[7] = state[6];
      state[6] = state[5];
      state[5] = temp;

      temp = state[8];
      state[8] = state[10];
      state[10] = temp;
      temp = state[9];
      state[9] = state[11];
      state[11] = temp;

      temp = state[12];
      state[12] = state[13];
      state[13] = state[14];
      state[14] = state[15];
      state[15] = temp;
    }

    void mix_columns(unsigned char state[AES_BLOCK]) noexcept {
      unsigned char gf_product[AES_BLOCK] = {};

      // TODO: analyse in the future if this loop can speed up by branchless programming.
      for (size_t i = 0; i < 4; ++i) {
        for (size_t k = 0; k < 4; ++k) {
          gf_product[i * 4] ^= (cmds[i][k] == 1) ? state[k * 4] : gf[cmds[i][k]][state[k * 4]];
          gf_product[i * 4 + 1] ^= (cmds[i][k] == 1) ? state[k * 4 + 1] : gf[cmds[i][k]][state[k * 4 + 1]];
          gf_product[i * 4 + 2] ^= (cmds[i][k] == 1) ? state[k * 4 + 2] : gf[cmds[i][k]][state[k * 4 + 2]];
          gf_product[i * 4 + 3] ^= (cmds[i][k] == 1) ? state[k * 4 + 3] : gf[cmds[i][k]][state[k * 4 + 3]];
        }
      }

      for (size_t i = 0; i < AES_BLOCK; i += 4) {
        state[i] = gf_product[i];
        state[i + 1] = gf_product[i + 1];
        state[i + 2] = gf_product[i + 2];
        state[i + 3] = gf_product[i + 3];
      }
    }

    void inverse_mix_columns(unsigned char state[AES_BLOCK]) noexcept {
      unsigned char gf_product[AES_BLOCK] = {};

      for (size_t i = 0; i < 4; ++i) {
        for (size_t k = 0; k < 4; ++k) {
          gf_product[i * 4] ^= gf[inverse_cmds[i][k]][state[k * 4]];
          gf_product[i * 4 + 1] ^= gf[inverse_cmds[i][k]][state[k * 4 + 1]];
          gf_product[i * 4 + 2] ^= gf[inverse_cmds[i][k]][state[k * 4 + 2]];
          gf_product[i * 4 + 3] ^= gf[inverse_cmds[i][k]][state[k * 4 + 3]];
        }
      }

      for (size_t i = 0; i < AES_BLOCK; i += 4) {
        state[i] = gf_product[i];
        state[i + 1] = gf_product[i + 1];
        state[i + 2] = gf_product[i + 2];
        state[i + 3] = gf_product[i + 3];
      }
    }

    void add_round_key(unsigned char state[AES_BLOCK], unsigned char *key) noexcept {
      for (size_t i = 0; i < 4; ++i) {
        state[i * 4] ^= key[i];
        state[i * 4 + 1] ^= key[i + 4];
        state[i * 4 + 2] ^= key[i + 8];
        state[i * 4 + 3] ^= key[i + 12];
      }
    }

    void sub_dword(unsigned char *dword) noexcept {
      dword[0] = sbox[dword[0]];
      dword[1] = sbox[dword[1]];
      dword[2] = sbox[dword[2]];
      dword[3] = sbox[dword[3]];
    }

    void rot_dword(unsigned char *dword) noexcept {
      unsigned char temp = dword[0];
      dword[0] = dword[1];
      dword[1] = dword[2];
      dword[2] = dword[3];
      dword[3] = temp;
    }

    void xor_dword(unsigned char *dest, unsigned char *a, unsigned char *b) noexcept {
      dest[0] = a[0] ^ b[0];
      dest[1] = a[1] ^ b[1];
      dest[2] = a[2] ^ b[2];
      dest[3] = a[3] ^ b[3];
    }

    void rcon_n(unsigned char *dword, size_t n) noexcept {
      unsigned char cbyte = 0x01;
      for (size_t i = 0; i < n - 1; ++i) {
        cbyte = (cbyte << 1) ^ (((cbyte >> 7) & 1) * 0x1b);
      }

      dword[0] = cbyte;
      dword[1] = dword[2] = dword[3] = 0x00;
    }
#endif

    void state_transpose(unsigned char *state) noexcept {
      for (size_t i = 0; i < 4; i++) {
        for (size_t j = 0; j < i; j++) {
          std::swap(state[i * 4 + j], state[j * 4 + i]);
        }
      }
    }

    public:

#ifdef HARDWARE_ACCELERATION_INTEL_AESNI
    static constexpr const char AES_TECHNOLOGY[] = "INTEL AES-NI";
#elif defined(HARDWARE_ACCELERATION_ARM_NEON_AES)
    static constexpr const char AES_TECHNOLOGY[] = "ARM NEON AES";
#else
    static constexpr const char AES_TECHNOLOGY[] = "Plain C++ AES";
#endif

    /**
     * @param key A `unsigned char *` array that contains the AES key.
     * This key should either be **16, 24, 32** bytes, or `128`, `192`, `256` bits.
     */
    Aes(unsigned char key[key_bits]) : round_keys() {
      constexpr bool invalid_aes_key_bit_size = key_bits == 128 || key_bits == 192 || key_bits == 256;
      static_assert(invalid_aes_key_bit_size, "The valid values are only: 128, 192 & 256");

#ifdef HARDWARE_ACCELERATION_INTEL_AESNI
      if constexpr (key_bits == 128) {
        AES_128_Key_Expansion(key, round_keys);
      } else if constexpr (key_bits == 192) {
        AES_192_Key_Expansion(key, round_keys);
      } else if constexpr (key_bits == 256) {
        AES_256_Key_Expansion(key, round_keys);
      }
#elif defined(HARDWARE_ACCELERATION_ARM_NEON_AES)
      // key expansion
      unsigned char temp[4];
      unsigned char rcon[4];

      size_t i = 0;

      while (i < Nk * 4) {
        round_keys[i] = key[i];
        round_keys[i + 1] = key[i + 1];
        i += 2;
      }

      while (i < round_keys_size) {
        temp[0] = round_keys[i - 4];
        temp[1] = round_keys[i - 4 + 1];
        temp[2] = round_keys[i - 4 + 2];
        temp[3] = round_keys[i - 4 + 3];

        if (i / 4 % Nk == 0) {
          rot_dword(temp);
          sub_dword(temp);
          rcon_n(rcon, i / (Nk * 4));
          xor_dword(temp, rcon, temp);
        } else if (Nk > 6 && i / 4 % Nk == 4) {
          sub_dword(temp);
        }

        round_keys[i + 0] = round_keys[i - 4 * Nk] ^ temp[0];
        round_keys[i + 1] = round_keys[i + 1 - 4 * Nk] ^ temp[1];
        round_keys[i + 2] = round_keys[i + 2 - 4 * Nk] ^ temp[2];
        round_keys[i + 3] = round_keys[i + 3 - 4 * Nk] ^ temp[3];

        i += 4;
      }
#else
      // key expansion
      unsigned char temp[4];
      unsigned char rcon[4];

      size_t i = 0;

      while (i < Nk * 4) {
        round_keys[i] = key[i];
        round_keys[i + 1] = key[i + 1];
        i += 2;
      }

      while (i < round_keys_size) {
        temp[0] = round_keys[i - 4];
        temp[1] = round_keys[i - 4 + 1];
        temp[2] = round_keys[i - 4 + 2];
        temp[3] = round_keys[i - 4 + 3];

        if (i / 4 % Nk == 0) {
          rot_dword(temp);
          sub_dword(temp);
          rcon_n(rcon, i / (Nk * 4));
          xor_dword(temp, rcon, temp);
        } else if (Nk > 6 && i / 4 % Nk == 4) {
          sub_dword(temp);
        }

        round_keys[i + 0] = round_keys[i - 4 * Nk] ^ temp[0];
        round_keys[i + 1] = round_keys[i + 1 - 4 * Nk] ^ temp[1];
        round_keys[i + 2] = round_keys[i + 2 - 4 * Nk] ^ temp[2];
        round_keys[i + 3] = round_keys[i + 3 - 4 * Nk] ^ temp[3];

        i += 4;
      }
#endif
    }

    ~Aes() {
      std::memset(round_keys, 0x00, round_keys_size);
    }

    /// @brief Performs AES encryption to a 16 byte block of memory.
    ///
    /// @note This method will overwrite the input block of memory.
    ///
    /// @param block 16 byte block of memory.
    void encrypt_block(unsigned char *block) {
#ifdef HARDWARE_ACCELERATION_INTEL_AESNI
      // load the current block & current round key into the registers
      __m128i *xmm_round_keys = (__m128i *) round_keys;
      __m128i state = _mm_loadu_si128((__m128i *) &block[0]);

      // original key
      state = _mm_xor_si128(state, xmm_round_keys[0]);

      // perform usual rounds
      for (size_t i = 1; i < Nr - 1; i += 2) {
        state = _mm_aesenc_si128(state, xmm_round_keys[i]);
        state = _mm_aesenc_si128(state, xmm_round_keys[i + 1]);
      }

      // last round
      state = _mm_aesenc_si128(state, xmm_round_keys[Nr - 1]);
      state = _mm_aesenclast_si128(state, xmm_round_keys[Nr]);

      // store from register to array
      _mm_storeu_si128((__m128i *) (block), state);
#elif defined(HARDWARE_ACCELERATION_ARM_NEON_AES)
      uint8x16_t *neon_round_keys = (uint8x16_t *) round_keys;
      uint8x16_t state = vld1q_u8(block);

      // Initial round
      state = vaesmcq_u8(vaeseq_u8(state, neon_round_keys[0]));

      // 8 main rounds
      for (size_t i = 1; i < Nr - 1; i += 2) {
        state = vaesmcq_u8(vaeseq_u8(state, neon_round_keys[i]));
        state = vaesmcq_u8(vaeseq_u8(state, neon_round_keys[i + 1]));
      }

      // last 2 final round
      state = vaeseq_u8(state, neon_round_keys[Nr - 1]);
      state = veorq_u8(state, neon_round_keys[Nr]);

      // store the result to block
      vst1q_u8(block, state);
#else
      state_transpose(block);

      add_round_key(block, &round_keys[0]);

      for (size_t round = 1; round <= Nr - 1; ++round) {
        sub_bytes(block);
        shift_rows(block);
        mix_columns(block);
        add_round_key(block, &round_keys[round * Nb * 4]);
      }

      sub_bytes(block);
      shift_rows(block);
      add_round_key(block, &round_keys[Nb * Nr * 4]);

      state_transpose(block);
#endif
    }

    /// @brief Performs AES decryption to a 16 byte block of memory.
    ///
    /// @note This method will overwrite the input block of memory.
    ///
    /// @param block 16 byte block of memory.
    void decrypt_block(unsigned char *block) {
#ifdef HARDWARE_ACCELERATION_INTEL_AESNI
      // load the current block & current round key into the registers
      __m128i *xmm_round_keys = (__m128i *) round_keys;
      __m128i state = _mm_loadu_si128((__m128i *) &block[0]);

      // first round
      state = _mm_xor_si128(state, xmm_round_keys[Nr]);

      // usual rounds
      for (size_t i = Nr - 1; i > 1; i -= 2) {
        state = _mm_aesdec_si128(state, _mm_aesimc_si128(xmm_round_keys[i]));
        state = _mm_aesdec_si128(state, _mm_aesimc_si128(xmm_round_keys[i - 1]));
      }

      // last round
      state = _mm_aesdec_si128(state, _mm_aesimc_si128(xmm_round_keys[1]));
      state = _mm_aesdeclast_si128(state, xmm_round_keys[0]);

      // store from register to array
      _mm_storeu_si128((__m128i *) block, state);
#elif defined(HARDWARE_ACCELERATION_ARM_NEON_AES)
      uint8x16_t *neon_round_keys = (uint8x16_t *) round_keys;
      uint8x16_t state = vld1q_u8(block);

      // Initial round
      state = vaesimcq_u8(vaesdq_u8(state, neon_round_keys[Nr]));

      // 8 main rounds
      for (size_t i = Nr - 1; i > 1; i -= 2) {
        state = vaesimcq_u8(vaesdq_u8(state, vaesimcq_u8(neon_round_keys[i])));
        state = vaesimcq_u8(vaesdq_u8(state, vaesimcq_u8(neon_round_keys[i - 1])));
      }

      // final 2 rounds
      state = vaesdq_u8(state, vaesimcq_u8(neon_round_keys[1]));
      state = veorq_u8(state, neon_round_keys[0]);

      // store the result to recover
      vst1q_u8(block, state);
#else
      state_transpose(block);

      add_round_key(block, &round_keys[Nb * Nr * 4]);

      for (size_t round = Nr - 1; round > 0; --round) {
        inverse_sub_bytes(block);
        inverse_shift_rows(block);
        add_round_key(block, &round_keys[round * Nb * 4]);
        inverse_mix_columns(block);
      }

      inverse_sub_bytes(block);
      inverse_shift_rows(block);
      add_round_key(block, &round_keys[0]);

      state_transpose(block);
#endif
    }
  };
} // namespace Cipher

#endif