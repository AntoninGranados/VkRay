#include "doctest/doctest.h"

#include <fstream>
#include <sstream>
#include <string>

#include "utils/resources.hpp"

TEST_CASE("Resources serves the embedded copy of a builtin file") {
    std::ifstream file("src/config/parameters.json", std::ios::binary);
    std::stringstream buffer;
    buffer << file.rdbuf();

    const auto resource = Resources::read("builtin:/config/parameters.json");
    REQUIRE(resource.has_value());
    CHECK(*resource == buffer.str());
}

TEST_CASE("Resources normalizes builtin paths") {
    CHECK(Resources::find("builtin:/shaders/core/../core/pathtracing.glsl").has_value());
}

TEST_CASE("Resources serves added builtin files") {
    Resources::add("builtin:/shaders/generated/test.glsl", "content");
    CHECK(Resources::read("builtin:/shaders/generated/test.glsl") == "content");
}

TEST_CASE("Resources reads non-builtin paths from disk only") {
    CHECK(Resources::read("src/config/parameters.json").has_value());
    CHECK_FALSE(Resources::read("config/parameters.json").has_value());
}
