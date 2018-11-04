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
    BlobTracker(int _ID, ofRectangle _rect, int _liveSpan);
    
	// returns true if the event is alive
	bool isAlive();

	// returns true if this is the last lifecycle
    bool isDying();

	// returns true if this event is dead and can be removed
	bool checkForDisposal();

	// dispose this event in any case in the next recycling 
	void dispose();

	// return true if it matches and is not dying and hasn't been matched before.
    bool isMatching(ofRectangle _rect, int maxDistance);

	// sets this event as beeing matched.
	void setAsMatched();

	// returns true if the event has already been matched.
	bool hasBeenMatched();
    
    void updatePrepare();
    void updateBody(ofRectangle _rect, glm::vec3 _bodyBlobCenter, glm::vec2 _bodyBlobSize, glm::vec3 _headTop, glm::vec3 _headCenter, float _eyelevel, float _smoothPos);
    void updateHead(glm::vec3 _headBlobCenter, glm::vec2 _headBlobSize, glm::vec3 _eyeCenter, float _smoothPos);
    
	// returns true if it has been updated
	bool hasBeenUpdated();

    ofVec3f getCurrentHeadCenter();

	int getElapsedMillis();
    
    void drawBodyBox();
    void drawHeadTop();
    void drawHeadBlob();
    void drawEyeCenter();
    
    bool hasBodyUpdated;
    bool hasHeadUpdated;
	bool mHasBeenMatched;

    bool mIsDying;
    
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
    ofPlanePrimitive headBlob;
    
    ofSpherePrimitive bodyHeadTopSphere;
    ofSpherePrimitive headCenterSphere;
    ofSpherePrimitive eyeCenterSphere;
    
    ofRectangle baseRectangle2d;
    
	glm::vec3     bodyBlobCenter;
	glm::vec2     bodyBlobSize;
    
	glm::vec3     headTop;

	glm::vec3     headCenter;

	glm::vec3     headBlobCenter;
	glm::vec2     headBlobSize;
    
	glm::vec3     eyeCenter;
    
	glm::vec3     eyeGaze;
    
    float       eyeLevel;
     
    ofVboMesh contourMesh;
    vector <ofVec3f> countour;
    
};


