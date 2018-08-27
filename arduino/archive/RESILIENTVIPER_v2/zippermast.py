__author__ = 'thomas.ballas'

import time
from crc_ccitt import crc16xmodem
import serial
import utilities
import servo_controller

# CONSTANTS
MAST_MIN = 210
MAST_MAX = 2100
MAST_HEIGHT = 1400
NAME = "zippermast"


def mast_up(commander):
    servo = servo_controller.Servo()
    servo.open_lid()
    set_mast_height(commander, MAST_HEIGHT)


def mast_down(commander):
    set_mast_height(commander, 0)
    servo = servo_controller.Servo()
    servo.close_lid()


def set_mast_height(commander, target_height):
    commander.relay_on("zipmast_relay")
    time.sleep(.1)
    commander.relay_on("zipmast_wake")
    time.sleep(.5)
    commander.relay_off("zipmast_wake")
    cur_ht = check_zippermast(commander)
    if "NO RESPONSE" not in cur_ht:
        commander.device_stats["mast"] = cur_ht
    target_mm = int(target_height) + MAST_MIN
    cur_mm = int(commander.device_stats["mast"])
    # height change below granularity of movement; doing nothing
    if in_range(target_mm, cur_mm, 25):
        commander.debug_logger(utilities.logger(NAME, "[STATUS] User tried to set mast height to current mast height."))
    # recurse with height set to zero - function will set to MAST_MIN
    elif target_mm < MAST_MIN:
        commander.debug_logger(utilities.logger(NAME, "[STATUS] User tried to set mast height below {} mm threshold.".format(MAST_MIN)))
        return set_mast_height(commander, str(0))
    # recurse with target height set to greatest acceptable input: (MAST_MAX - MAST_MIN)
    elif target_mm > MAST_MAX:
        commander.debug_logger(utilities.logger(NAME, "[STATUS] User tried to set mast height above {} mm threshold.".format(MAST_MAX)))
        return set_mast_height(commander, str(MAST_MAX - MAST_MIN))
    # input was within acceptable parameters; continue with raise mast
    elif target_mm > cur_mm:
        travel_mm = target_mm - cur_mm
        commander.debug_logger(utilities.logger(NAME, "[STATUS] Raising mast for {} mm".format(travel_mm)))
        if travel_mm < 100:
            mast_speed = "25"
        else:
            mast_speed = "100"
        raise_mast(commander, mast_speed)
        done = False
        fail_ctr = 0
        while not done and fail_ctr < 3:
            cur_ht = check_zippermast(commander)
            if "NO RESPONSE" not in cur_ht:
                commander.device_stats["mast"] = cur_ht
                fail_ctr = 0
            else:
                fail_ctr += 1
                continue
            if int(cur_ht) >= target_mm:
                done = True
            else:
                if (target_mm - int(cur_ht)) < 100:
                    raise_mast(commander, "25")
                time.sleep(0.1)
        stop_raise_mast(commander)
    # input was within acceptable parameters, begin lower mast
    else:
        travel_mm = cur_mm - target_mm
        commander.debug_logger(utilities.logger(NAME, "[STATUS] Lowering mast for {} mm".format(travel_mm)))
        if travel_mm < 100:
            mast_speed = "25"
        else:
            mast_speed = "100"
        lower_mast(commander, mast_speed)
        done = False
        fail_ctr = 0
        while not done and fail_ctr < 3:
            cur_ht = check_zippermast(commander)
            if "NO RESPONSE" not in cur_ht:
                commander.device_stats["mast"] = cur_ht
                fail_ctr = 0
            else:
                fail_ctr += 1
            if float(cur_ht) <= target_mm:
                done = True
            else:
                if (int(cur_ht) - target_mm) < 100:
                    lower_mast(commander, "25")
                time.sleep(0.25)
        stop_lower_mast(commander)
    commander.relay_off("zipmast_relay")
    return "{}".format(int(commander.device_stats["mast"]) - MAST_MIN)


def raise_mast(commander, speed):
    if commander.device != "zipbox":
        commander.debug_logger(utilities.logger(NAME, "[ERROR] Tried to execute 'raise_mast' from outside zippermast box. Attempting to forward."))
        return commander.forward_command(commander.device_ip["zipbox"], commander.server_port, "rmt")
    zip_mast_pcmov(commander, "up", speed)
    return "MAST_UP"


def stop_raise_mast(commander):
    if commander.device != "zipbox":
        commander.debug_logger(utilities.logger(NAME, "[ERROR] Tried to execute 'raise_mast' from outside zippermast box. Attempting to forward."))
        return commander.forward_command(commander.device_ip["zipbox"], commander.server_port, "srmt")
    # sleep appropriate amount of time for boot
    response = "NO RESPONSE"
    second_check = ""
    fail_ctr = 0
    while not in_range(response, second_check, 10) and fail_ctr < 5:
        response = zip_mast_pcmov(commander, "up", "0")
        if response is not "NO RESPONSE":
            commander.device_stats["mast"] = response
            time.sleep(0.1)
            second_check = check_zippermast(commander)
        else:
            fail_ctr += 1
        response = zip_mast_pcmov(commander, "up", "0")
    return "MAST_UP"


def in_range(response, second_check, ranger):
    if response is "NO RESPONSE" or second_check is "NO RESPONSE" or response == "" or second_check == "":
        return False
    else:
        ht_one = int(response)
        ht_two = int(second_check)
        if (ht_one - ranger) <= ht_two <= (ht_one + ranger):
            return True
    return False


