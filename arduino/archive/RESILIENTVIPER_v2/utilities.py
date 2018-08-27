__author__ = 'thomas.ballas'

import time
import os


def logger(source, msg):
    log = "{} - {}".format(source, msg)
    print "{} - {}".format(time.ctime(), log)
    return log


def host_online(ip):
    hostname = ip
    response = os.system("ping -c 1 " + hostname)
    if response == 0:
        return True
    else:
        return False
