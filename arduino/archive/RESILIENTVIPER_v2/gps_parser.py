import serial
import json


class GPS_reader:

    def __init__(self):
        self.config = "config.json"
        try:
            with open(self.config, 'r') as f:
                json_str = f.read()
            config_dict = json.loads(json_str)
            gps = config_dict.get('gps', {})
            self.location = gps.get('port', '/dev/gps')
            self.baudrate = gps.get('baudrate', 9600)
            self.timeout = gps.get('timeout', 10)
            self.rtscts = bool(gps.get('rtscts', 0))
            self.dsrdtr = bool(gps.get('dsrdtr', 0))
            self.TCP_SERVER_PORT = config_dict.get('ports', {}).get('thor', 5432)
        except:
            print "[ERROR] CONFIG FILE MISSING - USING DEFAULT VALUES"
            self.location = '/dev/gps'
            self.baudrate = 19200
            self.rtscts = True
            self.dsrdtr = False
            self.timeout = 60
            self.TCP_SERVER_PORT = 5432
        self.modem = None

    def open(self):
        if not self.modem:
            self.modem = serial.Serial(port=self.location, baudrate=self.baudrate, timeout=self.timeout, rtscts=self.rtscts, dsrdtr=self.dsrdtr)

    def close(self):
        if self.modem:
            self.modem.close()
            self.modem = None

    def get_lat_lng_alt(self):
        gps_dict = {}
        if not self.modem:
            self.open()
        lines = []
        for i in range(10):
            lines.append(self.modem.readline())
        gps_lines = [x for x in lines if "GPGGA" in x]
        if len(gps_lines) > 0:
            line = gps_lines[0]
            line_csv = line.split(',')
            if len(line_csv) > 10:
                lat = line_csv[2].replace('.','')
                n_s = line_csv[3].replace('.','').upper()
                lng = line_csv[4].replace('.','')
                e_w = line_csv[5].replace('.','').upper()
                alt = line_csv[9]
                alt_units = line_csv[10]
                lat = lat[0:2] + '.' + lat[2:]
                lng = lng[0:3] + '.' + lng[3:]
                if n_s == "S":
                    lat = "%.5f" % (float(lat) * (-1.0))
                if e_w == "E":
                    lng = "%.5f" % (float(lng) * (-1.0))
                gps_dict = {"lat": lat, "lng": lng, "alt": alt}
        self.close()
        return gps_dict

    def read_gps(self):
        gps_dict = self.get_lat_lng_alt()
        if not gps_dict:
            print "[ERROR] No data returned from GPS read"
        else:
            print gps_dict
        return gps_dict
