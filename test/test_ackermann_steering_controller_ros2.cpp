// Copyright (c) 2022, Stogl Robotics Consulting UG (haftungsbeschränkt) (template)
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "test_ackermann_steering_controller_ros2.hpp"

#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using ros2_ackermann_cont::CMD_MY_ITFS;
using ros2_ackermann_cont::control_mode_type;
using ros2_ackermann_cont::STATE_MY_ITFS;

class AckermannSteeringControllerRos2Test : public AckermannSteeringControllerRos2Fixture<TestableAckermannSteeringControllerRos2>
{
};

TEST_F(AckermannSteeringControllerRos2Test, all_parameters_set_configure_success)
{
  SetUpController();

  ASSERT_TRUE(controller_->params_.joints.empty());
  ASSERT_TRUE(controller_->params_.state_joints.empty());
  ASSERT_TRUE(controller_->params_.interface_name.empty());
  ASSERT_EQ(controller_->params_.reference_timeout, 0.0);

  ASSERT_EQ(controller_->on_configure(rclcpp_lifecycle::State()), NODE_SUCCESS);

  ASSERT_THAT(controller_->params_.joints, testing::ElementsAreArray(joint_names_));
  ASSERT_TRUE(controller_->params_.state_joints.empty());
  ASSERT_THAT(controller_->state_joints_, testing::ElementsAreArray(joint_names_));
  ASSERT_EQ(controller_->params_.interface_name, interface_name_);
  ASSERT_EQ(controller_->params_.reference_timeout, 0.1);
}

TEST_F(AckermannSteeringControllerRos2Test, check_exported_intefaces)
{
  SetUpController();

  ASSERT_EQ(controller_->on_configure(rclcpp_lifecycle::State()), NODE_SUCCESS);

  auto command_intefaces = controller_->command_interface_configuration();
  ASSERT_EQ(command_intefaces.names.size(), joint_command_values_.size());
  for (size_t i = 0; i < command_intefaces.names.size(); ++i) {
    EXPECT_EQ(command_intefaces.names[i], joint_names_[i] + "/" + interface_name_);
  }

  auto state_intefaces = controller_->state_interface_configuration();
  ASSERT_EQ(state_intefaces.names.size(), joint_state_values_.size());
  for (size_t i = 0; i < state_intefaces.names.size(); ++i) {
    EXPECT_EQ(state_intefaces.names[i], joint_names_[i] + "/" + interface_name_);
  }

  // check ref itfs
  auto reference_interfaces = controller_->export_reference_interfaces();
  ASSERT_EQ(reference_interfaces.size(), joint_names_.size());
  for (size_t i = 0; i < joint_names_.size(); ++i) {
    const std::string ref_itf_name = std::string(controller_->get_node()->get_name()) + "/" +
                                     joint_names_[i] + "/" + interface_name_;
    EXPECT_EQ(reference_interfaces[i].get_name(), ref_itf_name);
    EXPECT_EQ(reference_interfaces[i].get_prefix_name(), controller_->get_node()->get_name());
    EXPECT_EQ(
      reference_interfaces[i].get_interface_name(), joint_names_[i] + "/" + interface_name_);
  }
}

TEST_F(AckermannSteeringControllerRos2Test, activate_success)
{
  SetUpController();

  ASSERT_EQ(controller_->on_configure(rclcpp_lifecycle::State()), NODE_SUCCESS);
  ASSERT_EQ(controller_->on_activate(rclcpp_lifecycle::State()), NODE_SUCCESS);

  // check that the message is reset
  auto msg = controller_->input_ref_.readFromNonRT();
  EXPECT_EQ((*msg)->displacements.size(), joint_names_.size());
  for (const auto & cmd : (*msg)->displacements) {
    EXPECT_TRUE(std::isnan(cmd));
  }
  EXPECT_EQ((*msg)->velocities.size(), joint_names_.size());
  for (const auto & cmd : (*msg)->velocities) {
    EXPECT_TRUE(std::isnan(cmd));
  }

  ASSERT_TRUE(std::isnan((*msg)->duration));

  EXPECT_EQ(controller_->reference_interfaces_.size(), joint_names_.size());
  for (const auto & interface : controller_->reference_interfaces_) {
    EXPECT_TRUE(std::isnan(interface));
  }
}

