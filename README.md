# HeadSpace RealSense, the RealSense network solution for *interaction space developers*.

by Martin Fröhlich

## Cameras

Tested:
    Intel® RealSense™ Depth Cameras D435

Untested:
    Intel® RealSense™ Depth Cameras D415
    Intel® RealSense™ Depth Modules D400, D410, D420, D430
    Intel® RealSense™ Vision Processor D4m
    Intel® RealSense™ Tracking Module (limited support)

## Supported Platforms

Tested:
    Windows 10 (Build 1803 or later)

Untested:
        Ubuntu 16.04/18.04 LTS (Linux Kernels 4.4, 4.8 ,4.10, 4.13 and 4.15)
        Windows 8.1 *
        Mac OS* (High Sierra 10.13.2)

****hardware frame synchronization is currently (as in Sept. 2018) not available for the D400 series

## Overview

It is based on openFrameworks and Intel RealSense SDK, should run on Windows (tested).

### Functionality

HeadSpace runs as an application on a dedicated computer with one attached RealSense device. The machine needs to be powerfull enough to do all the tracking analysis necessary. 

It is built for a accurate tracking of bodies. The tracker can send the bodies position and heigth, head-toptip-position, head-center-position, eye-center-position and gaze direction.

It has a simple but powerfull calibration functionality. This makes it very easy to tell how the kinect is located towards a floor, ceilling or a wall.

All data is in a metric cartesian coordinatesystem referenced from a floor.

The results are sent as UDP and/or TCP data over the network to a requesting client.

## Communication

On startup, HeadSpace will send every second a OSC-broadcast-announcement to the broadcast address (of the newtork the machine resides in) to port 43500. The server itself will listen to a different port (default = 43600), indicated inside the broadcast message.

Assuming the IP-address of the kinectServer is 192.168.1.100, it will send the handshake to 192.168.1.255 / 43500.

> **/ks/server/broadcast** \<kinectid> \<serverID> \<ServerIP> \<ServerListeningPort>

for example

> **/ks/server/broadcast** A00363A14660053A 0 192.168.1.100 43600

every client in the network can respond to this broadcast and send a handshake request back to the HeadSpace:

> **/ks/request/handshake** \<ClientListeningPort>

upon receiving this request, the server will send the calibration data. If the client misses some of the calibration data it has to resend the handshake request. 

In order to get the trackingdata, the server then needs an update message every 10 seconds.

> **/ks/request/update** \<ClientListeningPort>
> 
upon receiving this request, the server will send a continous stream of the tracking data for the next 11 seconds. Since the server will keep on sending its broadcast message every 10 seconds, the clients resend of the update-message can be triggered by the broadcast-message and thus the connection will never drop.

HeadSpace will stop sending the stream of tracking data if no update-message is received anymore and drop the registration of the client. 

If no client is registered anymore, the server will again start sending the broadcast message every second.

If the server encounters a problem or stops, it will atempt to broadcast an exit message:

> **/ks/server/broadcast/exit** \<kinectid> \<serverID>


##calibration data

####transformation

> **/ks/server/calib/trans** \<serverID> \<x-rotate[deg]> \<y-rotate[deg]> \<z-translate[m]>
> 

with this info the kinect transformation matrix in relation to the floor can be calcualted like this (example code for openframeworks):

    kinectTransform = ofMatrix4x4();
    kinectTransform.rotate(<x-rotate>, 1, 0, 0);
    kinectTransform.rotate(<y-rotate>, 0, 1, 0);
    kinectTransform.translate(0, 0, <z-translate>);

The transformation matrix is mainly used to correctly transform the frustum and pointcloud data.

---
####kinects frustum:

> **/ks/server/calib/frustum** \<serverID> \<left[m]> \<right[m]> \<bottom[m]> \<top[m]> \<near[m]> \<far[m]>
> 

the frustum needs to translated by the above transformation matrix to be in the correct space

---
####sensorbox:

> **/ks/server/calib/sensorbox** \<serverID> \<left(x-axis)[m]> \<right(x-axis)[m]> \<bottom(z-axis)[m]> \<top(z-axis)[m]> \<near(y-axis)[m]> \<far(y-axis)[m]>  


