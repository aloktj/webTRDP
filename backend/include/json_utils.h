#pragma once

#include <json/json.h>

#include "trdp_engine.hpp"

namespace trdp {

Json::Value pdRuntimeToJson(const PdRuntime &pd, const TrdpEngine &engine);

}  // namespace trdp

