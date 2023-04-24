#include "UartEmul.hpp"

#ifdef UARTEMUL_USING_NO_TEMPLATE_IMP
namespace uartemul
{
inline namespace v2
{
std::mutex              UartEmul::_mutex;
std::condition_variable UartEmul::_txRequest;
bool                    UartEmul::_txReady = true;
bool                    UartEmul::_txTerminate = false;
ce::ceSerial            UartEmul::_com;
std::vector<uint8_t>    UartEmul::_rxBuff;
const uint8_t*          UartEmul::_txBuff = nullptr;
uint32_t                UartEmul::_txBuffLen = 0;
uint32_t                UartEmul::_portNum;
uint8_t                 UartEmul::_txTempData;

UartEmul::UartEmul()
    : _txThread(txThread)
{
}
UartEmul::~UartEmul()
{
    std::unique_lock<std::mutex> lock(_mutex);
    //critical section
    _txTerminate = true;
    _txRequest.notify_one();
    lock.unlock();
    //end critical section
    _txThread.join();
}
bool UartEmul::isActive()
{
    return _com.IsOpened();
}
bool UartEmul::init(uint32_t baudRate,uint32_t portNum,const char* devName)
{
    if( _com.IsOpened() )
       _com.Close();
    _portNum = portNum;
#ifdef ceWINDOWS
//    std::cout << "\\\\.\\COM"+std::to_string(portNum) << std::endl;
    if( /*strlen(devName) == 0*/ devName == nullptr )
        _com.SetPort("\\\\.\\COM"+std::to_string(_portNum));
    else
        _com.SetPort("\\\\.\\"+std::string(devName)+std::to_string(_portNum));
#else   //linux
//    std::cout << "\\\\.\\tty"+std::to_string(_portNum) << std::endl;
//    _com.SetPort("\\\\.\\tty"+std::to_string(_portNum));
//    _com.SetPort("/dev/pts/"+std::to_string(_portNum));
    if( /*strlen(devName) == 0*/ devName == nullptr )
        _com.SetPort("/dev/tty"+std::to_string(_portNum));
    else
        _com.SetPort("/dev/"+std::string(devName)+std::to_string(_portNum));
//    _com.SetPort("/dev/tty"+std::to_string(_portNum));
#endif
    _com.SetBaudRate(baudRate);
    _com.SetDataSize(8);
    _com.SetParity('N');
    _com.SetStopBits(1);
    _com.Open();
    return _com.IsOpened();
}
uint32_t UartEmul::rx_available()
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
uint8_t UartEmul::rx_read()
{
    if( _rxBuff.size() == 0 )
        return 0;
    //auto len = _rxBuff.size();
    auto data = _rxBuff[0];
    _rxBuff.erase(_rxBuff.begin());
    return data;
}
bool UartEmul::tx_ready()
{
    std::unique_lock<std::mutex> lock(_mutex,std::defer_lock);
    if( !lock.try_lock() )
        return false;

    //critical section
    return (_txBuff == nullptr) && (_txBuffLen == 0);
}
bool UartEmul::tx_write(const uint8_t *buff,uint32_t buffLen)
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
bool UartEmul::tx_write(uint8_t data)
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
const uint8_t* UartEmul::rx_peek()
{
    return _rxBuff.begin().base();
}
void UartEmul::rx_consume(uint32_t len)
{
    len = std::min(len,rx_available());
    _rxBuff.erase(_rxBuff.begin(),_rxBuff.begin()+len);
}
void UartEmul::txThread()
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
}//inline namespace v2
}//namespace uartemul
#endif //UARTEMUL_USING_NO_TEMPLATE_IMP
