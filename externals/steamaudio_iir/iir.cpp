//
// Copyright 2017-2023 Valve Corporation.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include "iir.h"

// #include "context.h"
#include "math_functions.h"

namespace ipl
{
// --------------------------------------------------------------------------------------------------------------------
// IIRFilterer
// --------------------------------------------------------------------------------------------------------------------

IIRFilterer::IIRFilterer()
{
#if defined(IPL_ENABLE_FLOAT8)
    mResetFilterDispatch =
        (gSIMDLevel() >= SIMDLevel::AVX) ? &IIRFilterer::resetFilter_float8 : &IIRFilterer::resetFilter_float4;
    mSetFilterDispatch =
        (gSIMDLevel() >= SIMDLevel::AVX) ? &IIRFilterer::setFilter_float8 : &IIRFilterer::setFilter_float4;
    mApplyDispatch = (gSIMDLevel() >= SIMDLevel::AVX) ? &IIRFilterer::apply_float8 : &IIRFilterer::apply_float4;
#else
    mResetFilterDispatch = &IIRFilterer::resetFilter_float4;
    mSetFilterDispatch = &IIRFilterer::setFilter_float4;
    mApplyDispatch = &IIRFilterer::apply_float4;
#endif

    resetFilter();
    reset();
}

void IIRFilterer::reset()
{
    mXm1 = 0.0f;
    mXm2 = 0.0f;
    mYm1 = 0.0f;
    mYm2 = 0.0f;
}

void IIRFilterer::copyState(const IIRFilterer& other)
{
    mXm1 = other.mXm1;
    mXm2 = other.mXm2;
    mYm1 = other.mYm1;
    mYm2 = other.mYm2;
}

void IIRFilterer::resetFilter()
{
    (this->*mResetFilterDispatch)();
}

void IIRFilterer::resetFilter_float4()
{
    memset(mCoeffs4, 0, 4 * 8 * sizeof(float));
}

void IIRFilterer::setFilter(const IIR& filter)
{
    (this->*mSetFilterDispatch)(filter);
}

void IIRFilterer::setFilter_float4(const IIR& filter)
{
    mFilter = filter;

    mCoeffs4[0][0] = 0.0f;
    mCoeffs4[1][0] = 0.0f;
    mCoeffs4[2][0] = 0.0f;
    mCoeffs4[3][0] = filter.b0;
    mCoeffs4[4][0] = filter.b1;
    mCoeffs4[5][0] = filter.b2;
    mCoeffs4[6][0] = -filter.a1;
    mCoeffs4[7][0] = -filter.a2;

    mCoeffs4[0][1] = 0.0f;
    mCoeffs4[1][1] = 0.0f;
    mCoeffs4[2][1] = filter.b0;
    mCoeffs4[3][1] = filter.b1;
    mCoeffs4[4][1] = filter.b2;
    mCoeffs4[5][1] = 0.0f;
    mCoeffs4[6][1] = -filter.a2;
    mCoeffs4[7][1] = 0.0f;

    mCoeffs4[0][2] = 0.0f;
    mCoeffs4[1][2] = filter.b0;
    mCoeffs4[2][2] = filter.b1;
    mCoeffs4[3][2] = filter.b2;
    mCoeffs4[4][2] = 0.0f;
    mCoeffs4[5][2] = 0.0f;
    mCoeffs4[6][2] = 0.0f;
    mCoeffs4[7][2] = 0.0f;

    mCoeffs4[0][3] = filter.b0;
    mCoeffs4[1][3] = filter.b1;
    mCoeffs4[2][3] = filter.b2;
    mCoeffs4[3][3] = 0.0f;
    mCoeffs4[4][3] = 0.0f;
    mCoeffs4[5][3] = 0.0f;
    mCoeffs4[6][3] = 0.0f;
    mCoeffs4[7][3] = 0.0f;

    for (auto i = 0; i < 8; ++i)
    {
        mCoeffs4[i][1] += -filter.a1 * mCoeffs4[i][0];
        mCoeffs4[i][2] += -filter.a1 * mCoeffs4[i][1] + -filter.a2 * mCoeffs4[i][0];
        mCoeffs4[i][3] += -filter.a1 * mCoeffs4[i][2] + -filter.a2 * mCoeffs4[i][1];
    }
}

float IIRFilterer::apply(float in)
{
    auto x = in + 1e-9f;
    auto y = mFilter.b0 * x + mFilter.b1 * mXm1 + mFilter.b2 * mXm2 - mFilter.a1 * mYm1 - mFilter.a2 * mYm2;
    mXm2 = mXm1;
    mXm1 = x;
    mYm2 = mYm1;
    mYm1 = y;

    return y;
}

void IIRFilterer::apply(int size, const float* in, float* out)
{
    (this->*mApplyDispatch)(size, in, out);
}

void IIRFilterer::apply_float4(int size, const float* in, float* out)
{
    auto coeffxp3 = float4::load(&mCoeffs4[0][0]);
    auto coeffxp2 = float4::load(&mCoeffs4[1][0]);
    auto coeffxp1 = float4::load(&mCoeffs4[2][0]);
    auto coeffx = float4::load(&mCoeffs4[3][0]);
    auto coeffxm1 = float4::load(&mCoeffs4[4][0]);
    auto coeffxm2 = float4::load(&mCoeffs4[5][0]);
    auto coeffym1 = float4::load(&mCoeffs4[6][0]);
    auto coeffym2 = float4::load(&mCoeffs4[7][0]);

    auto xm2 = float4::set1(mXm2);
    auto xm1 = float4::set1(mXm1);
    auto ym2 = float4::set1(mYm2);
    auto ym1 = float4::set1(mYm1);

    auto simdSize = size & ~3;

    if (simdSize > 0)
    {
        mXm1 = in[simdSize - 1];
        mXm2 = in[simdSize - 2];
    }

    auto epsilon = float4::set1(1e-9f);

    for (auto i = 0; i < simdSize; i += 4)
    {
        auto in4 = float4::loadu(&in[i]);
        in4 = float4::add(in4, epsilon);

        auto x = float4::replicate<0>(in4);
        auto xp1 = float4::replicate<1>(in4);
        auto xp2 = float4::replicate<2>(in4);
        auto xp3 = float4::replicate<3>(in4);

        auto y = float4::mul(coeffxp3, xp3);
        y = float4::add(y, float4::mul(coeffxp2, xp2));
        y = float4::add(y, float4::mul(coeffxp1, xp1));
        y = float4::add(y, float4::mul(coeffx, x));
        y = float4::add(y, float4::mul(coeffxm1, xm1));
        y = float4::add(y, float4::mul(coeffxm2, xm2));
        y = float4::add(y, float4::mul(coeffym1, ym1));
        y = float4::add(y, float4::mul(coeffym2, ym2));

        xm2 = xp2;
        xm1 = xp3;
        ym2 = float4::replicate<2>(y);
        ym1 = float4::replicate<3>(y);

        float4::storeu(&out[i], y);
    }

    if (simdSize > 0)
    {
        mYm1 = out[simdSize - 1];
        mYm2 = out[simdSize - 2];
    }

    for (auto i = simdSize; i < size; ++i)
    {
        out[i] = apply(in[i]);
    }
}

} // namespace ipl
