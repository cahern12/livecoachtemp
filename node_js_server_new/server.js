/*
=======================================================================
AUTHOR               : Colin Ahern
CREATE DATE      	 : 09/15/2017
COMPANY              : Zel Technologies
PURPOSE              :
NOTES                :  
                        This is the main class which handles all of the
                        routing for the site, and acts as a central location
                        for the socket information to receive data from
                        the avr to send to client and to receive commands
                        to pass to the camera system.
 =======================================================================
Change History   	 :  
 
    
=======================================================================
*/

//USE THIS FOR THE FTP SERVER INSTALL
//http://www.thegeekstuff.com/2010/11/vsftpd-setup

const app = require('http').createServer(handler);
const url = require('url');
const fs = require('fs');
const SerialPort = require('serialport');
//const i2c = require('i2c');
const io = require('socket.io')(app);
const exec = require('child_process').exec;
const datetime = require('date-and-time');
const ip = require('ip');

//variables for the bash scripts
var record_stream;
var shutdown_script;
var restart_script;
var stream;

//const compass = require('./compass.js');
const avr_controller = require('./avr_controller.js');
const camera_controller = require('./camera_controller.js');
var shutdown_counter = 0;
var video_files = [];

//create the recording directory if it does not exist. 
var recording_directory = './recording_streams/';
if(!fs.existsSync(recording_directory))
    fs.mkdirSync(recording_directory);

//put all the files into a object to send to recording_page.js
fs.readdir(recording_directory, (err, files) => {
    files.forEach(file => {
        video_files.push(file.toString());
        console.log(file.toString());
    });
});

/*------------------------------------------------------------
    JSON CONFIG
-------------------------------------------------------------*/
console.log("--------------CONFIG------------------");
var jsonContent = JSON.parse(fs.readFileSync("settings.json"));
console.log("Server Port: ", jsonContent.server_port);
console.log("AVR Port: ", jsonContent.avr_port);
console.log("AVR Baud: ", jsonContent.avr_baud);
console.log("i2c address: ", jsonContent.i2c_address);
console.log("i2c device: ", jsonContent.i2c_device);
console.log("Camera Port: ", jsonContent.camera_port);
console.log("Camera Baud: ", jsonContent.camera_baud);
console.log("Available IPs: ", jsonContent.available_ips); 
console.log("--------------------------------------");
console.log("");

//This will open a server at localhost:5000. Navigate to this in your browser.
app.listen(jsonContent.server_port);

/*------------------------------------------------------------
    HTTP HANDLER
-------------------------------------------------------------*/
function parse_valid_ips(request)  
{ 
    var ip = request.headers['x-forwarded-for'] || 
        request.connection.remoteAddress || 
        request.socket.remoteAddress || 
        request.connection.socket.remoteAddress; 
    ip = ip.split(',')[0]; 
    ip = ip.split(':').slice(-1); //in case the ip returned in a format: "::ffff:146.xxx.xxx.xxx" 
 
    var tmp = ip.toString(); 
    if(jsonContent.available_ips.includes(tmp)) 
        return true; 
    else 
        return false; 
} 
function process_request(file_path, content_type, res)
{
    index = fs.readFile(file_path, function(error, data)
    {
        if(error)
        {
            res.writeHead(500);
            return res.end("Error: unable to load " + file_path);
        }
        res.writeHead(200,{'Content-Type': content_type});
        res.end(data);
    });
}
function handler (req, res) 
{
    // Using URL to parse the requested URL
    var path = url.parse(req.url).pathname;
    //if(parse_valid_ips(req)) 
    { 
        // Managing the root route 
        if (path == '/')                                                    //Manage routes for index.html 
            process_request(__dirname+'/public/index.html', 'text/html', res); 
        else if( /\.(json)$/.test(path) )                                   //Manage routes for json files 
            process_request(__dirname+path, 'application/json', res); 
        else if(/\.(css)$/.test(path))                                      //Manage routes for css files 
            process_request(__dirname+'/public'+path, 'text/css', res);      
        else if( /\.(js)$/.test(path) || /\.(png)$/.test(path) || /\.(ico)$/.test(path) ) //Manage routes for js/png/ico files 
            process_request(__dirname+'/public'+path, 'text/plain', res); 
        else if(path == '/recording') //FOR NEW RECORDING PAGE;
            process_request(__dirname+'/public/recording.html', 'text/html', res);
        else if(/\.(mp4)$/.test(path))
            process_request(__dirname+path, 'video/mp4', res);
        else if(/\.(mkv)$/.test(path))
            process_request(__dirname+path, 'video/x-matroska', res);
        else  
        { 
            res.writeHead(404); 
            res.end("Error: 404 - File not found."); 
        } 
    } 
    /*else 
    { 
        console.log('INVALID IP!'); 
        process_request(__dirname+'/public/invalid.html', 'text/html', res) 
    } */
}

