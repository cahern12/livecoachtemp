__author__ = 'thomas.ballas'


import json
import pprint
import socket
import threading
import time
import power_relay_controller
import ibps_state
import thor_commands
import utilities
import zippermast
from devices.raspberry import Raspberry
import ir_modem
import gps_parser
import RPi.GPIO as GPIO


class Commander:
    def __init__(self, device, prc_port, ir_zm_port, batt_port, gps_port):
        self.device = device
        self.hardware = Raspberry()
        self.config = "config.json"
        self.debug_log = './commander_debug_log.txt'
		GPIO.setmode(GPIO.BCM) # Broadcom pin-numbering scheme
		GPIO.setup(18, GPIO.OUT) # LED pin set as output    

		self.commands = {"off": self.shutdown,
                         "hbt": self.heartbeat,
                         "tout": self.get_timeout,
                         "gps": self.get_gps,
                         "cyc": self.pnnl_power_cycle,
                         "boot": self.system_online,
                         "ion": self.ir_on,
                         "iof": self.ir_off,
                         "con": self.comms_on,
                         "cof": self.comms_off,
                         "cmn": self.cam_on,
                         "cmf": self.cam_off,
                         "zon": self.zip_on,
                         "zof": self.zip_off,
                         "pon": self.pnnl_on,
                         "pof": self.pnnl_off,
                         "stat": self.status,
                         "smb": self.status_master_batt,
                         "szb": self.status_zip_batt,
                         "tmp": self.check_temp,
                         "czb": self.charge_zip_batt,
                         "sczb": self.stop_charge_zip_batt,
                         "smh": self.set_mast_height,
                         "von": self.video_on,
                         "vof": self.video_off}
        self.physical_ports = {'prc_port': prc_port,
                               'ir_port': ir_zm_port,
                               'zm_port': ir_zm_port,
                               'batt_port': batt_port,
                               'gps_port': gps_port}
        self.mast_speed = "100"
        self.last_beat = time.time()
        self.timeout = 600
        self.mast_min = 210
        self.mast_max = 2400
        self.ir_thread = None
        # {name: tuple(%, Ah, V, A}
        self.gps_loc = {}
        self.batt_stats = {"master": {"%": "0", "Ah": "0", "V": "0", "A": "0"},
                           "zipbox": {"%": "0", "Ah": "0", "V": "0", "A": "0"}}
        self.relay_stats = {"pnnl_relay": "0",
                            "comms_relay": "0",
                            "zipbox_relay": "0",
                            "zipbatt_relay": "0",
                            "iridium_relay": "0",
                            "zipmast_relay": "0",
                            "camera_relay": "0"}
        self.device_stats = {"pnnl_box": "0",
                             "comm_box": "0",
                             "zipbox": "0",
                             "iridium": "0",
                             "camera": "0",
                             "mast": "210",
                             "m_temp": "",
                             "z_temp": ""}
        try:
            with open(self.config, 'r') as fc:
                json_strc = fc.read()
            config_dict = json.loads(json_strc)
            self.device_ip = config_dict.get("network", {})
            self.server_port = config_dict.get("ports", {}).get("rv", 8080)
            self.pnnl_port = config_dict.get("ports", {}).get("thor", 5432)
            self.relay_nums = config_dict.get("relays", {})
        except:
            print "[ERROR] CRITICAL FAILURE - CONFIG FILE MISSING"
            exit(0)
        if device == "master":
            self.device_stats["m_temp"] = self.check_temp()
        else:
            self.device_stats["z_temp"] = self.check_temp()
            gps_thread = threading.Thread(target=self.get_gps)
            gps_thread.start()
            zm_height = zippermast.check_zippermast(self)
            if zm_height != "NO RESPONSE":
                self.device_stats["mast"] = zm_height
        self.check_batt(device)
        self.debug_logger("[STATUS] Created IP table of {}".format(self.device_ip))
        if self.device == "master":
            if utilities.host_online(self.device_ip["zipbox"]):
                self.device_stats["zipbox"] = "1"
                self.relay_stats["zipbox_relay"] = "1"
            if utilities.host_online(self.device_ip["pnnl"]):
                self.device_stats["pnnl_box"] = "1"
                self.relay_stats["pnnl_relay"] = "1"
            ir_mod = ir_modem.Ir_class()
            if ir_mod.iridium_modem_on():
                self.device_stats["iridium"] = "1"
            self.ir_thread = threading.Thread(target=ir_mod.Receiver)
            self.ir_thread.start()


    def shutdown(self):
        stat = "status:"
        if self.device != "master":
            self.debug_logger("[ERROR] Tried to execute 'off' from outside master box. Attempting to forward.")
            return self.forward_command(self.device_ip["master"], self.server_port, "off")
        if self.device_stats["pnnl_box"] != "0":
            stat += self.forward_command(self.device_ip["zipbox"], self.server_port, "pof")
        if self.device_stats["mast"] != "0":
            stat += self.forward_command(self.device_ip["zipbox"], self.server_port, "smh:0")
        stat += self.cam_off()
        stat += self.zip_off()
        stat += self.stop_charge_zip_batt()
        stat += self.comms_off()
        return stat

    def heartbeat(self):
        if self.device != "master":
            self.debug_logger("[ERROR] Tried to execute 'hbt' from outside master box. Attempting to forward.")
            return self.forward_command(self.device_ip["master"], self.server_port, "hbt")
        self.last_beat = time.time()
        return "T-{}".format(self.check_timeout())

    def check_timeout(self):
        return int(self.timeout - (time.time() - self.last_beat))

    def get_timeout(self):
        if self.device != "master":
            self.debug_logger("[ERROR] Tried to execute 'tout' from outside master box. Attempting to forward.")
            return self.forward_command(self.device_ip["master"], self.server_port, "tout")
        return "T-{}".format(self.check_timeout())

    def get_gps(self):
        if self.device != "zipbox":
            self.debug_logger("[ERROR] Tried to execute 'gps' from outside zippermast box. Attempting to forward.")
            return self.forward_command(self.device_ip["zipbox"], self.server_port, "gps")
        gps_reader = gps_parser.GPS_reader()
        self.gps_loc = gps_reader.read_gps()
        return "{}".format(json.dumps(self.gps_loc))

    def system_online(self):
        if self.device != "master":
            self.debug_logger("[ERROR] Tried to execute 'boot' from outside master box. Attempting to forward.")
            return self.forward_command(self.device_ip["master"], self.server_port, "boot")
        self.heartbeat()
        self.zip_on()
        time.sleep(1)
        self.comms_on()
        self.heartbeat()
        t = 0
        while not utilities.host_online(self.device_ip["zipbox"]):
            if t > 30:
                return "SYSTEM_ONLINE?!ZIPBOX"
            time.sleep(1)
            t += 1
        t = 0
        self.device_stats["zipbox"] = "1"
        self.heartbeat()
        while self.forward_command(self.device_ip["zipbox"], self.server_port, "tout") == "ERROR_TARGET_OFFLINE":
            if t > 30:
                return "SYSTEM_ONLINE?!ZIPBOX_SERVER"
            time.sleep(1)
            t += 1
        self.pnnl_on()
        t = 0
        while not utilities.host_online(self.device_ip["pnnl"]):
            if t > 30:
                return "SYSTEM_ONLINE?!PNNL"
            time.sleep(1)
            t += 1
        self.device_stats["pnnl_box"] = "1"
        self.heartbeat()
        return "SYSTEM_ONLINE"

    def ir_on(self):
        self.device_stats["iridium"] = "1"
        if self.device != "master":
            self.debug_logger("[ERROR] Tried to execute 'ir_on' from outside master box. Attempting to forward.")
            return self.forward_command(self.device_ip["master"], self.server_port, "ion")
        # send signal to comms box relay - circuit closed
        self.relay_on("iridium_relay")
        return "IR_RELAY_ON"

    def ir_off(self):
        self.device_stats["iridium"] = "0"
        if self.device != "master":
            self.debug_logger("[ERROR] Tried to execute 'ir_off' from outside master box. Attempting to forward.")
            return self.forward_command(self.device_ip["master"], self.server_port, "iof")
        # send signal to comms box relay - circuit open
        self.relay_off("iridium_relay")
        return "IR_RELAY_OFF"

    def comms_on(self):
        self.device_stats["comm_box"] = "1"
        if self.device != "master":
            self.debug_logger("[ERROR] Tried to execute 'comms_on' from outside master box. Attempting to forward.")
            return self.forward_command(self.device_ip["master"], self.server_port, "con")
        # send signal to comms box relay - circuit closed
        self.relay_on("comms_relay")
        return "COMM_RELAY_ON"

    def comms_off(self):
        self.device_stats["comm_box"] = "0"
        if self.device != "master":
            self.debug_logger("[ERROR] Tried to execute 'comms_off' from outside master box. Attempting to forward.")
            return self.forward_command(self.device_ip["master"], self.server_port, "cof")
        # send signal to comms box relay - circuit open
        self.relay_off("comms_relay")
        return "COMM_RELAY_OFF"

    def cam_on(self):
        self.device_stats["camera"] = "1"
        if self.device != "zipbox":
            self.debug_logger("[ERROR] Tried to execute 'cam_on' from outside zippermast box. Attempting to forward.")
            return self.forward_command(self.device_ip["zipbox"], self.server_port, "cmn")
        # send signal to zip box relay - circuit closed
        self.relay_on("camera_relay")
        return "CAMERA_RELAY_ON"

    def cam_off(self):
        self.device_stats["camera"] = "0"
        if self.device != "zipbox":
            self.debug_logger("[ERROR] Tried to execute 'cam_off' from outside zippermast box. Attempting to forward.")
            return self.forward_command(self.device_ip["zipbox"], self.server_port, "cmf")
        # send signal to zip box relay - circuit closed
        self.relay_off("camera_relay")
        return "CAMERA_RELAY_OFF"

    def zip_on(self):
        self.device_stats["zipbox"] = "1"
        if self.device != "master":
            self.debug_logger("[ERROR] Tried to execute 'zip_on' from outside master box. Attempting to forward.")
            return self.forward_command(self.device_ip["master"], self.server_port, "zon")
        # send signal to zip box relay - circuit closed
        self.relay_on("zipbox_relay")
        return "ZIPBOX_RELAY_ON"

    def zip_off(self):
        self.device_stats["zipbox"] = "0"
        if self.device != "master":
            self.debug_logger("[ERROR] Tried to execute 'zip_off' from outside master box. Attempting to forward.")
            return self.forward_command(self.device_ip["master"], self.server_port, "zof")
        # send signal to zip box relay - circuit open
        self.relay_off("zipbox_relay")
        return "ZIPBOX_RELAY_OFF"

    def pnnl_on(self):
        self.device_stats["pnnl_box"] = "1"
        if self.device != "zipbox":
            self.debug_logger("[ERROR] Tried to execute 'pnnl_on' from outside zippermast box. Attempting to forward.")
            return self.forward_command(self.device_ip["zipbox"], self.server_port, "pon")
        # send signal to pnnl box relay - circuit closed
        self.relay_on("pnnl_relay")
        return "PNNL_RELAY_ON"

    def pnnl_off(self):
        self.device_stats["pnnl_box"] = "0"
        if self.device != "zipbox":
            self.debug_logger("[ERROR] Tried to execute 'pnnl_off' from outside zippermast box. Attempting to forward.")
            return self.forward_command(self.device_ip["zipbox"], self.server_port, "pof")
        thor_commands.send_thor_command(self.device_ip["pnnl"], self.pnnl_port, "SHUTDOWN")
        time.sleep(10)
        if self.device_stats["camera"] != "0":
            self.cam_off()
        # send signal to pnnl box relay - circuit open
        self.relay_off("pnnl_relay")
        return "PNNL_RELAY_OFF"

    def pnnl_power_cycle(self):
        stat = self.pnnl_off()
        time.sleep(1)
        stat = self.pnnl_on()
        return "PNNL_POWER_CYCLED"

    def video_on(self):
        if self.device != "zipbox":
            self.debug_logger("[ERROR] Tried to execute 'video_on' from outside zippermast box. Attempting to forward.")
            return self.forward_command(self.device_ip["zipbox"], self.server_port, "von")
        zippermast.mast_up(self)
        self.cam_on()

    def video_off(self):
        if self.device != "zipbox":
            self.debug_logger("[ERROR] Tried to execute 'video_off' from outside zippermast box. Attempting to forward.")
            return self.forward_command(self.device_ip["zipbox"], self.server_port, "vof")
        self.cam_off()
        zippermast.mast_down(self)

    def status(self):
        # check system status:
        if self.device == "zipbox":
            t = threading.Thread(target=self.status_zip_batt)
            t.start()
        else:
            t = threading.Thread(target=self.status_master_batt)
            t.start()

        iridium_online = self.device_stats["iridium"]

        # is comms relay on; ping comms box
        comm_relay = self.relay_stats["comms_relay"]
        comm_online = "0"

        # is zippermast up
        mast_online = "0"
        # is zippermast relay on;
        zipmast_relay = self.relay_stats["zipmast_relay"]
        zip_mast = "{}".format(int(self.device_stats["mast"]) - self.mast_min)
        # are zippermast box batteries charging
        zip_charge_relay = self.relay_stats["zipbatt_relay"]

        # is zippermast box relay on; ping zippermast box
        zipbox_relay = self.relay_stats["zipbox_relay"]
        zipbox_online = "0"

        # is pnnl relay on; ping pnnl box
        pnnl_relay = self.relay_stats["pnnl_relay"]
        pnnl_online = "0"

        camera_relay = self.relay_stats["camera_relay"]
        camera_online = "0"

        # get latest temperature
        if self.device == "master":
            self.device_stats["m_temp"] = self.check_temp()
            if utilities.host_online(self.device_ip["zipbox"]):
                zipbox_online = "1"
                self.relay_stats["zipbox_relay"] = "1"
                zipbox_relay = self.relay_stats["zipbox_relay"]
            if utilities.host_online(self.device_ip["comm"]):
                comm_online = "1"
                self.relay_stats["comm_relay"] = "1"
                zipbox_relay = self.relay_stats["zipbox_relay"]
        else:
            self.device_stats["z_temp"] = self.check_temp()
            if utilities.host_online(self.device_ip["pnnl"]):
                pnnl_online = "1"
                self.relay_stats["pnnl_relay"] = "1"
                pnnl_relay = self.relay_stats["pnnl_relay"]
            if zippermast.zippermast_on(self, self.physical_ports["zm_port"]):
                mast_online = "1"

        m_temp = self.device_stats["m_temp"]
        z_temp = self.device_stats["z_temp"]

        # master battery charge
        master_batt = self.batt_stats["master"]["%"]

        # zipbox battery charge
        zipbox_batt = self.batt_stats["zipbox"]["%"]

        if self.device == "master" and zipbox_online == "1":
            self.debug_logger("[STATUS] Attempting forward of stat to zipbox")
            onl = self.forward_command(self.device_ip["zipbox"], self.server_port, "stat")
            self.debug_logger("[STATUS] Received response of {}".format(onl))
            if onl is not "ERROR_NO_RESPONSE" and onl is not "ERROR_TARGET_OFFLINE":
                onl_dict = json.loads(onl)
                pnnl_relay = onl_dict["relay"]["pnl"]
                pnnl_online = onl_dict["hosts"]["pnnl"]
                self.relay_stats["pnnl_relay"] = pnnl_relay
                self.device_stats["pnnl_box"] = pnnl_online
                zipmast_relay = onl_dict["relay"]["zmt"]
                self.relay_stats["zipmast_relay"] = zipmast_relay
                mast_online = onl_dict["dev"]["mast"]
                zip_mast = onl_dict["mast"]
                self.device_stats["mast"] = "{}".format(int(zip_mast) + self.mast_min)
                zipbox_batt = onl_dict["batt"]["zchr"]
                z_temp = onl_dict["temp"]["z_temp"]
                self.device_stats["z_temp"] = z_temp
                camera_relay = onl_dict["relay"]["cam"]
                camera_online = onl_dict["dev"]["cam"]
                self.relay_stats["camera_relay"] = camera_relay

        status = {"hosts": {"zip": zipbox_online, "pnnl": pnnl_online, "comm": comm_online},
                  "dev": {"irr": iridium_online, "cam": camera_online, "mast": mast_online},
                  "relay": {"zpb": zipbox_relay, "zpc": zip_charge_relay, "com": comm_relay,
                            "zmt": zipmast_relay, "pnl": pnnl_relay, "cam": camera_relay},
                  "batt": {"mchr": master_batt, "zchr": zipbox_batt},
                  "temp": {"m_temp": m_temp, "z_temp": z_temp},
                  "mast": zip_mast}

        response = json.dumps(status)
        print response
        jsond = json.loads(response)
        pprint.pprint(jsond)
        return response

    def status_master_batt(self):
        if self.device != "master":
            self.debug_logger(
                "[ERROR] Tried to execute 'check_master_batt' from outside master box. Attempting to forward.")
            return self.forward_command(self.device_ip["master"], self.server_port, "smb")
        # serial into battery controller - get current charge level
        stat = self.check_batt("master")
        return stat

    def status_zip_batt(self):
        if self.device != "zipbox":
            self.debug_logger(
                "[ERROR] Tried to execute 'check_zip_batt' from outside zippermast box. Attempting to forward.")
            return self.forward_command(self.device_ip["zipbox"], self.server_port, "szb")
        # serial into battery controller - get current charge level
        stat = self.check_batt("zipbox")
        return stat

    def check_batt(self, device_designator):
        battery_status = ibps_state.batt_log(None)
        self.debug_logger("[STATUS] Returned from IBPS with: {}".format(battery_status))
        if ("READ TIMEOUT" in battery_status) or ("FAILED CONNECTION" in battery_status) or (battery_status == ""):
            for each in self.batt_stats[device_designator]:
                self.batt_stats[device_designator][each] = "UNK"
        else:
            self.batt_stats[device_designator] = json.loads(battery_status)
        return battery_status

    def check_temp(self):
        try:
            return self.hardware.get_temp()
        except:
            print "FAILED TO GET TEMP"

    def charge_zip_batt(self):
        if self.device != "master":
            self.debug_logger(
                "[ERROR] Tried to execute 'charge_zip_batt' from outside master box. Attempting to forward.")
            return self.forward_command(self.device_ip["master"], self.server_port, "czb")
        # send signal to zip box charge relay - circuit closed
        self.relay_on("zipbatt_relay")
        return "ZIP_BATT_CHARGING"

    def stop_charge_zip_batt(self):
        if self.device != "master":
            self.debug_logger("[ERROR] Tried to execute 'stop_charge_zip_batt' from outside master box. Attempting to forward.")
            return self.forward_command(self.device_ip["master"], self.server_port, "sczb")
        # send signal to zip box charge relay - circuit closed
        self.relay_off("zipbatt_relay")
        return "ZIP_BATT_STOPPED"

    def set_mast_height(self, target_height):
        if self.device != "zipbox":
            self.debug_logger("[ERROR] Tried to execute 'set_mast_height' from outside zippermast box. Attempting to forward.")
            stat = self.forward_command(self.device_ip["zipbox"], self.server_port, "smh:{}".format(target_height))
        else:
            stat = zippermast.set_mast_height(self, target_height)
        self.device_stats["mast"] = str(int(stat) + zippermast.MAST_MIN)
        return stat

    def relay_on(self, relay):
        relay_num = self.relay_nums[relay]
        resp = power_relay_controller.relay_on(self.physical_ports['prc_port'], relay_num)
        if resp == 1:
            self.debug_logger("[STATUS] Successfully turned on {}".format(relay))
            self.relay_stats[relay] = "1"
        else:
            self.debug_logger("[ERROR] Failed to turn on {}".format(relay))
        return

    def relay_off(self, relay):
        relay_num = self.relay_nums[relay]
        resp = power_relay_controller.relay_off(self.physical_ports['prc_port'], relay_num)
        if resp == 0:
            self.debug_logger("[STATUS] Successfully turned off {}".format(relay))
            self.relay_stats[relay] = "0"
        else:
            self.debug_logger("[ERROR] Failed to turn off {}".format(relay))
        return

    def debug_logger(self, msg):
        if msg is None:
            self.debug_logger("NULL MSG")
        else:
            try:
                fd = open(self.debug_log, 'a')
                new_msg = time.ctime() + '\t:\t' + str(msg) + '\n'
                print new_msg
                fd.write(new_msg)
                fd.flush()
                fd.close()
            except:
                print "[CRITICAL FAILURE] Log failure"
        return

    def forward_command(self, ip, port, command):
        self.debug_logger("[STATUS] Attempting to forward command '{}' to {}:{}".format(command, ip, port))
        response = "ERROR_NO_RESPONSE"
        if not utilities.host_online(ip):
            response = "ERROR_TARGET_OFFLINE"
            return response
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect((ip, port))
        except:
            response = "ERROR_TARGET_OFFLINE"
            return response
        try:
            sock.sendall(command)
            response = sock.recv(1024)
            self.debug_logger("Received: {}".format(response))
        except:
            self.debug_logger("[ERROR] Forwarding could not complete. Target likely offline.")
        finally:
            sock.close()
        return response

if __name__ == '__main__':
    cmdr = Commander("master", "", "", "", "")
    config = {"network": cmdr.device_ip, "ports": {"rv": cmdr.server_port, "thor": cmdr.pnnl_port}, "relays": cmdr.relay_nums}
    json_str = json.dumps(config, indent=2)
    with open('config.json', 'w') as f:
        f.write(json_str)
        # pprint.pprint(config, f)
    with open('config.json', 'r') as f:
        json_str2 = f.read()
    print json_str
    config_2 = json.loads(json_str2)
    print config == config_2

