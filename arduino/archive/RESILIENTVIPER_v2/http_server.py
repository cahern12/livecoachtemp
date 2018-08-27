# __author__ = 'thomas.ballas'

import time
import os
import socket
import threading
import SocketServer
import command_list
import ir_modem
from devices.raspberry import Raspberry


class ThreadedTCPRequestHandler(SocketServer.BaseRequestHandler):
    # def __init__(self, request, client_address, server):
    #     SocketServer.BaseRequestHandler.__init__(self, request, client_address, server)

    def handle(self):
        req_data = self.request.recv(1024)
        command_pieces = req_data.split(":")
        command = command_pieces[0].strip()
        print "The command was: {}.".format(command)
        if command in self.server.commander.commands:
            if len(command_pieces) > 1:
                data = self.server.commander.commands[command](command_pieces[1].strip())
            else:
                data = self.server.commander.commands[command]()
            if data is None:
                data = "ERROR-WRONGTARGET"
        else:
            data = "ERROR-BADCOMMAND"
        # cur_thread = threading.current_thread()
        response = "{}".format(data)
        self.request.sendall(response)


class ThreadedTCPServer(SocketServer.ThreadingMixIn, SocketServer.TCPServer):
    pass


def client(ip_addr, port_num, message):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((ip_addr, port_num))
    try:
        sock.sendall(message)
        response = sock.recv(1024)
        print "Received: {}".format(response)
    finally:
        sock.close()


if __name__ == "__main__":
    exec_dir = os.path.dirname(os.path.abspath(__file__))
    os.chdir(exec_dir)
    # Port 0 means to select an arbitrary unused port
    HOST, PORT = "0.0.0.0", 8080

    server = ThreadedTCPServer((HOST, PORT), ThreadedTCPRequestHandler)
    ip, port = server.server_address
    device = "master"
    debugging = True
    ports = dict(prc="", iridium="", ibps="", gps="", zippermast="")
    if Raspberry.check_peripheral("prc"):
        ports["prc"] = "/dev/prc"
    if Raspberry.check_peripheral("ibps"):
        ports["ibps"] = "/dev/ibps"
    if device == "master":
        if Raspberry.check_peripheral("iridium"):
            ports["iridium"] = "/dev/iridium"
        server.commander = command_list.Commander(device, ports["prc"], ports["iridium"], ports["ibps"], ports["gps"])
    else:
        if Raspberry.check_peripheral("gps"):
            ports["gps"] = "/dev/gps"
        if Raspberry.check_peripheral("zippermast"):
            ports["zippermast"] = "/dev/zippermast"
        server.commander = command_list.Commander(device, ports["prc"], ports["zippermast"], ports["ibps"], ports["gps"])


    # Start a thread with the server -- that thread will then start one
    # more thread for each request
    server_thread = threading.Thread(target=server.serve_forever)
    # Exit the server thread when the main thread terminates
    server_thread.daemon = True
    server_thread.start()
    print "Server loop running in thread:{}, with ip:{}, and port:{}".format(server_thread.name, ip, port)

    # client(ip, port, "onl")

    # server.shutdown()
    # server.server_close()

    while True:
        time.sleep(60)
        mins = int(time.strftime("%M"))
        print "{} - Main server log heartbeat".format(time.ctime())
        if server.commander.ir_thread:
            if not server.commander.ir_thread.isAlive():
                print "{} - Iridium modem thread has died. Attempting restart.".format(time.ctime())
                server.commander.ir_thread.run()

        if device == "master" and not debugging:
            if server.commander.check_timeout() <= 0:
                server.commander.shutdown()
        # if device == "master" and server.commander.device_stats["comm_box"] == "0" and (mins % 15) < 5:
        #     server.commander.comms_on()
        # if (mins % 15) == 5:
        #     stat = server.commander.status()
        # if device == "master" and server.commander.device_stats["comm_box"] != "0" \
        #         and server.commander.device_stats["camera"] == "0" and server.commander.device_stats["pnnl_box"] == "0" \
        #         and server.commander.device_stats["zipbox"] == "0" and (mins % 15) == 5:
        #     server.commander.comms_off()
        #     stat = server.commander.status()
