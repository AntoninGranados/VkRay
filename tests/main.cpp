#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest/doctest.h"

int main(int argc, char** argv) {
    doctest::Context context(argc, argv);
    context.setOption("reporters", "gtest");
    return context.run();
}
