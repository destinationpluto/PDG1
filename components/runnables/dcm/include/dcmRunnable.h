#pragma once

#include <iostream>

#include "handler/awsHandler.h"
#include "handler/otaHandler.h"
#include "handler/wifiHandler.h"
#include "input/dcm_input.h"
#include "memport.h"
#include "output/dcm_output.h"
#include "output/llc_output.h"
#include "output/tic_output.h"
#include "runnable.h"
#include "smooth/core/Application.h"
#include "smooth/core/network/Wifi.h"

namespace runnable {
namespace dcm {

static const std::string G_TASK_TAG("[RUN::DCM]");

using ticPort_p = tinymemport::TDataPort<runnable::tic::CTicOutput> *;
using llcPort_p = tinymemport::TDataPort<runnable::llc::CLlcOutput> *;
using dcmPort_p = tinymemport::TDataPort<runnable::dcm::output::CDCMOutput> *;
using wifiInstance_p = smooth::core::network::Wifi *;

class CDCMRunnable : public CRunnable {
 public:
  CDCMRunnable()
      : m_ticInputPort(),
        m_llcInputPort(),
        m_dcmOutputPort(),
        m_input(),
        m_output(),
        m_wifiHandler(m_input, m_output, m_wifi_p),
        m_AWSHandler(m_input, m_output),
        m_OTAHandler(m_input, m_output) {}

  void init() override;
  void run() override;

  void attachInputPorts(ticPort_p f_ticInputPort, llcPort_p f_llcInputPort);
  void attachWifi(wifiInstance_p f_wifi);
  void attachOutputPorts(dcmPort_p f_dcmOutputPort);

 private:
  ticPort_p m_ticInputPort;
  llcPort_p m_llcInputPort;
  dcmPort_p m_dcmOutputPort;
  wifiInstance_p m_wifi_p;

  runnable::dcm::input::CDCMInput m_input;
  runnable::dcm::output::CDCMOutput m_output;
  runnable::dcm::handler::CWifiHandler m_wifiHandler;
  runnable::dcm::handler::CAWSHandler m_AWSHandler;
  runnable::dcm::handler::COTAHandler m_OTAHandler;

  void collectInput();
  void doWork();
  void sendOutput();
};
}  // namespace dcm
}  // namespace runnable
