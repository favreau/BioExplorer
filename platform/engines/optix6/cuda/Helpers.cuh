/*
 *
 * The Blue Brain BioExplorer is a tool for scientists to extract and analyse
 * scientific data from visualization
 *
 * This file is part of Blue Brain BioExplorer <https://github.com/BlueBrain/BioExplorer>
 *
 * Copyright 2020-2023 Blue BrainProject / EPFL
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "Context.cuh"

#include <optixu/optixu_math_namespace.h>

// Convert a float3 in [0,1)^3 to a uchar4 in [0,255]^4 -- 4th channel is set to 255
#ifdef __CUDACC__
static __device__ __inline__ optix::uchar4 make_color(const optix::float4& c)
{
    return optix::make_uchar4(static_cast<unsigned char>(__saturatef(c.x) * 255.99f),  /* R */
                              static_cast<unsigned char>(__saturatef(c.y) * 255.99f),  /* G */
                              static_cast<unsigned char>(__saturatef(c.z) * 255.99f),  /* B */
                              static_cast<unsigned char>(__saturatef(c.w) * 255.99f)); /* A */
}
#endif

// Sample Phong lobe relative to U, V, W frame
static __host__ __device__ __inline__ optix::float3 sample_phong_lobe(optix::float2 sample, float exponent,
                                                                      optix::float3 U, optix::float3 V, optix::float3 W)
{
    const float power = expf(logf(sample.y) / (exponent + 1.0f));
    const float phi = sample.x * 2.0f * (float)M_PIf;
    const float scale = sqrtf(1.0f - power * power);

    const float x = cosf(phi) * scale;
    const float y = sinf(phi) * scale;
    const float z = power;

    return x * U + y * V + z * W;
}

static __device__ inline bool boxIntersection(const float3& volumeOffset, const float3& volumeDimensions,
                                              const float3& volumeElementSpacing, const optix::Ray& ray, float& t0,
                                              float& t1)
{
    const float3 boxmin = volumeOffset;
    const float3 boxmax = volumeOffset + volumeDimensions / volumeElementSpacing;

    const float3 a = (boxmin - ray.origin) / ray.direction;
    const float3 b = (boxmax - ray.origin) / ray.direction;
    const float3 near = fminf(a, b);
    const float3 far = fmaxf(a, b);
    t0 = fmaxf(near);
    t1 = fminf(far);

    return (t0 <= t1);
}

// Sample Phong lobe relative to U, V, W frame
static __host__ __device__ __inline__ optix::float3 sample_phong_lobe(const optix::float2& sample, float exponent,
                                                                      const optix::float3& U, const optix::float3& V,
                                                                      const optix::float3& W, float& pdf,
                                                                      float& bdf_val)
{
    const float cos_theta = powf(sample.y, 1.0f / (exponent + 1.0f));

    const float phi = sample.x * 2.0f * M_PIf;
    const float sin_theta = sqrtf(1.0f - cos_theta * cos_theta);

    const float x = cosf(phi) * sin_theta;
    const float y = sinf(phi) * sin_theta;
    const float z = cos_theta;

    const float powered_cos = powf(cos_theta, exponent);
    pdf = (exponent + 1.0f) / (2.0f * M_PIf) * powered_cos;
    bdf_val = (exponent + 2.0f) / (2.0f * M_PIf) * powered_cos;

    return x * U + y * V + z * W;
}

// Get Phong lobe PDF for local frame
static __host__ __device__ __inline__ float get_phong_lobe_pdf(float exponent, const optix::float3& normal,
                                                               const optix::float3& dir_out,
                                                               const optix::float3& dir_in, float& bdf_val)
{
    using namespace optix;

    float3 r = -reflect(dir_out, normal);
    const float cos_theta = fabs(dot(r, dir_in));
    const float powered_cos = powf(cos_theta, exponent);

    bdf_val = (exponent + 2.0f) / (2.0f * M_PIf) * powered_cos;
    return (exponent + 1.0f) / (2.0f * M_PIf) * powered_cos;
}

// Create ONB from normal.  Resulting W is parallel to normal
static __host__ __device__ __inline__ void create_onb(const optix::float3& n, optix::float3& U, optix::float3& V,
                                                      optix::float3& W)
{
    using namespace optix;

    W = normalize(n);
    U = cross(W, optix::make_float3(0.0f, 1.0f, 0.0f));

    if (fabs(U.x) < 0.001f && fabs(U.y) < 0.001f && fabs(U.z) < 0.001f)
        U = cross(W, make_float3(1.0f, 0.0f, 0.0f));

    U = normalize(U);
    V = cross(W, U);
}

// Create ONB from normalized vector
static __device__ __inline__ void create_onb(const optix::float3& n, optix::float3& U, optix::float3& V)
{
    using namespace optix;

    U = cross(n, make_float3(0.0f, 1.0f, 0.0f));

    if (dot(U, U) < 1e-3f)
        U = cross(n, make_float3(1.0f, 0.0f, 0.0f));

    U = normalize(U);
    V = cross(n, U);
}

