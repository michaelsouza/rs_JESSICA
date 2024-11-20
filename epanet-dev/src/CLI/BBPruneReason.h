// src/CLI/PruneReason.h
#pragma once
#include <string>

enum class PruneReason
{
  ACTUATIONS,
  COST,
  PRESSURES,
  LEVELS,
  STABILITY,
  SPLIT
};

// Utility function to convert PruneReason to string for logging
inline std::string to_string(const PruneReason &reason)
{
  switch (reason)
  {
  case PruneReason::ACTUATIONS:
    return "actuations";
  case PruneReason::COST:
    return "cost";
  case PruneReason::PRESSURES:
    return "pressures";
  case PruneReason::LEVELS:
    return "levels";
  case PruneReason::STABILITY:
    return "stability";
  default:
    return "unknown";
  }
}
