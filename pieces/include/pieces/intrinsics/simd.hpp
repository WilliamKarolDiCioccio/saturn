#pragma once

#if defined(SIMD_X86_AVX512F)
#include <immintrin.h>
#define SIMD_LEVEL_NAME "AVX-512F"
#elif defined(SIMD_X86_AVX2)
#include <immintrin.h>
#define SIMD_LEVEL_NAME "AVX2"
#elif defined(SIMD_X86_AVX)
#include <immintrin.h>
#define SIMD_LEVEL_NAME "AVX"
#elif defined(SIMD_X86_SSE4_1)
#include <smmintrin.h>
#define SIMD_LEVEL_NAME "SSE4.1"
#elif defined(SIMD_X86_SSE2)
#include <emmintrin.h>
#define SIMD_LEVEL_NAME "SSE2"
#elif defined(SIMD_ARM_NEON)
#include <arm_neon.h>
#define SIMD_LEVEL_NAME "NEON"
#else
#define SIMD_LEVEL_NAME "SCALAR"
#endif

// Function attributes
#if defined(_MSC_VER)
#define SIMD_FORCE_INLINE __forceinline
#define SIMD_ALIGN(n) __declspec(align(n))
#else
#define SIMD_FORCE_INLINE inline __attribute__((always_inline))
#define SIMD_ALIGN(n) __attribute__((aligned(n)))
#endif

// Branch hints (GCC/Clang only; MSVC ignores)
#ifndef SIMD_LIKELY
#if defined(__GNUC__) || defined(__clang__)
#define SIMD_LIKELY(x) __builtin_expect(!!(x), 1)
#define SIMD_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define SIMD_LIKELY(x) (x)
#define SIMD_UNLIKELY(x) (x)
#endif
#endif

// A portable restrict
#if defined(_MSC_VER)
#define SIMD_RESTRICT __restrict
#else
#define SIMD_RESTRICT __restrict__
#endif

// Prefetch hint (only valid on x86; no-op elsewhere)
#if defined(SIMD_X86_SSE2) || defined(SIMD_X86_SSE4_1) || defined(SIMD_X86_AVX) || \
    defined(SIMD_X86_AVX2) || defined(SIMD_X86_AVX512F)
#define SIMD_PREFETCH(addr) _mm_prefetch((const char*)(addr), _MM_HINT_T0)
#else
#define SIMD_PREFETCH(addr) ((void)0)
#endif

// Generic load/store/zero macros
#if defined(SIMD_ARM_NEON)

#define SIMDF_LOAD_PS(p) vld1q_f32(p)
#define SIMDF_STORE_PS(p, v) vst1q_f32(p, v)
#define SIMDF_ZERO() vdupq_n_f32(0.0f)

#define SIMDI_LOAD_EPI32(p) vld1q_s32(p)
#define SIMDI_STORE_EPI32(p, v) vst1q_s32(p, v)
#define SIMDI_ZERO() vdupq_n_s32(0)

#elif defined(SIMD_X86_SSE2) || defined(SIMD_X86_SSE4_1)

#define SIMDF_LOAD_PS(p) _mm_loadu_ps(p)
#define SIMDF_STORE_PS(p, v) _mm_storeu_ps(p, v)
#define SIMDF_ZERO() _mm_setzero_ps()

#define SIMDI_LOAD_EPI32(p) _mm_loadu_si128((__m128i const*)(p))
#define SIMDI_STORE_EPI32(p, v) _mm_storeu_si128((__m128i*)(p), v)
#define SIMDI_ZERO() _mm_setzero_si128()

#elif defined(SIMD_X86_AVX) || defined(SIMD_X86_AVX2) || defined(SIMD_X86_AVX512F)

#define SIMDF_LOAD_PS(p) _mm256_loadu_ps(p)
#define SIMDF_STORE_PS(p, v) _mm256_storeu_ps(p, v)
#define SIMDF_ZERO() _mm256_setzero_ps()

#define SIMDI_LOAD_EPI32(p) _mm256_loadu_si256((__m256i const*)(p))
#define SIMDI_STORE_EPI32(p, v) _mm256_storeu_si256((__m256i*)(p), v)
#define SIMDI_ZERO() _mm256_setzero_si256()

#else

#define SIMDF_LOAD_PS(p) (*(float*)(p))
#define SIMDF_STORE_PS(p, v) (*(float*)(p) = (v))
#define SIMDF_ZERO() (0.0f)

#define SIMDI_LOAD_EPI32(p) (*(int*)(p))
#define SIMDI_STORE_EPI32(p, v) (*(int*)(p) = (v))
#define SIMDI_ZERO() (0)

#endif

// Utility: horizontal sum of float vector
#if defined(SIMD_X86_SSE4_1)
#define SIMD_HSUM_PS(v) _mm_cvtss_f32(_mm_hadd_ps(v, v))
#elif defined(SIMD_ARM_NEON)
static SIMD_FORCE_INLINE float simd_hsum_f32(float32x4_t v)
{
    float32x2_t lo = vget_low_f32(v);
    float32x2_t hi = vget_high_f32(v);
    float32x2_t sum = vadd_f32(lo, hi);
    sum = vpadd_f32(sum, sum);
    return vget_lane_f32(sum, 0);
}

#define SIMD_HSUM_PS(v) simd_hsum_f32(v)
#else
static SIMD_FORCE_INLINE float simd_hsum_f32(float* p) { return p[0] + p[1] + p[2] + p[3]; }

#define SIMD_HSUM_PS(v) simd_hsum_f32((float*)&(v))
#endif
