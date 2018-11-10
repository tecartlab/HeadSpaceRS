//
//  BlobTracker.h
//  kinectTCPServer
//
//  Created by maybites on 14.02.14.
//
//
#pragma once

#include "ofMain.h"
#include "ofxOpenCv.h"
#include "ofVec3f.h"
#include "ofxGuiExtended.h"
#include "ofConstants.h"
#include "Planef.h"
#include "Linef.h"
#include "OrthoCamera.h"

#include "BlobTracker.h"

#include <cmath>

#define N_MAX_BLOBS 30
#define SCALE 0.001

struct BodyBlob {
	glm::vec3 headTop;
	ofRectangle bound;
	bool hasBeenTaken;
};

class BlobFinder {
    
public:
    void setup(ofxGui &gui);
    void allocate(int &value);
    
	void captureBegin();
	void captureEnd();

	void captureMaskBegin();
	void captureMaskEnd();

	void clearMask();

	void loadMask();
	void saveMask();

    void update();
    bool hasParamUpdate();

    void updateSensorBox(int & value);
    
    void drawSensorBox();
    void drawBodyBlobs2d(ofRectangle _rect);

    void drawBodyBlobsBox();
    void drawBodyBlobsHeadTop();
    
    void drawGazePoint();

    vector <BlobTracker> blobEvents;
	vector <BodyBlob> detectedHeads;

    ofVec3f kinectPos;
    
    ofPixels greyRef_store1;
    ofPixels greyRef_store2;
    ofPixels greyRef_store3;

    ///////////////////
    // FBO CAPTURING //
    ///////////////////
    orthoCamera captureCam;
    
    //ofImage capturedImage;
    ofFbo captureFBO;

	ofFbo       maskFbo;
	ofFbo       fbo;

	ofShader    maskShader;

	ofImage     maskImg;
	ofImage     brushImg;


    /////////////////
    //COLOR CONTOUR//
    /////////////////
        
    ofPixels fbopixels;
    
    ofxCvColorImage colorImg;
    
	ofxCvGrayscaleImage blobRef; // body blob reference image
	ofxCvGrayscaleImage grayImage; // grayscale depth image
    ofxCvGrayscaleImage grayEyeLevel; // the eyelevel thresholded image
    ofxCvGrayscaleImage grayThreshFar; // the far thresholded image
 
    ofxCvContourFinder contourFinder;
    ofxCvContourFinder contourEyeFinder;
    
    bool bThreshWithOpenCV;
    
    int nearThreshold;
    int farThreshold;
    
    //////////////
    //PROPERTIES//
    //////////////

    bool parameterHasUpdated;

    ofxGuiPanel *panel;
	ofxGuiGroup *blobSmoothGroup;
	ofxGuiGroup *sensorBoxGuiGroup;
	ofxGuiGroup *blobGuiGroup;
	ofxGuiGroup *blobEyeGroup;

    ofParameter<int> sensorBoxLeft;
    ofParameter<int> sensorBoxRight;
    ofParameter<int> sensorBoxTop;
    ofParameter<int> sensorBoxBottom;
    ofParameter<int> sensorBoxFront;
    ofParameter<int> sensorBoxBack;
    ofParameter<int> nearFrustum;
    ofParameter<int> farFrustum;
       
	ofParameter<int> blobAreaMinStp2;
	ofParameter<int> blobAreaMinStp1;
	ofParameter<int> blobAreaMax;
    ofParameter<int> countBlob;

    ofParameter<float> eyeLevel;
    ofParameter<float> eyeInset;

	ofParameter<bool> useGazePoint;

    ofParameter<ofVec3f> gazePoint;
    ofSpherePrimitive gazePointer;

	ofParameter<int> eventBreathSize;
	ofParameter<float> eventMaxSize;
	ofParameter<float> smoothFactor;

	ofParameter<int> sensorFboSize;

	ofParameter<bool> useMask;

	ofVec2f captureScreenSize;

    ofVboMesh sensorBox;
   
    ofVec3f normal;
    float p;

};


