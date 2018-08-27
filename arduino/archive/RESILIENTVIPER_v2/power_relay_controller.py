import serial
# import time
import utilities


class PowerRelayController:
    def __init__(self, serial_port='/dev/prc', baud_rate=115200):
        self.serial_port = serial_port
        self.baud_rate = baud_rate
        self.modem = None

    def open_modem(self):
        self.modem = serial.Serial(self.serial_port, self.baud_rate)
        PowerRelayController.logger('Modem on')

    def close_modem(self):
        if self.modem is None:
            PowerRelayController.logger('No modem exists')
            return
        self.modem.close()
        PowerRelayController.logger('Modem off')

    def relay_off(self, relay_number):
        if self.modem is None:
            PowerRelayController.logger('No modem exists')
            return

        if not self.modem.isOpen():
            PowerRelayController.logger('Modem is closed')
            return

        if type(relay_number) == int:
            pass
        else:
            PowerRelayController.logger('Enter Number for relay')
            return

        if relay_number == 1:
            relay_string = '\xFE\x00'
        elif relay_number == 2:
            relay_string = '\xFE\x01'
        elif relay_number == 3:
            relay_string = '\xFE\x02'
        elif relay_number == 4:
            relay_string = '\xFE\x03'
        else:
            PowerRelayController.logger('Enter Number for relay')
            return

        self.modem.write(relay_string)

    def relay_on(self, relay_number):
        if self.modem is None:
            PowerRelayController.logger('No modem exists')
            return

        if not self.modem.isOpen():
            PowerRelayController.logger('Modem is closed')
            return

        if type(relay_number) == int:
            pass
        else:
            PowerRelayController.logger('Enter Number for relay')
            return

        if relay_number == 1:
            relay_string = '\xFE\x08'
        elif relay_number == 2:
            relay_string = '\xFE\x09'
        elif relay_number == 3:
            relay_string = '\xFE\x0a'
        elif relay_number == 4:
            relay_string = '\xFE\x0b'
        else:
            PowerRelayController.logger('Enter Number for relay')
            return

        self.modem.write(relay_string)

    @staticmethod
    def logger(msg):
        utilities.logger("prc", msg)


def relay_on(prc_port, relay_num):
    try:
        prc = PowerRelayController(serial_port=prc_port)
        prc.open_modem()
        prc.relay_on(relay_num)
        prc.close_modem()
        PowerRelayController.logger("[STATUS] Successfully turned on relay {}".format(relay_num))
        return 1
    except:
        PowerRelayController.logger("[ERROR] Failed to turn on relay {}".format(relay_num))
        return 0


def relay_off(prc_port, relay_num):
    try:
        prc = PowerRelayController(serial_port=prc_port)
        prc.open_modem()
        prc.relay_off(relay_num)
        prc.close_modem()
        PowerRelayController.logger("[STATUS] Successfully turned off {}".format(relay_num))
        return 0
    except:
        PowerRelayController.logger("[ERROR] Failed to turn off {}".format(relay_num))
        return 1


# if __name__ == '__main__':
#    prc=PowerRelayController()
#    prc.open_modem()
#    prc.relay1_on()
#    prc.relay_on(1)
#    time.sleep(2)
#    prc.relay1_off()
#    prc.close_modem()
