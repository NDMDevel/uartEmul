#ifndef UARTEMUL_HPP
#define UARTEMUL_HPP

#include "ceSerial/ceSerial.h"
#include <cstring>
#include <cstdint>
#include <vector>
#include <condition_variable>
#include <thread>
#include <mutex>

//#include <iostream>

/*
v1: templated uartemul class
The template is needed because the working thread is static.
With the template, the class will be of a diferent type for each
port, and each port will implement its own thread.

v2: non template
Without template, the working thread will be the same for diferents
ports, resulting an invalid behavior.
*/

namespace uartemul
{
inline namespace v1
{
#define proto_header template<uint32_t portNum>
#define proto_instance UartEmul<portNum>
#define proto(rettype) proto_header rettype proto_instance

proto_header
class UartEmul
{
public:
    UartEmul();
    ~UartEmul();
    static bool     isActive();
    static bool     init(uint32_t baudRate,const char* devName = nullptr);
    static uint32_t rx_available();
    static uint8_t  rx_read();
    static bool     tx_ready();
    static bool     tx_write(const uint8_t *buff,uint32_t buffLen);
    static bool     tx_write(uint8_t data);
    static const uint8_t* rx_peek();
    static void     rx_consume(uint32_t len);
private:
    static void txThread();

private:
    std::thread                     _txThread;
    static std::mutex               _mutex;
    static std::condition_variable  _txRequest;
    static bool                     _txReady;
    static bool                     _txTerminate;
    static ce::ceSerial             _com;
    static std::vector<uint8_t>     _rxBuff;
    static const uint8_t*           _txBuff;
    static uint32_t                 _txBuffLen;
    static uint8_t                  _txTempData;
};

proto(std::mutex)::_mutex;
proto(std::condition_variable)::_txRequest;
proto(bool)::_txReady = true;
proto(bool)::_txTerminate = false;
proto(ce::ceSerial)::_com;
proto(std::vector<uint8_t>)::_rxBuff;
proto(const uint8_t*)::_txBuff = nullptr;
proto(uint32_t)::_txBuffLen = 0;
proto(uint8_t)::_txTempData;

proto()::UartEmul()
    : _txThread(txThread)
{
}
proto()::~UartEmul()
{
    std::unique_lock<std::mutex> lock(_mutex);
    //critical section
    _txTerminate = true;
    _txRequest.notify_one();
    lock.unlock();
    //end critical section
    _txThread.join();
}
proto(bool)::isActive()
{
    return _com.IsOpened();
}
proto(bool)::init(uint32_t baudRate,const char* devName)
{
    if( _com.IsOpened() )
       _com.Close();
#ifdef ceWINDOWS
//    std::cout << "\\\\.\\COM"+std::to_string(portNum) << std::endl;
    if( /*strlen(devName) == 0*/ devName == nullptr )
        _com.SetPort("\\\\.\\COM"+std::to_string(portNum));
    else
        _com.SetPort("\\\\.\\"+std::string(devName)+std::to_string(portNum));
#else   //linux
//    std::cout << "\\\\.\\tty"+std::to_string(portNum) << std::endl;
//    _com.SetPort("\\\\.\\tty"+std::to_string(portNum));
//    _com.SetPort("/dev/pts/"+std::to_string(portNum));
    if( /*strlen(devName) == 0*/ devName == nullptr )
        _com.SetPort("/dev/tty"+std::to_string(portNum));
    else
        _com.SetPort("/dev/"+std::string(devName)+std::to_string(portNum));
//    _com.SetPort("/dev/tty"+std::to_string(portNum));
#endif
    _com.SetBaudRate(baudRate);
    _com.SetDataSize(8);
    _com.SetParity('N');
    _com.SetStopBits(1);
    _com.Open();
    return _com.IsOpened();
}
proto(uint32_t)::rx_available()
{
    bool available = true;
    while( available )
    {
        uint8_t data = _com.ReadChar(available);
        if( available )
            _rxBuff.push_back(data);
    }
    return _rxBuff.size();
}
proto(uint8_t)::rx_read()
{
    if( _rxBuff.size() == 0 )
        return 0;
    //auto len = _rxBuff.size();
    auto data = _rxBuff[0];
    _rxBuff.erase(_rxBuff.begin());
    return data;
}
proto(bool)::tx_ready()
{
    std::unique_lock<std::mutex> lock(_mutex,std::defer_lock);
    if( !lock.try_lock() )
        return false;

    //critical section
    return (_txBuff == nullptr) && (_txBuffLen == 0);
}
proto(bool)::tx_write(const uint8_t *buff,uint32_t buffLen)
{
    std::unique_lock<std::mutex> lock(_mutex,std::defer_lock);
    if( !lock.try_lock() || buff == nullptr || buffLen == 0 )
        return false;

    //critical section
    _txBuff    = buff;
    _txBuffLen = buffLen;
    //end critical section

    _txRequest.notify_one();
    return true;
}
proto(bool)::tx_write(uint8_t data)
{
    std::unique_lock<std::mutex> lock(_mutex,std::defer_lock);
    if( !lock.try_lock() )
        return false;

    //critical section
    _txTempData = data;
    _txBuff    = &_txTempData;
    _txBuffLen = 1;
    //end critical section

    _txRequest.notify_one();
    return true;
}
proto(const uint8_t*)::rx_peek()
{
    return _rxBuff.begin().base();
}
proto(void)::rx_consume(uint32_t len)
{
    len = std::min(len,rx_available());
    _rxBuff.erase(_rxBuff.begin(),_rxBuff.begin()+len);
}
//#include <iostream>
proto(void)::txThread()
{
    std::unique_lock<std::mutex> lock(_mutex,std::defer_lock);
    while( true )
    {
        _txRequest.wait(lock);

        //critical section
        if( _txTerminate )
            return;
        //send the data
//        std::cout << "thread send\n";
        for( uint32_t idx=0 ; idx<_txBuffLen ; idx++ )
        {
            _com.WriteByte(_txBuff[idx]);
//            std::cout << int(_txBuff[idx]) << std::endl;
        }
        _txBuff    = nullptr;
        _txBuffLen = 0;
        //end critical section
    }
}

#undef proto_header
#undef proto_instance
#undef proto
}//inline namespace v1

#ifdef UARTEMUL_USING_NO_TEMPLATE_IMP
inline namespace v2
{
#error "not used v2, will produce wrong thread implementations"
class UartEmul
{
public:
    UartEmul();
    ~UartEmul();
    static bool     isActive();
    static bool     init(uint32_t baudRate,uint32_t portNum,const char* devName = nullptr);
    static uint32_t rx_available();
    static uint8_t  rx_read();
    static bool     tx_ready();
    static bool     tx_write(const uint8_t *buff,uint32_t buffLen);
    static bool     tx_write(uint8_t data);
    static const uint8_t* rx_peek();
    static void     rx_consume(uint32_t len);
private:
    static void txThread();

private:
    std::thread                     _txThread;
    static std::mutex               _mutex;
    static std::condition_variable  _txRequest;
    static bool                     _txReady;
    static bool                     _txTerminate;
    static ce::ceSerial             _com;
    static std::vector<uint8_t>     _rxBuff;
    static const uint8_t*           _txBuff;
    static uint32_t                 _txBuffLen;
    static uint32_t                 _portNum;
    static uint8_t                  _txTempData;
};

}//inline namespace v2
#endif //UARTEMUL_USING_NO_TEMPLATE_IMP
}//namespace uartemul
#endif // UARTEMUL_HPP
