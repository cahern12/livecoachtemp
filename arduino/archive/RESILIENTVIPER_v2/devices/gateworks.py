__author__ = 'thomas.ballas'

import subprocess
import time
import os


class Gateworks:

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
        try:
            with open('/sys/class/hwmon/hwmon0/device/temp0_input', 'r') as t:
                temp = t.read().strip()
        except:
            temp = "UNK"
        return temp

    @staticmethod
    def sleep(seconds):
        os.system("sleep {}".format(seconds))


    @staticmethod
    def check_peripheral(peripheral):
        command = "ls /dev/"
        resp = Gateworks.exec_command(command)
        if len(resp) > 0:
            temp_tuple = resp[0]
            if peripheral in temp_tuple:
                return True
        return False

