#!/bin/bash
cd ~/Desktop/zel_tech/Thrud/node_js_server/
vlc -vvv v4l2:///dev/video5 --sout '#transcode{vcodec=theo,vb=800,acodec=vorb,ab=128,channels=2,samplerate=44100}:http{mux=ogg,dst=:8080/}' && sudo node start ./server.js
#FOREVER START!! vlc -vvv v4l2:///dev/video1 --sout '#transcode{vcodec=theo,vb=800,acodec=vorb,ab=128,channels=2,samplerate=44100}:http{mux=ogg,dst=:8080/}' && sudo forever start ./app.js