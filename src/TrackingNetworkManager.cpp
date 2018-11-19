//
//  TrackingNetworkManager.cpp
//  kinectServer
//
//  Created by maybites on 14.02.14.
//
//

#include "TrackingNetworkManager.h"

TrackingNetworkManager::TrackingNetworkManager(){
}


void TrackingNetworkManager::setup(ofxGui &gui, string _kinectSerial){
    mDeviceSerial = _kinectSerial;

    //RegularExpression regEx("\\b\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\b");
    
    string localAddress = "127.0.0.1";
    
    for(int i = 0; i < localIpAddresses.size(); i++){
		ofLog(OF_LOG_NOTICE, "try to find local ip addresses.. not sure if this function works properly...");
		if(matchesInRegex(localIpAddresses[i], "\\b^(?:(?!127).)+\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\b").size() == 1) {
            localAddress = localIpAddresses[i];
			ofLog(OF_LOG_NOTICE, "found valid address" + localAddress);
			//            broadcastAddress = serverAddress.substr(0, serverAddress.find_last_of(".") + 1 ) + "255";
        }
    }

    panel = gui.addPanel();
    
    
    panel->loadTheme("theme/theme_light.json");
    panel->setName("Broadcasting..");
	panel->add<ofxGuiIntInputField>(mServerID.set("ServerID", 0, 0, 10));

    streamingBodyBlob.addListener(this, &TrackingNetworkManager::listenerBool);
    streamingHeadBlob.addListener(this, &TrackingNetworkManager::listenerBool);
    streamingHead.addListener(this, &TrackingNetworkManager::listenerBool);
    streamingEye.addListener(this, &TrackingNetworkManager::listenerBool);

    broadcastIP.addListener(this, &TrackingNetworkManager::listenerString);
    broadcastPort.addListener(this, &TrackingNetworkManager::listenerInt);
    listeningIP.addListener(this, &TrackingNetworkManager::listenerString);
    listeningPort.addListener(this, &TrackingNetworkManager::listenerInt);
    
    broadcastGroup = panel->addGroup("Broadcast TX");
    //panel->add(broadcastLabel.set("Broadcast"));
    broadcastGroup->add<ofxGuiTextField>(broadcastIP.set("TX IP","127.0.0.1"));
    broadcastGroup->add<ofxGuiIntInputField>(broadcastPort.set("TX Port", NETWORK_BROADCAST_PORT, NETWORK_BROADCAST_PORT, NETWORK_BROADCAST_PORT + 99));
    
    listeningGroup = panel->addGroup("Listening RX");
    listeningGroup->add<ofxGuiTextField>(listeningIP.set("RX IP",localAddress));
    listeningGroup->add<ofxGuiIntInputField>(listeningPort.set("RX Port", NETWORK_LISTENING_PORT, NETWORK_LISTENING_PORT, NETWORK_LISTENING_PORT + 99));

    
    streamingGuiGroup.setName("Streaming");
    //streamingGuiGroup.add(streamingBodyBlob.set("bodyBlob", true));
    streamingGuiGroup.add(streamingHeadBlob.set("headBlob", true));
    //streamingGuiGroup.add(streamingHead.set("head", true));
    //streamingGuiGroup.add(streamingEye.set("eye", true));
    panel->addGroup(streamingGuiGroup);

    panel->loadFromFile("broadcast.xml");
    

    //Server side
    //listen for incoming messages on a port; setup OSC receiver with usage:
    serverReceiver.setup(listeningPort.get());
    broadcastSender.setup(broadcastIP.get(), broadcastPort.get());
    ofLog(OF_LOG_NOTICE, "Choosen BroadcastAddress:  " + broadcastIP.get());
    
	maxServerMessages = 38;
    
    broadCastTimer = ofGetElapsedTimeMillis();
    
    scale = 0.001; // transform mm to m
    frameNumber = 0;
}