TEST_F(AckermannSteeringControllerRos2Test, update_success)
{
  SetUpController();

  ASSERT_EQ(controller_->on_configure(rclcpp_lifecycle::State()), NODE_SUCCESS);
  ASSERT_EQ(controller_->on_activate(rclcpp_lifecycle::State()), NODE_SUCCESS);

  ASSERT_EQ(
    controller_->update_reference_from_subscribers(),
    controller_interface::return_type::OK);
  ASSERT_EQ(
    controller_->update_and_write_commands(
      controller_->get_node()->now(), rclcpp::Duration::from_seconds(0.01)),
    controller_interface::return_type::OK);
}

TEST_F(AckermannSteeringControllerRos2Test, deactivate_success)
{
  SetUpController();

  ASSERT_EQ(controller_->on_configure(rclcpp_lifecycle::State()), NODE_SUCCESS);
  ASSERT_EQ(controller_->on_activate(rclcpp_lifecycle::State()), NODE_SUCCESS);
  ASSERT_EQ(controller_->on_deactivate(rclcpp_lifecycle::State()), NODE_SUCCESS);
}

TEST_F(AckermannSteeringControllerRos2Test, reactivate_success)
{
  SetUpController();

  ASSERT_EQ(controller_->on_configure(rclcpp_lifecycle::State()), NODE_SUCCESS);
  ASSERT_EQ(controller_->on_activate(rclcpp_lifecycle::State()), NODE_SUCCESS);
  ASSERT_EQ(controller_->command_interfaces_[CMD_MY_ITFS].get_value(), 101.101);
  ASSERT_EQ(controller_->on_deactivate(rclcpp_lifecycle::State()), NODE_SUCCESS);
  ASSERT_TRUE(std::isnan(controller_->command_interfaces_[CMD_MY_ITFS].get_value()));
  ASSERT_EQ(controller_->on_activate(rclcpp_lifecycle::State()), NODE_SUCCESS);
  ASSERT_TRUE(std::isnan(controller_->command_interfaces_[CMD_MY_ITFS].get_value()));

  ASSERT_EQ(
    controller_->update_reference_from_subscribers(),
    controller_interface::return_type::OK);
  ASSERT_EQ(
    controller_->update_and_write_commands(
      controller_->get_node()->now(), rclcpp::Duration::from_seconds(0.01)),
    controller_interface::return_type::OK);
}

TEST_F(AckermannSteeringControllerRos2Test, test_setting_slow_mode_service)
{
  SetUpController();

  rclcpp::executors::MultiThreadedExecutor executor;
  executor.add_node(controller_->get_node()->get_node_base_interface());
  executor.add_node(service_caller_node_->get_node_base_interface());

  // initially set to false
  ASSERT_EQ(*(controller_->control_mode_.readFromRT()), control_mode_type::FAST);

  ASSERT_EQ(controller_->on_configure(rclcpp_lifecycle::State()), NODE_SUCCESS);
  ASSERT_EQ(controller_->on_activate(rclcpp_lifecycle::State()), NODE_SUCCESS);

  // should stay false
  ASSERT_EQ(*(controller_->control_mode_.readFromRT()), control_mode_type::FAST);

  // set to true
  ASSERT_NO_THROW(call_service(true, executor));
  ASSERT_EQ(*(controller_->control_mode_.readFromRT()), control_mode_type::SLOW);

  // set back to false
  ASSERT_NO_THROW(call_service(false, executor));
  ASSERT_EQ(*(controller_->control_mode_.readFromRT()), control_mode_type::FAST);
}

