#pragma once
#include <stdlib.h>
#include <stdexcept>
#include <cassert>
#include <unistd.h>


#define LOG(format, ...) fprintf (stderr, format, __VA_ARGS__);