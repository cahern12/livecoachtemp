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
var socket = io(location.host);
var select_box = document.getElementById('streams');
var video = document.getElementById('embedded_video');

//Set the select box values from the server
socket.on('video_streams', function(data)
{
    for(var x = 0; x < data.length; x++)
    {
        var el = document.createElement('option');
        el.textContent = data[x].trim();
        el.value = data[x].trim();
        select_box.appendChild(el);
    }
});

function play_video()
{
    console.log(select_box.value);
    if(select_box.value == "select_stream")
        alert('you must select a stream..');
    else
        video.setAttribute('src', '../../recording_streams/' + select_box.value);
}

function download_video()
{
    var link = document.createElement('a');
    link.download = "stream.mp4";
    link.href = '../../recording_streams/' + select_box.value;
    link.click();
}