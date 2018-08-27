import serial
import socket
import time
import json


class Ir_class:
    def __init__(self):
        self.config = "config.json"
        try:
            with open(self.config, 'r') as f:
                json_str = f.read()
            config_dict = json.loads(json_str)
            ir_dict = config_dict.get('iridium', {})
            self.location = ir_dict.get('port', '/dev/iridium')
            self.baudrate = ir_dict.get('baudrate', 19200)
            self.timeout = ir_dict.get('timeout', 60)
            self.rtscts = bool(ir_dict.get('rtscts', 1))
            self.dsrdtr = bool(ir_dict.get('dsrdtr', 0))
            self.TCP_SERVER_PORT = config_dict.get('ports', {}).get('rv', 8080)
        except:
            print "[ERROR] CONFIG FILE MISSING - USING DEFAULT VALUES"
            self.location = '/dev/iridium'
            self.baudrate = 19200
            self.rtscts = True
            self.dsrdtr = False
            self.timeout = 60
            self.TCP_SERVER_PORT = 8080

        self.rxUpdateRate = 60
        self.TCP_SERVER_IP = "127.0.0.1"

        self.modem = serial.Serial(baudrate=self.baudrate,
                                   timeout=self.timeout,
                                   rtscts=self.rtscts,
                                   dsrdtr=self.dsrdtr)
        # separated to prevent automatic opening of port
        self.modem.port = self.location

        self.rxKillFlag = False
        self.txKillFlag = False

    ### Hotel Functions #######################################################

    def close(self):
        self.modem.close()

    def open(self):
        self.modem.open()

    def set_baudRate(self, baudRate):
        self.baudrate = baudRate
        self.modem.close()
        self.modem.open()

    def set_location(self, location):
        self.location = location
        self.modem.close()
        self.modem.open()

    def set_rxUpdateRate(self, rate):
        self.rxUpdateRate = rate
        self.modem.close()
        self.modem.open()

    @staticmethod
    def UdpPushMessage(mesg, port, to_addr):
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
        serv_addy = (to_addr, port)
        sock.sendto(mesg, serv_addy)
        sock.close()

    def CheckSat(self):
        print "{} - In CheckSat".format(time.ctime())
        if not self.modem.isOpen():
            print "{} - Modem was discovered closed.".format(time.ctime())
            self.modem.open()
        print "{} - Writing AT+SBDI".format(time.ctime())
        self.modem.write('AT+SBDI\r')
        time.sleep(10)
        values = ""
        print "{} - Null readline".format(time.ctime())
        values_l = []
        values_l.append(self.modem.readline())
        print "{} - Real readline".format(time.ctime())
        values_l.append(self.modem.readline())
        values_l.append(self.modem.readline())
        values_l.append(self.modem.readline())
        print "Values = {}".format(values_l)
        vals = list(x for x in values_l if "SBDI:" in x)
        if len(vals) > 0:
            values = vals[0].split(':')[1].split(',')
        print "Values = {}".format(values)
        self.close()
        return values

    ### Reciever Funcitons ####################################################

    def ReadBuffer(self):
        self.open()
        self.modem.write('AT+SBDRT\r')
        self.modem.readline()
        self.modem.readline()

        message = self.modem.readline()
        message = message.strip()

        self.modem.readline()

        self.close()
        print message
        return message

    @staticmethod
    def execute_command(ip, port, message):
        response = "NO_RESPONSE"
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect((ip, port))
        try:
            sock.sendall(message)
            response = sock.recv(1024)
            print "Received: {}".format(response)
        finally:
            sock.close()
        return response

    def ClearReadBuffer(self):
        self.open()
        self.modem.write('AT+SBDD1\r')
        self.modem.readline()
        self.modem.readline()
        self.modem.readline()
        self.modem.readline()
        self.close()

    def Receiver(self):
        print "{} - In IR receiver".format(time.ctime())
        if self.rxKillFlag:
            print 'Kill Flag Active'
            return

        while not self.rxKillFlag:
            print "{} - Restarted IR reciever loop".format(time.ctime())
            time.sleep(self.rxUpdateRate)
            SatCheck = self.CheckSat()
            print "{} - Completed SatCheck".format(time.ctime())

            if type(SatCheck) is str:
                print "{} - Received nothing from SatCheck".format(time.ctime())
                continue

            if int(SatCheck[2]) == 0:
                print 'No Message Available' + str(time.asctime())

            elif int(SatCheck[2]) == 1:
                print 'Message Available' + str(time.asctime())
                message = self.ReadBuffer()
                print message
                if message != "":
                    sys_response = self.execute_command(self.TCP_SERVER_IP, self.TCP_SERVER_PORT, message)
                    print sys_response

                    mesg = sys_response.replace(" ", "")
                    mesgs = []
                    if len(mesg) > 120:
                        segs = int(len(mesg) / 110) + 1
                        for i in range(segs):
                            mesg_seg = mesg[110 * i:110 * (i + 1)]
                            mesgs.append(mesg_seg)
                            print "Created message segment #{} containing:{}".format(i, mesg_seg)
                    else:
                        mesgs = [mesg]

                    for msg in mesgs:
                        self.Transmitter(msg)

                self.ClearReadBuffer()

            elif int(SatCheck[2]) == 2:
                print '{} Error - Not able to Reach Sat'.format(time.asctime())

                ### Transmitter Funcitons #################################################


    def WriteToBuffer(self, mesg):
        self.open()
        mesg = mesg.replace(" ", "").strip()
        in_mesg = 'AT+SBDWT=' + str(mesg) + '\r'
        byts = self.modem.write(in_mesg)
        self.modem.readline()
        self.modem.readline()
        print "{} - Wrote {} bytes containing the message: {}".format(time.ctime(), byts, in_mesg)
        self.close()

    def ClearWriteBuffer(self):
        self.open()
        self.modem.write('AT+SBDD0\r')
        self.modem.readline()
        self.modem.readline()
        self.modem.readline()
        self.modem.readline()
        self.close()

    def Transmitter(self, mesg):
        print """{} - Attempting to transmit '{}'""".format(time.ctime(), mesg)
        self.ClearWriteBuffer()
        self.WriteToBuffer(mesg)

        if self.txKillFlag:
            print 'Tx Kill Flag Active'
            return

        print 'tx starting'
        while not self.txKillFlag:
            time.sleep(1)
            SatCheck = self.CheckSat()
            if type(SatCheck) is not list:
                print "{} - CheckSat returned Null when trying to transmit".format(time.ctime())
                time.sleep(10)
                continue
            if int(SatCheck[0]) == 0:
                print('{} Error - No message to transmit?'.format(time.ctime()))
                self.ClearWriteBuffer()
                break
            if int(SatCheck[0]) == 1:
                print('Message Delivered  ' + str(time.asctime()))
                self.ClearWriteBuffer()
                break
            if int(SatCheck[0]) == 2:
                print('Error Occured' + str(time.asctime()))
                time.sleep(10)

    def iridium_modem_on(self):
        print ("[STATUS] checking for iridium on port '{}'".format(self.location))
        try:
            self.open()

            resp = self.modem.write("AT\r")
            print "[STATUS] Wrote {} bytes.".format(resp)
            # time.sleep(1)
            response = "NULL"
            check = ""
            t = time.time()
            while response is not "" or time.time() < (t + 5):
                isopen = self.modem.isOpen()
                bytes_in_queue = self.modem.inWaiting()
                if isopen and bytes_in_queue == 0:
                    print "[STATUS] Modem is open: {} and {} bytes in waiting.".format(isopen, bytes_in_queue)
                    self.modem.write("AT\r")
                else:
                    print "[STATUS] {} bytes in waiting.".format(bytes_in_queue)
                response = self.modem.read(bytes_in_queue)
                check += response
                print ("[STATUS] current value of response is '{}'".format(response))
                if "Battery" in response:
                    break
                if "OK" in response:
                    break
            self.modem.flushOutput()
            self.modem.flushInput()
            self.modem.close()
            if "OK" in check:
                return True
        except:
            print "[ERROR] Failed to access Iridium modem"
        return False



if __name__ == "__main__":
    Ir_class().Receiver()
