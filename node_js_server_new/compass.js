/*=======================================================================
AUTHOR               : Colin Ahern
CREATE DATE      	 : 09/15/2017
COMPANY              : Zel Technologies
PURPOSE              :
NOTES                :  
 =======================================================================
Change History   	 :  
 
    
=======================================================================
*/
const i2c = require('i2c');
const fs = require('fs');
const sleep = require('sleep');

var i2c_address = 0x19;

var jsonContent = JSON.parse(fs.readFileSync('settings.json'));
var compass_device;
var accelData = new Object();
var magData = new Object();
var headingData = new Object();

var interval = 5000; //5 seconds

module.exports =
{
    init: function()
    {
        compass_device = new i2c(i2c_address, {device: jsonContent.i2c_device});
        console.log("connected to compass/i2c");
        //cycle and read every 5 seconds.
        setInterval(read_all, interval);
    },
    headingData, 
    accelData,
    magData
}

//this function is called every half second to read/update the data objects. 
function read_all()
{
    read_accel();
    read_mag();
    read_heading();
    read_tilt();
}
function read_accel()
{
    readGeneric(0x40, 0);
}
function read_mag()
{
    readGeneric(0x45, 1);
}
function read_heading()
{
    readGeneric(0x50, 2);
}
function read_tilt()  //for temperature
{
    readGeneric(0x55, 3);
}

function readGeneric(cmd, data_controller)
{
    compass_device.writeByte(cmd, function(err) { if(err) console.log(err); } );
    sleep.msleep(2) //sleep for 2 miliseconds. 

    compass_device.read(6, function(err, data)
    {
        if(err)
            console.log('houston, we have a problem :(');
        else
        {
            //set the values of NaN to 0 (null values)
            for(var x = 0; x < data.length; x++)
            {
                if(isNaN(parseInt(data[x])))
                    data[x] = 0;
            }

            if(data_controller == 0) //accel
            {
                accelData.ax = data[0] << 8;
                accelData.ax |= data[1];
                accelData.ay = data[2] << 8;
                accelData.ay |= data[3];
                accelData.az = data[4] << 8;
                accelData.az |= data[5];

                accelData.ax = 'Accel X: ' + accelData.ax;
                accelData.ay = 'Accel Y: ' + accelData.ay;
                accelData.az = 'Accel Z: ' + accelData.az;
                //console.log(accelData.ax, accelData.ay, accelData.az);
            }
            else if(data_controller == 1) //magnetic
            {
                magData.mx = data[0] << 8;
                magData.mx |= data[1];
                magData.my = data[2] << 8;
                magData.my |= data[3];
                magData.mz = data[4] << 8;
                magData.mz |= data[5];

                magData.mx = 'Mag X: ' + magData.mx;
                magData.my = 'Mag Y: ' + magData.my;
                magData.mz = 'Mag Z: ' + magData.mz;
                //console.log(magData.mx, magData.my, magData.mz);
            }
            else if(data_controller == 2) //heading
            {
                headingData.heading = data[0] << 8;
                headingData.heading |= data[1];
                headingData.pitch = data[2] << 8;
                headingData.pitch |= data[3];
                headingData.roll = data[4] << 8;
                headingData.roll |= data[5];

                headingData.heading = 'Heading: ' + headingData.heading/10;
                headingData.pitch = 'Pitch: ' + headingData.pitch/10;
                headingData.roll = 'Roll: ' + headingData.roll/10;
                //console.log(headingData.heading, headingData.pitch, headingData.roll);
            }
            else if(data_controller == 3)  //tilt - only care about temp (celsius) data (we got the pitch and roll above)
            {
                headingData.temp = data[4] << 8;
                headingData.temp |= data[5];

                headingData.temp = 'Temp: ' + headingData.temp/10 + ' C';
                //console.log(headingData.temp);
            }
        }
    });
}
/*

OLD WAY OF DOING IT 
function read_data_and_parse(which_value)
{
    compass_device.read(6, function(err, data)
    {
        if(err)
        {
            console.log("houston, we have a problem");
            console.log(err);
            return;
        }
        else
        {
            for(var x = 0; x < data.length; x++)
            {
                if(isNaN(parseInt(data[x])))
                    data[x] = 0;
            }

            if(which_value == 0)
            {
                accelData.ax = parseInt(data[0]) * 256 + parseInt(data[1]);
                accelData.ay = parseInt(data[2]) * 256 + parseInt(data[3]);
                accelData.az = parseInt(data[4]) * 256 + parseInt(data[5]);
                console.log("Accel Data: " + accelData.ax + ", " + accelData.ay + ", " + accelData.az);
            }
            else if(which_value == 1)
            {
                magData.mx = parseInt(data[0]) * 256 + parseInt(data[1]);
                magData.my = parseInt(data[2]) * 256 + parseInt(data[3]);
                magData.mz = parseInt(data[4]) * 256 + parseInt(data[5]);
                console.log("Mag Data: " + magData.mx + ", " + magData.my + ", " + magData.mz);
            }
            else if(which_value == 2)   ////////DO NOT CONVERT TO RADIANS!!!!!!!!!!!!
            {
                headingData.heading = (parseInt(data[0]) * 256 + parseInt(data[1]))/10 * Math.PI / 180;
                headingData.pitch = (parseInt(data[2]) * 256 + parseInt(data[3]))/10 * Math.PI / 180;
                headingData.roll = (parseInt(data[4]) * 256 + parseInt(data[5]))/10 * Math.PI / 180;
                console.log("Heading: " + headingData.heading + ", " + headingData.pitch + ", " + headingData.roll);
            }
            console.log("----------");
        }
    });
}*/