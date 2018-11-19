//
//  BlobTracker.cpp
//  kinectTCPServer
//
//  Created by maybites on 14.02.14.
//
//

#include "BlobTracker.h"

BlobTracker::BlobTracker(int _ID, int _liveSpan, ofRectangle _rect, glm::vec3 _headBlobCenter, glm::vec2 _headBlobSize, glm::vec3 _headTop){
	mID = _ID;
	mBreathSize = _liveSpan;
	mCountDown = mBreathSize;
    mIsDying = false;
	mIsDead = false;
	mLifeCycles = ofGetElapsedTimeMillis();
	mHasBeenUpdated = false;
	update(_rect, _headBlobCenter, _headBlobSize, _headTop, 0);
}

void BlobTracker::updatePrepare()
{
	mHasBeenUpdated = false;
}

float BlobTracker::getDistance(glm::vec3 _headTop) {
	return glm::distance(_headTop, headTop);
}

bool BlobTracker::hasBeenUpdated()
{
	return mHasBeenUpdated;
}

void BlobTracker::update(ofRectangle _rect, glm::vec3 _headBlobCenter, glm::vec2 _headBlobSize, glm::vec3 _headTop, float _smoothPos) {
	baseRectangle2d = _rect;
	headBlobCenter = (1 - _smoothPos) * _headBlobCenter + headBlobCenter * _smoothPos;
	headBlobSize = (1 - _smoothPos) * _headBlobSize + headBlobSize * _smoothPos;
	headTop = (1 - _smoothPos) * _headTop + headTop * _smoothPos;
	mCountDown = ofGetElapsedTimeMillis() + mBreathSize;
	mHasBeenUpdated = true;
}

bool BlobTracker::isActive()
{
	return (isAlive() && getAgeInMillis() > mBreathSize)?true: false;
}

bool BlobTracker::isAlive()
{
	checkForDisposal();
	return !mIsDying;
}

bool BlobTracker::isDying() {
	checkForDisposal();
	return mIsDying;
}

bool BlobTracker::isDead() {
	return mIsDead;
}

bool BlobTracker::checkForDisposal() {
	if (mIsDying) {
		return true;
	}
	if (mCountDown < ofGetElapsedTimeMillis()) {
		mIsDying = true;
		mCountDown = 0;
	}
	return false;
}

int BlobTracker::getAgeInMillis()
{
	return ofGetElapsedTimeMillis() - mLifeCycles;
}

void BlobTracker::drawBodyBox(){
    //ofLog(OF_LOG_NOTICE, "bodyBox.size : " + ofToString(bodyBox.getSize()));
    //ofLog(OF_LOG_NOTICE, "bodyBox.pos : " + ofToString(bodyBox.getPosition()));
    if(hasBeenUpdated()){
        bodyBox.set(headBlobSize.x, headBlobSize.y, headBlobCenter.z);
        bodyBox.setPosition(headBlobCenter.x, headBlobCenter.y, headBlobCenter.z / 2);
        bodyBox.drawWireframe();
        
    }
}

void BlobTracker::drawHeadTop(){
    if(hasBeenUpdated()){
        bodyHeadTopSphere.setRadius(0.1f);
        bodyHeadTopSphere.setPosition(headTop.x, headTop.y, headTop.z);
        bodyHeadTopSphere.drawWireframe();
    }
}

/*
void BlobTracker::drawEyeCenter(){
    if(hasBeenUpdated()){
        eyeCenterSphere.setRadius(20);
        eyeCenterSphere.setPosition(eyeCenter);
        eyeCenterSphere.draw();
        
        contourMesh.clear();
        contourMesh.setMode(OF_PRIMITIVE_LINES);
        
        for(int i = 0; i < countour.size(); i++){
            contourMesh.addVertex(countour[i]);
        }
        contourMesh.draw();
    }
}
*/