void TrackingNetworkManager::listenerString(string & _string){
    ofLog(OF_LOG_NOTICE, "listenerString " + _string + " from");
}

void TrackingNetworkManager::listenerInt(int & _int){
    ofLog(OF_LOG_NOTICE, "listenerInt " + ofToString(_int) + " ");
}

void TrackingNetworkManager::listenerBool(bool & _bool){
    ofLog(OF_LOG_NOTICE, "listenerBool " + ofToString(_bool) +
          " streamingBodyBlob:" + ofToString(streamingBodyBlob.get()) +
          " streamingHeadBlob:" + ofToString(streamingHeadBlob.get()) +
          " streamingHead:" + ofToString(streamingHead.get()) +
          " streamingEye:" + ofToString(streamingEye.get()));
}


//--------------------------------------------------------------
void TrackingNetworkManager::update(BlobFinder & _blobFinder, Frustum & _frustum, ofMatrix4x4 _trans){
    frameNumber++;
    
    long currentMillis = ofGetElapsedTimeMillis();
	//Check if its about time to send a broadcast message
    if(knownClients.size() > 0 && (currentMillis - broadCastTimer) > BROADCAST_CLIENT_FREQ){
        sendBroadCastAddress();
        checkTrackingClients(currentMillis);
    } else if(knownClients.size() == 0 && (currentMillis - broadCastTimer) > BROADCAST_NOCLIENT_FREQ){
        sendBroadCastAddress();
    }
    
    //send trackingdata to all connected clients
    sendTrackingData(_blobFinder);
    
    // OSC receiver queues up new messages, so you need to iterate
	// through waiting messages to get each incoming message
    
	// check for waiting messages
	while(serverReceiver.hasWaitingMessages()){
		// get the next message
		ofxOscMessage m;
		serverReceiver.getNextMessage(m);
		//Log received message for easier debugging of participants' messages:
        ofLog(OF_LOG_NOTICE, "Server recvd msg " + getOscMsgAsString(m) + " from " + m.getRemoteIp());
        
		// check the address of the incoming message
		if(m.getAddress() == "/ks/request/handshake"){
			//Identify host of incoming msg
			string incomingHost = m.getRemoteIp();
			//See if incoming host is a new one:
			// get the first argument (listeningport) as an int
			if(m.getNumArgs() == 1 && m.getArgType(0) == OFXOSC_TYPE_INT32){
                knownClients[getTrackingClientIndex(incomingHost, m.getArgAsInt32(0))].update(currentMillis);
                // Send calib-data
                sendCalibFrustum(_frustum, incomingHost, m.getArgAsInt32(0));
                sendCalibSensorBox(_blobFinder, incomingHost, m.getArgAsInt32(0));
                sendCalibTrans(_trans, incomingHost, m.getArgAsInt32(0));
                sendGazePoint(_blobFinder, incomingHost, m.getArgAsInt32(0));
            }else{
                ofLog(OF_LOG_WARNING, "Server recvd malformed message. Expected: /ks/request/handshake <ClientListeningPort> | received: " + getOscMsgAsString(m) + " from " + incomingHost);
            }
		} else if(m.getAddress() == "/ks/request/update"){
			//Identify host of incoming msg
			string incomingHost = m.getRemoteIp();
			//See if incoming host is a new one:
			// get the first argument (listeningport) as an int
			if(m.getNumArgs() == 1 && m.getArgType(0) == OFXOSC_TYPE_INT32){
                knownClients[getTrackingClientIndex(incomingHost, m.getArgAsInt32(0))].update(currentMillis);
			}else{
                ofLog(OF_LOG_WARNING, "Server recvd malformed message. Expected: /ks/request/update <ClientListeningPort> | received: " + getOscMsgAsString(m) + " from " + incomingHost);
            }
		}
		// handle getting random OSC messages here
		else{
			ofLogWarning("Server got weird message: " + m.getAddress());
		}
	}
    
	//this is purely workaround for a mysterious OSCpack bug on 64bit linux
	// after startup, reinit the receivers
	// must be a timing problem, though - in debug, stepping through, it works.
	if(ofGetFrameNum() == 60){
		serverReceiver.setup(listeningPort.get());
	}
}

