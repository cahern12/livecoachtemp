/*
=======================================================================
AUTHOR               : Colin Ahern
CREATE DATE      	 : 09/15/2017
COMPANY              : Zel Technologies
PURPOSE              :
NOTES                :  
                    This class monitors the avr serial and continuously updates
                    a data variable with the appropriate information. The calling
                    class (server.js) initializes this class to "kick off" the
                    serial read and (server.js)will obtain the latest information
                    roughly every second to send to the client.
 =======================================================================
Change History   	 :  
 
    
=======================================================================
*/
const SerialPort = require('serialport');
const fs = require('fs');
var jsonContent = JSON.parse(fs.readFileSync('settings.json'));
var is_sys_shutdown = false, is_zm_down = false, is_zm_up = false;
var set_camera_on = false, set_camera_off = false;
var data = {
    time: "",
    date: "",
    location: "",
    speed: "",
    altitude: "",
    satellites: "",
    ser_waiting: "",
    battery_mins_to_empty: "",
    battery_avg_percent: ""
}

module.exports =
{
    init : function()
    {
        console.log('init avr serial');
        start_serial();
    },
    shutdown_avr : function()
    {
        is_sys_shutdown = true;
    },
    zm_down : function()
    {
        console.log('zm down');
        is_zm_down = true;
    },
    zm_up : function()
    {
        console.log('zm up');
        is_zm_up = true;
    },
    camera_on : function()
    {
        console.log('camera on');
        set_camera_on = true;
    },
    camera_off : function()
    {
        console.log('camera off');
        set_camera_off = true;
    },
    data
}

function start_serial()
{
    const mc_serial = new SerialPort('/dev/ttyUSB1', { baudRate: 9600 } );
    const parsers = SerialPort.parsers;
    const parser = new parsers.Readline( { delimiter: "\r\n" } );
    mc_serial.on('open', function(){
        console.log('mc serial connected');
    });
    mc_serial.pipe(parser);

    parser.on('data', function(received_data)
    {
        if(received_data.includes("Invalid Data From GPS"))
            set_values_to_null();
        else
            set_received_value(received_data);
    });

    //check to see if flags are set to let avr know.
    setInterval(function()
    {      
        if(is_sys_shutdown)
        {
            mc_serial.write("SHUTDOWN");
            is_sys_shutdown = false;
        }
        else if(is_zm_down)
        {
            mc_serial.write("ZMDOWN");
            is_zm_down = false;
        }
        else if(is_zm_up)
        {
            mc_serial.write("ZMUP");
            is_zm_up = false;
        }
        else if(set_camera_on)
        {
            mc_serial.write("CAMON");
            set_camera_on = false;
        }
        else if(set_camera_off)
        {
            mc_serial.write("CAMOFF");
            set_camera_off = false;
        }
    }, 2000 );   //called every 2 seconds
}

function set_values_to_null()
{
    data.time = "";
    data.date = "";
    data.location = "";
    data.speed = "";
    data.altitude = "";
    data.satellites = "";
    data.ser_waiting = "Invalid Data From AVR";
    data.battery_mins_to_empty = "";
    data.battery_avg_percent = "";
}
function set_received_value(value)
{
    data.ser_waiting = "";

    if(value.includes("Time"))
        data.time = value;
    else if(value.includes("Date"))
        data.date = value;
    else if(value.includes("Location"))
        data.location = value;
    else if(value.includes("Speed"))
        data.speed = value;
    else if(value.includes("Altitude"))
        data.altitude = value;
    else if(value.includes("Satellites"))
        data.satellites = value;
    else if(value.includes("MinsToEmpty"))
        data.battery_mins_to_empty;
    else if(value.inclues("AvgChg"))
        data.battery_avg_percent;
}