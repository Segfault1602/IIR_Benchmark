#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>

#include "basic_filter.h"
#include "cmsis_filter.h"
#include "filter_coeffs.h"

#ifdef __APPLE__
#include "vdsp_filter.h"
#endif

#ifdef IPP_FILTER
#include "ipp_filter.h"
#endif

TEST_CASE_TEMPLATE_DEFINE("Filter", T, test_id)
{
    constexpr size_t size = 32;
    std::array<float, size> input = {0};
    input[0] = 1.f;
    std::array<float, size> output;

    T filter;
    filter.process(input, output);

    for (size_t i = 0; i < size; ++i)
    {
        CHECK(output[i] == doctest::Approx(kTestSOSExpectedOutput[i]).epsilon(0.0001));
    }
}

TYPE_TO_STRING(CMSISFilterDF2T);

TEST_CASE_TEMPLATE_INVOKE(test_id, CMSISFilterDF2T, CMSISFilterDF1, BasicFilter, CascadedIIRDF2T, CascadedIIRDF1
#ifdef __APPLE__
                          ,
                          vDSPFilter
#endif
#ifdef IPP_FILTER
                          ,
                          IppFilter
#endif
);