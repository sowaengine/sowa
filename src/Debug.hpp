/**
 * @file Debug.hpp
 * @author Lexographics
 * @brief Debugging utilities
 * @version 0.1
 * @date 2021-12-12
 */
#pragma once
#include <stdlib.h>
#include <stdexcept>
#include <cassert>
#include <unistd.h>


#define LOG(format, ...) fprintf (stderr, format "\n", __VA_ARGS__);
#define LOG_ERROR(format, ...) fprintf (stderr, "ERROR: " format "\n" , __VA_ARGS__);