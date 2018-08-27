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
function zm_down()
{
    console.log("zm down");
    socket.emit('zm_down');
}
function zm_up()
{
    console.log("zm up");
    socket.emit('zm_up');
}
function camera_on()
{
    console.log('camera on');
    socket.emit('camera_on');
}
function camera_off()
{
    console.log('camera off');
    socket.emit('camera_off');
}