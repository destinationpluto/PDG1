#include "llcRunnable.h"

namespace runnable {
namespace llc {

void CLLCRunnable::init() {}

void CLLCRunnable::run() {
  if (!m_ticInputPort->hasAlloc()) {
    Log::error(G_TASK_TAG, "TIC Input Port is nullptr");
    return;
  }
  collectInput();
  doWork();
  updateStorage();
  sendOutput();
}

void CLLCRunnable::attachInputPorts(ticPort_p f_ticInputPort,
                                    dcmPort_p f_dcmInputPort) {
  Log::info(G_TASK_TAG, "Attaching input ports");
  m_ticInputPort = f_ticInputPort;
  m_dcmInputPort = f_dcmInputPort;
}

void CLLCRunnable::attachOutputPorts(llcPort_p f_llcOutputPort) {
  Log::info(G_TASK_TAG, "Attaching output ports");
  m_llcOutputPort = f_llcOutputPort;
}

void CLLCRunnable::collectInput() {
  m_inputData.m_ticInput = *(m_ticInputPort->getData());
  m_inputData.m_dcmInput = *(m_dcmInputPort->getData());
  CLlcOutput defaultOut;
  m_output = defaultOut;
}

void CLLCRunnable::doWork() {
  // handle tic function state
  if (m_inputData.m_ticInput.m_funcState ==
      runnable::tic::ETICState::INACTIVE) {
    // set output safe state
    setSafeStateOutput();
    return;
  }

  if (m_inputData.m_ticInput.m_funcState ==
      runnable::tic::ETICState::DEGRADED) {
    // set output safe state
    setSafeStateOutput();
    return;
  }

  if (m_inputData.m_ticInput.m_funcState == runnable::tic::ETICState::ACTIVE) {
    if (m_inputData.m_ticInput.m_deviceState !=
        runnable::tic::EDeviceState::DEVICE_ACTIVE_CONFIGURED) {
      // go to safe state since critical device not active
      setSafeStateOutput();
      return;
    }

    m_output.m_funcState = ELLCState::ACTIVE;

    if (m_inputData.m_ticInput.m_touchInteraction ==
            runnable::tic::ETouchInteraction::SLIDING_DETECTED ||
        m_inputData.m_ticInput.m_touchInteraction ==
            runnable::tic::ETouchInteraction::TOUCH_DETECTED) {
      // get slider coord.
      if (m_inputData.m_ticInput.m_coordinateState ==
          runnable::tic::ECoordinateState::AVAILABLE) {
        // set a new setpoint
        setNewSetpointFromSlider();
      } else {
        // keep previous output
        m_output.m_lightState = m_prevOutput.m_lightState;
        m_output.m_dimLevel = m_prevOutput.m_dimLevel;
      }

    } else {
      // keep previous output
      m_output.m_lightState = m_prevOutput.m_lightState;
      m_output.m_dimLevel = m_prevOutput.m_dimLevel;

      // check if aws request exists
      if (m_inputData.m_dcmInput.m_awsDesiredLightLvl ==
          runnable::dcm::output::EAWSDesiredLightState::NEW_LVL_DESIRED) {
        m_output.m_dimLevel = m_inputData.m_dcmInput.m_desiredLvl;
        m_output.m_lightState = setpointToLightState(m_output.m_dimLevel);
      }
    }
  }
}

void CLLCRunnable::updateStorage() {
  // storage empty for now
}

void CLLCRunnable::sendOutput() {
  m_prevOutput = m_output;
  m_llcOutputPort->setData(m_output);
  m_hwDelegate.setHW(m_output);
}

void CLLCRunnable::setSafeStateOutput() {
  m_output.m_funcState = ELLCState::INACTIVE;
  m_output.m_lightState = ELightState::LIGHT_OFF;
  m_output.m_dimLevel = 0U;
}

void CLLCRunnable::setNewSetpointFromSlider() {
  const uint8_t setpoint =
      convertSliderCoordToLLCSetpoint(m_inputData.m_ticInput.m_sliderLevel);

  m_output.m_funcState = ELLCState::ACTIVE;
  m_output.m_lightState = setpointToLightState(setpoint);
  m_output.m_dimLevel = setpoint;
}

ELightState CLLCRunnable::setpointToLightState(const uint8_t f_setpoint) {
  switch (f_setpoint) {
    case 0U:
      return ELightState::LIGHT_OFF;
      break;
    case 100U:
      return ELightState::LIGHT_FULL_ON;
      break;
    default:
      return ELightState::LIGHT_DIM;
      break;
  }
}

uint8_t CLLCRunnable::convertDimCoordToLLCSetpoint(const uint8_t f_dimCoord) {
  const uint8_t newLinRange = 99;  // 1-99-DIM
  const uint8_t oldLinRange = hw::G_SLIDER_DIM_UPP_BOUND -
                              hw::G_SLIDER_DIM_LOW_BOUND - hw::G_SLIDER_HYST;
  uint8_t minDimCoordinate(hw::G_SLIDER_DIM_LOW_BOUND + hw::G_SLIDER_HYST);

  if (f_dimCoord >= minDimCoordinate &&
      f_dimCoord <= hw::G_SLIDER_DIM_UPP_BOUND) {
    const uint8_t scaledValue =
        uint8_t(((float(newLinRange) / float(oldLinRange)) *
                 float(f_dimCoord - minDimCoordinate)) +
                1.F);
    return scaledValue;
  } else if (f_dimCoord < minDimCoordinate) {
    return 1U;
  } else {
    return 99U;
  }
}

uint8_t CLLCRunnable::convertSliderCoordToLLCSetpoint(
    const uint8_t f_sliderCoord) {
  uint8_t minDimCoordinate(hw::G_SLIDER_DIM_LOW_BOUND);
  uint8_t maxDimCoordinate(hw::G_SLIDER_DIM_UPP_BOUND);
  // Hysteresis
  if (m_prevOutput.m_lightState == ELightState::LIGHT_OFF) {
    minDimCoordinate = hw::G_SLIDER_DIM_LOW_BOUND + hw::G_SLIDER_HYST;
    maxDimCoordinate = hw::G_SLIDER_DIM_UPP_BOUND;
  } else if (m_prevOutput.m_lightState == ELightState::LIGHT_DIM) {
    minDimCoordinate = hw::G_SLIDER_DIM_LOW_BOUND;
    maxDimCoordinate = hw::G_SLIDER_DIM_UPP_BOUND;
  } else if (m_prevOutput.m_lightState == ELightState::LIGHT_FULL_ON) {
    minDimCoordinate = hw::G_SLIDER_DIM_LOW_BOUND;
    maxDimCoordinate = hw::G_SLIDER_DIM_UPP_BOUND - hw::G_SLIDER_HYST;
  }

  if (f_sliderCoord < minDimCoordinate) {
    return 0U;
  } else if (f_sliderCoord >= minDimCoordinate &&
             f_sliderCoord <= maxDimCoordinate) {
    return convertDimCoordToLLCSetpoint(f_sliderCoord);
  } else {
    return 100U;
  }
}

}  // namespace llc
}  // namespace runnable