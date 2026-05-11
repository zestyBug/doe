#if !defined(MINI_TEST_HPP)
#define MINI_TEST_HPP

#ifdef DEBUG
#include <stdio.h>
#endif
#include <iosfwd>
#include <functional>
#include <vector>
#include <string>
#include <sstream>

namespace mtest {

    struct Test {
        const char *name;
        std::function<void()> func;
    };

    inline std::vector<Test>& get_tests() {
        static std::vector<Test> tests;
        return tests;
    }

    struct Register {
        Register(const char *name, std::function<void()> func) {
            get_tests().push_back({name, func});
        }
    };

    inline void run_all() {
        int passed = 0, failed = 0;
        for (auto& test : get_tests()) {
            try {
                test.func();
            #ifdef DEBUG
                printf("✔ %s\n", test.name);
            #endif
                ++passed;
            } catch (const std::exception& ex) {
            #ifdef DEBUG
                printf("✘ %s - %s\n", test.name, ex.what());
            #endif
                ++failed;
            }
        }
    #ifdef DEBUG
        printf("\n=== SUMMARY ===\nPassed: %d\nFailed: %d\n", passed, failed);
    #endif
    }

    // Assertion helpers

    template<typename T, typename U>
    void expect_ge(const T& a, const U& b, const std::string& file, int line) {
        if (!(a >= b)) {
            std::ostringstream oss;
            oss << file << ":" << line << " EXPECT_GE: " << a << " < " << b;
            throw std::runtime_error(oss.str());
        }
    }

    template<typename T, typename U>
    void expect_eq(const T& a, const U& b, const std::string& file, int line) {
        if (!(a == b)) {
            std::ostringstream oss;
            oss << file << ":" << line << " EXPECT_EQ: " << a << " != " << b;
            throw std::runtime_error(oss.str());
        }
    }

    template<typename T, typename U>
    void expect_ne(const T& a, const U& b, const std::string& file, int line) {
        if (!(a != b)) {
            std::ostringstream oss;
            oss << file << ":" << line << " EXPECT_NE: " << a << " == " << b;
            throw std::runtime_error(oss.str());
        }
    }

} // namespace mtest

// Macro to register a test function from a class. The function is public, static and void(*)(void).
#define CLASS_TEST(class,name) \
    static mtest::Register reg_##class##name(#class "::" #name, &class::name); \
    void class::name()

// Macro to define test
#define TEST(name) void name(); \
    static mtest::Register reg_##name(#name, name); \
    void name()

// Assertion macros

#define EXPECT_GE(a, b) mtest::expect_ge((a), (b), __FILE__, __LINE__)
#define EXPECT_EQ(a, b) mtest::expect_eq((a), (b), __FILE__, __LINE__)
#define EXPECT_NE(a, b) mtest::expect_ne((a), (b), __FILE__, __LINE__)


#endif // MINI_TEST_HPP
