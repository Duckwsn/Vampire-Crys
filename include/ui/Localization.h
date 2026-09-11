#pragma once

#include "core/GameSettings.h"

namespace Localization {
void SetLanguage(GameLanguage language);
GameLanguage Language();
bool IsPortuguese();
const char* Text(const char* english, const char* portuguese);
const char* Known(const char* english);
}
