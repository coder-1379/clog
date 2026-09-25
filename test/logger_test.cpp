#include <iostream>
#include <clog/logger.h>

// g++ -std=c++11 -pthread -I.. logger_test.cpp ../clog/logger.cpp -o logger_test.bin

int main()
{
    clog::logger::configure(clog::logger::DEBUG, "%d %t %l %F:%L %M %m");
    //clog::logger::configure("logger.log", clog::logger::DEBUG, "%d %t %l %F:%L %M %m");
    LOG_INFO("hello %d", 100);
}