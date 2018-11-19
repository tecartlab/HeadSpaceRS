//
//  BlobTracker.h
//  kinectTCPServer
//
//  Created by maybites on 14.02.14.
//
//
#pragma once

#include "ofMain.h"
#include "ofConstants.h"
#include "TrackedBlob.h"

#include <cmath>
#include <vector>
#include <iterator>

//defines after how many frames without update a Blob dies.
#define N_EMPTYFRAMES 10

class BlobTracker {
    
public:
    BlobTracker(int _ID, int _liveSpan, ofRectangle _rect, glm::vec3 _headBlobCenter, glm::vec2 _headBlobSize, glm::vec3 _headTop);
    
	// returns true if the event is alive
	bool isActive();

	// returns true if the event is alive
	bool isAlive();

	// returns true if this is the last lifecycle
    bool isDying();

	// returns true if this is the last lifecycle and the event end has been sent
	bool isDead();

	// returns true if this event is dead and can be removed
	bool checkForDisposal();

	// return true if it matches and is not dying and hasn't been matched before.
    float getDistance(glm::vec3 _headTop);

	// returns true if the event has already been matched.
	bool hasBeenUpdated();
    
    void updatePrepare();
    void update(ofRectangle _rect, glm::vec3 _headBlobCenter, glm::vec2 _headBlobSize, glm::vec3 _headTop, float _smoothPos);
    
	int getAgeInMillis();
    
    void drawBodyBox();
    void drawHeadTop();
    
	bool mHasBeenUpdated;

	bool mIsDying;
	bool mIsDead;

    int sortPos;

	// this event ID
	int mID;

	// the number of frames this event survives without any blob match
	int mBreathSize;

	// the number of frames left until this event dies
	int mCountDown;

	// livetime in milliseconds
	int mLifeCycles;

    ofBoxPrimitive bodyBox;
    
    ofSpherePrimitive bodyHeadTopSphere;
    ofSpherePrimitive headCenterSphere;
    ofSpherePrimitive eyeCenterSphere;
    
    ofRectangle baseRectangle2d;
        
	glm::vec3     headTop;

	glm::vec3     headBlobCenter;
	glm::vec2     headBlobSize;
    
	glm::vec3     eyeCenter;
    
	glm::vec3     eyeGaze;
    
    float       eyeLevel;
     
    ofVboMesh contourMesh;
    vector <ofVec3f> countour;
    
};


