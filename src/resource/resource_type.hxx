#ifndef SW_RESOURCE_TYPE_HXX
#define SW_RESOURCE_TYPE_HXX
#pragma once

#include <stdlib.h>

typedef int RID;
typedef size_t ResourceType;

const size_t ResourceType_None = 0;
const size_t ResourceType_ImageTexture = 1;
const size_t ResourceType_AudioStream = 2;
const size_t ResourceType_SpriteSheetAnimation = 3;

#endif // SW_RESOURCE_TYPE_HXX