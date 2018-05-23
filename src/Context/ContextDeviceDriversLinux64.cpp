#include "ContextDeviceDriversLinux64.h"

#include <iostream>

namespace ceagle {

void ContextDeviceDriversLinux64::parse(std::string file) {
#if DEBUG_CONTEXT
    std::cout << "ContextDeviceDriversLinux64 parsing ..." << std::endl;
#endif
    if ((file.find("ldv_s_kbtab_driver_usb_driver") != std::string::npos)
        || (file.find("ldv_blast_assert()") != std::string::npos)
        || (file.find("ldv_error()") != std::string::npos)) {
        this->m_result.matched = true;
        this->m_result.message.push_back("should be a device driver benchmark");
    } else {
        this->m_result.matched = false;
        this->m_result.message.push_back("found nothing like device driver");
    }
    // message are number of file
    //std::cout << "ContextDeviceDriversLinux64 finished finding" << std::endl;
#if DEBUG_CONTEXT
    std::cout << "ContextDeviceDriversLinux64 found "  << this->m_result.matched << " " << this->m_result.message[0] << std::endl;
#endif
}

}
