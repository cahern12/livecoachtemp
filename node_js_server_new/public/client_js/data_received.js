/*
=======================================================================
AUTHOR               : Colin Ahern
CREATE DATE      	 : 09/15/2017
COMPANY              : Zel Technologies
PURPOSE              :
NOTES                :  
 =======================================================================
Change History   	 :  
 
    
=======================================================================
*/
var socket = io(get_server_ip());

socket.on('serial_data', function(data)
{
    //console.log(data);
    time.innerHTML = data.time;
    date.innerHTML = data.date;
    set_location.innerHTML = data.location;
    speed.innerHTML = data.speed;
    altitude.innerHTML = data.altitude;
    satellites.innerHTML = data.satellites;
    serial_waiting.innerHTML = data.ser_waiting;
    mins_to_empty.innerHTML = data.battery_mins_to_empty;
    percent_left.innerHTML = data.battery_avg_percent;

});

socket.on('compass_data', function(data)
{
    //console.log(data);
    heading.innerHTML = data.heading;
    pitch.innerHTML = data.pitch;
    roll.innerHTML = data.roll;
    temp.innerHTML = data.temp; 
});