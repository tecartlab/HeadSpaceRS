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
	mCountDown--;
}

bool BlobTracker::isMatching(ofRectangle _rect, int maxDistance){
	if (!mIsDying) {
		if (baseRectangle2d.inside(_rect.getCenter()) || glm::distance(baseRectangle2d.getCenter(), _rect.getCenter()) < maxDistance) {
			mCountDown = mBreathSize;
			baseRectangle2d = _rect;
			return true;
		}
	}
    return false;
}

void BlobTracker::setAsMatched()
{
	mHasBeenMatched = true;
}

bool BlobTracker::hasBeenMatched()
{
	return mHasBeenMatched;
}

void BlobTracker::updateBody(ofRectangle _rect, glm::vec3 _bodyBlobCenter, glm::vec2 _bodyBlobSize, glm::vec3 _headTop, glm::vec3 _headCenter, float _eyelevel, float _smoothPos){
 	bodyBlobCenter = (1 - _smoothPos) * _bodyBlobCenter + bodyBlobCenter * _smoothPos;
	bodyBlobSize = (1 - _smoothPos) * _bodyBlobSize + bodyBlobSize * _smoothPos;
	headTop = (1 - _smoothPos) * _headTop + headTop * _smoothPos;
	headCenter = (1 - _smoothPos) * _headCenter + headCenter * _smoothPos;
	eyeLevel = _eyelevel;
	hasBodyUpdated = true;
}

void BlobTracker::updateHead(glm::vec3 _headBlobCenter, glm::vec2 _headBlobSize, glm::vec3 _eyeCenter, float _smoothPos) {
	headBlobCenter = (1 - _smoothPos) * _headBlobCenter + headBlobCenter * _smoothPos;
	headBlobSize = (1 - _smoothPos) * _headBlobSize + headBlobSize * _smoothPos;
	eyeCenter = (1 - _smoothPos) * _eyeCenter + eyeCenter * _smoothPos;
	eyeGaze = glm::normalize(headCenter - eyeCenter);
	hasHeadUpdated = true;
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
	if (mCountDown == 0) {
		mIsDying = true;
	}
	return (mCountDown < 0) ? true : false;
}

void BlobTracker::dispose() {
	mIsDying = true;
	mCountDown = -1;
}

bool BlobTracker::hasBeenUpdated()
{
	return hasHeadUpdated && hasBodyUpdated;
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
        bodyBox.set(bodyBlobSize.x, bodyBlobSize.y, bodyBlobCenter.z);
        bodyBox.setPosition(bodyBlobCenter.x, bodyBlobCenter.y, bodyBlobCenter.z / 2);
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
