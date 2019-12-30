#ifndef JOY_SIMD_H
#define JOY_SIMD_H

#define MM(mm, i) (mm).m128_f32[i]
#define MMI(mm, i) (mm).m128i_u32[i]

#define MM_UNPACK_COLOR_CHANNEL(texel, shift) _mm_mul_ps(_mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(texel, shift), mmFF)), mmOneOver255)

#define MM_UNPACK_COLOR_CHANNEL0(texel) _mm_mul_ps(_mm_cvtepi32_ps(_mm_and_si128(texel, mmFF)), mmOneOver255)

#define MM_LERP(a, b, t) _mm_add_ps(a, _mm_mul_ps(_mm_sub_ps(b, a), t))


#endif