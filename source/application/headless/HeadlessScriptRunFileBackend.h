#pragma once

// Compatibility type name only: GUI and CLI now execute the SAME
// widget-free ScriptRunExecutionService implementation.
#include "script/ScriptRunExecutionService.h"

using HeadlessScriptRunFileBackend = ScriptRunExecutionService;