TEST_F(AckermannSteeringControllerRos2Test, test_update_logic_chainable_fast)
{// 1. age<ref_timeout 2.age>ref_timeout
  SetUpController();
  rclcpp::executors::MultiThreadedExecutor executor;
  executor.add_node(controller_->get_node()->get_node_base_interface());
  executor.add_node(service_caller_node_->get_node_base_interface());

  ASSERT_EQ(controller_->on_configure(rclcpp_lifecycle::State()), NODE_SUCCESS);
  controller_->set_chained_mode(false);
  ASSERT_EQ(controller_->on_activate(rclcpp_lifecycle::State()), NODE_SUCCESS);
  ASSERT_FALSE(controller_->is_in_chained_mode());

  // set command statically
  static constexpr double TEST_DISPLACEMENT = 23.24;
  joint_command_values_[STATE_MY_ITFS] = 111;
  std::shared_ptr<ControllerReferenceMsg> msg = std::make_shared<ControllerReferenceMsg>();
  msg->header.stamp = controller_->get_node()->now() - controller_->ref_timeout_ - rclcpp::Duration::from_seconds(0.1);
  msg->joint_names = joint_names_;
  msg->displacements.resize(joint_names_.size(), TEST_DISPLACEMENT);
  msg->velocities.resize(joint_names_.size(), std::numeric_limits<double>::quiet_NaN());
  msg->duration = std::numeric_limits<double>::quiet_NaN();
  controller_->input_ref_.writeFromNonRT(msg);
  const auto age_of_last_command = controller_->get_node()->now() - (*(controller_->input_ref_.readFromNonRT()))->header.stamp;
  EXPECT_EQ(*(controller_->control_mode_.readFromRT()), control_mode_type::FAST);

  ASSERT_FALSE(age_of_last_command <= controller_->ref_timeout_);
  ASSERT_EQ((*(controller_->input_ref_.readFromRT()))->displacements[0], TEST_DISPLACEMENT);
  ASSERT_EQ(
    controller_->update_reference_from_subscribers(),
    controller_interface::return_type::OK);
  ASSERT_EQ(
    controller_->update_and_write_commands(
      controller_->get_node()->now(), rclcpp::Duration::from_seconds(0.01)),
    controller_interface::return_type::OK);

  EXPECT_EQ(*(controller_->control_mode_.readFromRT()), control_mode_type::FAST);
  EXPECT_EQ(joint_command_values_[STATE_MY_ITFS], 111);
  ASSERT_TRUE(std::isnan(controller_->reference_interfaces_[0]));
  for (const auto & interface : controller_->reference_interfaces_) {
    EXPECT_TRUE(std::isnan(interface));
  }

  std::shared_ptr<ControllerReferenceMsg> msg_2 = std::make_shared<ControllerReferenceMsg>();
  msg_2->header.stamp = controller_->get_node()->now() - rclcpp::Duration::from_seconds(0.01);
  msg_2->joint_names = joint_names_;
  msg_2->displacements.resize(joint_names_.size(), TEST_DISPLACEMENT);
  msg_2->velocities.resize(joint_names_.size(), std::numeric_limits<double>::quiet_NaN());
  msg_2->duration = std::numeric_limits<double>::quiet_NaN();
  controller_->input_ref_.writeFromNonRT(msg_2);
  const auto age_of_last_command_2 = controller_->get_node()->now() - (*(controller_->input_ref_.readFromNonRT()))->header.stamp;
  EXPECT_EQ(*(controller_->control_mode_.readFromRT()), control_mode_type::FAST);

  ASSERT_TRUE(age_of_last_command_2 <= controller_->ref_timeout_);
  ASSERT_EQ((*(controller_->input_ref_.readFromRT()))->displacements[0], TEST_DISPLACEMENT);
  ASSERT_EQ(
    controller_->update_reference_from_subscribers(),
    controller_interface::return_type::OK);
  ASSERT_EQ(
    controller_->update_and_write_commands(
      controller_->get_node()->now(), rclcpp::Duration::from_seconds(0.01)),
    controller_interface::return_type::OK);

  EXPECT_EQ(*(controller_->control_mode_.readFromRT()), control_mode_type::FAST);
  EXPECT_EQ(joint_command_values_[STATE_MY_ITFS], TEST_DISPLACEMENT);//exact value
  EXPECT_NE(joint_command_values_[STATE_MY_ITFS], 111);
  ASSERT_EQ((*(controller_->input_ref_.readFromRT()))->displacements[0], TEST_DISPLACEMENT);
  for (const auto & interface : controller_->reference_interfaces_) {
    EXPECT_FALSE(std::isnan(interface));
  }
}

