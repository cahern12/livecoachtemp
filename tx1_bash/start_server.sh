#!/bin/bash

### BEGIN INIT INFO
# Provides: start_server
# Required-Start: $local_fs $syslog $remote_fs dbus
# Required-Stop:  $local_fs $syslog $remote_fs
# Default-Start:  2 3 4 5
# Default-Stop:   0 1 6
# Short-Description: start_server
### END INIT INFO

cd /home/ubuntu/Desktop/Thrud/node_js_server_new/
sudo node ./server.js | logger
