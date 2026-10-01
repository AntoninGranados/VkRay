#include "doctest/doctest.h"

#include <ostream>
#include <string>

namespace {

class GTestStyleReporter : public doctest::IReporter {
public:
    explicit GTestStyleReporter(const doctest::ContextOptions& in) : stream(*in.cout) {}

    void report_query(const doctest::QueryData& in) override {
        for (unsigned i = 0; i < in.num_data; ++i) stream << in.data[i]->m_name << "\n";
    }

    void test_run_start() override {}

    void test_run_end(const doctest::TestRunStats& stats) override {
        const int passed =
            static_cast<int>(stats.numTestCasesPassingFilters) - static_cast<int>(stats.numTestCasesFailed);
        stream << "[==========] " << stats.numTestCasesPassingFilters << " tests ran.\n";
        stream << "[  PASSED  ] " << passed << " tests.\n";
        if (stats.numTestCasesFailed > 0) stream << "[  FAILED  ] " << stats.numTestCasesFailed << " tests.\n";
    }

    void test_case_start(const doctest::TestCaseData& data) override {
        currentName = data.m_name;
        stream << "[ RUN      ] " << currentName << "\n";
    }

    void test_case_reenter(const doctest::TestCaseData&) override {}

    void test_case_end(const doctest::CurrentTestCaseStats& stats) override {
        const int ms = static_cast<int>(stats.seconds * 1000.0);
        stream << (stats.testCaseSuccess ? "[       OK ] " : "[  FAILED  ] ") << currentName << " (" << ms << " ms)\n";
    }

    void test_case_exception(const doctest::TestCaseException& exception) override {
        stream << "  Unhandled exception: " << exception.error_string << "\n";
    }

    void subcase_start(const doctest::SubcaseSignature&) override {}
    void subcase_end() override {}

    void log_assert(const doctest::AssertData& data) override {
        if (!data.m_failed) return;
        stream << doctest::skipPathFromFilename(data.m_file) << ":" << data.m_line << ": Failure\n"
               << "  Expected: " << data.m_expr << "\n"
               << "    Actual: " << data.m_decomp << "\n";
    }

    void log_message(const doctest::MessageData&) override {}
    void test_case_skipped(const doctest::TestCaseData&) override {}

private:
    std::ostream& stream;
    std::string currentName;
};

} // namespace

DOCTEST_REGISTER_REPORTER("gtest", 1, GTestStyleReporter);
