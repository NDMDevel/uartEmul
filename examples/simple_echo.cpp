/*
 *  README:
 * 
 * Build this example with a c++20 compatible compiler:
 *
 * In console type:
 * g++ --std=c++20 ..\ceSerial\ceSerial.cpp .\simple_echo.cpp -o simple_echo.exe
 * ./simple_echo.exe
 */

#include "..\UartEmul.hpp"
#include <iostream>

int main()
{
    using Uart15 = UartEmul<15>;
    Uart15 uart;
    uart.init(115200);
    std::cout << "Opening port tty/com 15 at 115200 Bauds..." << std::endl;
    std::cout << "You should open a new port connected to this (15) and send\nsome data to see the echo." << std::endl;
    bool run = true;
    while( run )
    {
        while( !uart.rx_available() );
        auto data = uart.rx_read();
        if( data == '@' )
            return 0;
        std::cout << char(data) << std::endl;
        while( !uart.tx_write(&data,1) );
    }
    return 0;
}