void TrackingNetworkManager::sendTrackingData(BlobFinder & _blobFinder){
    // send frame number
    ofxOscMessage frame;
    frame.setAddress("/ks/server/track/frame/start");
    frame.addIntArg(mServerID.get());
    frame.addIntArg(frameNumber);
    frame.addIntArg(streamingBodyBlob.get());
    frame.addIntArg(streamingHeadBlob.get());
    frame.addIntArg(streamingHead.get());
    frame.addIntArg(streamingEye.get());
    sendMessageToTrackingClients(frame);
 
    for(int i = 0; i < _blobFinder.blobEvents.size(); i++){
		if (_blobFinder.blobEvents[i].isActive() && _blobFinder.blobEvents[i].hasBeenUpdated()) {
			if (streamingHeadBlob.get()) {
				ofxOscMessage headBlob;
				headBlob.setAddress("/ks/server/track/headblob");
				headBlob.addIntArg(mServerID.get());
				headBlob.addIntArg(frameNumber);
				headBlob.addIntArg(_blobFinder.blobEvents[i].mID);
				headBlob.addIntArg(_blobFinder.blobEvents[i].sortPos);
				headBlob.addIntArg(_blobFinder.blobEvents[i].getAgeInMillis());
				headBlob.addFloatArg(_blobFinder.blobEvents[i].headBlobCenter.x);
				headBlob.addFloatArg(_blobFinder.blobEvents[i].headBlobCenter.y);
				headBlob.addFloatArg(_blobFinder.blobEvents[i].headBlobCenter.z);
				headBlob.addFloatArg(_blobFinder.blobEvents[i].headBlobSize.x);
				headBlob.addFloatArg(_blobFinder.blobEvents[i].headBlobSize.y);

				sendMessageToTrackingClients(headBlob);
			}
			if (streamingHead.get()) {
				ofxOscMessage head;
				head.setAddress("/ks/server/track/head");
				head.addIntArg(mServerID.get());
				head.addIntArg(frameNumber);
				head.addIntArg(_blobFinder.blobEvents[i].mID);
				head.addIntArg(_blobFinder.blobEvents[i].sortPos);
				head.addIntArg(_blobFinder.blobEvents[i].getAgeInMillis());
				head.addFloatArg(_blobFinder.blobEvents[i].headTop.x);
				head.addFloatArg(_blobFinder.blobEvents[i].headTop.y);
				head.addFloatArg(_blobFinder.blobEvents[i].headTop.z);

				sendMessageToTrackingClients(head);
			}
			if (streamingEye.get()) {
				ofxOscMessage eye;
				eye.setAddress("/ks/server/track/eye");
				eye.addIntArg(mServerID.get());
				eye.addIntArg(_blobFinder.blobEvents[i].mID);
				eye.addIntArg(_blobFinder.blobEvents[i].sortPos);
				eye.addIntArg(_blobFinder.blobEvents[i].getAgeInMillis());
				eye.addFloatArg(_blobFinder.blobEvents[i].eyeCenter.x);
				eye.addFloatArg(_blobFinder.blobEvents[i].eyeCenter.y);
				eye.addFloatArg(_blobFinder.blobEvents[i].eyeCenter.z);
				eye.addFloatArg(_blobFinder.blobEvents[i].eyeGaze.x);
				eye.addFloatArg(_blobFinder.blobEvents[i].eyeGaze.y);
				eye.addFloatArg(_blobFinder.blobEvents[i].eyeGaze.z);

				sendMessageToTrackingClients(eye);
			}
		}
		else if (!_blobFinder.blobEvents[i].isDead() && _blobFinder.blobEvents[i].isDying()) {
			ofxOscMessage bodyBlob;
			bodyBlob.setAddress("/ks/server/track/end");
			bodyBlob.addIntArg(mServerID.get());
			bodyBlob.addIntArg(frameNumber);
			bodyBlob.addIntArg(_blobFinder.blobEvents[i].mID);
			bodyBlob.addIntArg(_blobFinder.blobEvents[i].getAgeInMillis());

			sendMessageToTrackingClients(bodyBlob);
			_blobFinder.blobEvents[i].mIsDead = true;
		}
    }
	// send frame number
	ofxOscMessage framedone;
	framedone.setAddress("/ks/server/track/frame/end");
	framedone.addIntArg(mServerID.get());
	framedone.addIntArg(frameNumber);
	sendMessageToTrackingClients(framedone);
}