// Compute the origin ray differential for transfer
static __host__ __device__ __inline__ optix::float3 differential_transfer_origin(optix::float3 dPdx, optix::float3 dDdx,
                                                                                 float t, optix::float3 direction,
                                                                                 optix::float3 normal)
{
    float dtdx = -optix::dot((dPdx + t * dDdx), normal) / optix::dot(direction, normal);
    return (dPdx + t * dDdx) + dtdx * direction;
}

// Compute the direction ray differential for a pinhole camera
static __host__ __device__ __inline__ optix::float3 differential_generation_direction(optix::float3 d,
                                                                                      optix::float3 basis)
{
    float dd = optix::dot(d, d);
    return (dd * basis - optix::dot(d, basis) * d) / (dd * sqrtf(dd));
}

// Compute the direction ray differential for reflection
static __host__ __device__ __inline__ optix::float3 differential_reflect_direction(optix::float3 dPdx,
                                                                                   optix::float3 dDdx,
                                                                                   optix::float3 dNdP, optix::float3 D,
                                                                                   optix::float3 N)
{
    using namespace optix;

    float3 dNdx = dNdP * dPdx;
    float dDNdx = dot(dDdx, N) + dot(D, dNdx);
    return dDdx - 2 * (dot(D, N) * dNdx + dDNdx * N);
}

// Compute the direction ray differential for refraction
static __host__ __device__ __inline__ optix::float3 differential_refract_direction(optix::float3 dPdx,
                                                                                   optix::float3 dDdx,
                                                                                   optix::float3 dNdP, optix::float3 D,
                                                                                   optix::float3 N, float ior,
                                                                                   optix::float3 T)
{
    using namespace optix;

    float eta;
    if (dot(D, N) > 0.f)
    {
        eta = ior;
        N = -N;
    }
    else
    {
        eta = 1.f / ior;
    }

    float3 dNdx = dNdP * dPdx;
    float mu = eta * dot(D, N) - dot(T, N);
    float TN = -sqrtf(1 - eta * eta * (1 - dot(D, N) * dot(D, N)));
    float dDNdx = dot(dDdx, N) + dot(D, dNdx);
    float dmudx = (eta - (eta * eta * dot(D, N)) / TN) * dDNdx;
    return eta * dDdx - (mu * dNdx + dmudx * N);
}

// Color space conversions
static __host__ __device__ __inline__ optix::float3 Yxy2XYZ(const optix::float3& Yxy)
{
    // avoid division by zero
    if (Yxy.z < 1e-4)
        return optix::make_float3(0.0f, 0.0f, 0.0f);

    return optix::make_float3(Yxy.y * (Yxy.x / Yxy.z), Yxy.x, (1.0f - Yxy.y - Yxy.z) * (Yxy.x / Yxy.z));
}

static __host__ __device__ __inline__ optix::float3 XYZ2rgb(const optix::float3& xyz)
{
    const float R = optix::dot(xyz, optix::make_float3(3.2410f, -1.5374f, -0.4986f));
    const float G = optix::dot(xyz, optix::make_float3(-0.9692f, 1.8760f, 0.0416f));
    const float B = optix::dot(xyz, optix::make_float3(0.0556f, -0.2040f, 1.0570f));
    return optix::make_float3(R, G, B);
}

static __host__ __device__ __inline__ optix::float3 Yxy2rgb(optix::float3 Yxy)
{
    using namespace optix;

    // avoid division by zero
    if (Yxy.z < 1e-4)
        return make_float3(0.0f, 0.0f, 0.0f);

    // First convert to xyz
    float3 xyz = make_float3(Yxy.y * (Yxy.x / Yxy.z), Yxy.x, (1.0f - Yxy.y - Yxy.z) * (Yxy.x / Yxy.z));

    const float R = dot(xyz, make_float3(3.2410f, -1.5374f, -0.4986f));
    const float G = dot(xyz, make_float3(-0.9692f, 1.8760f, 0.0416f));
    const float B = dot(xyz, make_float3(0.0556f, -0.2040f, 1.0570f));
    return make_float3(R, G, B);
}

static __host__ __device__ __inline__ optix::float3 rgb2Yxy(optix::float3 rgb)
{
    using namespace optix;

    // convert to xyz
    const float X = dot(rgb, make_float3(0.4124f, 0.3576f, 0.1805f));
    const float Y = dot(rgb, make_float3(0.2126f, 0.7152f, 0.0722f));
    const float Z = dot(rgb, make_float3(0.0193f, 0.1192f, 0.9505f));

    // avoid division by zero
    // here we make the simplifying assumption that X, Y, Z are positive
    float denominator = X + Y + Z;
    if (denominator < 1e-4)
        return make_float3(0.0f, 0.0f, 0.0f);

    // convert xyz to Yxy
    return make_float3(Y, X / (denominator), Y / (denominator));
}

