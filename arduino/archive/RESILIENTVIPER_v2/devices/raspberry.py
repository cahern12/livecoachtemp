__author__ = 'thomas.ballas'

import subprocess
import time
import os


class Raspberry:

    def __init__(self):
        pass

    @staticmethod
    def exec_command(command):
        com_l = command.split()
        proc = subprocess.Popen(com_l, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        resp = proc.communicate()
        return resp

    @staticmethod
    def get_temp():
        command = "/opt/vc/bin/vcgencmd measure_temp"
        temp = "UNK"
        resp = Raspberry.exec_command(command)
        if len(resp) > 0:
            temp_tuple = resp[0]
            if "temp=" in temp_tuple and "C" in temp_tuple:
                tmp_temp = temp_tuple.split("=")[1].split("'")[0]
                if len(tmp_temp) > 0:
                    temp = tmp_temp
        return temp

    @staticmethod
    def sleep(seconds):
        # os.system("sudo sh -c 'echo 0 > /sys/devices/platform/soc/20980000.usb/buspower'")
        os.system("sudo sh -c 'echo 0 > /sys/devices/platform/soc/3f980000.usb/buspower'")
        time.sleep(1)
        os.system("sudo sh ./resources/tv_off.sh")
        time.sleep(seconds - 2)
        # os.system("sudo sh -c 'echo 1 > /sys/devices/platform/soc/20980000.usb/buspower'")
        os.system("sudo sh -c 'echo 1 > /sys/devices/platform/soc/3f980000.usb/buspower'")
        time.sleep(1)
        os.system("sudo sh ./resources/tv_on.sh")

    @staticmethod
    def check_peripheral(peripheral):
        command = "ls /dev/"
        resp = Raspberry.exec_command(command)
        if len(resp) > 0:
            temp_tuple = resp[0]
            if peripheral in temp_tuple:
                return True
        return False

# print Raspberry.get_temp()
# Raspberry.sleep(15)
# print Raspberry.check_peripheral("ttyAMA0")
