/*
=======================================================================
AUTHOR               : Colin Ahern
CREATE DATE      	 : 09/15/2017
COMPANY              : Zel Technologies
PURPOSE              :
NOTES                :  
                buffer values: 
                00 ->
                01 ->
                02 ->
                03 -> Mode
                04 -> Focus
                05 -> Thermal stuff (We dont care about it)
                06 -> Zoom
                07 -> Pitch???
                08 -> EO/IR
                09 -> Roll
                10 -> FOV
                11 -> 
                12 -> horizontal change
                13 -> horizontal change
                14 -> vertical change
                15 -> vertical change
                16 -> Vertical Rate 
                17 -> Horizontal Rate
                18 -> 
                19 -> Checksum -> ALWAYS RECALCULATE EVERYTIME BUFFER CHANGES
 =======================================================================
Change History   	:  
 
    
=======================================================================
*/
const SerialPort = require('serialport');
const fs = require('fs');
var jsonContent = JSON.parse(fs.readFileSync("settings.json"));
var buf = new Buffer([0xB0, 0x3B, 0x4F, 0x6C, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0xAA, 
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00], "hex");   //initial message holding at 0 position
calc_checksum();        //always calc_checksum!

var zoom_counter = 0;
var focus_counter = 0;
var is_zoom_change = false;
var is_focus_change = false;
var is_reset_zoom = false;

//eo, ir
var eo_ir_buf = new Buffer([0x00, 0x80], "hex");
//no change, zoom out, zoom in, fov controlled
var zoom_cmd_buf = new Buffer([0x00, 0x01, 0x02, 0x03], "hex");
//no change, minus, plus, auto/infinity
var focus_buf = new Buffer([0x00, 0x01, 0x02, 0x04], "hex");

//These are the functions that can get called from the calling file (server.js)
module.exports =
{
    init : function()
    {
        start_serial();         //kick off the serial stuff.
    },
    sub_focus : function()
    {
        console.log('sub focus');
        buf[4] = focus_buf[1];
        calc_checksum();
        is_focus_change = true;
    },
    add_focus : function()
    {
        console.log('add focus');
        buf[4] = focus_buf[2];
        calc_checksum();
        is_focus_change = true;
    },
    focus_auto : function()
    {
        console.log('auto focus clicked');
        buf[4] = focus_buf[3];
        calc_checksum();
    },
    set_eo : function()
    {
        console.log('set to eo');
        buf[8] = eo_ir_buf[0]; //setting it to 0x00
        calc_checksum();
    },
    set_ir : function()
    {
        console.log('set to ir');
        buf[8] = eo_ir_buf[1]; //setting it to 0x80
        calc_checksum();
    },
    add_zoom : function()
    {
        console.log('adding zoom');
        buf[6] = zoom_cmd_buf[2];
        calc_checksum();
        is_zoom_change = true;
    },
    sub_zoom : function()
    {
        console.log('sub zoom');
        buf[6] = zoom_cmd_buf[1];
        calc_checksum();
        is_zoom_change = true;
    },
    reset_zoom : function()
    {
        console.log('reset zoom');
        buf[6] = zoom_cmd_buf[1];
        calc_checksum();
        is_reset_zoom = true
    },
    //setting camera back to original
    reset_slew : function()
    {
        console.log('reset slew');
        buf[12] = 0x00;  
        buf[13] = 0x00;         
        buf[14] = 0x00;
        buf[15] = 0x00;    
        calc_checksum();
    },
    //the actual position(s) are generated from right_div.js
    vertical_position : function(amount)  
    {
        console.log('el changed to: ' + amount);
        current_vertical_position = amount;
        vertical_position_camera(current_vertical_position);
        calc_checksum();
    },
    horizontal_position : function(amount)
    {
        console.log('az changed to: ' + amount);
        current_horizontal_position = amount;
        horizontal_position_camera(current_horizontal_position);
        calc_checksum();
    },
}

