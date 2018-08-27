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

var record_click = document.getElementById('record_img');
var stop_record_click = document.getElementById('stop_img');
var inner_left_click = document.getElementById('inner_left_img');
var inner_right_click = document.getElementById('inner_right_img');
var inner_up_click = document.getElementById('inner_up_img');
var inner_down_click = document.getElementById('inner_down_img');

var outer_left_click = document.getElementById('outer_left_img');
var outer_right_click = document.getElementById('outer_right_img');
var outer_up_click = document.getElementById('outer_up_img');
var outer_down_click = document.getElementById('outer_down_img');

var zoom_value = 1;
var vertical_position = 0, horizontal_position = 0;

/*------------------------------------------------------------
    BOTTOM DIV CONTROL
-------------------------------------------------------------*/
function send_horizontal_click(step_amt, which_way)
{
    if(which_way == 0) //left
    {
        horizontal_position += Number(step_amt);
        if(horizontal_position >= 360)
            horizontal_position = horizontal_position - 360;  
    }
    else if(which_way == 1) //right
    {
        horizontal_position -= Number(step_amt);
        if(horizontal_position < 0)
            horizontal_position = 360 + horizontal_position; //add since negative number
    }
    socket.emit('horizontal_position', {amt: horizontal_position });
    document.getElementById('txt_az').value = horizontal_position;
}
function send_vertical_click(step_amt, which_way)
{
    var previous_vertical = vertical_position;
    if(which_way == 0) //up
    {
        vertical_position += Number(step_amt);
        if(vertical_position >= 44)
        {
            vertical_position = previous_vertical;
            alert('error - cannot go up from here');
        }
    }
    else if(which_way == 1) //down
    {
        vertical_position -= Number(step_amt);
        if(vertical_position <= -44)
        {
            vertical_position = previous_vertical;
            alert('error - cannot go down from here');
        }
    }
    socket.emit('vertical_position', {amt: vertical_position });
    document.getElementById('txt_el').value = vertical_position;
}
outer_left_click.onclick = function()
{
    console.log("outer left clicked!");
    send_horizontal_click(6, 0); //6 degrees left
}
outer_right_click.onclick = function()
{
    console.log("outer left clicked!");
    send_horizontal_click(6, 1); //6 degrees right
}
inner_left_click.onclick = function()
{
    console.log("inner left clicked!");
    send_horizontal_click(3, 0); //3 degrees left
}
inner_right_click.onclick = function()
{
    console.log("inner right clicked!");
    send_horizontal_click(3, 1);
}
inner_up_click.onclick = function()
{
    console.log("inner up clicked!");
    send_vertical_click(3, 0);
}
inner_down_click.onclick = function()
{
    console.log("inner down clicked!");
    send_vertical_click(3, 1);
}
outer_up_click.onclick = function()
{
    console.log('outer up clicked');
    send_vertical_click(6, 0);
}
outer_down_click.onclick = function()
{
    console.log('outer down clicked');
    send_vertical_click(6, 1);
}
function add_zoom()
{
    ++zoom_value;
    console.log('add zoom');
    socket.emit('add_zoom');
}
function sub_zoom()
{
    --zoom_value;
    console.log('sub zoom');
    socket.emit('sub_zoom');
}
function reset_zoom_click()
{
    zoom_value = 1;
    console.log('reset zoom');
    socket.emit('reset_zoom');
}
function txt_az_change(change_amt)
{
    if(change_amt >= 360 || change_amt < 0)
        alert('error. value must be between 0 and 359');
    else
    {
        console.log("horizontal value changed to: " + change_amt);
        horizontal_position = Number(change_amt);
        socket.emit('horizontal_position', {amt: horizontal_position });
        document.getElementById('txt_az').value = horizontal_position;
    }
}
function txt_el_change(change_amt)
{
    if(change_amt < -44 || change_amt > 44)
        alert('error. value must be between -44 and 44');
    else
    {
        console.log("vertical value changed to: " + change_amt);
        vertical_position = Number(change_amt);
        socket.emit('vertical_position', {amt: vertical_position });
        document.getElementById('txt_el').value = vertical_position;
    }
}
function reset_slew_click()
{
    console.log('reset slew');
    vertical_position = 0;
    horizontal_position = 0;
    socket.emit('reset_slew');
    document.getElementById('txt_az').value = 0;
    document.getElementById('txt_el').value = 0;
}

/*------------------------------------------------------------
    Putting video event listener here since it is easier
-------------------------------------------------------------*/
var video = document.getElementById('embedded_video');
video.addEventListener('click', send_click_position, false);

function send_click_position(event)
{
    var x = new Number();
    var y = new Number();
    
    x = event.x;
    y = event.y;

    x -= video.offsetLeft;
    y -= video.offsetTop;

    //set orgin in the center;
    x -= video.clientWidth / 2;
    y -= video.clientHeight / 2;
    y = -y;

    //normalize to 1
    x /= (video.clientWidth / 2);
    y /= (video.clientHeight / 2);

    console.log('before: ', x, y);
    //convert to degrees
    x *= (100 / (Math.pow(1.03, zoom_value)) / 2);
    y *= (100 / (Math.pow(1.03, zoom_value)) / 2 / 4 * 3);


    console.log('after: ', x,y);
    //console.log((horizontal_position + x) % 360);
    //console.log((vertical_position +y) % 90);


    /*if(x < 0) //turn left
        horizontal_position += Math.round((Math.abs(x) / 2 - 8));
    else
        horizontal_position -= Math.round(x / 2 - 8);

    console.log('horizontal: ', horizontal_position);
    socket.emit('horizontal_position', {amt: horizontal_position });   
    document.getElementById('txt_az').value = horizontal_position;*/
}