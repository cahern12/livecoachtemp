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
var video = document.getElementById('embedded_video');
var glob_ip_address = location.host;
var parsed_ip = glob_ip_address.substring(0, glob_ip_address.indexOf(':'));
var glob_video_url = 'http://' + parsed_ip + ':8080/';

//THIS IS A WORK AROUND UNTIL ON THE TX1 (IT WILL WORK)
//DELETE THIS LINE WHEN IT IS PUT ONTO THE BOARD
//glob_video_url = 'http://192.168.2.173:8080';
video.setAttribute('src', glob_video_url);

function get_server_ip()
{
    return glob_ip_address;
}