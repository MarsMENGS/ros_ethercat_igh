
/*********************************************************************
*
* Software License Agreement (BSD License)
*
*  Copyright (c) 2018, Vectioneer B.V.
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*   * Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*   * Redistributions in binary form must reproduce the above
*     copyright notice, this list of conditions and the following
*     disclaimer in the documentation and/or other materials provided
*     with the distribution.
*   * Neither the name of the Vectioneer nor the names of its
*     contributors may be used to endorse or promote products derived
*     from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
*  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
*  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
*  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
*  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
*  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
*  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
*  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
*
* Author: Alexey Zakharov
* Author: Viatcheslav Tretyakov
*********************************************************************/

#include "MainControlLoop.h"
#include <cstdio>

using namespace mcx;

MainControlLoop::MainControlLoop(unsigned int number_of_drives, unsigned int number_of_ios) : number_of_drives_{
        number_of_drives}, number_of_ios_{number_of_ios} {

    opmode_ = new int[number_of_drives]{};
    controlword_ = new int[number_of_drives]{};
    statusword_ = new int[number_of_drives]{};
    drives_ = new Drive[number_of_drives];
    if (number_of_ios_ > 0) {
        dio_devices_ = new DigitalIO[number_of_ios];
    }

}

MainControlLoop::~MainControlLoop() {

    delete[] opmode_;
    delete[] controlword_;
    delete[] statusword_;
    delete[] drives_;
    delete[] dio_devices_;
}

void MainControlLoop::create_(const char *name, parameter_server::Parameter *parameter_server, uint64_t dt_micro_s) {

    sub_drives_ = nh_.subscribe<motorcortex_msgs::DriveOutList>("/drive_control", 1,
                                                                &MainControlLoop::drivesControlCallback, this);
    if (number_of_ios_ > 0) {
        sub_dios_ = nh_.subscribe<motorcortex_msgs::DigitalOutputsList>("/digital_outputs", 1,
                                                                        &MainControlLoop::diosControlCallback, this);
    }

    drive_feedback_pub_ = nh_.advertise<motorcortex_msgs::DriveInList>("/drive_feedback", 1);
    if (number_of_ios_ > 0) {
        digital_inputs_pub_ = nh_.advertise<motorcortex_msgs::DigitalInputsList>("/digital_inputs", 1);
    }

    service_get_sdo_ = nh_.advertiseService("get_sdo_config", &MainControlLoop::getSDOSrv, this);
    service_set_sdo_ = nh_.advertiseService("set_sdo_config", &MainControlLoop::setSDOSrv, this);
    service_save_cfg_ = nh_.advertiseService("save_sdo_config", &MainControlLoop::saveCfgParamsSrv, this);
    service_restore_cfg_ = nh_.advertiseService("restore_default_sdo_config",  &MainControlLoop::restoreCfgParamsSrv, this);

    std::string axisName = "axis";
    for (unsigned int i = 0; i < number_of_drives_; i++) {
        std::string indexedName = axisName + std::to_string(i + 1);
        createSubmodule(&drives_[i], indexedName.c_str());
    }

    std::string deviceName = "device";
    for (unsigned int i = 0; i < number_of_ios_; i++) {
        std::string indexedName = deviceName + std::to_string(i + 1);
        createSubmodule(&dio_devices_[i], indexedName.c_str());
    }

}

bool MainControlLoop::initPhase1_() {
    // user input
    addParameter("controlword", mcx::parameter_server::ParameterType::OUTPUT, controlword_, number_of_drives_);
    addParameter("opmode", mcx::parameter_server::ParameterType::OUTPUT, opmode_, number_of_drives_);
    addParameter("statusword", mcx::parameter_server::ParameterType::INPUT, statusword_, number_of_drives_);

    addParameter("read_sdo", mcx::parameter_server::ParameterType::OUTPUT, &read_sdo_);
    addParameter("read_sdo_update_sec", mcx::parameter_server::ParameterType::PARAMETER, &read_sdo_time_max_sec_);

    return true;
}

bool MainControlLoop::initPhase2_() {
    return true;
}

bool MainControlLoop::startOp_() {
    return true;
}

bool MainControlLoop::stopOp_() {
    return true;
}

