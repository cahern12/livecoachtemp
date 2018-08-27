import time
import json
import serial
import utilities


class IBPS:
    def __init__(self):

        self.config = "config.json"
        try:
            with open(self.config, 'r') as fc:
                json_str = fc.read()
            config_dict = json.loads(json_str)
            ibps_dict = config_dict.get('ibps', {})
            self.serial_port = ibps_dict.get('port', '/dev/ibps')
            self.baud_rate = ibps_dict.get('baudrate', 19200)
            self.rtscts = bool(ibps_dict.get('rtscts', 1))
            self.dsrdtr = bool(ibps_dict.get('dsrdtr', 0))
            self.timeout = ibps_dict.get('timeout', 3)
        except:
            print "[ERROR] CONFIG FILE MISSING - USING DEFAULT VALUES"
            self.serial_port = '/dev/ibps'
            self.baud_rate = 19200
            self.rtscts = True
            self.dsrdtr = False
            self.timeout = 3
        self.modem = None

    def open_modem(self):
        self.modem = serial.Serial(self.serial_port, self.baud_rate, rtscts=self.rtscts, timeout=self.timeout)
        IBPS.logger('serial modem on')
        time.sleep(1)

    def close_modem(self):
        if self.modem is None:
            return
        self.modem.write(" ")
        time.sleep(1)
        self.modem.flushInput()
        self.modem.flushOutput()
        self.modem.close()
        IBPS.logger('Modem off')

    def get_status(self):
        if self.modem is None:
            IBPS.logger("FAILED CONNECTION")
            return "FAILED CONNECTION"
        self.modem.write(' ')
        IBPS.logger("wrote space")
        time.sleep(0.5)
        self.modem.flushInput()
        self.modem.flushOutput()
        IBPS.logger("flushed I/O")
        self.modem.write('b')
        IBPS.logger("wrote b")
        ln = ""
        try:
            ln += self.modem.read()
        except:
            IBPS.logger("READ FAILURE 1")
        IBPS.logger("first read")
        curtime = time.time()
        counter = 0
        while counter < 2:
            try:
                ln += self.modem.read()
            except:
                IBPS.logger("READ FAILURE 2")
            counter = ln.count("Voltage |")
            if time.time() > (curtime + 10):
                IBPS.logger("READ TIMEOUT")
                IBPS.logger(ln)
                return "READ TIMEOUT"
        if "Voltage |" not in ln or "Current |" not in ln \
                or "Charge% |" not in ln or "Cap Ah " not in ln or "Current: " not in ln:
            IBPS.logger("READ TIMEOUT 2")
            IBPS.logger(ln)
            return "READ TIMEOUT 2"
        self.modem.write(' ')
        IBPS.logger(ln)
        data = {}
        if 'Ah' not in data:
            lst = ln.split("\n")
            ln = []
            ln = [x for x in lst if "Voltage |" in x and ":" not in x]
            if ln:
                temp = ln[0].split('|')[-1].split()
                volts = temp[0:]
                data["V"] = volts

            ln = []
            ln = [x for x in lst if "Current |" in x and ":" not in x]
            if ln:
                temp = ln[0].split('|')[-1].split()
                current = temp[0:]
                data["A"] = current

            ln = []
            ln = [x for x in lst if "Charge%" in x]
            if ln:
                temp = ln[0].split('|')[-1].split()
                charge = temp[0:]
                data["%"] = charge

            ln = []
            ln = [x for x in lst if "Ah" in x]
            if ln:
                temp = ln[0].split('|')[-1].split()
                ah = temp[0:]
                data["Ah"] = ah
        self.modem.flushInput()
        self.modem.flushOutput()
        IBPS.logger(data)
        return json.dumps(data)

    @staticmethod
    def logger(msg):
        utilities.logger("ibps", msg)


def batt_log(b=None):
    stat = ""
    counter = 0
    batt_logger = None
    while (((stat is "") or ("READ TIMEOUT" in stat) or ("FAILED CONNECTION" in stat)) and (counter < 5)):
        try:
            IBPS.logger("Beginning battery query")
            batt_logger = IBPS()
            IBPS.logger("Created battery logger")
            batt_logger.open_modem()
            IBPS.logger("Opened battery modem")
            stat = batt_logger.get_status()
            IBPS.logger("Got battery status")
            IBPS.logger(stat)
            batt_logger.close_modem()
            IBPS.logger("Closed modem to battery")
        except:
            try:
                batt_logger.close_modem()
            except:
                IBPS.logger("Failed to close modem")
            IBPS.logger("Failed to query battery")
        counter += 1

    if b is not None:
        try:
            b.write(time.ctime() + ", " + str(time.time()) + ", " + str(stat) + "\n")
            b.flush()
            IBPS.logger("Flushed battery data to file")
        except:
            IBPS.logger("Failed to write battery data to file")
    return stat


if __name__ == "__main__":
    with open("batt_log.txt", "a") as f:
        while True:
            batt_log(b=f)
            time.sleep(30)
