#include "doctest/doctest.h"

#include "core/render/sample_accumulator.hpp"

TEST_CASE("SampleAccumulator starts at zero") {
    SampleAccumulator acc;
    CHECK(acc.getSampleCount() == 0);
}

TEST_CASE("SampleAccumulator increment accumulates") {
    SampleAccumulator acc;
    CHECK(acc.increment() == 1);
    CHECK(acc.increment() == 2);
    CHECK(acc.increment() == 3);
    CHECK(acc.getSampleCount() == 3);
}

TEST_CASE("SampleAccumulator restart resets to zero") {
    SampleAccumulator acc;
    acc.increment();
    acc.increment();
    acc.restart();
    CHECK(acc.getSampleCount() == 0);
}

TEST_CASE("SampleAccumulator unbounded target never finishes") {
    SampleAccumulator acc;
    for (int i = 0; i < 100; i++) acc.increment();
    CHECK_FALSE(acc.isRenderFinished());
}

TEST_CASE("SampleAccumulator finishes once target reached") {
    SampleAccumulator acc;
    acc.setTargetSampleCount(3);
    CHECK_FALSE(acc.isRenderFinished());
    acc.increment();
    acc.increment();
    CHECK_FALSE(acc.isRenderFinished());
    acc.increment();
    CHECK(acc.isRenderFinished());
}

TEST_CASE("SampleAccumulator zero target finishes immediately") {
    SampleAccumulator acc;
    acc.setTargetSampleCount(0);
    CHECK(acc.isRenderFinished());
}