def lower_mast(commander, speed):
    if commander.device != "zipbox":
        commander.debug_logger(utilities.logger(NAME, "[ERROR] Tried to execute 'lower_mast' from outside zippermast box. Attempting to forward."))
        return commander.forward_command(commander.device_ip["zipbox"], commander.server_port, "lmt")
    # send signal to zippermast relay - circuit closed
    zip_mast_pcmov(commander, "down", speed)
    return "MAST_DOWN"


def stop_lower_mast(commander):
    if commander.device != "zipbox":
        commander.debug_logger(utilities.logger(NAME, "[ERROR] Tried to execute 'lower_mast' from outside zippermast box. Attempting to forward."))
        return commander.forward_command(commander.device_ip["zipbox"], commander.server_port, "slmt")
    response = "NO RESPONSE"
    second_check = ""
    fail_ctr = 0
    while not in_range(response, second_check, 10) and fail_ctr < 5:
        response = zip_mast_pcmov(commander, "down", "0")
        if response is not "NO RESPONSE":
            commander.device_stats["mast"] = response
            time.sleep(0.1)
            second_check = check_zippermast(commander)
        else:
            fail_ctr += 1
    return "MAST_DOWN"


def check_zippermast(commander):
    was_on = True
    if commander.relay_stats["zipmast_relay"] != "1":
        was_on = False
        commander.relay_on("zipmast_relay")
        time.sleep(.5)
        commander.relay_on("zipmast_wake")
        time.sleep(.5)
        commander.relay_off("zipmast_wake")

    commander.debug_logger(utilities.logger(NAME, "[STATUS] Attempting to query zippermast for height."))
    mast_response = "NO RESPONSE"
    # transmit pcmov_down to zippermast
    try:
        modem = serial.Serial(port=commander.physical_ports['zm_port'], baudrate=38400, rtscts=True, timeout=1.0)
        commander.debug_logger(utilities.logger(NAME, "{}".format(modem)))
        modem.flushInput()
        modem.flushOutput()
        mast_response = modem.readline()
        commander.debug_logger(utilities.logger(NAME, "[STATUS] Zippermast response: {}".format(mast_response)))
        modem.flushInput()
        modem.flushOutput()
        modem.close()
    except:
        commander.debug_logger(utilities.logger(NAME, "[ERROR] Failed to write command to zippermast"))

    if not was_on:
        commander.relay_off("zipmast_relay")

    if mast_response is not "" and "$PPSST" in mast_response:
        mast_resps = mast_response.split(",")
        if mast_resps > 3:
            commander.debug_logger(utilities.logger(NAME, "{}".format(mast_resps)))
            height_mm = mast_resps[2]
            return height_mm
    return "NO RESPONSE"


def zip_mast_pcmov(commander, direction, speed):
    directions = {"down": "0",
                  "up": "1"}
    # generate PCMOV message
    zm_pcmov = "PCMOV"
    zm_id = "1"  # value unknown
    zm_dir = directions[direction]
    zm_speed = speed  # 0-100, equiv. to mm/sec
    zm_delay = "0"  # time delay in seconds
    pcmov = "$" + ",".join([zm_pcmov, zm_id, zm_dir, zm_speed, zm_delay])
    crc = hex(crc16xmodem(pcmov[1:]))[2:]
    pcmov = pcmov + "*{}\r\n".format(crc)
    commander.debug_logger(utilities.logger(NAME, "[STATUS] Generated PCMOV instruction: {}".format(pcmov)))
    mast_response = "NO RESPONSE"
    # transmit pcmov_down to zippermast
    try:
        modem = serial.Serial(port=commander.physical_ports['zm_port'], baudrate=38400, rtscts=True, timeout=1.0)
        commander.debug_logger(utilities.logger(NAME, "{}".format(modem)))
        # for i in range(15):
        modem.flushInput()
        modem.flushOutput()
        modem.write(pcmov)
        mast_response = modem.readline()
        commander.debug_logger(utilities.logger(NAME, "[STATUS] Zippermast response: {}".format(mast_response)))
        modem.flushInput()
        modem.flushOutput()
        modem.close()
        commander.debug_logger(utilities.logger(NAME, "[STATUS] Wrote message to port {}".format(commander.zm_port)))
    except:
        commander.debug_logger(utilities.logger(NAME, "[ERROR] Failed to write command to zippermast"))
    if mast_response is not "" and "$PPSST" in mast_response:
        mast_resps = mast_response.split(",")
        if mast_resps > 3:
            height_mm = mast_resps[2]
            return height_mm
    return "NO RESPONSE"


def zippermast_on(commander, zm_port):
    mast_response = ""
    try:
        modem = serial.Serial(port=zm_port, baudrate=38400, rtscts=True, timeout=.5)
        modem.flushInput()
        modem.flushOutput()
        time.sleep(.5)
        mast_response = modem.readline()
        mast_response += modem.readline()
        mast_response += modem.readline()
        modem.flushInput()
        modem.flushOutput()
        modem.close()
        commander.debug_logger(utilities.logger(NAME, "[STATUS] Read from zippermast: '{}'".format(mast_response)))
    except:
        commander.debug_logger(utilities.logger(NAME, "[ERROR] Failed to read from zippermast"))
    if "$PPSST" in mast_response:
        return True
    return
