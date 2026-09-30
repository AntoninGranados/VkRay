#include "doctest/doctest.h"

#include "core/animation/animation_clock.hpp"

TEST_CASE("AnimationClock starts paused at frame zero") {
    AnimationClock clock(100, 24.0);
    CHECK(clock.isPaused());
    CHECK(clock.getFrame() == 0);
}

TEST_CASE("AnimationClock reset(time) derives frame from fps") {
    AnimationClock clock(100, 24.0);
    clock.reset(2.0);
    CHECK(clock.getFrame() == 48);
}

TEST_CASE("AnimationClock reset(frame) derives time from fps") {
    AnimationClock clock(100, 24.0);
    clock.reset(48);
    CHECK(clock.getTime() == doctest::Approx(2.0));
}

TEST_CASE("AnimationClock step advances frame and time") {
    AnimationClock clock(100, 24.0);
    clock.step(1.0 / 24.0);
    CHECK(clock.getFrame() == 1);
}

TEST_CASE("AnimationClock step wraps to frame zero at endFrame") {
    AnimationClock clock(10, 24.0);
    clock.reset(9);
    clock.step(1.0 / 24.0);
    CHECK(clock.getFrame() == 0);
    CHECK(clock.getTime() == doctest::Approx(1.0 / 24.0));
}

TEST_CASE("AnimationClock stepFixed wraps to frame zero at endFrame") {
    AnimationClock clock(10, 24.0);
    clock.reset(9);
    clock.stepFixed();
    CHECK(clock.getFrame() == 0);
    CHECK(clock.getTime() == doctest::Approx(clock.getFixedDt()));
}

TEST_CASE("AnimationClock sample reports change only once per frame") {
    AnimationClock clock(100, 24.0);
    CHECK(clock.sample());
    CHECK_FALSE(clock.sample());
    clock.step(1.0 / 24.0);
    CHECK(clock.sample());
    CHECK_FALSE(clock.sample());
}

TEST_CASE("AnimationClock play/pause/toggle") {
    AnimationClock clock(100, 24.0);
    clock.play();
    CHECK_FALSE(clock.isPaused());
    clock.pause();
    CHECK(clock.isPaused());
    clock.toggle();
    CHECK_FALSE(clock.isPaused());
}
