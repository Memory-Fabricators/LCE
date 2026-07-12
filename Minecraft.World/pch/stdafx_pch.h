// Meson forbids a target's precompiled header from living in the same
// directory as the sources it applies to (see
// https://mesonbuild.com/Precompiled-headers.html). The real stdafx.h stays
// in Minecraft.World/ as the single source of truth (it's also included
// verbatim, by relative path, from other subtrees such as
// Minecraft.Client/SDL3); this wrapper in its own pch/ subdirectory just
// forwards to it.
#pragma once
#include "../stdafx.h"
