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
var current_camera_mode_status = "EO";
var record_click = document.getElementById('record_img');
var stop_record_click = document.getElementById('stop_img');
var record_notification = document.getElementById('recording_notification');
var is_recording = false;

/*------------------------------------------------------------
    TOP DIV CONTROL
-------------------------------------------------------------*/
function shutdown_click()
{
    console.log("shutdown clicked");
    socket.emit('shutdown_clicked');
}
function restart_click()
{
    console.log("restart clicked");
    socket.emit('restart_clicked');
}
function eo_click()
{
    if(current_camera_mode_status == "EO")
        console.log("System is already EO.");
    else
    {
        console.log("eo clicked");
        socket.emit('eo_clicked');
        current_camera_mode_status = "EO";
    }
}
function ir_click()
{
    if(current_camera_mode_status == "IR")
        console.log("System is already IR.");
    else
    {
        console.log("ir clicked");
        socket.emit('ir_clicked');
        current_camera_mode_status = "IR";
    }
}
record_click.onclick = function()
{
    console.log("record clicked!");
    socket.emit('record');
    is_recording = true;
    record_notification.style.display = 'block';
}
stop_record_click.onclick = function()
{
    console.log("stop record clicked");
    if(is_recording)
    {
        socket.emit('stop_recording');
        record_notification.style.display = 'none';
        is_recording = false;
    }
    else
        console.log('stream is not recording!');
}
function subtract_focus_click()
{
    console.log('sub focus');
    socket.emit('sub_focus');
}
function add_focus_click()
{
    console.log('add focus');
    socket.emit('add_focus');
}
function auto_focus_click()
{
    console.log('auto focus');
    socket.emit('auto_focus');
}