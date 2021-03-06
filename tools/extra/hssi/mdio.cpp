#include "mdio.h"
#include <iostream>
#include <chrono>
#include <thread>

namespace intel
{
namespace fpga
{
namespace hssi
{

using namespace std;
using namespace std::chrono;

mdio::mdio(przone_interface::ptr_t przone)
: przone_(przone)
{
}

bool mdio::write(uint8_t device_addr, uint8_t port_addr, uint16_t reg_addr, uint32_t value)
{
    uint32_t addr = (mdio_device_address_mask & (device_addr << mdio_device_address))
                  | (mdio_port_address_mask & (port_addr << mdio_port_addres))
                  | (mdio_register_address_mask & (reg_addr << mdio_register_address));

    przone_->write(mdio_wr_data_reg,  addr); //Write contents of MDIO addr reg
    przone_->write(mdio_ctrl_reg, mdio_write | mdio_address_reg); //Write request to MDIO addr reg
    wait_for_mdio_tx();
    przone_->write(mdio_wr_data_reg, value); //Write contents of MDIO data reg
    przone_->write(mdio_ctrl_reg, mdio_write | mdio_access_reg); //Write request for data reg
    wait_for_mdio_tx();

    return true;
}

bool mdio::read(uint8_t device_addr, uint8_t port_addr, uint16_t reg_addr, uint32_t &value)
{
    uint32_t addr = (mdio_device_address_mask & (device_addr << mdio_device_address))
                  | (mdio_port_address_mask & (port_addr << mdio_port_addres))
                  | (mdio_register_address_mask & (reg_addr << mdio_register_address));

    przone_->write(mdio_wr_data_reg,  addr); //Write contents of MDIO addr reg
    przone_->write(mdio_ctrl_reg, mdio_write | mdio_address_reg); //Write request to MDIO addr reg
    wait_for_mdio_tx();
    przone_->write(mdio_ctrl_reg, mdio_read | mdio_access_reg); //Write read request for data
    wait_for_mdio_tx();

    uint32_t temp;
    if(przone_->read(mdio_rd_data_reg, temp))
        {
            value = temp;
        }
        else
        {
            std::cerr << "WARNING: Could not complete MDIO read" << std::endl;
        }

     return true;
}

bool mdio::wait_for_mdio_tx(uint32_t timeout_usec)
{
    auto begin  = high_resolution_clock::now();
    uint32_t stat;
    while (true)
    {
        auto delta = high_resolution_clock::now() - begin;
        if (delta >= microseconds(timeout_usec))
        {
            log_.warn() << "Timed out waiting for MDIO TX to stop" << std::endl;
            return false;
        }

        if (!przone_->read(mdio_ctrl_reg, stat))
        {
            std::cerr << "ERROR: Waiting for MDIO ready" << std::endl;
            return false;
        }

        if ((~stat & mdio_write) && (~stat & mdio_read))
        {
            return true;
        }
        else
        {
            std::this_thread::sleep_for(microseconds(10));
        }
    }
}

} // end of namespace hssi
} // end of namespace fpga
} // end of namespace intel

