// Meson forbids a target's precompiled header from living in the same
// directory as the sources it applies to (see
// https://mesonbuild.com/Precompiled-headers.html). The real stdafx.h stays
// in Minecraft.Client/ as the single source of truth; this wrapper in its
// own pch/ subdirectory just forwards to it.
#pragma once
#include "../stdafx.h"
