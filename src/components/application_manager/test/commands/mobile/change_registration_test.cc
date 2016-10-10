/*
 * Copyright (c) 2016, Ford Motor Company
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided with the
 * distribution.
 *
 * Neither the name of the Ford Motor Company nor the names of its contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdint.h>
#include <string>
#include <set>

#include "application_manager/commands/mobile/change_registration_request.h"

#include "gtest/gtest.h"
#include "utils/shared_ptr.h"
#include "utils/helpers.h"
#include "utils/make_shared.h"
#include "utils/custom_string.h"
#include "smart_objects/smart_object.h"
#include "application_manager/commands/command_request_test.h"
#include "application_manager/smart_object_keys.h"
#include "policy/usage_statistics/mock_statistics_manager.h"
#include "application_manager/mock_application.h"
#include "application_manager/mock_application_manager.h"
#include "application_manager/mock_message_helper.h"
#include "application_manager/mock_hmi_capabilities.h"
#include "application_manager/event_engine/event.h"
#include "application_manager/mock_hmi_interface.h"

namespace test {
namespace components {
namespace commands_test {

namespace am = application_manager;
using am::commands::CommandImpl;
using am::ApplicationManager;
using am::commands::MessageSharedPtr;
using am::ApplicationSharedPtr;
using am::MockMessageHelper;
using am::MockHmiInterfaces;
using ::testing::_;
using ::utils::SharedPtr;
using ::testing::Return;
using ::testing::ReturnRef;
using am::commands::ChangeRegistrationRequest;
using ::test::components::application_manager_test::MockApplication;

namespace custom_str = utils::custom_string;
namespace strings = ::application_manager::strings;
namespace hmi_response = ::application_manager::hmi_response;

namespace {
const int32_t kCommandId = 1;
const uint32_t kAppId = 1u;
const uint32_t kCmdId = 1u;
const uint32_t kConnectionKey = 2u;
}  // namespace

class ChangeRegistrationRequestTest
    : public CommandRequestTest<CommandsTestMocks::kIsNice> {
 public:
  sync_primitives::Lock app_set_lock_;
  sync_primitives::Lock lock_;

  MessageSharedPtr CreateMsgFromMobile() {
    MessageSharedPtr msg = CreateMessage(smart_objects::SmartType_Map);
    (*msg)[strings::params][strings::connection_key] = kConnectionKey;
    smart_objects::SmartObject msg_params =
        smart_objects::SmartObject(smart_objects::SmartType_Map);
    msg_params[strings::hmi_display_language] = mobile_apis::Language::EN_US;
    msg_params[strings::language] = mobile_apis::Language::EN_US;
    (*msg)[strings::msg_params] = msg_params;
    return msg;
  }
  void PrepareExpectationBeforeRun() {
    ON_CALL(app_mngr_, hmi_capabilities())
        .WillByDefault(ReturnRef(hmi_capabilities_));
    smart_objects::SmartObject supported_languages(
        smart_objects::SmartType_Array);
    supported_languages[0] = static_cast<int32_t>(mobile_apis::Language::EN_US);
    EXPECT_CALL(hmi_capabilities_, ui_supported_languages())
        .WillOnce(Return(&supported_languages));
    EXPECT_CALL(hmi_capabilities_, vr_supported_languages())
        .WillOnce(Return(&supported_languages));
    EXPECT_CALL(hmi_capabilities_, tts_supported_languages())
        .WillOnce(Return(&supported_languages));

    EXPECT_CALL(app_mngr_, hmi_interfaces())
        .WillRepeatedly(ReturnRef(hmi_interfaces_));
    EXPECT_CALL(
        hmi_interfaces_,
        GetInterfaceFromFunction(hmi_apis::FunctionID::UI_ChangeRegistration))
        .WillOnce(Return(am::HmiInterfaces::HMI_INTERFACE_UI));
    EXPECT_CALL(hmi_interfaces_,
                GetInterfaceState(am::HmiInterfaces::HMI_INTERFACE_UI))
        .WillOnce(Return(am::HmiInterfaces::STATE_AVAILABLE));

    EXPECT_CALL(
        hmi_interfaces_,
        GetInterfaceFromFunction(hmi_apis::FunctionID::VR_ChangeRegistration))
        .WillOnce(Return(am::HmiInterfaces::HMI_INTERFACE_VR));
    EXPECT_CALL(hmi_interfaces_,
                GetInterfaceState(am::HmiInterfaces::HMI_INTERFACE_VR))
        .WillOnce(Return(am::HmiInterfaces::STATE_AVAILABLE));

    EXPECT_CALL(
        hmi_interfaces_,
        GetInterfaceFromFunction(hmi_apis::FunctionID::TTS_ChangeRegistration))
        .WillOnce(Return(am::HmiInterfaces::HMI_INTERFACE_TTS));
    EXPECT_CALL(hmi_interfaces_,
                GetInterfaceState(am::HmiInterfaces::HMI_INTERFACE_TTS))
        .WillOnce(Return(am::HmiInterfaces::STATE_AVAILABLE));
  }

  void CreateResponseFromHMI(MessageSharedPtr msg,
                             hmi_apis::Common_Result::eType result,
                             const std::string& info) {
    (*msg)[strings::params][hmi_response::code] = static_cast<int32_t>(result);
    (*msg)[strings::msg_params][strings::info] = info;
  }

  typedef TypeIf<kMocksAreNice,
                 NiceMock<application_manager_test::MockHMICapabilities>,
                 application_manager_test::MockHMICapabilities>::Result
      MockHMICapabilities;
  MockHMICapabilities hmi_capabilities_;
  MockHmiInterfaces hmi_interfaces_;
};

typedef ChangeRegistrationRequestTest::MockHMICapabilities MockHMICapabilities;

TEST_F(ChangeRegistrationRequestTest, OnEvent_VR_UNSUPPORTED_RESOURCE) {
  MessageSharedPtr msg_from_mobile = CreateMsgFromMobile();

  utils::SharedPtr<ChangeRegistrationRequest> command =
      CreateCommand<ChangeRegistrationRequest>(msg_from_mobile);
  MockAppPtr mock_app = CreateMockApp();
  ON_CALL(app_mngr_, application(_)).WillByDefault(Return(mock_app));
  ON_CALL(*mock_app, app_id()).WillByDefault(Return(1));
  am::ApplicationSet application_set;
  const utils::custom_string::CustomString name("name");
  MockAppPtr app = CreateMockApp();
  app->set_name(name);

  DataAccessor<am::ApplicationSet> accessor(application_set, app_set_lock_);

  application_set.insert(app);

  EXPECT_CALL(app_mngr_, applications()).WillOnce(Return(accessor));
  EXPECT_CALL(*app, name()).WillOnce(ReturnRef(name));
  PrepareExpectationBeforeRun();
  command->Run();

  MessageSharedPtr ui_response = CreateMessage(smart_objects::SmartType_Map);
  MessageSharedPtr vr_response = CreateMessage(smart_objects::SmartType_Map);
  MessageSharedPtr tts_response = CreateMessage(smart_objects::SmartType_Map);
  CreateResponseFromHMI(
      ui_response, hmi_apis::Common_Result::WARNINGS, "ui_info");
  CreateResponseFromHMI(vr_response,
                        hmi_apis::Common_Result::UNSUPPORTED_RESOURCE,
                        "unsupported_resource");
  CreateResponseFromHMI(
      tts_response, hmi_apis::Common_Result::SUCCESS, "tts_info");

  am::event_engine::Event event_ui(hmi_apis::FunctionID::UI_ChangeRegistration);
  event_ui.set_smart_object(*ui_response);
  am::event_engine::Event event_vr(hmi_apis::FunctionID::VR_ChangeRegistration);
  event_vr.set_smart_object(*vr_response);
  am::event_engine::Event event_tts(
      hmi_apis::FunctionID::TTS_ChangeRegistration);
  event_tts.set_smart_object(*tts_response);

  MessageSharedPtr response_to_mobile;

  EXPECT_CALL(
      app_mngr_,
      ManageMobileCommand(_, am::commands::Command::CommandOrigin::ORIGIN_SDL))
      .WillOnce(DoAll(SaveArg<0>(&response_to_mobile), Return(true)));

  command->on_event(event_ui);
  command->on_event(event_vr);
  command->on_event(event_tts);

  EXPECT_EQ(
      (*response_to_mobile)[strings::msg_params][strings::success].asBool(),
      true);
  EXPECT_EQ(
      (*response_to_mobile)[strings::msg_params][strings::result_code].asInt(),
      static_cast<int32_t>(mobile_apis::Result::UNSUPPORTED_RESOURCE));
}

}  // namespace commands_test
}  // namespace components
}  // namespace tests