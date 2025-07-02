#pragma once

#include <chrono>
#include <iostream>
#include <string>

// Test result tracking
struct TestResult {
    std::string TestName;
    bool Passed;
    std::string ErrorMessage;

    TestResult(const std::string& name, bool passed, const std::string& error = "")
        : TestName(name), Passed(passed), ErrorMessage(error) {}
};

// Simple test runner
class TestRunner {
  private:
    int m_TotalTests = 0;
    int m_PassedTests = 0;

  public:
    void RunTest(const std::string& testName, bool condition, const std::string& errorMsg = "") {
        m_TotalTests++;
        if (condition) {
            m_PassedTests++;
            std::cout << "[PASS] " << testName << std::endl;
        } else {
            std::cout << "[FAIL] " << testName;
            if (!errorMsg.empty()) { std::cout << " - " << errorMsg; }
            std::cout << std::endl;
        }
    }

    void PrintSummary() {
        std::cout << "\n=== Test Summary ===" << std::endl;
        std::cout << "Total Tests: " << m_TotalTests << std::endl;
        std::cout << "Passed: " << m_PassedTests << std::endl;
        std::cout << "Failed: " << (m_TotalTests - m_PassedTests) << std::endl;
        std::cout << "Success Rate: "
                  << (m_TotalTests > 0 ? (m_PassedTests * 100 / m_TotalTests) : 0) << "%"
                  << std::endl;
    }
};

// Utility functions
namespace TestUtils {
// Simple timer for performance testing
class Timer {
  private:
    std::chrono::high_resolution_clock::time_point m_Start;

  public:
    Timer() : m_Start(std::chrono::high_resolution_clock::now()) {}

    long long ElapsedMs() const {
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(end - m_Start).count();
    }
};
} // namespace TestUtils