TEST_F(AckermannSteeringControllerRos2Test, test_update_logic_chainable_slow)
{// 1. age<ref_timeout 2.age>ref_timeout
  SetUpController();
  rclcpp::executors::MultiThreadedExecutor executor;
  executor.add_node(controller_->get_node()->get_node_base_interface());
  executor.add_node(service_caller_node_->get_node_base_interface());

  ASSERT_EQ(controller_->on_configure(rclcpp_lifecycle::State()), NODE_SUCCESS);
  controller_->set_chained_mode(false);
  ASSERT_EQ(controller_->on_activate(rclcpp_lifecycle::State()), NODE_SUCCESS);
  ASSERT_FALSE(controller_->is_in_chained_mode());

  // set command statically
  static constexpr double TEST_DISPLACEMENT = 23.24;
  joint_command_values_[STATE_MY_ITFS] = 111;
  std::shared_ptr<ControllerReferenceMsg> msg = std::make_shared<ControllerReferenceMsg>();
  msg->header.stamp = controller_->get_node()->now() - controller_->ref_timeout_ - rclcpp::Duration::from_seconds(0.1);
  msg->joint_names = joint_names_;
  msg->displacements.resize(joint_names_.size(), TEST_DISPLACEMENT);
  msg->velocities.resize(joint_names_.size(), std::numeric_limits<double>::quiet_NaN());
  msg->duration = std::numeric_limits<double>::quiet_NaN();
  controller_->input_ref_.writeFromNonRT(msg);
  const auto age_of_last_command = controller_->get_node()->now() - (*(controller_->input_ref_.readFromNonRT()))->header.stamp;
  controller_->control_mode_.writeFromNonRT(control_mode_type::SLOW);

  EXPECT_EQ(*(controller_->control_mode_.readFromRT()), control_mode_type::SLOW);
  //age_of_last_command > ref_timeout_
  ASSERT_FALSE(age_of_last_command <= controller_->ref_timeout_);
  ASSERT_EQ((*(controller_->input_ref_.readFromRT()))->displacements[0], TEST_DISPLACEMENT);
  ASSERT_EQ(
    controller_->update_reference_from_subscribers(),
    controller_interface::return_type::OK);
  ASSERT_EQ(
    controller_->update_and_write_commands(
      controller_->get_node()->now(), rclcpp::Duration::from_seconds(0.01)),
    controller_interface::return_type::OK);

  EXPECT_EQ(*(controller_->control_mode_.readFromRT()), control_mode_type::SLOW);
  EXPECT_EQ(joint_command_values_[STATE_MY_ITFS], 111);
  ASSERT_TRUE(std::isnan(controller_->reference_interfaces_[0]));
  for (const auto & interface : controller_->reference_interfaces_) {
    EXPECT_TRUE(std::isnan(interface));
  }

  std::shared_ptr<ControllerReferenceMsg> msg_2 = std::make_shared<ControllerReferenceMsg>();
  msg_2->header.stamp = controller_->get_node()->now() - rclcpp::Duration::from_seconds(0.01);
  msg_2->joint_names = joint_names_;
  msg_2->displacements.resize(joint_names_.size(), TEST_DISPLACEMENT);
  msg_2->velocities.resize(joint_names_.size(), std::numeric_limits<double>::quiet_NaN());
  msg_2->duration = std::numeric_limits<double>::quiet_NaN();
  controller_->input_ref_.writeFromNonRT(msg_2);
  const auto age_of_last_command_2 = controller_->get_node()->now() - (*(controller_->input_ref_.readFromNonRT()))->header.stamp;
  //age_of_last_command_2 < ref_timeout_
  ASSERT_TRUE(age_of_last_command_2 <= controller_->ref_timeout_);
  ASSERT_EQ((*(controller_->input_ref_.readFromRT()))->displacements[0], TEST_DISPLACEMENT);
  ASSERT_EQ(
    controller_->update_reference_from_subscribers(),
    controller_interface::return_type::OK);
  ASSERT_EQ(
    controller_->update_and_write_commands(
      controller_->get_node()->now(), rclcpp::Duration::from_seconds(0.01)),
    controller_interface::return_type::OK);

  EXPECT_EQ(*(controller_->control_mode_.readFromRT()), control_mode_type::SLOW);
  EXPECT_EQ(joint_command_values_[STATE_MY_ITFS], TEST_DISPLACEMENT/4);//exact value
  EXPECT_NE(joint_command_values_[STATE_MY_ITFS], 111);
  ASSERT_EQ((*(controller_->input_ref_.readFromRT()))->displacements[0], TEST_DISPLACEMENT/2);
  for (const auto & interface : controller_->reference_interfaces_) {
    EXPECT_FALSE(std::isnan(interface));
  }
}