bool MainControlLoop::iterateOp_(const container::TaskTime &system_time, container::UserTime *user_time) {

    if (read_sdo_time_sec_ >= read_sdo_time_max_sec_) {
        read_sdo_time_sec_ = 0;
        read_sdo_ = true;
    } else {
        read_sdo_ = false;
        read_sdo_time_sec_ += getDtSec();
    }

    ros::spinOnce();

    motorcortex_msgs::DriveInList driveFeedbackList;

    for (unsigned int i = 0; i < number_of_drives_; i++) {
        drives_[i].iterate(system_time, user_time);
        controlword_[i] = drives_[i].getControlWord();
        opmode_[i] = drives_[i].getOpMode();
        drives_[i].setStatusWord(statusword_[i]);
        driveFeedbackList.drives_feedback.push_back(drives_[i].getDriveFeedback());
    }

    drive_feedback_pub_.publish(driveFeedbackList);

    if (number_of_ios_ > 0) {
        motorcortex_msgs::DigitalInputsList digitalInputsList;

        for (unsigned int i = 0; i < number_of_ios_; i++) {
            dio_devices_[i].iterate(system_time, user_time);
            digitalInputsList.devices_feedback.push_back(dio_devices_[i].getDIOFeedback());
        }

        digital_inputs_pub_.publish(digitalInputsList);
    }

    return true;
}

void MainControlLoop::drivesControlCallback(const motorcortex_msgs::DriveOutList::ConstPtr &drives_command_msg) {
    unsigned int max_counter = std::min(number_of_drives_, drives_command_msg->drive_command.size());
    for (unsigned int i = 0; i < max_counter; i++) {
        drives_[i].setDriveCommand(drives_command_msg->drive_command[i]);
    }
}

void MainControlLoop::diosControlCallback(const motorcortex_msgs::DigitalOutputsList::ConstPtr &dios_command_msg) {
    unsigned int max_counter = std::min(number_of_ios_, dios_command_msg->devices_command.size());
    for (unsigned int i = 0; i < max_counter; i++) {
        dio_devices_[i].setDigitalOutputs(dios_command_msg->devices_command[i]);
    }
}

bool MainControlLoop::getSDOSrv(motorcortex_msgs::GetSDOCfg::Request &req,
                                motorcortex_msgs::GetSDOCfg::Response &res) {
    unsigned int max_counter = std::min(number_of_drives_, req.read_cfg.size());
    for (unsigned int i = 0; i < max_counter; i++) {
        //ToDO: request an update
        drives_[i].requestSDOUpdate(req.read_cfg[i]);

        //ToDo: receive the updated data
        DriveSdo::SDOCfg SDOCfg = drives_[i].getSDOCfg();
        res.torque_controller_cfg.push_back(SDOCfg.torqueControllerCfg);
        res.velocity_controller_cfg.push_back(SDOCfg.velocityControllerCfg);
        res.position_controller_cfg.push_back(SDOCfg.positionControllerCfg);
    }

    return true;
}

bool MainControlLoop::setSDOSrv(motorcortex_msgs::SetSDOCfg::Request &req,
                                motorcortex_msgs::SetSDOCfg::Response &res) {
    for (unsigned int i = 0; i < number_of_drives_; i++) {
        DriveSdo::SDOCfg sdoCfg;
        if (i < req.torque_controller_cfg.size()) {
            sdoCfg.torqueControllerCfg = req.torque_controller_cfg[i];
        }
        if (i < req.velocity_controller_cfg.size()) {
            sdoCfg.velocityControllerCfg = req.velocity_controller_cfg[i];
        }
        if (i < req.velocity_controller_cfg.size()) {
            sdoCfg.positionControllerCfg = req.position_controller_cfg[i];
        }
        //ToDo: set the configuration
        drives_[i].setSDOCfg(sdoCfg);

        //ToDo: notify of success or failure
        res.success.push_back(true);
    }

    return true;
}

bool MainControlLoop::saveCfgParamsSrv(motorcortex_msgs::SaveCfgParams::Request &req,
                                       motorcortex_msgs::SaveCfgParams::Response &res) {

    unsigned int max_counter = std::min(number_of_drives_, req.save.size());
    LOG_INFO("save called %i", max_counter);
    for (unsigned int i = 0; i < max_counter; i++) {
        LOG_INFO("save: %s", req.save[i] ? "true" : "false");
        if (req.save[i]) {
            drives_[i].saveAllCfg(0x65766173);//"save"
        }

        //ToDo: notify of success or failure
        res.success.push_back(true);
    }
    return true;
}

bool MainControlLoop::restoreCfgParamsSrv(motorcortex_msgs::RestoreCfgParams::Request &req,
                                          motorcortex_msgs::RestoreCfgParams::Response &res) {
    unsigned int max_counter = std::min(number_of_drives_, req.reset_to_default.size());
    for (unsigned int i = 0; i < max_counter; i++) {
        LOG_INFO("reset to default: %s", req.reset_to_default[i] ? "true" : "false");
        if (req.reset_to_default[i]) {
            drives_[i].restoreAllCfg(0x64616f6c);//"load"
        }

        //ToDo: notify of success or failure
        res.success.push_back(true);
    }

    return true;
}