
#pragma once

// psi46test version (should match DTB SW version):

#define TITLE        "psi46test for DTB"
#define VERSION      "V3.05"
#define TIMESTAMP    "11.9.2014"

// === set profiling options ================================================
// if defined a profiling infos are collected during execution
// and a report is created after termination (only Windows)

// #define ENABLE_PROFILING

// add profiling infos for rpc calls (ENABLE_PROFILING must be defined)
// #define ENABLE_RPC_PROFILING


// === thread safe CTestboard class =========================================
// if defined -> CTestboard is thread safe (Boost thread library needed)

// #define ENABLE_MULTITHREADING