TEST_F(AckermannSteeringControllerRos2Test, publish_status_success)
{
  SetUpController();

  ASSERT_EQ(controller_->on_configure(rclcpp_lifecycle::State()), NODE_SUCCESS);
  ASSERT_EQ(controller_->on_activate(rclcpp_lifecycle::State()), NODE_SUCCESS);

  ASSERT_EQ(
    controller_->update_reference_from_subscribers(),
    controller_interface::return_type::OK);
  ASSERT_EQ(
    controller_->update_and_write_commands(
      controller_->get_node()->now(), rclcpp::Duration::from_seconds(0.01)),
    controller_interface::return_type::OK);

  ControllerStateMsg msg;
  subscribe_and_get_messages(msg);

  ASSERT_EQ(msg.set_point, 101.101);
}

TEST_F(AckermannSteeringControllerRos2Test, receive_message_and_publish_updated_status)
{
  SetUpController();
  rclcpp::executors::MultiThreadedExecutor executor;
  executor.add_node(controller_->get_node()->get_node_base_interface());

  ASSERT_EQ(controller_->on_configure(rclcpp_lifecycle::State()), NODE_SUCCESS);
  ASSERT_EQ(controller_->on_activate(rclcpp_lifecycle::State()), NODE_SUCCESS);

  ASSERT_EQ(
    controller_->update_reference_from_subscribers(),
    controller_interface::return_type::OK);
  ASSERT_EQ(
    controller_->update_and_write_commands(
      controller_->get_node()->now(), rclcpp::Duration::from_seconds(0.01)),
    controller_interface::return_type::OK);
//here
  ControllerStateMsg msg;
  subscribe_and_get_messages(msg);

  ASSERT_EQ(msg.set_point, 101.101);

  publish_commands(controller_->get_node()->now());
  ASSERT_TRUE(controller_->wait_for_commands(executor));

  ASSERT_EQ(
    controller_->update_reference_from_subscribers(),
    controller_interface::return_type::OK);
  ASSERT_EQ(
    controller_->update_and_write_commands(
      controller_->get_node()->now(), rclcpp::Duration::from_seconds(0.01)),
    controller_interface::return_type::OK);
//here
  EXPECT_EQ(joint_command_values_[CMD_MY_ITFS], 0.45);

  subscribe_and_get_messages(msg);

  ASSERT_EQ(msg.set_point, 0.45);
}

TEST_F(AckermannSteeringControllerRos2Test, test_message_timeout)
{
  SetUpController();
  rclcpp::executors::MultiThreadedExecutor executor;
  executor.add_node(controller_->get_node()->get_node_base_interface());

  ASSERT_EQ(controller_->on_configure(rclcpp_lifecycle::State()), NODE_SUCCESS);
  ASSERT_EQ(controller_->on_activate(rclcpp_lifecycle::State()), NODE_SUCCESS);

// try to set command with time before timeout - command is not updated
  auto reference = controller_->input_ref_.readFromNonRT();
  auto old_timestamp = (*reference)->header.stamp;
  EXPECT_EQ((*(controller_->input_ref_.readFromNonRT()))->joint_names.size(), joint_names_.size());
  EXPECT_EQ((*(controller_->input_ref_.readFromNonRT()))->joint_names[0], joint_names_[0]);
  EXPECT_TRUE(std::isnan((*reference)->displacements[0]));
  EXPECT_TRUE(std::isnan((*reference)->velocities[0]));
  EXPECT_TRUE(std::isnan((*reference)->duration));
  publish_commands(controller_->get_node()->now() - controller_->ref_timeout_ - rclcpp::Duration::from_seconds(0.1));
  ASSERT_TRUE(controller_->wait_for_commands(executor));
  ASSERT_EQ(old_timestamp, (*(controller_->input_ref_.readFromNonRT()))->header.stamp);
  EXPECT_EQ((*(controller_->input_ref_.readFromNonRT()))->joint_names.size(), joint_names_.size());
  EXPECT_EQ((*(controller_->input_ref_.readFromNonRT()))->joint_names[0], joint_names_[0]);
  EXPECT_TRUE(std::isnan((*reference)->displacements[0]));
  EXPECT_TRUE(std::isnan((*reference)->velocities[0]));
  EXPECT_TRUE(std::isnan((*reference)->duration));
}



