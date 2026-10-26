#pragma once

// Immediate-mode UI. Include this INSTEAD of ui/retained/*, one style per project.
//
// Frame:
//   Input::update();
//   UI::beginFrame(UI::readMouse(), dt);
//   ... UI::panel / label / button / toggle / slider / progressBar / image ...
//   UI::endFrame();
//   Input::lateUpdate();

#include "ui/ImState.h"
#include "ui/ImWidgets.h"
#include "ui/core/UICore.h"
