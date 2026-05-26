#pragma once

/* C++ compile times are "a bit of a prick"
 *  (similar to purple guy).
 * We use this header to precompile all
 *  "static" headers we have.
 * On my machine this saves about 2 seconds.
 */

#define RAYGUI_IMPLEMENTATION

#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <signal.h>
#include <array>
#include <vector>
#include <raylib.h>
#include "raygui.h"
#include "entt.hpp"
#include "XXX.h"