TEST_F(AckermannSteeringControllerRos2Test, test_message_wrong_num_joints)
{
  SetUpController();
  rclcpp::executors::MultiThreadedExecutor executor;
  executor.add_node(controller_->get_node()->get_node_base_interface());

  ASSERT_EQ(controller_->on_configure(rclcpp_lifecycle::State()), NODE_SUCCESS);
  ASSERT_EQ(controller_->on_activate(rclcpp_lifecycle::State()), NODE_SUCCESS);

  // try to set command with time before timeout - command is not updated
  auto reference = controller_->input_ref_.readFromNonRT();
  auto old_timestamp = (*reference)->header.stamp;
  EXPECT_EQ((*(controller_->input_ref_.readFromNonRT()))->joint_names.size(), joint_names_.size());
  EXPECT_EQ((*(controller_->input_ref_.readFromNonRT()))->joint_names[0], joint_names_[0]);
  EXPECT_TRUE(std::isnan((*reference)->displacements[0]));
  EXPECT_TRUE(std::isnan((*reference)->velocities[0]));
  EXPECT_TRUE(std::isnan((*reference)->duration));
  publish_commands(controller_->get_node()->now(), {"joint1","joint2"});
  ASSERT_TRUE(controller_->wait_for_commands(executor));
  EXPECT_EQ((*(controller_->input_ref_.readFromNonRT()))->joint_names.size(), joint_names_.size());
  EXPECT_EQ((*(controller_->input_ref_.readFromNonRT()))->joint_names[0], joint_names_[0]);
  EXPECT_TRUE(std::isnan((*(controller_->input_ref_.readFromNonRT()))->displacements[0]));
  EXPECT_TRUE(std::isnan((*(controller_->input_ref_.readFromNonRT()))->velocities[0]));
  EXPECT_TRUE(std::isnan((*(controller_->input_ref_.readFromNonRT()))->duration));
}

TEST_F(AckermannSteeringControllerRos2Test, test_message_accepted)
{
  SetUpController();
  rclcpp::executors::MultiThreadedExecutor executor;
  executor.add_node(controller_->get_node()->get_node_base_interface());

  ASSERT_EQ(controller_->on_configure(rclcpp_lifecycle::State()), NODE_SUCCESS);
  ASSERT_EQ(controller_->on_activate(rclcpp_lifecycle::State()), NODE_SUCCESS);

  // try to set command with time before timeout - command is not updated
  auto reference = controller_->input_ref_.readFromNonRT();
  EXPECT_EQ((*(controller_->input_ref_.readFromNonRT()))->joint_names.size(), joint_names_.size());
  EXPECT_EQ((*(controller_->input_ref_.readFromNonRT()))->joint_names[0], joint_names_[0]);
  EXPECT_TRUE(std::isnan((*reference)->displacements[0]));
  EXPECT_TRUE(std::isnan((*reference)->velocities[0]));
  EXPECT_TRUE(std::isnan((*reference)->duration));
  publish_commands(controller_->get_node()->now());
  ASSERT_TRUE(controller_->wait_for_commands(executor));
  EXPECT_EQ((*(controller_->input_ref_.readFromNonRT()))->joint_names.size(), joint_names_.size());
  EXPECT_NE((*(controller_->input_ref_.readFromNonRT()))->joint_names[0], joint_names_[0]);
  EXPECT_FALSE(std::isnan((*(controller_->input_ref_.readFromNonRT()))->displacements[0]));
  EXPECT_FALSE(std::isnan((*(controller_->input_ref_.readFromNonRT()))->velocities[0]));
  EXPECT_FALSE(std::isnan((*(controller_->input_ref_.readFromNonRT()))->duration));
  EXPECT_EQ((*(controller_->input_ref_.readFromNonRT()))->displacements[0], 0.45);
  EXPECT_EQ((*(controller_->input_ref_.readFromNonRT()))->velocities[0], 0.0);
  EXPECT_EQ((*(controller_->input_ref_.readFromNonRT()))->duration, 1.25);

}

