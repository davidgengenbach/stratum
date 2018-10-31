// Copyright 2018 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "stratum/hal/lib/common/admin_service.h"

#include <grpc++/grpc++.h>
#include <memory>

#include "gflags/gflags.h"
#include "net/util/ports.h"
#include "stratum/hal/lib/common/error_buffer.h"
#include "stratum/hal/lib/common/switch_mock.h"
#include "stratum/lib/security/auth_policy_checker_mock.h"
#include "stratum/lib/test_utils/matchers.h"
#include "stratum/lib/utils.h"
#include "stratum/public/lib/error.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/memory/memory.h"
#include "absl/strings/substitute.h"
#include "absl/synchronization/mutex.h"

namespace stratum {
namespace hal {

MATCHER_P(EqualsProto, proto, "") { return ProtoEqual(arg, proto); }

class AdminServiceTest : public ::testing::TestWithParam<OperationMode> {
 protected:
  void SetUp() override {
    mode_ = GetParam();
    switch_mock_ = absl::make_unique<SwitchMock>();
    auth_policy_checker_mock_ = absl::make_unique<AuthPolicyCheckerMock>();
    error_buffer_ = absl::make_unique<ErrorBuffer>();
    admin_service_ = absl::make_unique<AdminService>(
        mode_, switch_mock_.get(), auth_policy_checker_mock_.get(),
        error_buffer_.get());
    // Create a mock service to test the functionality.
    std::string url =
        "localhost:" + std::to_string(net_util::PickUnusedPortOrDie());
    ::grpc::ServerBuilder builder;
    builder.AddListeningPort(url, ::grpc::InsecureServerCredentials());
    builder.RegisterService(admin_service_.get());
    server_ = builder.BuildAndStart();
    ASSERT_NE(server_, nullptr);
    stub_ = ::gnoi::system::System::NewStub(
        ::grpc::CreateChannel(url, ::grpc::InsecureChannelCredentials()));
    ASSERT_NE(stub_, nullptr);
  }

  void TearDown() override { server_->Shutdown(); }

  OperationMode mode_;
  std::unique_ptr<AdminService> admin_service_;
  std::unique_ptr<SwitchMock> switch_mock_;
  std::unique_ptr<AuthPolicyCheckerMock> auth_policy_checker_mock_;
  std::unique_ptr<ErrorBuffer> error_buffer_;
  std::unique_ptr<::grpc::Server> server_;
  std::unique_ptr<::gnoi::system::System::Stub> stub_;
};

TEST_P(AdminServiceTest, ColdbootSetupSuccess) {
  ASSERT_OK(admin_service_->Setup(false));
  const auto& errors = error_buffer_->GetErrors();
  EXPECT_TRUE(errors.empty());

  // cleanup
  ASSERT_OK(admin_service_->Teardown());
}

TEST_P(AdminServiceTest, WarmbootSetupSuccess) {
  ASSERT_OK(admin_service_->Setup(true));
  const auto& errors = error_buffer_->GetErrors();
  EXPECT_TRUE(errors.empty());

  // cleanup
  ASSERT_OK(admin_service_->Teardown());
}

TEST_P(AdminServiceTest, TimeSuccess) {
  ::grpc::ClientContext context;
  ::gnoi::system::TimeRequest req;
  ::gnoi::system::TimeResponse resp;

  // Invoke the RPC and validate the results.
  ::grpc::Status status = stub_->Time(&context, req, &resp);
  EXPECT_TRUE(status.ok());

  // cleanup
  ASSERT_OK(admin_service_->Teardown());
}

TEST_P(AdminServiceTest, RebootSuccess) {
  ::grpc::ClientContext context;
  ::gnoi::system::RebootRequest req;
  ::gnoi::system::RebootResponse resp;

  // Invoke the RPC and validate the results.
  ::grpc::Status status = stub_->Reboot(&context, req, &resp);
  EXPECT_TRUE(status.ok());

  // cleanup
  ASSERT_OK(admin_service_->Teardown());
}

TEST_P(AdminServiceTest, RebootStatusSuccess) {
  ::grpc::ClientContext context;
  ::gnoi::system::RebootStatusRequest req;
  ::gnoi::system::RebootStatusResponse resp;

  // Invoke the RPC and validate the results.
  ::grpc::Status status = stub_->RebootStatus(&context, req, &resp);
  EXPECT_TRUE(status.ok());

  // cleanup
  ASSERT_OK(admin_service_->Teardown());
}

TEST_P(AdminServiceTest, CancelRebootSuccess) {
  ::grpc::ClientContext context;
  ::gnoi::system::CancelRebootRequest req;
  ::gnoi::system::CancelRebootResponse resp;

  // Invoke the RPC and validate the results.
  ::grpc::Status status = stub_->CancelReboot(&context, req, &resp);
  EXPECT_TRUE(status.ok());

  // cleanup
  ASSERT_OK(admin_service_->Teardown());
}

INSTANTIATE_TEST_CASE_P(AdminServiceTestWithMode, AdminServiceTest,
                        ::testing::Values(OPERATION_MODE_STANDALONE,
                                          OPERATION_MODE_COUPLED,
                                          OPERATION_MODE_SIM));

}  // namespace hal
}  // namespace stratum