function start_serial()
{
    const camera_serial = new SerialPort( jsonContent.camera_port, { baudRate: parseInt(jsonContent.camera_baud) }); 
    const parsers = SerialPort.parsers;
    const parser = new parsers.Readline( { delimiter: "\r\n" } );
    camera_serial.on('open', function(){
        //console.log('camera serial connected');
    });
    camera_serial.pipe(parser);

    parser.on('data', function(received_data)
    {
        console.log(received_data);
    });
    
    setInterval(function()
    {
        camera_serial.write(buf);

        if(is_zoom_change)
            zoom_focus_helper(10, 0); //do the current command roughly 10 times (~ 1 second) then stop
        else if(is_reset_zoom)
            zoom_focus_helper(50, 0); //do the current command roughly 50 times (~ 5 second) then stop
        else if(is_focus_change)
            zoom_focus_helper(10, 1); //do the current command roughly 10 times (~ 1 second) then stop     
    }, 100 );   //called every 100ms
}

function zoom_focus_helper(num_of_times, what_to_update)
{
    zoom_counter++;
    if(zoom_counter == num_of_times)
    {
        zoom_counter = 0;
        is_zoom_change = false;
        is_reset_zoom = false;
        is_focus_change = false;
        if(what_to_update == 0)             //0 is zoom stuff
            buf[6] = zoom_cmd_buf[0];
        else if(what_to_update == 1)        //1 is focus. 
            buf[4] = focus_buf[0];
        calc_checksum();
    }
}

//used to calculate the new checksum when the buffer changes. 
function calc_checksum()
{
    var check = 0;
    for(var x = 0; x < buf.length -1; x++) 
        check += buf[x];
    buf[19] = check;
}

//used to determine the vertical position to write to camera. 
//the camera can go higher vertically but doubt we need to go that high
function vertical_position_camera(value)
{
    if(value < 0 && value > -22)
    {
        buf[14] = (value - 0) * 10;
        buf[15] = -1;
    }
    else if(value <= -22 && value > -44)
    {
        buf[14] = (value - 22) * 10;
        buf[15] = -2;
    }
    else if(value >= 0 && value < 22)
    {
        buf[14] = (value - 0) * 10;
        buf[15] = 0;
    }
    else if(value >= 22 && value < 44)
    {
        buf[14] = (value - 22) * 10;
        buf[15] = 1;
    }
}

//used to determine the horizontal position to write to camera. 
function horizontal_position_camera(value)
{
    if(value >= 0 && value < 22)
    {
        buf[13] = 0;
        buf[12] = (value - 0) * 10;
    }
    else if(value >= 22 && value < 44)
    {
        buf[13] = 1;
        buf[12] = (value - 22) * 10;
    }
    else if(value >= 44 && value < 66)
    {
        buf[13] = 2;
        buf[12] = (value - 44) * 10;
    }
    else if(value >= 66 && value < 90)
    {
        buf[13] = 3;
        buf[12] = (value - 66) * 10;
    }
    else if(value >= 90 && value < 112)
    {
        buf[13] = 4;
        buf[12] = (value - 90) * 10;
    }
    else if(value >= 112 && value < 134)
    {
        buf[13] = 5;
        buf[12] = (value - 112) * 10;
    }
    else if(value >= 134 && value < 156)
    {
        buf[13] = 6;
        buf[12] = (value - 134) * 10;
    }
    else if(value >= 156 && value < 180)
    {
        buf[13] = 7;
        buf[12] = (value - 156) * 10;
    }
    else if(value >= 180 && value < 202)
    {
        buf[13] = 8;
        buf[12] = (value - 180) * 10;
    }
    else if(value >= 202 && value < 224)
    {
        buf[13] = 9;
        buf[12] = (value - 202) * 10;
    }
    else if(value >= 224 && value < 246)
    {
        buf[13] = 10;
        buf[12] = (value - 224) * 10;
    }
    else if(value >= 246 && value < 270)
    {
        buf[13] = 11;
        buf[12] = (value - 246) * 10;
    }
    else if(value >= 270 && value < 292)
    {
        buf[13] = 12;
        buf[12] = (value - 270) * 10;
    }
    else if(value >= 292 && value < 314)
    {
        buf[13] = 13;
        buf[12] = (value - 292) * 10;
    }
    else if(value >= 314 && value < 336)
    {
        buf[13] = 14;
        buf[12] = (value - 314) * 10;
    }
    else if(value >= 336 && value < 359)
    {
        buf[13] = 15;
        buf[12] = (value - 336) * 10;
    }
}