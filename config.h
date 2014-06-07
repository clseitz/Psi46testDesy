// config.h

#pragma once

// === psi46test version ====================================================

#define TITLE        "psi46test for DTB"
#define VERSION      "V2.2"
#define TIMESTAMP    "4.6.2014"


// === set profiling options ================================================
// if defined a profiling infos are collected during execution
// and a report is created after termination (only Windows)

// #define ENABLE_PROFILING

// add profiling infos for rpc calls (ENABLE_PROFILING must be defined)
// #define ENABLE_RPC_PROFILING


// === thread safe CTestboard class =========================================
// if defined -> CTestboard is thread safe (Boost thread library needed)

// #define ENABLE_MULTITHREADING
