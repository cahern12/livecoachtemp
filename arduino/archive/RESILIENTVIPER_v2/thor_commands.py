__author__ = 'thomas.ballas'
import socket
import utilities


def send_thor_command(ip, port, command):
    try:
        cl_soc = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        cl_soc.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        cl_soc.connect((ip, port))
        cl_soc.send(command)
        resp = cl_soc.recv(1024)
        utilities.logger("thor_commands", resp)
        cl_soc.close()
        return 0
    except:
        return 1


