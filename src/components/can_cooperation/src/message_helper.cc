/*
 Copyright (c) 2013, Ford Motor Company
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.

 Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following
 disclaimer in the documentation and/or other materials provided with the
 distribution.

 Neither the name of the Ford Motor Company nor the names of its contributors
 may be used to endorse or promote products derived from this software
 without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
 */

#include <string>
#include "can_cooperation/message_helper.h"

namespace can_cooperation {
using functional_modules::MobileFunctionID;
namespace {
std::map<MobileFunctionID, std::string> GenerateAPINames() {
  std::map<MobileFunctionID, std::string> result;
  result.insert(std::make_pair<MobileFunctionID, std::string>(MobileFunctionID::GRANT_ACCESS, "GrantAccess"));
  result.insert(std::make_pair<MobileFunctionID, std::string>(MobileFunctionID::CANCEL_ACCESS, "CancelAccess"));
  result.insert(std::make_pair<MobileFunctionID, std::string>(MobileFunctionID::START_SCAN, "StartScan"));
  result.insert(std::make_pair<MobileFunctionID, std::string>(MobileFunctionID::STOP_SCAN, "StopScan"));
  result.insert(std::make_pair<MobileFunctionID, std::string>(MobileFunctionID::TUNE_RADIO, "TuneRadio"));
  result.insert(std::make_pair<MobileFunctionID, std::string>(MobileFunctionID::TUNE_UP, "TuneUp"));
  result.insert(std::make_pair<MobileFunctionID, std::string>(MobileFunctionID::TUNE_DOWN, "TuneDown"));
  result.insert(std::make_pair<MobileFunctionID, std::string>(MobileFunctionID::ON_CONTROL_CHANGED, "OnControlChanged"));
  result.insert(std::make_pair<MobileFunctionID, std::string>(MobileFunctionID::ON_RADIO_DETAILS, "OnRadioDetails"));
  result.insert(std::make_pair<MobileFunctionID, std::string>(MobileFunctionID::ON_PRESETS_CHANGED, "OnPresetsChanged"));
  result.insert(std::make_pair<MobileFunctionID, std::string>(MobileFunctionID::CLIMATE_CONTROL_ON, "ClimateControlOn"));
  result.insert(std::make_pair<MobileFunctionID, std::string>(MobileFunctionID::GET_SEAT_CONTROL, "GetSeatControl"));
  return result;
}
}

uint32_t MessageHelper::next_correlation_id_ = 1;
const std::map<MobileFunctionID, std::string>
MessageHelper::kMobileAPINames = GenerateAPINames();

uint32_t MessageHelper::GetNextCANCorrelationID() {
  return next_correlation_id_++;
}

const std::string MessageHelper::GetMobileAPIName(MobileFunctionID func_id) {
  std::map<MobileFunctionID, std::string>::const_iterator it =
    kMobileAPINames.find(func_id);
  if (kMobileAPINames.end() != it) {
    return it->second;
  } else {
    return "";
  }
}

std::string MessageHelper::ValueToString(const Json::Value& value) {
  Json::FastWriter writer;

  return writer.write(value);
}

Json::Value MessageHelper::StringToValue(const std::string& string) {
  Json::Reader reader;

  Json::Value json;

  if (reader.parse(string, json)) {
    return json;
  }

  return Json::Value(Json::ValueType::nullValue);
}

}  // namespace can_cooperation