---
####gaze:

> **/ks/server/calib/gazepoint** \<serverID> \<gazePosX[m]> \<gazePosY[m]> \<gazePosZ[m]>
> 



##tracking data

####frame

> **/ks/server/track/frame** \<serverID> \<frameNo> \<sendBodyBlob> \<sendHeadBlob> \<sendHead> \<sendEye>

the message is sent each time at the beginning of a new frame. 'sendBodyBlob', 'sendHeadBlob', 'sendHead', 'sendEye' are 1 if beeing sent.

---
####bodyBlob

> **/ks/server/track/bodyblob** \<serverID> \<frameNo> \<blobID> \<sortPos> \<bodyBlobPosX[m]> \<bodyBlobPosY[m]> \<bodyBlobWidth(x-axis)[m]> \<bodyBlobDepth(y-axis)[m]> \<bodyHeight(z-axis)[m]>

---
####headBlob

> **/ks/server/track/headblob** \<serverID> \<frameNo> \<blobID> \<sortPos> \<headBlobPosX[m]> \<headBlobPosY[m]> \<headBlobPosZ[m]> \<headBlobWidth(x-axis)[m]> \<headBlobDepth(y-axis)[m]>

---
####head

> **/ks/server/track/head** \<serverID> \<frameNo> \<blobID> \<sortPos> \<headTopPosX[m]> \<headTopPosY[m]> \<headTopPosZ[m]> \<headCenterPosX[m] \<headCenterPosY[m] \<headCenterPosZ[m]>
> 

---
####eye

> **/ks/server/track/eye** \<serverID> \<frameNo> \<blobID> \<sortPos> \<eyePosX[m]> \<eyePosY[m]> \<eyePosZ[m]> \<eyeGazeX> \<eyeGazeY> \<eyeGazeZ>
> 

eyeGazeX, Y, Z is a normalized vector. Beware: The gaze is calculated based on a defined gaze-point and not through facial feature tracking. It assumes that each tracked person looks at this gaze-point.

---
####framedone

> **/ks/server/track/framedone** \<serverID> \<frameNo>
> 
the last message sent for the current frame.

---

## Download

To grab a copy of HeadSpace for your platform, check here [download page](http://github.com/tecartlab/HeadSpaceRS/releases).  
 
The `master` branch of this repository corresponds to the most recent release. This GitHub repository contains code and libs for all the platforms.

## Building

### Dependecies

Microsoft Visual Studio Community edition 2017 https://visualstudio.microsoft.com/de/downloads/
Intel® RealSense™ SDK 2.0 (build 2.16.0) https://github.com/IntelRealSense/librealsense
Openframeworks release 0.10.0 [download page](http://openframeworks.cc/download).
Openframeworks addon [ofxRSSDK](https://github.com/tecartlab/ofxRSSDK)
Openframeworks addon [ofxGuiExtended](https://github.com/frauzufall/ofxGuiExtended)

### Instructions

drop this repositoriy into the \<openframeworksfolder>/apps/\<yourappfolder>

drop the addons into the \<openframeworksfolder>/addons/ folder

#### Visual Studio
Examles require to be linked to the installed RealSense SDK. All the examples assume the SDK is installed under C:\Program Files(x86)\Intel RealSense SDK 2.0

if otherwise, change the following:

* Menu > Project > Properties > C/C++ > General > Additional Include directories > (Edit...) > (RSSKD_Dir)\include
* Menu > Project > Properties > Linker > General > Additional Library directories > (Edit...) > (RSSKD_Dir)\lib\x64


## How to
to come...

## Version
HeadSpace uses [Semantic Versioning](http://semver.org/), 

Version 0.0.1
- initial release

## Credits

(c) by tecartlab.com

created by Martin Froehlich for [tecartlab.com](http://tecartlab.com)

loosely based on a concept by Andrew Sempre and his [performance-space](https://bitbucket.org/tezcatlipoca/performance-space)

##Licence
MIT and see license.md