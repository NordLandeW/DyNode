#include <doctest/doctest.h>

#include <cstddef>
#include <limits>

#include "decoder.h"

TEST_CASE("VideoFrameSizeCalculationUsesWideCheckedArithmetic") {
    size_t frameSize = 0;

    CHECK(video_detail::try_calculate_frame_size(1920, 1080, frameSize));
    CHECK(frameSize == static_cast<size_t>(1920) * 1080 * 4);

    CHECK(video_detail::try_calculate_frame_size(0, 1080, frameSize));
    CHECK(frameSize == 0);
}

TEST_CASE("VideoFrameSizeCalculationRejectsOverflow") {
    size_t frameSize = 0;
    const UINT maxDimension = (std::numeric_limits<UINT>::max)();

    CHECK_FALSE(video_detail::try_calculate_frame_size(
        maxDimension, maxDimension, frameSize));
    CHECK_FALSE(video_detail::try_calculate_frame_size(
        maxDimension, maxDimension - 1, frameSize));
}