static __host__ __device__ __inline__ optix::float3 tonemap(const optix::float3& hdr_value, float Y_log_av, float Y_max)
{
    using namespace optix;

    float3 val_Yxy = rgb2Yxy(hdr_value);

    float Y = val_Yxy.x; // Y channel is luminance
    const float a = 0.04f;
    float Y_rel = a * Y / Y_log_av;
    float mapped_Y = Y_rel * (1.0f + Y_rel / (Y_max * Y_max)) / (1.0f + Y_rel);

    float3 mapped_Yxy = make_float3(mapped_Y, val_Yxy.y, val_Yxy.z);
    float3 mapped_rgb = Yxy2rgb(mapped_Yxy);

    return mapped_rgb;
}

static __device__ inline optix::float3 tonemap(const optix::float3& color)
{
    return color / (color + make_float3(1.0f));
}

static __device__ inline optix::float2 getEquirectangularUV(const optix::float3& R)
{
    return make_float2(atan2f(R.z, R.x) * M_1_PIf / 2.f + 0.5f, asinf(R.y) * M_1_PIf + 0.5f);
}

static __device__ inline float3 max(const float3& a, const float3& b)
{
    return make_float3(max(a.x, b.x), max(a.y, b.y), max(a.z, b.z));
}

static __device__ inline float3 pow(const float3& a, const float exp)
{
    return make_float3(pow(a.x, exp), pow(a.y, exp), pow(a.z, exp));
}

static __device__ inline float4 SRGBtoLinear(const float4& srgb)
{
    return make_float4(pow(make_float3(srgb), 2.2f), srgb.w);
}

static __device__ inline float3 linearToSRGB(const float3& color)
{
    return pow(color, 1.f / 2.2f);
}

static __device__ void applyClippingPlanes(const float3& origin, const float3& direction, float& near, float& far)
{
    for (int i = 0; i < nbClippingPlanes; ++i)
    {
        float4 clipPlane = clippingPlanes[i];
        const float3 planeNormal = {clipPlane.x, clipPlane.y, clipPlane.z};
        float rn = ::optix::dot(direction, planeNormal);
        if (rn == 0.f)
            rn = sceneEpsilon;
        float d = clipPlane.w;
        float t = -(::optix::dot(planeNormal, origin) + d) / rn;
        if (rn > 0.f) // opposite direction plane
            near = max(near, t);
        else
            far = min(far, t);
    }
}

static __device__ void compose(const float4& src, float4& dst, const float alphaCorrection)
{
    const float alpha = alphaCorrection * src.w;
    dst =
        make_float4((1.f - dst.w) * alpha * make_float3(src) + dst.w * make_float3(dst), dst.w + alpha * (1.f - dst.w));
}

static __device__ inline float3 frac(const float3 x)
{
    return x - optix::floor(x);
}

static __device__ inline float mix(const float x, const float y, const float a)
{
    return x * (1.f - a) + y * a;
}

static __device__ inline float hash(float n)
{
    return frac(make_float3(sin(n + 1.951f) * 43758.5453f)).x;
}

static __device__ inline float hash(const float2& x)
{
    return hash(x.x + hash(x.y));
}

static __device__ float noise(const float3& x)
{
    // hash based 3d value noise
    float3 p = optix::floor(x);
    float3 f = frac(x);

    f = f * f * (make_float3(3.0f) - make_float3(2.0f) * f);
    float n = p.x + p.y * 57.0f + 113.0f * p.z;
    return mix(mix(mix(hash(n + 0.0f), hash(n + 1.0f), f.x), mix(hash(n + 57.0f), hash(n + 58.0f), f.x), f.y),
               mix(mix(hash(n + 113.0f), hash(n + 114.0f), f.x), mix(hash(n + 170.0f), hash(n + 171.0f), f.x), f.y),
               f.z);
}

static __device__ inline float3 mod(const float3& v, const int m)
{
    return make_float3(v.x - m * floor(v.x / m), v.y - m * floor(v.y / m), v.z - m * floor(v.z / m));
}

static __device__ float cells(const float3& p, float cellCount)
{
    const float3 pCell = p * cellCount;
    float d = 1.0e10;
    for (int xo = -1; xo <= 1; xo++)
    {
        for (int yo = -1; yo <= 1; yo++)
        {
            for (int zo = -1; zo <= 1; zo++)
            {
                float3 tp = floor(pCell) + make_float3(xo, yo, zo);

                tp = pCell - tp - noise(mod(tp, cellCount / 1));

                d = min(d, optix::dot(tp, tp));
            }
        }
    }
    d = min(d, 1.0f);
    d = max(d, 0.0f);
    return d;
}

static __device__ float worleyNoise(const float3& p, float cellCount)
{
    return cells(p, cellCount);
}

static __device__ float3 refractedVector(const float3 direction, const float3 normal, const float n1, const float n2)
{
    if (n2 == 0.f)
        return direction;
    const float eta = n1 / n2;
    const float cos1 = -optix::dot(direction, normal);
    const float cos2 = 1.f - eta * eta * (1.f - cos1 * cos1);
    if (cos2 > 0.f)
        return ::optix::normalize(eta * direction + (eta * cos1 - sqrt(cos2)) * normal);
    return direction;
}

#define OPTIX_DUMP_FLOAT(VALUE) rtPrintf(#VALUE " %f\n", VALUE)
#define OPTIX_DUMP_INT(VALUE) rtPrintf(#VALUE " %i\n", VALUE)