void TrackingNetworkManager::sendCalibFrustum(Frustum & _frustum, string _ip, int _port){
    ofxOscMessage frustum;
    frustum.setAddress("/ks/server/calib/frustum");
    frustum.addIntArg(mServerID.get());
    frustum.addFloatArg(_frustum.left);
    frustum.addFloatArg(_frustum.right);
    frustum.addFloatArg(_frustum.bottom);
    frustum.addFloatArg(_frustum.top);
    frustum.addFloatArg(_frustum.near1);
    frustum.addFloatArg(_frustum.far1);
    
    broadcastSender.setup(_ip, _port);
    broadcastSender.sendMessage(frustum);
}

void TrackingNetworkManager::sendCalibTrans(ofMatrix4x4 & _trans, string _ip, int _port){
    ofxOscMessage trans;
    trans.setAddress("/ks/server/calib/trans");
    trans.addIntArg(mServerID.get());
	trans.addFloatArg(_trans._mat[0].x);
	trans.addFloatArg(_trans._mat[0].y);
	trans.addFloatArg(_trans._mat[0].z);
	trans.addFloatArg(_trans._mat[0].w);
	trans.addFloatArg(_trans._mat[1].x);
	trans.addFloatArg(_trans._mat[1].y);
	trans.addFloatArg(_trans._mat[1].z);
	trans.addFloatArg(_trans._mat[1].w);
	trans.addFloatArg(_trans._mat[2].x);
	trans.addFloatArg(_trans._mat[2].y);
	trans.addFloatArg(_trans._mat[2].z);
	trans.addFloatArg(_trans._mat[2].w);
	trans.addFloatArg(_trans._mat[3].x);
	trans.addFloatArg(_trans._mat[3].y);
	trans.addFloatArg(_trans._mat[3].z);
	trans.addFloatArg(_trans._mat[3].w);

    broadcastSender.setup(_ip, _port);
    broadcastSender.sendMessage(trans);
}

void TrackingNetworkManager::sendCalibSensorBox(BlobFinder & _blobFinder, string _ip, int _port){
    ofxOscMessage sensorbox;
    sensorbox.setAddress("/ks/server/calib/sensorbox");
    sensorbox.addIntArg(mServerID.get());
    sensorbox.addFloatArg(_blobFinder.sensorBoxLeft.get() * scale);
    sensorbox.addFloatArg(_blobFinder.sensorBoxRight.get() * scale);
    sensorbox.addFloatArg(_blobFinder.sensorBoxBottom.get() * scale);
    sensorbox.addFloatArg(_blobFinder.sensorBoxTop.get() * scale);
    sensorbox.addFloatArg(_blobFinder.sensorBoxFront.get() * scale);
    sensorbox.addFloatArg(_blobFinder.sensorBoxBack.get() * scale);
    
    broadcastSender.setup(_ip, _port);
    broadcastSender.sendMessage(sensorbox);
}

