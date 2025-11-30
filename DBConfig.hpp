#pragma once 
#ifndef DBCONFIG_HPP
#define DBCONFIG_HPP

#include <iostream>

// ============================================
// Database Configuration Settings
// ============================================

// Page Size Configuration
constexpr int DB_PAGE_SIZE = 4096;
// constexpr int DB_PAGE_SIZE = 64;

// ============================================
// Verbose Mode Configuration
// ============================================
// Set to 1 to enable all debug prints (for development/debugging)
// Set to 0 to disable all debug prints (for performance/benchmarking)
#define VERBOSE_MODE 0

// Conditional print macro
// Usage: VERBOSE_PRINT("Message: " << variable << " more text");
#if VERBOSE_MODE
    #define VERBOSE_PRINT(msg) std::cout << msg << std::endl
#else
    #define VERBOSE_PRINT(msg) ((void)0)  // No-op when disabled, zero runtime cost
#endif

#endif // DBCONFIG_HPP  