TEST_F(AckermannSteeringControllerRos2Test, test_update_logic)
{
  SetUpController();
  rclcpp::executors::MultiThreadedExecutor executor;
  executor.add_node(controller_->get_node()->get_node_base_interface());
  executor.add_node(service_caller_node_->get_node_base_interface());

  ASSERT_EQ(controller_->on_configure(rclcpp_lifecycle::State()), NODE_SUCCESS);
  ASSERT_EQ(controller_->on_activate(rclcpp_lifecycle::State()), NODE_SUCCESS);

  auto reference = controller_->input_ref_.readFromNonRT();

  // set command statically
  static constexpr double TEST_DISPLACEMENT = 23.24;
  joint_command_values_[STATE_MY_ITFS] = 111;
  std::shared_ptr<ControllerReferenceMsg> msg = std::make_shared<ControllerReferenceMsg>();
  msg->header.stamp = controller_->get_node()->now() - controller_->ref_timeout_ - rclcpp::Duration::from_seconds(0.1);
  msg->joint_names = joint_names_;
  msg->displacements.resize(joint_names_.size(), TEST_DISPLACEMENT);
  msg->velocities.resize(joint_names_.size(), std::numeric_limits<double>::quiet_NaN());
  msg->duration = std::numeric_limits<double>::quiet_NaN();
  controller_->input_ref_.writeFromNonRT(msg);
  const auto age_of_last_command = controller_->get_node()->now() - (*(controller_->input_ref_.readFromNonRT()))->header.stamp;

  ASSERT_FALSE(age_of_last_command <= controller_->ref_timeout_);
  ASSERT_EQ((*(controller_->input_ref_.readFromRT()))->displacements[0], TEST_DISPLACEMENT);
  ASSERT_EQ(
    controller_->update_reference_from_subscribers(),
    controller_interface::return_type::OK);
  ASSERT_EQ(
    controller_->update_and_write_commands(
      controller_->get_node()->now(), rclcpp::Duration::from_seconds(0.01)),
    controller_interface::return_type::OK);

  EXPECT_EQ(joint_command_values_[STATE_MY_ITFS], 111);
  ASSERT_TRUE(std::isnan(controller_->reference_interfaces_[0]));
  
  std::shared_ptr<ControllerReferenceMsg> msg_2 = std::make_shared<ControllerReferenceMsg>();
  msg_2->header.stamp = controller_->get_node()->now();
  msg_2->joint_names = joint_names_;
  msg_2->displacements.resize(joint_names_.size(), TEST_DISPLACEMENT);
  msg_2->velocities.resize(joint_names_.size(), std::numeric_limits<double>::quiet_NaN());
  msg_2->duration = std::numeric_limits<double>::quiet_NaN();
  controller_->input_ref_.writeFromNonRT(msg_2);
  const auto age_of_last_command_2 = controller_->get_node()->now() - (*(controller_->input_ref_.readFromNonRT()))->header.stamp;
  
  ASSERT_TRUE(age_of_last_command_2 <= controller_->ref_timeout_);
  ASSERT_EQ((*(controller_->input_ref_.readFromRT()))->displacements[0], TEST_DISPLACEMENT);
  ASSERT_EQ(
    controller_->update_reference_from_subscribers(),
    controller_interface::return_type::OK);
  ASSERT_EQ(
    controller_->update_and_write_commands(
      controller_->get_node()->now(), rclcpp::Duration::from_seconds(0.01)),
    controller_interface::return_type::OK);

  EXPECT_EQ(joint_command_values_[STATE_MY_ITFS], TEST_DISPLACEMENT);//exact value
  EXPECT_NE(joint_command_values_[STATE_MY_ITFS], 111);
  ASSERT_EQ((*(controller_->input_ref_.readFromRT()))->displacements[0], TEST_DISPLACEMENT);
}


