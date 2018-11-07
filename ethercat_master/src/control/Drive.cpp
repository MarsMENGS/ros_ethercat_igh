/*
* Add Comment here
*/

#include "Drive.h"

using namespace mcx;

void Drive::create_(const char* name, parameter_server::Parameter* parameter_server, uint64_t dt_micro_s) {

}

bool Drive::initPhase1_() {

    driveFeedback_.analog_inputs.reserve(4);
    driveFeedback_.digital_inputs.reserve(4);

    //addParameter("", mcx::parameter_server::ParameterType::INPUT, &);
    addParameter("statusword", mcx::parameter_server::ParameterType::INPUT, &driveFeedback_.statusword);
    addParameter("driveEnabled", mcx::parameter_server::ParameterType::INPUT, &driveFeedback_.drive_enabled);
    addParameter("driveErrorCode", mcx::parameter_server::ParameterType::INPUT, &driveFeedback_.drive_error_code);
    addParameter("slaveTimestamp", mcx::parameter_server::ParameterType::INPUT, &driveFeedback_.slave_timestamp);
    addParameter("positionValue", mcx::parameter_server::ParameterType::INPUT, &driveFeedback_.position_value);
    addParameter("velocityValue", mcx::parameter_server::ParameterType::INPUT, &driveFeedback_.velocity_value);
    addParameter("torqueValue", mcx::parameter_server::ParameterType::INPUT, &driveFeedback_.torque_value);
    addParameter("secondaryPositionValue", mcx::parameter_server::ParameterType::INPUT, &driveFeedback_.secondary_position_value);
    addParameter("secondaryVelocityValue", mcx::parameter_server::ParameterType::INPUT, &driveFeedback_.secondary_velocity_value);
    addParameter("analogInput1", mcx::parameter_server::ParameterType::INPUT, &driveFeedback_.analog_inputs[0]);
    addParameter("analogInput2", mcx::parameter_server::ParameterType::INPUT, &driveFeedback_.analog_inputs[1]);
    addParameter("analogInput3", mcx::parameter_server::ParameterType::INPUT, &driveFeedback_.analog_inputs[2]);
    addParameter("analogInput4", mcx::parameter_server::ParameterType::INPUT, &driveFeedback_.analog_inputs[3]);
    addParameter("digitalInput1", mcx::parameter_server::ParameterType::INPUT, &driveFeedback_.digital_inputs[0]);
    addParameter("digitalInput2", mcx::parameter_server::ParameterType::INPUT, &driveFeedback_.digital_inputs[1]);
    addParameter("digitalInput3", mcx::parameter_server::ParameterType::INPUT, &driveFeedback_.digital_inputs[2]);
    addParameter("digitalInput4", mcx::parameter_server::ParameterType::INPUT, &driveFeedback_.digital_inputs[3]);

  return true;
}

bool Drive::initPhase2_() {
  return true;
}

bool Drive::startOp_() {
  return true;
}

bool Drive::stopOp_() {
  return true;
}

bool Drive::iterateOp_(const container::TaskTime& system_time, container::UserTime* user_time) {
  return true;
}