/*------------------------------------------------------------
    Compass
-------------------------------------------------------------*/
console.log("----------------I2C-------------------");
//compass.init();
console.log("--------------------------------------");
console.log("");

/*------------------------------------------------------------
    AVR Serial
-------------------------------------------------------------*/
console.log("-------------AVR-SP-------------------");
avr_controller.init();
console.log("--------------------------------------");
console.log("");

/*------------------------------------------------------------
    Camera Serial
-------------------------------------------------------------*/
console.log("-------------AVR-SP-------------------");
camera_controller.init();
console.log("--------------------------------------");
console.log("");

/*------------------------------------------------------------
    CLIENT/SERVER SOCKETS
-------------------------------------------------------------*/
io.sockets.on('connection', function(socket)
{
    socket.emit('video_streams', video_files);  //send to recording_page.js when socket is connected.

    //roughly every second get the latest avr/compass data and send it to the client.
    setInterval(function()
    {
        socket.emit('serial_data', avr_controller.data );
        //socket.emit('compass_data', compass.headingData); 
    }, 1000);


    socket.on('shutdown_clicked', function()
    {
        console.log('shutdown clicked');
        avr_controller.shutdown_avr();

        console.log('waiting 5 seconds....');
        //wait 5 seconds to make sure avr command made it then shutdown tx1
        setTimeout(function()
        {
            console.log('send shutdown command');
            //shutdown_script = do_bash_commands('../tx1_bash/shutdown.sh');
        }, 5000);
    });

    socket.on('restart_clicked', function()
    {
        console.log('restart clicked');
        restart_script = do_bash_commands('../tx1_bash/restart.sh');
    });

    socket.on('record', function()
    {
        var record_file_name = datetime.format(new Date(), 'YYYY/MM/DD HH:mm:ss');
        var video_url = 'http://' + ip.address() + ':8080';

        record_stream = exec('cvlc -vvv ' + video_url + 
            ' --sout "#transcode{venc=x264,vb=800,fps=20,acodec=none}:file{dst=./recording_streams/' + 
            record_file_name + '.mkv}"', 
            {detached: true }); 
    }); 
    socket.on('stop_recording', function()
    {
        try{
            process.kill(record_stream.pid);
            process.kill(record_stream.pid + 1); //two processes kick off when streams starts - kill both.
            console.log('record processes: ', record_stream.pid, record_stream.pid+1, ' killed');

            video_files = [];
            fs.readdir(recording_directory, (err, files) => {
                files.forEach(file => {
                    video_files.push(file.toString());
                    console.log(file.toString());
                });
            });
            socket.emit('video_streams', video_files);
        } 
        catch(e){
            console.log('no record processes to delete');
        }
    }); 
    socket.on('camera_on', function()
    {
       avr_controller.camera_on();
       //wait for camera to turn on then start stream.
       /*setTimeout(function()
       {
           stream = do_bash_commands('../tx1_bash/start_stream.sh');
       }, 5000);*/

    });
    socket.on('camera_off', function()
    {
        /*try {
            process.kill(stream.pid);
            console.log('stream process: ', stream.pid, ' killed');
        }
        catch(e) {
            console.log('no streams processes to delete');
        }*/
        avr_controller.camera_off();
    });
    socket.on('zm_down', function()
    {
        avr_controller.zm_down();
    });
    socket.on('zm_up', function()
    {
        avr_controller.zm_up();
    });
    socket.on('sub_focus', function()
    {
        camera_controller.sub_focus();
    });
    socket.on('add_focus', function()
    {
       camera_controller.add_focus(); 
    });
    socket.on('auto_focus', function()
    {
        camera_controller.focus_auto();
    });
    socket.on('reset_slew', function()
    {
        camera_controller.reset_slew();
    });
    socket.on('eo_clicked', function()
    {
        camera_controller.set_eo();
    });
    socket.on('ir_clicked', function()
    {
        camera_controller.set_ir();
    });
    socket.on('add_zoom', function()
    {
        camera_controller.add_zoom();
    });
    socket.on('sub_zoom', function()
    {
        camera_controller.sub_zoom();
    });
    socket.on('reset_zoom', function()
    {
        camera_controller.reset_zoom();
    });
    socket.on('horizontal_position', function(data)
    {
        camera_controller.horizontal_position(data.amt);
    });
    socket.on('vertical_position', function(data)
    {
        camera_controller.vertical_position(data.amt);
    });
});

function do_bash_commands(cmd)
{
    var temp = exec(cmd, (error, stdout, stderr) => {
        //console.log('stdout ', stdout);
        //console.log('stderr ', stderr);
        //if(error !== null)
        //    console.log('exec error: ' + error);
    });
    return temp;   
}