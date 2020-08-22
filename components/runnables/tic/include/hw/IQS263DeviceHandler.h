
#pragma once

#include "hw/IQS263B.h"
#include "hw/IQS263Config.h"
#include "memport.h"
#include "output/tic_output.h"
#include "smooth/core/Application.h"
#include "smooth/core/io/i2c/I2CMasterDevice.h"
#include "smooth/core/io/i2c/Master.h"
#include "smooth/core/logging/log.h"
#include "ticRunnable.h"

namespace runnable {
namespace tic {
namespace hw {

static const std::string G_TASK_TAG("[RUN::TIC]");

class CIQS263DeviceHandler {
 public:
  CIQS263DeviceHandler(runnable::tic::CTicOutput& f_out)
      : m_output(f_out),
        m_i2c_master(I2C_NUM_0,        // I2C Port 0
                     hw::G_PIN_SCL,    // SCL pin
                     false,            // SCL internal pullup NOT enabled
                     hw::G_PIN_SDA,    // SDA pin
                     false,            // SDA internal pullup NOT enabled
                     hw::G_I2C_FREQ),  // clock frequency - 100kHz
        m_touchDevice(),
        m_deviceAvailable(false),
        m_deviceConfigured(false) {
    m_touchDevice = m_i2c_master.create_device<hw::IQS263B>(hw::G_IQS263_ADDR);
  }

  void configureDevice() {
    m_deviceAvailable = m_touchDevice->is_present();
    if (m_deviceAvailable) {
      Log::info(G_TASK_TAG,
                "IQS263B Device available --- Attempting Initialization...");
    } else {
      Log::error(G_TASK_TAG,
                 "IQS263B Device NOT available --- Aborting Configuration");
    }

    m_deviceConfigured = m_touchDevice->configure_device();
    if (m_deviceConfigured) {
      Log::info(G_TASK_TAG, "IQS263B intialization --- {}",
                m_deviceConfigured ? "Succeeded" : "Failed");
    } else {
      Log::error(G_TASK_TAG, "IQS263B intialization --- {}",
                 m_deviceConfigured ? "Succeeded" : "Failed");
    }
  }

  void handleDeviceAndSetOutput() {
    if (!m_deviceAvailable) {
      m_deviceAvailable = m_touchDevice->is_present();
      m_output.m_funcState = ETICState::INACTIVE;
      m_output.m_deviceState = (EDeviceState::DEVICE_NOT_PRESENT),
      m_output.m_touchInteraction = (ETouchInteraction::NO_INTERACTION),
      m_output.m_coordinateState = (ESliderCoordinates::NOT_AVAILABLE),
      m_output.m_sliderLevel = (0U);
      return;
    }
    if (!m_deviceConfigured) {
      m_deviceConfigured = m_touchDevice->configure_device();
      m_output.m_funcState = ETICState::INACTIVE;
      m_output.m_deviceState = (EDeviceState::DEVICE_NOT_PRESENT),
      m_output.m_touchInteraction = (ETouchInteraction::NO_INTERACTION),
      m_output.m_coordinateState = (ESliderCoordinates::NOT_AVAILABLE),
      m_output.m_sliderLevel = (0U);
      return;
    }
    m_output.m_funcState = ETICState::ACTIVE;
    m_output.m_deviceState = EDeviceState::DEVICE_ACTIVE_CONFIGURED;

    CSystemFlagsEventsData sysEventData;

    if (m_touchDevice->read_system_flags_events(sysEventData)) {
      if (sysEventData.m_slide == ESlideEvent::SLIDE) {
        m_output.m_touchInteraction = ETouchInteraction::SLIDING_DETECTED;
      } else if (sysEventData.m_touch == ETouchEvent::TOUCH) {
        m_output.m_touchInteraction = ETouchInteraction::TOUCH_DETECTED;
      } else if (sysEventData.m_prox == EProxEvent::PROX) {
        m_output.m_touchInteraction = ETouchInteraction::PROXIMITY_DETECTED;
      } else {
        m_output.m_touchInteraction = ETouchInteraction::NO_INTERACTION;
      }
    } else {
      m_output.m_touchInteraction = ETouchInteraction::NOT_AVAILABLE;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    CSliderCoordinateData sliderData;

    if (m_touchDevice->read_wheel_coordinates(sliderData)) {
      if (sliderData.m_commState == ECommState::SUCCESS) {
        m_output.m_coordinateState = ESliderCoordinates::AVAILABLE;
        m_output.m_sliderLevel = sliderData.m_sliderCoord;
      }
    } else {
      m_output.m_coordinateState = ESliderCoordinates::NOT_AVAILABLE;
    }
  }

 private:
  runnable::tic::CTicOutput& m_output;
  smooth::core::io::i2c::Master m_i2c_master;
  std::unique_ptr<runnable::tic::hw::IQS263B> m_touchDevice{};

  bool m_deviceAvailable;
  bool m_deviceConfigured;
};

}  // namespace hw
}  // namespace tic
}  // namespace runnable