void TrackingNetworkManager::sendGazePoint(BlobFinder & _blobFinder, string _ip, int _port){
    ofxOscMessage sensorbox;
    sensorbox.setAddress("/ks/server/calib/gazepoint");
    sensorbox.addIntArg(mServerID.get());
    sensorbox.addFloatArg(_blobFinder.gazePoint.get().x);
    sensorbox.addFloatArg(_blobFinder.gazePoint.get().y);
    sensorbox.addFloatArg(_blobFinder.gazePoint.get().z);
    
    broadcastSender.setup(_ip, _port);
    broadcastSender.sendMessage(sensorbox);
}

void TrackingNetworkManager::sendMessageToTrackingClients(ofxOscMessage _msg){
    for(int j = 0; j < knownClients.size(); j++){
        broadcastSender.setup(knownClients[j].clientDestination, knownClients[j].clientSendPort);
        broadcastSender.sendMessage(_msg);
    }
}

void TrackingNetworkManager::checkTrackingClients(long _currentMillis){
    for(int i = 0; i < knownClients.size(); i++){
        if(!knownClients[i].stillAlive(_currentMillis)){
            ofLog(OF_LOG_NOTICE, "Server removed TrackingClient ip: " + knownClients[i].clientDestination + " port:  " + ofToString(knownClients[i].clientSendPort));
            knownClients[i] = knownClients.back();
            knownClients.pop_back();
            i--;
        }
    }
}

int TrackingNetworkManager::getTrackingClientIndex(string _ip, int _port){
    for(int i = 0; i < knownClients.size(); i++){
        if(knownClients[i].clientDestination.find(_ip) != string::npos && knownClients[i].clientSendPort == _port){
            return i;
        }
    }
    knownClients.push_back(TrackingClient(_ip, _port));
    ofLog(OF_LOG_NOTICE, "Server added new TrackingClient ip: " + _ip + " port:  " + ofToString(_port) + " knownClients:  " + ofToString(knownClients.size()));
    return knownClients.size() -1;
}

void TrackingNetworkManager::sendBroadCastAddress(){
    ofxOscMessage broadcast;
    broadcast.setAddress("/ks/server/broadcast");
	broadcast.addStringArg(mDeviceSerial);
	broadcast.addIntArg(mServerID.get());
	broadcast.addStringArg(listeningIP.get());
	broadcast.addIntArg(listeningPort.get());
    
    broadcastSender.setup(broadcastIP.get(), broadcastPort.get());
    broadcastSender.sendMessage(broadcast);
    
    broadCastTimer = ofGetElapsedTimeMillis();
    //ofLog(OF_LOG_NOTICE, "Sent Broadcastmessage");
}

//--------------------------------------------------------------
string TrackingNetworkManager::getOscMsgAsString(ofxOscMessage m){
	string msg_string;
	msg_string = m.getAddress();
	msg_string += ":";
	for(int i = 0; i < m.getNumArgs(); i++){
		// get the argument type
		msg_string += " " + m.getArgTypeName(i);
		msg_string += ":";
		// display the argument - make sure we get the right type
		if(m.getArgType(i) == OFXOSC_TYPE_INT32){
			msg_string += ofToString(m.getArgAsInt32(i));
		}
		else if(m.getArgType(i) == OFXOSC_TYPE_FLOAT){
			msg_string += ofToString(m.getArgAsFloat(i));
		}
		else if(m.getArgType(i) == OFXOSC_TYPE_STRING){
			msg_string += m.getArgAsString(i);
		}
		else{
			msg_string += "unknown";
		}
	}
	return msg_string;
}

vector<string> TrackingNetworkManager::matchesInRegex(string _str, string _reg) {
	regex regEx(_reg, regex_constants::icase);
	vector<string> results;
	auto wordsBegin = sregex_iterator(_str.begin(), _str.end(), regEx);
	auto wordsEnd = sregex_iterator();

	for (std::sregex_iterator i = wordsBegin; i != wordsEnd; ++i) {
		smatch m = *i;
		results.push_back(m.str());
	}
	return results;
}


