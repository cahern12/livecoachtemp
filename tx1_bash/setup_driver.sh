#!/bin/bash
cd /usr/src/linux-headers-3.10.96-tegra/
sudo make modules_prepare
cd /home/ubuntu/Desktop/sdk_812_1.1.14_linux/driver/
sudo su
make jetson=y
make install
make load
#completed. 