/*!*******************************************************************************************
 *  \file       ignition_bridge.cpp
 *  \brief      Implementation of an Ignition Gazebo bridge to ROS 
 *  \authors    Miguel Fernández Cortizas
 *              Pedro Arias Pérez
 *              David Pérez Saura
 *              Rafael Pérez Seguí
 *
 *  \copyright  Copyright (c) 2022 Universidad Politécnica de Madrid
 *              All Rights Reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ********************************************************************************/

#include "ignition_bridge.hpp"

namespace ignition_platform
{
    poseCallbackType IgnitionBridge::poseCallback_ = [](const geometry_msgs::msg::PoseStamped &msg){};
    odometryCallbackType IgnitionBridge::odometryCallback_ = [](const nav_msgs::msg::Odometry &msg){};

    std::unordered_map<std::string, cameraCallbackType> IgnitionBridge::callbacks_camera_ = {};
    std::unordered_map<std::string, cameraInfoCallbackType> IgnitionBridge::callbacks_camera_info_ = {};


    IgnitionBridge::IgnitionBridge(std::string name_space)
    {
        name_space_ = name_space;

        // Initialize the ignition node
        ign_node_ptr_ = std::make_shared<ignition::transport::Node>();

        // Initialize publishers
        command_twist_pub_ = ign_node_ptr_->Advertise<ignition::msgs::Twist>(
            name_space + ign_topic_command_twist_);

        // Initialize subscribers
        ign_node_ptr_->Subscribe(
                "model" + name_space + ign_topic_sensor_pose_,
                IgnitionBridge::ignitionPoseCallback);

        ign_node_ptr_->Subscribe(
                "model" + name_space + ign_topic_sensor_odometry_, 
                IgnitionBridge::ignitionOdometryCallback);

        return;
    };

    void IgnitionBridge::sendTwistMsg(const geometry_msgs::msg::Twist &ros_twist_msg)
    {
        ignition::msgs::Twist ign_twist_msg;
        ros_ign_bridge::convert_ros_to_ign(ros_twist_msg, ign_twist_msg);
        command_twist_pub_.Publish(ign_twist_msg);
        return;
    };

    void IgnitionBridge::setPoseCallback(poseCallbackType callback)
    {
        poseCallback_ = callback;
        return;
    };

    void IgnitionBridge::ignitionPoseCallback(const ignition::msgs::Pose &msg)
    {
        geometry_msgs::msg::PoseStamped pose_msg;
        ros_ign_bridge::convert_ign_to_ros(msg, pose_msg);
        poseCallback_(pose_msg);
        return;
    };

    void IgnitionBridge::setOdometryCallback(odometryCallbackType callback)
    {
        odometryCallback_ = callback;
        return;
    };

    void IgnitionBridge::ignitionOdometryCallback(const ignition::msgs::Odometry &msg)
    {
        nav_msgs::msg::Odometry odom_msg;
        // msg.pose().orientation().w();
        // std::cout << msg.pose().orientation().w() << std::endl;
        ros_ign_bridge::convert_ign_to_ros(msg, odom_msg);
        odometryCallback_(odom_msg);
        return;
    };

    // Cameras
    void IgnitionBridge::addSensor(
        std::string world_name,
        std::string sensor_name,
        std::string link_name,
        std::string sensor_type,
        cameraCallbackType cameraCallback,
        cameraInfoCallbackType cameraInfoCallback)
    {
        std::string camera_topic = "/world/" + world_name + "/model/" + name_space_ + "/model/" + sensor_name + "/link/" + link_name + "/sensor/" + sensor_type + "/image";
        callbacks_camera_.insert(std::make_pair(camera_topic, cameraCallback));

        ign_node_ptr_->Subscribe(
            camera_topic,
            IgnitionBridge::ignitionCameraCallback);


        std::string camera_info_topic = "/world/" + world_name + "/model/" + name_space_ + "/model/" + sensor_name + "/link/" + link_name + "/sensor/" + sensor_type + "/image";
        callbacks_camera_info_.insert(std::make_pair(camera_info_topic, cameraInfoCallback));
        ign_node_ptr_->Subscribe(
            camera_info_topic,
            IgnitionBridge::ignitionCameraInfoCallback);

        return;
    };

    void IgnitionBridge::ignitionCameraCallback(
        const ignition::msgs::Image &msg, 
        const ignition::transport::MessageInfo &msg_info)
    {
        sensor_msgs::msg::Image ros_image_msg;
        ros_ign_bridge::convert_ign_to_ros(msg, ros_image_msg);
        auto callback = callbacks_camera_.find(msg_info.Topic());
        if (callback != callbacks_camera_.end())
        {
            callback->second(ros_image_msg);
        }
        return;
    };

    void IgnitionBridge::ignitionCameraInfoCallback(
        const ignition::msgs::CameraInfo &msg, 
        const ignition::transport::MessageInfo &msg_info)
    {
        sensor_msgs::msg::CameraInfo ros_camera_info_msg;
        ros_ign_bridge::convert_ign_to_ros(msg, ros_camera_info_msg);
        auto callback = callbacks_camera_info_.find(msg_info.Topic());
        if (callback != callbacks_camera_info_.end())
        {
            callback->second(ros_camera_info_msg);
        }
        return;
    };

}