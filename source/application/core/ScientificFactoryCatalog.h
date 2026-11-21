#pragma once

#include <memory>
#include <vector>

#include "kernel/node/TFactory.h"

// One authoritative list of built-in scientific factories for GUI and CLI.
// The caller takes ownership of every returned factory. Runtime-loaded
// plugins, GUI-only components and photon exporter widgets are not included.
namespace ScientificFactoryCatalog
{
std::vector<std::unique_ptr<TFactory>> createBuiltInFactories();
}
