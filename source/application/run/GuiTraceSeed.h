#pragma once

#include <cstdint>
#include <limits>
#include <optional>

#include <QString>

// GUI-only input policy. Scientific execution receives the chosen seed via
// SimulationConfig; no changes to the ray tracer or random-number streams.
namespace GuiTraceSeed
{
inline bool parseFixedSeed(const QString& text, std::uint64_t* seed)
{
    if (!seed || text.isEmpty() || text.size() > 10)
        return false;
    for (const QChar digit : text) {
        if (digit < QLatin1Char('0') || digit > QLatin1Char('9'))
            return false;
    }
    bool ok = false;
    const qulonglong value = text.toULongLong(&ok, 10);
    // The headless --seed parser uses ulong, which is 32-bit on Windows.
    if (!ok || value > std::numeric_limits<std::uint32_t>::max())
        return false;
    *seed = static_cast<std::uint64_t>(value);
    return true;
}

// Preserve the historical automatic generator except when an explicit seed
// (including 0) was selected. Avoid evaluating the generator unnecessarily.
template <typename AutomaticSeed>
std::uint64_t resolve(const std::optional<std::uint64_t>& fixedSeed,
                      AutomaticSeed&& automaticSeed)
{
    return fixedSeed ? *fixedSeed : static_cast<std::uint64_t>(automaticSeed());
}
}
