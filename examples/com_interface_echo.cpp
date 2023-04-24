/*
 *  README:
 * 
 * Build this example with a c++20 compatible compiler:
 *
 * In console type:
 * g++ --std=c++20 ..\ceSerial\ceSerial.cpp .\com_interface_echo.cpp -o com_interface_echo.exe
 * ./com_interface_echo.exe
 */

#include <iostream>
#include "../../../desktopEmul/uart/UartEmul.hpp"
#include "../../../desktopEmul/timer/TimerEmul.hpp"
#include "../../../mcuLibrary/uart/ComInterface.hpp"

static   std::vector<uint8_t> data;
uint32_t master_proc(uint8_t *vec,uint32_t vecLen);
uint32_t slave_proc(uint8_t *vec,uint32_t vecLen);
void     master_error();

using Timer32= TimerEmul<uint32_t,std::chrono::milliseconds>;
using Timer16= TimerEmul<uint16_t,std::chrono::milliseconds>;
using Uart15 = UartEmul<15>;
using Uart16 = UartEmul<16>;
using ComMasterImp = ComMaster<Timer32,
                               100,
                               uint8_t,
                               32,
                               Uart15::rx_available,
                               Uart15::rx_read,
                               Uart15::tx_ready,
                               Uart15::tx_write,
                               master_proc,
                               master_error>;
using ComSlaveImp  = ComSlave< Timer32,
                               100,
                               uint8_t,
                               32,
                               Uart16::rx_available,
                               Uart16::rx_read,
                               Uart16::tx_ready,
                               Uart16::tx_write,
                               slave_proc>;

uint32_t master_proc(uint8_t *vec,uint32_t vecLen)
{
    std::cout << "master_proc: ";
    for( uint32_t idx=0 ; idx<vecLen ; idx++ )
    {
        std::cout << int(vec[idx]) << " ";
        data[idx] = vec[idx];
    }
    std::cout << std::endl;
    return 0;
}

static ComSlaveImp *slave;
uint32_t slave_proc(uint8_t *vec,uint32_t vecLen)
{
    std::cout << "slave_proc: ";
    for( uint32_t idx=0 ; idx<vecLen ; idx++ )
    {
        std::cout << int(vec[idx]) << " ";
        vec[idx] += 1;
    }
    std::cout << std::endl;

    return vecLen;
}
void master_error()
{
    std::cout << "master_error()" << std::endl;
}

int main()
{
    Uart15 uart15;
    Uart16 uart16;
    uart15.init(115200);
    uart16.init(115200);

    ComMasterImp master;
    ComSlaveImp slave;
    ::slave = &slave;
    master.start();
    slave.start();

    Timer32 tim;
    data.push_back(10);
    data.push_back(20);
    data.push_back(30);
    data.push_back(40);
    uint8_t _xor = 0;
    for( const auto& item : data )
        _xor ^= item;
    data.push_back(_xor);
    while( true )
    {
        master();
        slave();
        if( tim > 1000ms )
        {
            std::cout << "loop 1000ms" << std::endl;
            tim.reset();
            if( data[0] == 20 )
                break;
            master.send(data.data(),uint32_t(data.size()));
        }
    }
    ::slave = nullptr;
    return 0;
}
