//
//  BlobTracker.cpp
//  kinectTCPServer
//
//  Created by maybites on 14.02.14.
//
//

#include "BlobTracker.h"

BlobTracker::BlobTracker(int _ID, ofRectangle _rect, int _liveSpan){
	mID = _ID;
    baseRectangle2d = _rect;
    headBlob.setResolution(1, 1);
	mBreathSize = _liveSpan;
	mCountDown = mBreathSize;
    mIsDying = false;
	mLifeCycles = ofGetElapsedTimeMillis();
	mHasBeenMatched = false;
}

void BlobTracker::updatePrepare()
{
	hasBodyUpdated = false;
	hasHeadUpdated = false;
	mHasBeenMatched = false;
}

bool BlobTracker::isMatching(ofRectangle _rect, int maxDistance){
	if (!mIsDying) {
		if (baseRectangle2d.inside(_rect.getCenter()) || glm::distance(baseRectangle2d.getCenter(), _rect.getCenter()) < maxDistance) {
			return true;
		}
	}
    return false;
}

bool BlobTracker::hasBeenUpdated()
{
	return mHasBeenMatched;
}

void BlobTracker::update(ofRectangle _rect, glm::vec3 _headBlobCenter, glm::vec2 _headBlobSize, glm::vec3 _eyeCenter, float _smoothPos) {
	baseRectangle2d = _rect;
	headBlobCenter = (1 - _smoothPos) * _headBlobCenter + headBlobCenter * _smoothPos;
	headBlobSize = (1 - _smoothPos) * _headBlobSize + headBlobSize * _smoothPos;
	eyeCenter = (1 - _smoothPos) * _eyeCenter + eyeCenter * _smoothPos;
	eyeGaze = glm::normalize(headCenter - eyeCenter);
	mCountDown = ofGetElapsedTimeMillis() + mBreathSize;
	mHasBeenMatched = true;
}

bool BlobTracker::isAlive()
{
	checkForDisposal();
	return !mIsDying;
}

bool BlobTracker::isDying() {
	return mIsDying;
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

void BlobTracker::dispose() {
	mIsDying = true;
	mCountDown = -1;
}

ofVec3f BlobTracker::getCurrentHeadCenter(){
    return headCenter;
}

int BlobTracker::getElapsedMillis()
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
        bodyHeadTopSphere.setRadius(20);
        bodyHeadTopSphere.setPosition(headTop.x, headTop.y, headTop.z);
        bodyHeadTopSphere.drawWireframe();
    }
}

void BlobTracker::drawHeadBlob(){
    if(hasBeenUpdated()){
        headBlob.set(headBlobSize.x, headBlobSize.y);
        headBlob.setPosition(headBlobCenter);
        headBlob.drawWireframe();
        
        headCenterSphere.setRadius(20);
        headCenterSphere.setPosition(headCenter);
        headCenterSphere.draw();
    }
}

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
