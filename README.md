## Modbus RTU to iec 60870-5-104.

iec2modbus is open-source Modbus RTU (serial) to IEC 60870-5-104 (TCP) gateway for linux system.
Based on [lib60870-C](https://github.com/mz-automation/lib60870), [libmodbus](https://github.com/stephane/libmodbus), [json-c](https://github.com/json-c/json-c), [slog](https://github.com/kala13x/slog).
Modbus RTU run as master and iec104 as slave (server).

Features:

    Small footprint - suitable to run on embedded devices and SBCs like Raspberry Pi
    Multi-master - multiple IEC104 TCP clients can access to gateway
    Flexible RTU modes - speed/parity/stop-bits can be configured for RTU network
    Multi-serial - working with multiple serial buses at the same time



Installation instructions:
----------------------------

```
$ git clone https://github.com/3cky/mbusd.git mbusd.git
$ cd mbusd.git
$ mkdir -p build && cd build
$ cmake -DCMAKE_INSTALL_PREFIX=/usr ..
$ make
$ sudo make install
```