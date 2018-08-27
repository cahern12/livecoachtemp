from serial import Serial
import sys
import time


class Servo(object):
    ''' Python class to interface with Pololu servo controller.'''

    def __init__(self, serial_port='/dev/ttyAMA0', baud_rate=9600):
        self.serial_port = serial_port
        self.baud_rate = baud_rate
        self.modem = None

    def open_servo(self):
        self.modem = Serial(self.serial_port, self.baud_rate)
        time.sleep(0.1)

    def close_servo(self):
        time.sleep(0.1)
        self.modem.close()
        self.modem = None

    def setpos(self, angle):
        n = 0
        if angle > 180 or angle < 0:  # check if input angle is in correct range
            angle = 90

        if angle > 90:  # addressing the servo in the upper byte gives
            # angles between 90 and 180. see manual (Mini SSC II mode)
            angle -= 90
            n += 8
        msg = chr(0xFF) + chr(n) + chr(254 * angle / 180)
        self.modem.write(msg)

    def open_lid(self):
        self.open_servo()
        msg = chr(0xFF) + chr(0x08) + chr(254 * 150 / 180)
        self.modem.write(msg)
        time.sleep(0.2)
        self.modem.write(msg)
        self.close_servo()

    def close_lid(self):
        self.open_servo()
        msg = chr(0xFF) + chr(0) + chr(15)
        self.modem.write(msg)
        time.sleep(0.2)
        self.modem.write(msg)
        self.close_servo()

# if __name__ == '__main__':
#
