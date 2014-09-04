/* Copyright (c) 2013, Ford Motor Company
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

#include <gtest/gtest.h>
#ifdef __QNX__
#  include <qdb/qdb.h>
#else  // __QNX__
#  include <sqlite3.h>
#endif  // __QNX__
#include <vector>
#include "json/value.h"
#include "policy/sql_pt_representation.h"
#include "policy/policy_types.h"
#include "./types.h"
#include "./enums.h"

using policy::SQLPTRepresentation;
using policy::CheckPermissionResult;
using policy::EndpointUrls;

namespace test {
namespace components {
namespace policy {

#ifdef __QNX__
class DBMS {
  public:
    bool Open() {
      conn_ = qdb_connect(kDatabaseName_.c_str(), 0);
      return conn_ != NULL;
    }
    void Close() {
      qdb_disconnect(conn_);
    }
    bool Exec(const char* query) {
      return -1 != qdb_statement(conn_, query);
    }
  private:
    static qdb_hdl_t* conn_;
    static const std::string kDatabaseName_;
};
qdb_hdl_t* DBMS::conn_ = 0;
const std::string DBMS::kDatabaseName_ = "policy";
#else  // __QNX__
class DBMS {
  public:
    bool Open() {
      return SQLITE_OK == sqlite3_open(kFileName_.c_str(), &conn_);
    }
    void Close() {
      sqlite3_close(conn_);
      remove(kFileName_.c_str());
    }
    bool Exec(const char* query) {
      return SQLITE_OK == sqlite3_exec(conn_, query, NULL, NULL, NULL);
    }
  private:
    static sqlite3* conn_;
    static const std::string kFileName_;
};
sqlite3* DBMS::conn_ = 0;
const std::string DBMS::kFileName_ = "policy.sqlite";
#endif  // __QNX__

class SQLPTRepresentationTest : public ::testing::Test {
  protected:
    static DBMS* dbms;
    static SQLPTRepresentation* reps;

    static void SetUpTestCase() {
      reps = new SQLPTRepresentation;
      dbms = new DBMS;
      EXPECT_EQ(::policy::SUCCESS, reps->Init());
      EXPECT_TRUE(dbms->Open());
    }

    static void TearDownTestCase() {
      EXPECT_TRUE(reps->Drop());
      EXPECT_TRUE(reps->Close());
      delete reps;
      dbms->Close();
    }
};

DBMS* SQLPTRepresentationTest::dbms = 0;
SQLPTRepresentation* SQLPTRepresentationTest::reps = 0;

::testing::AssertionResult IsValid(const policy_table::Table& table) {
  if (table.is_valid()) {
    return ::testing::AssertionSuccess();
  } else {
    ::rpc::ValidationReport report(" - table");
    table.ReportErrors(&report);
    return ::testing::AssertionFailure() << ::rpc::PrettyFormat(report);
  }
}

TEST_F(SQLPTRepresentationTest, CheckPermissionsAllowed) {
  const char* query = "INSERT OR REPLACE INTO `application` (`id`, `memory_kb`,"
                      " `watchdog_timer_ms`) VALUES ('12345', 5, 10); "
                      "INSERT OR REPLACE INTO functional_group (`id`, `name`)"
                      "  VALUES (1, 'Base-4'); "
                      "INSERT OR REPLACE INTO `app_group` (`application_id`,"
                      " `functional_group_id`) VALUES ('12345', 1); "
                      "INSERT OR REPLACE INTO `rpc` (`name`, `parameter`, `hmi_level_value`,"
                      " `functional_group_id`) VALUES ('Update', 'gps', 'FULL', 1); "
                      "INSERT OR REPLACE INTO `rpc` (`name`, `parameter`, `hmi_level_value`,"
                      " `functional_group_id`) VALUES ('Update', 'speed', 'FULL', 1);";
  ASSERT_TRUE(dbms->Exec(query));

  CheckPermissionResult ret;
  ret = reps->CheckPermissions("12345", "FULL", "Update");
  EXPECT_TRUE(ret.hmi_level_permitted == ::policy::kRpcAllowed);
  ASSERT_EQ(2u, ret.list_of_allowed_params->size());
  EXPECT_EQ("gps", (*ret.list_of_allowed_params)[0]);
  EXPECT_EQ("speed", (*ret.list_of_allowed_params)[1]);
}

TEST_F(SQLPTRepresentationTest, CheckPermissionsAllowedWithoutParameters) {
  const char* query = "INSERT OR REPLACE INTO `application` (`id`, `memory_kb`,"
                      " `watchdog_timer_ms`) VALUES ('12345', 5, 10); "
                      "INSERT OR REPLACE INTO functional_group (`id`, `name`)"
                      "  VALUES (1, 'Base-4'); "
                      "INSERT OR REPLACE INTO `app_group` (`application_id`,"
                      " `functional_group_id`) VALUES ('12345', 1); "
                      "DELETE FROM `rpc`; "
                      "INSERT OR REPLACE INTO `rpc` (`name`, `hmi_level_value`,"
                      " `functional_group_id`) VALUES ('Update', 'LIMITED', 1);";
  ASSERT_TRUE(dbms->Exec(query));

  CheckPermissionResult ret;
  ret = reps->CheckPermissions("12345", "LIMITED", "Update");
  EXPECT_TRUE(ret.hmi_level_permitted == ::policy::kRpcAllowed);
  EXPECT_TRUE(!ret.list_of_allowed_params);
}

TEST_F(SQLPTRepresentationTest, CheckPermissionsDisallowed) {
  const char* query = "DELETE FROM `app_group`";
  ASSERT_TRUE(dbms->Exec(query));

  CheckPermissionResult ret;
  ret = reps->CheckPermissions("12345", "FULL", "Update");
  EXPECT_EQ(::policy::kRpcDisallowed, ret.hmi_level_permitted);
  EXPECT_TRUE(!ret.list_of_allowed_params);
}

TEST_F(SQLPTRepresentationTest, IsPTPReloaded) {
  const char* query = "UPDATE `module_config` SET `preloaded_pt` = 1";
  ASSERT_TRUE(dbms->Exec(query));
  EXPECT_TRUE(reps->IsPTPreloaded());
}

TEST_F(SQLPTRepresentationTest, GetUpdateUrls) {
  const char* query_delete = "DELETE FROM `endpoint`; ";
  ASSERT_TRUE(dbms->Exec(query_delete));
  EndpointUrls ret = reps->GetUpdateUrls(7);
  EXPECT_TRUE(ret.empty());

  const char* query_insert =
    "INSERT INTO `endpoint` (`application_id`, `url`, `service`) "
    "  VALUES ('12345', 'http://ford.com/cloud/1', '0x07');"
    "INSERT INTO `endpoint` (`application_id`, `url`, `service`) "
    "  VALUES ('12345', 'http://ford.com/cloud/2', '0x07');";

  ASSERT_TRUE(dbms->Exec(query_insert));
  ret = reps->GetUpdateUrls(7);
  ASSERT_EQ(2u, ret.size());
  EXPECT_EQ("http://ford.com/cloud/1", ret[0].url);
  EXPECT_EQ("http://ford.com/cloud/2", ret[1].url);

  ret = reps->GetUpdateUrls(0);
  EXPECT_TRUE(ret.empty());
}

TEST_F(SQLPTRepresentationTest, IgnitionCyclesBeforeExchangeAndIncrement) {
  const char* query_zeros = "UPDATE `module_meta` SET "
                            "  `ignition_cycles_since_last_exchange` = 0; "
                            "  UPDATE `module_config` SET `exchange_after_x_ignition_cycles` = 0";
  ASSERT_TRUE(dbms->Exec(query_zeros));
  EXPECT_EQ(0, reps->IgnitionCyclesBeforeExchange());
  reps->IncrementIgnitionCycles();
  EXPECT_EQ(0, reps->IgnitionCyclesBeforeExchange());

  const char* query_less_limit = "UPDATE `module_meta` SET "
                                 "  `ignition_cycles_since_last_exchange` = 5; "
                                 "  UPDATE `module_config` SET `exchange_after_x_ignition_cycles` = 10";
  ASSERT_TRUE(dbms->Exec(query_less_limit));
  EXPECT_EQ(5, reps->IgnitionCyclesBeforeExchange());
  reps->IncrementIgnitionCycles();
  EXPECT_EQ(4, reps->IgnitionCyclesBeforeExchange());

  const char* query_limit = "UPDATE `module_meta` SET "
                            "  `ignition_cycles_since_last_exchange` = 9; "
                            "  UPDATE `module_config` SET `exchange_after_x_ignition_cycles` = 10";
  ASSERT_TRUE(dbms->Exec(query_limit));
  EXPECT_EQ(1, reps->IgnitionCyclesBeforeExchange());
  reps->IncrementIgnitionCycles();
  EXPECT_EQ(0, reps->IgnitionCyclesBeforeExchange());

  const char* query_more_limit = "UPDATE `module_meta` SET "
                                 "  `ignition_cycles_since_last_exchange` = 12; "
                                 "  UPDATE `module_config` SET `exchange_after_x_ignition_cycles` = 10";
  ASSERT_TRUE(dbms->Exec(query_more_limit));
  EXPECT_EQ(0, reps->IgnitionCyclesBeforeExchange());

  const char* query_negative_limit = "UPDATE `module_meta` SET "
                                     "  `ignition_cycles_since_last_exchange` = 3; "
                                     "  UPDATE `module_config` SET `exchange_after_x_ignition_cycles` = -1";
  ASSERT_TRUE(dbms->Exec(query_negative_limit));
  EXPECT_EQ(0, reps->IgnitionCyclesBeforeExchange());

  const char* query_negative_current = "UPDATE `module_meta` SET "
                                       "  `ignition_cycles_since_last_exchange` = -1; "
                                       "  UPDATE `module_config` SET `exchange_after_x_ignition_cycles` = 2";
  ASSERT_TRUE(dbms->Exec(query_negative_current));
  EXPECT_EQ(0, reps->IgnitionCyclesBeforeExchange());
}

TEST_F(SQLPTRepresentationTest, KilometersBeforeExchange) {
  const char* query_zeros = "UPDATE `module_meta` SET "
                            "  `pt_exchanged_at_odometer_x` = 0; "
                            "  UPDATE `module_config` SET `exchange_after_x_kilometers` = 0";
  ASSERT_TRUE(dbms->Exec(query_zeros));
  EXPECT_EQ(0, reps->KilometersBeforeExchange(0));
  EXPECT_EQ(0, reps->KilometersBeforeExchange(-10));
  EXPECT_EQ(0, reps->KilometersBeforeExchange(10));

  const char* query_negative_limit = "UPDATE `module_meta` SET "
                                     "  `pt_exchanged_at_odometer_x` = 10; "
                                     "  UPDATE `module_config` SET `exchange_after_x_kilometers` = -10";
  ASSERT_TRUE(dbms->Exec(query_negative_limit));
  EXPECT_EQ(0, reps->KilometersBeforeExchange(0));
  EXPECT_EQ(0, reps->KilometersBeforeExchange(10));

  const char* query_negative_last = "UPDATE `module_meta` SET "
                                    "  `pt_exchanged_at_odometer_x` = -10; "
                                    "  UPDATE `module_config` SET `exchange_after_x_kilometers` = 20";
  ASSERT_TRUE(dbms->Exec(query_negative_last));
  EXPECT_EQ(0, reps->KilometersBeforeExchange(0));
  EXPECT_EQ(0, reps->KilometersBeforeExchange(10));

  const char* query_limit = "UPDATE `module_meta` SET "
                            "  `pt_exchanged_at_odometer_x` = 10; "
                            "  UPDATE `module_config` SET `exchange_after_x_kilometers` = 100";
  ASSERT_TRUE(dbms->Exec(query_limit));
  EXPECT_EQ(0, reps->KilometersBeforeExchange(120));
  EXPECT_EQ(60, reps->KilometersBeforeExchange(50));
  EXPECT_EQ(0, reps->KilometersBeforeExchange(5));
}

TEST_F(SQLPTRepresentationTest, DaysBeforeExchange) {
  const char* query_zeros = "UPDATE `module_meta` SET "
                            "  `pt_exchanged_x_days_after_epoch` = 0; "
                            "  UPDATE `module_config` SET `exchange_after_x_days` = 0";
  ASSERT_TRUE(dbms->Exec(query_zeros));
  EXPECT_EQ(0, reps->DaysBeforeExchange(0));
  EXPECT_EQ(0, reps->DaysBeforeExchange(-10));
  EXPECT_EQ(0, reps->DaysBeforeExchange(10));

  const char* query_negative_limit = "UPDATE `module_meta` SET "
                                     "  `pt_exchanged_x_days_after_epoch` = 10; "
                                     "  UPDATE `module_config` SET `exchange_after_x_days` = -10";
  ASSERT_TRUE(dbms->Exec(query_negative_limit));
  EXPECT_EQ(0, reps->DaysBeforeExchange(0));
  EXPECT_EQ(0, reps->DaysBeforeExchange(10));

  const char* query_negative_last = "UPDATE `module_meta` SET "
                                    "  `pt_exchanged_x_days_after_epoch` = -10; "
                                    "  UPDATE `module_config` SET `exchange_after_x_days` = 20";
  ASSERT_TRUE(dbms->Exec(query_negative_last));
  EXPECT_EQ(0, reps->DaysBeforeExchange(0));
  EXPECT_EQ(0, reps->DaysBeforeExchange(10));

  const char* query_limit = "UPDATE `module_meta` SET "
                            "  `pt_exchanged_x_days_after_epoch` = 10; "
                            "  UPDATE `module_config` SET `exchange_after_x_days` = 100";
  ASSERT_TRUE(dbms->Exec(query_limit));
  EXPECT_EQ(0, reps->DaysBeforeExchange(120));
  EXPECT_EQ(60, reps->DaysBeforeExchange(50));
  EXPECT_EQ(0, reps->DaysBeforeExchange(5));
}

TEST_F(SQLPTRepresentationTest, SecondsBetweenRetries) {
  std::vector<int> seconds;

  const char* query_delete = "DELETE FROM `seconds_between_retry`; ";
  ASSERT_TRUE(dbms->Exec(query_delete));
  ASSERT_TRUE(reps->SecondsBetweenRetries(&seconds));
  EXPECT_EQ(0u, seconds.size());

  const char* query_insert =
    "INSERT INTO `seconds_between_retry` (`index`, `value`) "
    "  VALUES (0, 10); "
    "INSERT INTO `seconds_between_retry` (`index`, `value`) "
    "  VALUES (1, 20); ";
  ASSERT_TRUE(dbms->Exec(query_insert));
  ASSERT_TRUE(reps->SecondsBetweenRetries(&seconds));
  ASSERT_EQ(2u, seconds.size());
  EXPECT_EQ(10, seconds[0]);
  EXPECT_EQ(20, seconds[1]);
}

TEST_F(SQLPTRepresentationTest, TimeoutResponse) {
  const char* query =
    "UPDATE `module_config` SET `timeout_after_x_seconds` = 60";
  ASSERT_TRUE(dbms->Exec(query));
  EXPECT_EQ(60, reps->TimeoutResponse());
}

#ifndef EXTENDED_POLICY
TEST_F(SQLPTRepresentationTest, SaveGenerateSnapshot) {
  Json::Value expect(Json::objectValue);
  expect["policy_table"] = Json::Value(Json::objectValue);

  Json::Value& policy_table = expect["policy_table"];
  policy_table["module_meta"] = Json::Value(Json::objectValue);
  policy_table["module_config"] = Json::Value(Json::objectValue);
  policy_table["usage_and_error_counts"] = Json::Value(Json::objectValue);
  policy_table["device_data"] = Json::Value(Json::objectValue);
  policy_table["functional_groupings"] = Json::Value(Json::objectValue);
  policy_table["consumer_friendly_messages"] = Json::Value(Json::objectValue);
  policy_table["app_policies"] = Json::Value(Json::objectValue);

  Json::Value& module_config = policy_table["module_config"];
  module_config["preloaded_pt"] = Json::Value(true);
  module_config["exchange_after_x_ignition_cycles"] = Json::Value(10);
  module_config["exchange_after_x_kilometers"] = Json::Value(100);
  module_config["exchange_after_x_days"] = Json::Value(5);
  module_config["timeout_after_x_seconds"] = Json::Value(500);
  module_config["seconds_between_retries"] = Json::Value(Json::arrayValue);
  module_config["seconds_between_retries"][0] = Json::Value(10);
  module_config["seconds_between_retries"][1] = Json::Value(20);
  module_config["seconds_between_retries"][2] = Json::Value(30);
  module_config["endpoints"] = Json::Value(Json::objectValue);
  module_config["endpoints"]["0x00"] = Json::Value(Json::objectValue);
  module_config["endpoints"]["0x00"]["default"] = Json::Value(Json::arrayValue);
  module_config["endpoints"]["0x00"]["default"][0] = Json::Value(
        "http://ford.com/cloud/default");
  module_config["notifications_per_minute_by_priority"] = Json::Value(
        Json::objectValue);
  module_config["notifications_per_minute_by_priority"]["emergency"] =
    Json::Value(1);
  module_config["notifications_per_minute_by_priority"]["navigation"] =
    Json::Value(2);
  module_config["notifications_per_minute_by_priority"]["VOICECOMM"] =
    Json::Value(3);
  module_config["notifications_per_minute_by_priority"]["communication"] =
    Json::Value(4);
  module_config["notifications_per_minute_by_priority"]["normal"] = Json::Value(
        5);
  module_config["notifications_per_minute_by_priority"]["none"] = Json::Value(
        6);
  module_config["vehicle_make"] = Json::Value("MakeT");
  module_config["vehicle_model"] = Json::Value("ModelT");
  module_config["vehicle_year"] = Json::Value(2014);

  Json::Value& usage_and_error_counts = policy_table["usage_and_error_counts"];
  usage_and_error_counts["app_level"] = Json::Value(Json::objectValue);
  usage_and_error_counts["app_level"]["12345"] = Json::Value(Json::objectValue);

  Json::Value& device_data = policy_table["device_data"];
  device_data["user_consent_records"] = Json::Value(Json::objectValue);

  Json::Value& functional_groupings = policy_table["functional_groupings"];
  functional_groupings["default"] = Json::Value(Json::objectValue);
  Json::Value& default_group = functional_groupings["default"];
  default_group["rpcs"] = Json::Value(Json::objectValue);
  default_group["rpcs"]["Update"] = Json::Value(Json::objectValue);
  default_group["rpcs"]["Update"]["hmi_levels"] = Json::Value(Json::arrayValue);
  default_group["rpcs"]["Update"]["hmi_levels"][0] = Json::Value("FULL");
  default_group["rpcs"]["Update"]["parameters"] = Json::Value(Json::arrayValue);
  default_group["rpcs"]["Update"]["parameters"][0] = Json::Value("speed");

  Json::Value& consumer_friendly_messages =
    policy_table["consumer_friendly_messages"];
  consumer_friendly_messages["version"] = Json::Value("1.2");

  Json::Value& app12345counters = usage_and_error_counts["app_level"]["12345"];
  app12345counters["app_registration_language_gui"] = "";
  app12345counters["app_registration_language_vui"] = "";
  app12345counters["count_of_rejected_rpc_calls"] = 0;
  app12345counters["count_of_rejections_duplicate_name"] = 0;
  app12345counters["count_of_rejections_nickname_mismatch"] = 0;
  app12345counters["count_of_rejections_sync_out_of_memory"] = 0;
  app12345counters["count_of_removals_for_bad_behavior"] = 0;
  app12345counters["count_of_rpcs_sent_in_hmi_none"] = 0;
  app12345counters["count_of_run_attempts_while_revoked"] = 0;
  app12345counters["count_of_user_selections"] = 0;
  app12345counters["minutes_in_hmi_background"] = 0;
  app12345counters["minutes_in_hmi_full"] = 0;
  app12345counters["minutes_in_hmi_limited"] = 0;
  app12345counters["minutes_in_hmi_none"] = 0;

  Json::Value& app_policies = policy_table["app_policies"];
  app_policies["default"] = Json::Value(Json::objectValue);
  app_policies["default"]["priority"] = Json::Value("EMERGENCY");
  app_policies["default"]["memory_kb"] = Json::Value(50);
  app_policies["default"]["watchdog_timer_ms"] = Json::Value(100);
  app_policies["default"]["groups"] = Json::Value(Json::arrayValue);
  app_policies["default"]["groups"][0] = Json::Value("default");

  policy_table::Table table(&expect);

  ASSERT_TRUE(IsValid(table));
  ASSERT_TRUE(reps->Save(table));
  utils::SharedPtr<policy_table::Table> snapshot = reps->GenerateSnapshot();
  EXPECT_TRUE(IsValid(*snapshot));
  EXPECT_EQ(table.ToJsonValue().toStyledString(),
            snapshot->ToJsonValue().toStyledString());
}
#endif  // EXTENDED_POLICY

}  // namespace policy
}  // namespace components
}  // namespace test

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