TEST_F(AckermannSteeringControllerRos2Test, test_ref_timeout_zero_for_update)
{
  SetUpController();
  rclcpp::executors::MultiThreadedExecutor executor;
  executor.add_node(controller_->get_node()->get_node_base_interface());
  executor.add_node(service_caller_node_->get_node_base_interface());

  ASSERT_EQ(controller_->on_configure(rclcpp_lifecycle::State()), NODE_SUCCESS);
  ASSERT_EQ(controller_->on_activate(rclcpp_lifecycle::State()), NODE_SUCCESS);

  auto reference = controller_->input_ref_.readFromNonRT();

  // set command statically
  static constexpr double TEST_DISPLACEMENT = 23.24;
  controller_->ref_timeout_= rclcpp::Duration::from_seconds(0.0);
  std::shared_ptr<ControllerReferenceMsg> msg = std::make_shared<ControllerReferenceMsg>();
  msg->header.stamp = controller_->get_node()->now() - rclcpp::Duration::from_seconds(0.0);
  msg->joint_names = joint_names_;
  msg->displacements.resize(joint_names_.size(), TEST_DISPLACEMENT);
  msg->velocities.resize(joint_names_.size(), std::numeric_limits<double>::quiet_NaN());
  msg->duration = std::numeric_limits<double>::quiet_NaN();
  controller_->input_ref_.writeFromNonRT(msg);
  const auto age_of_last_command = controller_->get_node()->now() - (*(controller_->input_ref_.readFromNonRT()))->header.stamp;
  
  ASSERT_EQ((*(controller_->input_ref_.readFromRT()))->displacements[0], TEST_DISPLACEMENT);
  ASSERT_EQ(
    controller_->update_reference_from_subscribers(),
    controller_interface::return_type::OK);
  ASSERT_EQ(
    controller_->update_and_write_commands(
      controller_->get_node()->now(), rclcpp::Duration::from_seconds(0.01)),
    controller_interface::return_type::OK);

  EXPECT_EQ(joint_command_values_[STATE_MY_ITFS], TEST_DISPLACEMENT);
  ASSERT_NE((*(controller_->input_ref_.readFromRT()))->displacements[0], TEST_DISPLACEMENT);
  ASSERT_TRUE(std::isnan((*(controller_->input_ref_.readFromRT()))->displacements[0]));
}

TEST_F(AckermannSteeringControllerRos2Test, test_ref_timeout_zero_for_reference_callback)
{
  SetUpController();
  rclcpp::executors::MultiThreadedExecutor executor;
  executor.add_node(controller_->get_node()->get_node_base_interface());
  executor.add_node(service_caller_node_->get_node_base_interface());

  ASSERT_EQ(controller_->on_configure(rclcpp_lifecycle::State()), NODE_SUCCESS);
  ASSERT_EQ(controller_->on_activate(rclcpp_lifecycle::State()), NODE_SUCCESS);

  EXPECT_EQ((*(controller_->input_ref_.readFromNonRT()))->joint_names.size(), joint_names_.size());
  EXPECT_EQ((*(controller_->input_ref_.readFromNonRT()))->joint_names[0], joint_names_[0]);
  EXPECT_TRUE(std::isnan((*(controller_->input_ref_.readFromNonRT()))->displacements[0]));
  EXPECT_TRUE(std::isnan((*(controller_->input_ref_.readFromNonRT()))->velocities[0]));
  EXPECT_TRUE(std::isnan((*(controller_->input_ref_.readFromNonRT()))->duration));
  controller_->ref_timeout_= rclcpp::Duration::from_seconds(0.0);  
  //reference_callback() is called implicitly when publish_commands() is called.
  publish_commands(controller_->get_node()->now());
  ASSERT_TRUE(controller_->wait_for_commands(executor));
  EXPECT_EQ((*(controller_->input_ref_.readFromNonRT()))->joint_names.size(), joint_names_.size());
  EXPECT_NE((*(controller_->input_ref_.readFromNonRT()))->joint_names[0], joint_names_[0]);
  EXPECT_FALSE(std::isnan((*(controller_->input_ref_.readFromNonRT()))->displacements[0]));
  EXPECT_FALSE(std::isnan((*(controller_->input_ref_.readFromNonRT()))->velocities[0]));
  EXPECT_FALSE(std::isnan((*(controller_->input_ref_.readFromNonRT()))->duration));
  EXPECT_EQ((*(controller_->input_ref_.readFromNonRT()))->displacements[0], 0.45);
  EXPECT_EQ((*(controller_->input_ref_.readFromNonRT()))->velocities[0], 0.0);
  EXPECT_EQ((*(controller_->input_ref_.readFromNonRT()))->duration, 1.25);
}


int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  rclcpp::init(argc, argv);
  int result = RUN_ALL_TESTS();
  rclcpp::shutdown();
  return result;
}