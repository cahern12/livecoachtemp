#!/bin/bash
cvlc -vvv v4l2:///dev/video6 --sout '#transcode{vcodec=theo,vb=200,fps=8,acodec=none}:http{mux=ogg,dst=:8080/}'