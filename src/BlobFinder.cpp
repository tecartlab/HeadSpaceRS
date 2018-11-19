//
//  BlobFinder.cpp
//  kinectTCPServer
//
//  Created by maybites on 14.02.14.
//
//

#include "BlobFinder.h"

void BlobFinder::setup(ofxGui &gui){

    //////////
    //GUI   //
    //////////
        
    panel = gui.addPanel();
    
    panel->loadTheme("theme/theme_light.json");
    panel->setName("Tracking...");

	sensorFboSize.addListener(this, &BlobFinder::allocate);

	blobSmoothGroup = panel->addGroup("Quality");
	blobSmoothGroup->add<ofxGuiIntInputField>(sensorFboSize.set("sensorFboSize", 1, 1, 3));
	blobSmoothGroup->add(smoothFactor.set("Smoothing", 0.5, 0., 1.));
	blobSmoothGroup->add(eventBreathSize.set("BreathSize", 2000, 0, 5000));
	blobSmoothGroup->add(eventMaxSize.set("MaxDistance", 400, 0, 1000));

	sensorBoxLeft.addListener(this, &BlobFinder::updateSensorBox);
	sensorBoxRight.addListener(this, &BlobFinder::updateSensorBox);
    sensorBoxFront.addListener(this, &BlobFinder::updateSensorBox);
    sensorBoxBack.addListener(this, &BlobFinder::updateSensorBox);
    sensorBoxTop.addListener(this, &BlobFinder::updateSensorBox);
    sensorBoxBottom.addListener(this, &BlobFinder::updateSensorBox);
    
	sensorBoxGuiGroup = panel->addGroup("SensorBox");
	sensorBoxGuiGroup->add(useMask.set("Use Mask", false));
	sensorBoxGuiGroup->add<ofxGuiIntInputField>(sensorBoxLeft.set("left", 1000));
	sensorBoxGuiGroup->add<ofxGuiIntInputField>(sensorBoxRight.set("right", -1000));
    sensorBoxGuiGroup->add<ofxGuiIntInputField>(sensorBoxFront.set("front", 1000));
    sensorBoxGuiGroup->add<ofxGuiIntInputField>(sensorBoxBack.set("back", -1000));
    sensorBoxGuiGroup->add<ofxGuiIntInputField>(sensorBoxTop.set("top", 2000));
    sensorBoxGuiGroup->add<ofxGuiIntInputField>(sensorBoxBottom.set("bottom", 1000));

	blobGuiGroup = panel->addGroup("Blobs");
	blobGuiGroup->add(blobAreaMax.set("AreaMax", 5, 0, 255));
	blobGuiGroup->add(blobAreaMinStp1.set("AreaMinStp1", 10, 0, 255));
	blobGuiGroup->add(blobAreaMinStp2.set("AreaMinStp2", 10, 0, 255));
	blobGuiGroup->add(countBlob.set("MaxBlobs", 5, 1, N_MAX_BLOBS));
 
	blobEyeGroup = panel->addGroup("Gazing");
	blobEyeGroup->add(useGazePoint.set("Use gaze point", true));
    blobEyeGroup->add(gazePoint.set("Gaze point", ofVec3f(0, 0, 1500), ofVec3f(-2000, 0, 0), ofVec3f(2000, 5000, 3000)));
    blobEyeGroup->add(eyeLevel.set("EyeLevel", 140, 0, 200));
    blobEyeGroup->add(eyeInset.set("EyeInset", .8, 0, 1));
    
    panel->loadFromFile("trackings.xml");

}

void BlobFinder::allocate(int &value){
	captureScreenSize = ofVec2f(pow(2, 8 + value), pow(2, 8 + value));
	ofLog(OF_LOG_NOTICE, "set capture fbo size to = " + ofToString(pow(2, 8 + value)));
	gazePointer.setRadius(1000);

    fbopixels.allocate(captureScreenSize.x, captureScreenSize.y, OF_PIXELS_RGB);
    
	colorImg.allocate(captureScreenSize.x, captureScreenSize.y);
    
	blobRef.allocate(captureScreenSize.x, captureScreenSize.y);
	grayImage.allocate(captureScreenSize.x, captureScreenSize.y);
	grayEyeLevel.allocate(captureScreenSize.x, captureScreenSize.y);
	grayThreshFar.allocate(captureScreenSize.x, captureScreenSize.y);
	
	nearThreshold = 230;
	farThreshold = 70;
	bThreshWithOpenCV = true;
    
    ofFbo::Settings s;
    s.width             = captureScreenSize.x;
    s.height			= captureScreenSize.y;
    s.internalformat    = GL_RGB;
    s.useDepth			= true;
    // and assigning this values to the fbo like this:
    captureFBO.allocate(s);

	maskFbo.allocate(s);
	fbo.allocate(s);

	loadMask();

	// Let's clear the FBO's
	// otherwise it will bring some junk with it from the memory    

	fbo.begin();
	ofClear(0, 0, 0, 255);
	fbo.end();


#ifdef TARGET_OPENGLES
	maskShader.load("shaders_gles/alphamask.vert", "shaders_gles/alphamask.frag");
#else
	if (ofIsGLProgrammableRenderer()) {
		string vertex = "#version 150\n\
    	\n\
		uniform mat4 projectionMatrix;\n\
		uniform mat4 modelViewMatrix;\n\
    	uniform mat4 modelViewProjectionMatrix;\n\
    	\n\
    	\n\
    	in vec4  position;\n\
    	in vec2  texcoord;\n\
    	\n\
    	out vec2 texCoordVarying;\n\
    	\n\
    	void main()\n\
    	{\n\
	        texCoordVarying = texcoord;\
    		gl_Position = modelViewProjectionMatrix * position;\n\
    	}";
		string fragment = "#version 150\n\
		\n\
		uniform sampler2DRect tex0;\
		uniform sampler2DRect maskTex;\
        in vec2 texCoordVarying;\n\
		\
        out vec4 fragColor;\n\
		void main (void){\
		vec2 pos = texCoordVarying;\
		\
		vec3 src = texture(tex0, pos).rgb;\
		float mask = texture(maskTex, pos).r;\
		\
		fragColor = vec4( src , mask);\
		}";
		maskShader.setupShaderFromSource(GL_VERTEX_SHADER, vertex);
		maskShader.setupShaderFromSource(GL_FRAGMENT_SHADER, fragment);
		maskShader.bindDefaults();
		maskShader.linkProgram();
	}
	else {
		string shaderProgram = "#version 120\n \
		#extension GL_ARB_texture_rectangle : enable\n \
		\
		uniform sampler2DRect tex0;\
		uniform sampler2DRect maskTex;\
		\
		void main (void){\
		vec2 pos = gl_TexCoord[0].st;\
		\
		vec3 src = texture2DRect(tex0, pos).rgb;\
		float mask = texture2DRect(maskTex, pos).r;\
		\
		gl_FragColor = texture2DRect(tex0, pos) * mask;\
		}";
		maskShader.setupShaderFromSource(GL_FRAGMENT_SHADER, shaderProgram);
		maskShader.linkProgram();
	}
#endif
}

void BlobFinder::captureBegin(){
    captureFBO.begin();
    ofClear(0, 0, 0, 0);
    captureCam.scale = 1;
    // FBO capturing
    captureCam.begin(ofRectangle(0, 0, captureScreenSize.x, captureScreenSize.y), sensorBoxLeft.get() * SCALE, sensorBoxRight.get() * SCALE, sensorBoxBack.get() * SCALE, sensorBoxFront.get() * SCALE, - sensorBoxTop.get() * SCALE, sensorBoxTop.get() * SCALE);
    
}

void BlobFinder::captureEnd(){
    captureCam.end();
    captureFBO.end();
}


void BlobFinder::clearMask() {
	maskFbo.begin();
	ofClear(255, 255, 255, 255);
	maskFbo.end();
	ofPixels maskFBOPixels;
	maskFbo.readToPixels(maskFBOPixels);
	maskImg.setFromPixels(maskFBOPixels);
}

void BlobFinder::captureMaskBegin() {
	maskFbo.begin();
	//ofClear(255, 255, 255, 255);
	captureCam.scale = 1;
	// FBO capturing
	captureCam.begin(ofRectangle(0, 0, captureScreenSize.x, captureScreenSize.y), sensorBoxLeft.get() * SCALE, sensorBoxRight.get() * SCALE, sensorBoxBack.get() * SCALE, sensorBoxFront.get() * SCALE, -sensorBoxTop.get() * SCALE, sensorBoxTop.get() * SCALE);
}

void BlobFinder::captureMaskEnd() {
	captureCam.end();
	maskFbo.end();

	ofPixels maskFBOPixels;
	maskFbo.readToPixels(maskFBOPixels);
	maskImg.setFromPixels(maskFBOPixels);
}

void BlobFinder::saveMask() {
	maskImg.save("mask.png");
}

void BlobFinder::loadMask() {
	maskImg.allocate(captureScreenSize.x, captureScreenSize.y, OF_IMAGE_COLOR_ALPHA);
	maskImg.load("mask.png");
	maskFbo.begin();
	maskImg.draw(0, 0);
	maskFbo.end();
}

void BlobFinder::update(){
	ofColor white = ofColor::white;
	ofColor black = ofColor::black;

	/****************************************************************
              	      PREPARE NEW FRAME
	*****************************************************************/

	int minID = 0;

	//tells all the blobs that the next frame is comming
	for (int e = 0; e < blobEvents.size(); e++) {
		blobEvents[e].updatePrepare();
		minID = (blobEvents[e].mID >= minID) ? blobEvents[e].mID + 1 : minID;
	}

	if (useMask.get()) {
		fbo.begin();
		// Cleaning everthing with alpha mask on 0 in order to make it transparent for default
		ofClear(0, 0, 0, 0);

		maskShader.begin();
		maskShader.setUniformTexture("maskTex", maskImg.getTexture(), 1);

		captureFBO.draw(0, 0);

		maskShader.end();
		fbo.end();

		fbo.readToPixels(fbopixels);
	}
	else {
		captureFBO.readToPixels(fbopixels);
	}

    colorImg.setFromPixels(fbopixels);

    // load grayscale captured depth image from the color source
    grayImage.setFromColorImage(colorImg);
    
    //grayImage.blurHeavily();
    
	ofPixelsRef blobRefPixls = blobRef.getPixels();
    
    ofPixelsRef greyref = grayImage.getPixels();
	ofPixelsRef eyeRef = grayEyeLevel.getPixels();
	
	eyeRef.setColor(black);

    float sensorFieldFront = sensorBoxFront.get() * SCALE;
    float sensorFieldBack = sensorBoxBack.get() * SCALE;
    float sensorFieldLeft = sensorBoxLeft.get() * SCALE;
    float sensorFieldRight = sensorBoxRight.get() * SCALE;
    float sensorFieldTop = sensorBoxTop .get() * SCALE;
    float sensorFieldBottom = sensorBoxBottom.get() * SCALE;
    float sensorFieldWidth = sensorFieldRight - sensorFieldLeft;
    float sensorFieldHeigth = sensorFieldTop - sensorFieldBottom;
    float sensorFieldDepth = sensorFieldBack - sensorFieldFront;
    
    int eyeLevelColor = eyeLevel.get() / sensorFieldHeigth * 255;
    
    int headTopThreshold = eyeLevelColor / 4;
    
    //ofLog(OF_LOG_NOTICE, "eyeLevelColor = " + ofToString(eyeLevelColor));
    
    //ofLog(OF_LOG_NOTICE, "eyref size : " + ofToString(eyeRef.size()));
  
	/****************************************************************
					  FIND BODY CONTOURS
	*****************************************************************/

	int minBlobSize = pow(blobAreaMinStp1.get() * sensorFboSize.get(), 2);
	int maxBlobSize = pow(blobAreaMax.get() * sensorFboSize.get(), 2);


	detectedHeads.clear();

	contourFinder.findContours(grayImage, minBlobSize, maxBlobSize, countBlob.get(), false);

    for (int i = 0; i < contourFinder.nBlobs; i++){
        ofRectangle bounds = contourFinder.blobs[i].boundingRect;
        float pixelBrightness = 0;
        float brightness = 0;
        
        // find the brightest pixel within the blob. this defines the height of the blob
        for(int x = bounds.x; x < bounds.x + bounds.width; x++){
            for(int y = bounds.y; y < bounds.y + bounds.height; y++){
                pixelBrightness = greyref.getColor(x, y).getBrightness();
                brightness = (pixelBrightness > brightness)?pixelBrightness: brightness;
            }
        }

		/*
		float averageBrightness = 0;
        int averageCounter = 0;

        // go through the pixels again and get the average brightness for the headTopThreshold
        for(int x = bounds.x; x < bounds.x + bounds.width; x++){
            for(int y = bounds.y; y < bounds.y + bounds.height; y++){
                pixelBrightness = greyref.getColor(x, y).getBrightness();
                if(pixelBrightness > brightness - headTopThreshold){
                    averageBrightness += pixelBrightness;
                    averageCounter++;
                }
            }
        }
        
        brightness = averageBrightness / averageCounter;
        */
        
        // find all the pixels down to the eyelevel threshold. this yealds an image with blobs that mark the size of the head at eyelevel.
        ofVec2f headtop2d = ofVec2f();
        int brighCounter = 0;
        for(int x = bounds.x; x < bounds.x + bounds.width; x++){
            for(int y = bounds.y; y < bounds.y + bounds.height; y++){
                pixelBrightness = greyref.getColor(x, y).getBrightness();
                if(pixelBrightness > (brightness - eyeLevelColor)){
                    //writes the pixels above the eyelevel into the eyeRef image
                    eyeRef.setColor(x, y, brightness);
                }else{
                    eyeRef.setColor(x, y, black);
                }
                if(pixelBrightness >= brightness - (eyeLevelColor / 4)){
                    headtop2d += ofVec2f(x, y);
                    brighCounter++;
                }
            }
        }
        headtop2d /= brighCounter;
        
        //ofLog(OF_LOG_NOTICE, "headtop2d = " + ofToString(headtop2d));
        		
		ofVec3f headTop = ofVec3f((headtop2d.x / captureScreenSize.x) * sensorFieldWidth + sensorFieldLeft, sensorFieldBack - (headtop2d.y / captureScreenSize.y ) * sensorFieldDepth, (brightness / 255.0) * sensorFieldHeigth + sensorFieldBottom);

        //ofVec3f headCenter = ofVec3f(headTop.x, headTop.y, headTop.z - eyeLevel.get());

		BodyBlob newBodyBlob;
		newBodyBlob.bound = bounds;
		newBodyBlob.headTop = headTop;	
		newBodyBlob.hasBeenTaken = false;

		detectedHeads.push_back(newBodyBlob);

	}

	/****************************************************************
		  UPDATE BODY EVENTS
	*****************************************************************/

	int matchBlobID = -1;
	int matchEventID = -1;
	float minDistance = 1000000;

	do {
		matchBlobID = -1;
		matchEventID = -1;
		minDistance = 1000000;
		for (int i = 0; i < blobEvents.size(); i++) {											// iterate through all the current events
			if (blobEvents[i].isAlive() && !blobEvents[i].hasBeenUpdated()) {					//   all those that haven't been updated yet 
				for (int j = 0; j < detectedHeads.size(); j++) {								// now we go through all the new blobs
					if (!detectedHeads[j].hasBeenTaken) {										//   but only if the blobs havent been taken before
						float dist = blobEvents[i].getDistance(detectedHeads[j].headTop);
						if (dist < eventMaxSize.get()*SCALE) {										//   and test if the distance is within the threshold
							if (minDistance > dist) {
								minDistance = dist;
								matchBlobID = j;
								matchEventID = i;
							}
						}
					}
				}
			}
		}
		if (matchEventID >= 0) {
			ofRectangle bounds = detectedHeads[matchBlobID].bound;
			ofVec3f headTop = detectedHeads[matchBlobID].headTop;

			//calculate the blob pos in worldspace
			ofVec3f blobPos = ofVec3f(((float)bounds.getCenter().x / captureScreenSize.x) * sensorFieldWidth + sensorFieldLeft, sensorFieldBack - ((float)bounds.getCenter().y / captureScreenSize.y) * sensorFieldDepth, headTop.z);

			//calculate the blob size in worldspace
			ofVec2f blobSize = ofVec2f(((float)bounds.getWidth() / captureScreenSize.x) * sensorFieldWidth, ((float)bounds.getHeight() / captureScreenSize.y) * sensorFieldDepth);

			//ofLogVerbose("Updating old event with ID: " + ofToString(blobEvents[matchEventID].mID));

			blobEvents[matchEventID].update(bounds, blobPos, blobSize, headTop, smoothFactor.get());
			detectedHeads[matchBlobID].hasBeenTaken = true;
		}
	} while (matchEventID >= 0);

	/****************************************************************
		  CREATE NEW BODY EVENTS
	*****************************************************************/
	for (int j = 0; j < detectedHeads.size(); j++) {
		if (!detectedHeads[j].hasBeenTaken) {			
			ofRectangle bounds = detectedHeads[j].bound;
			ofVec3f headTop = detectedHeads[j].headTop;

			//calculate the blob pos in worldspace
			ofVec3f blobPos = ofVec3f(((float)bounds.getCenter().x / captureScreenSize.x) * sensorFieldWidth + sensorFieldLeft, sensorFieldBack - ((float)bounds.getCenter().y / captureScreenSize.y) * sensorFieldDepth, headTop.z);

			//calculate the blob size in worldspace
			ofVec2f blobSize = ofVec2f(((float)bounds.getWidth() / captureScreenSize.x) * sensorFieldWidth, ((float)bounds.getHeight() / captureScreenSize.y) * sensorFieldDepth);

			//ofLogVerbose("Creating new event with ID: " + ofToString(minID));

			blobEvents.push_back(BlobTracker(minID++, eventBreathSize.get(), bounds, blobPos, blobSize, headTop));
		}
	}


	/****************************************************************
				  FIND HEAD CONTOURS
	*****************************************************************/
	/**

    //preprocesses the eyeRef image
    //grayEyeLevel.setFromPixels(eyeRef.getPixels(), eyeRef.getWidth(), eyeRef.getHeight());
    //grayEyeLevel.invert();
    //grayEyeLevel.threshold(20);
    //grayEyeLevel.invert();
    grayEyeLevel.blurGaussian();

    //ofLog(OF_LOG_NOTICE, "contourEyeFinder nBlobs : " + ofToString(contourEyeFinder.nBlobs));

	int minBlobSize2 = pow(blobAreaMinStp2.get() * sensorFboSize.get(), 2);

    //find head shape on eye height contours
    contourEyeFinder.findContours(grayEyeLevel, minBlobSize2, maxBlobSize, countBlob.get(), false);
    for(int i = 0; i < contourEyeFinder.nBlobs; i++){

		ofRectangle bounds = contourEyeFinder.blobs[i].boundingRect;

		int brightness = eyeRef.getColor((float)bounds.getCenter().x, (float)bounds.getCenter().y).getBrightness();
		float height = (brightness / 255.0) * sensorFieldHeigth + sensorFieldBottom;

		//calculate the blob pos in worldspace
		ofVec3f headBlobCenter = ofVec3f(((float)bounds.getCenter().x / captureScreenSize.x) * sensorFieldWidth + sensorFieldLeft, sensorFieldBack - ((float)bounds.getCenter().y / captureScreenSize.y) * sensorFieldDepth, height);

		//calculate the blob size in worldspace
		ofVec2f headBlobSize = ofVec2f(((float)bounds.getWidth() / captureScreenSize.x) * sensorFieldWidth, ((float)bounds.getHeight() / captureScreenSize.y) * sensorFieldDepth);

		//calculate the gazeVector
		//ofVec3f gaze = blobEvents[bid].getCurrentHeadCenter() - gazePoint.get();

		//gaze.z = 0;

		float smalestAngle = 180;
		ofVec3f eyePoint = headBlobCenter;

		//clears the contour storage
		blobEvents[bid].countour.clear();

		// findes the closest contour point to the eyegave-vector, takes its distance to the headCenter and calculated
		// the eye - center - point
		for (int v = 0; v < contourEyeFinder.blobs[i].pts.size(); v++) {
			ofVec3f headPoint = ofVec3f(((float)contourEyeFinder.blobs[i].pts[v].x / captureScreenSize.x) * sensorFieldWidth + sensorFieldLeft, sensorFieldBack - ((float)contourEyeFinder.blobs[i].pts[v].y / captureScreenSize.y) * sensorFieldDepth, blobEvents[bid].headCenter.z);

			blobEvents[bid].countour.push_back(headPoint);

			ofVec3f gaze2 = blobEvents[bid].getCurrentHeadCenter() - headPoint;

			float angle = gaze.angle(gaze2);

			if (smalestAngle > angle) {
				smalestAngle = angle;
				eyePoint = blobEvents[bid].getCurrentHeadCenter() - gaze.normalize().scale(gaze2.length() * eyeInset.get());
			}
		}

		bool foundBlob = false;

        for(int bid = 0; bid < blobEvents.size(); bid++){
            // find the blob
            if(!blobEvents[bid].hasBeenUpdated() && blobEvents[bid].isMatching(bounds, eventMaxSize.get())){
               
				/////////////////////////////////////////////////////////////////
						  UPDATE HEAD EVENTS
				/////////////////////////////////////////////////////////////////

				ofLogVerbose("Updating old event with ID: " + ofToString(blobEvents[bid].mID) + " headHight = " + ofToString(headBlobCenter.z));

				blobEvents[bid].update(bounds, headBlobCenter, headBlobSize, eyePoint, smoothFactor.get());
				foundBlob = true;
				break;
            }
        }
		// if none is found, create a new one.
		if (!foundBlob) {
			ofLogVerbose("Creating new event with ID: " + ofToString(minID));
			blobEvents.push_back(BlobTracker(minID++, bounds, eventBreathSize.get()));
			blobEvents.back().update(bounds, headBlobCenter, headBlobSize, eyePoint, smoothFactor.get());
		}

    }
	*/

	/****************************************************************
		  SORT EVENTS
	*****************************************************************/

	/*
	//sets the sort value to the current index.
	int sortPos = 0;

	for (int i = 0; i < blobEvents.size(); i++) {
		blobEvents[i].sortPos = sortPos++;
	}

	// if we are using the gaze point
	if (useGazePoint.get()) {
		if (blobEvents.size() > 0) {
			for (int i = 0; i < (blobEvents.size() - 1); i++) {
				for (int j = 1; j < blobEvents.size(); j++) {
					if ((blobEvents[i].headCenter - gazePoint.get()).length() < (blobEvents[j].headCenter - gazePoint.get()).length()) {
						if (blobEvents[i].sortPos > blobEvents[j].sortPos) {
							int savepos = blobEvents[j].sortPos;
							blobEvents[j].sortPos = blobEvents[i].sortPos;
							blobEvents[i].sortPos = savepos;
						}
					}
					else {
						if (blobEvents[i].sortPos < blobEvents[j].sortPos) {
							int savepos = blobEvents[j].sortPos;
							blobEvents[j].sortPos = blobEvents[i].sortPos;
							blobEvents[i].sortPos = savepos;
						}
					}
				}
			}
		}
	}
	*/

 
	/****************************************************************
					CLEANUP EVENTS
	*****************************************************************/

    //updates all alive blobs and removes all the blobs that havent had an update for a specific number of frames or have been killed
    for(int e = blobEvents.size() - 1; e >= 0; e--){
        if(blobEvents[e].isDead()){
			blobEvents.erase(blobEvents.begin() + e);
        }
    }
}

void BlobFinder::drawSensorBox()
{
	sensorBox.draw();
}

void BlobFinder::drawBodyBlobs2d(ofRectangle _rect){
    float xFactor = _rect.width / captureScreenSize.x;
    float yFactor = _rect.height / captureScreenSize.y;
    
    ofNoFill();
    for(int i = 0; i < blobEvents.size(); i++){
		if (blobEvents[i].hasBeenUpdated() && blobEvents[i].isActive()) {
			ofSetColor(255, 0, 0, 255);
			ofDrawRectangle(_rect.x + blobEvents[i].baseRectangle2d.x * xFactor, _rect.y + blobEvents[i].baseRectangle2d.y * yFactor, blobEvents[i].baseRectangle2d.width * xFactor, blobEvents[i].baseRectangle2d.height * yFactor);
			ofDrawBitmapString("blob[" + ofToString(blobEvents[i].mID) + "]\n alive = " + ofToString(blobEvents[i].getAgeInMillis()) + "\n sort = " + ofToString(blobEvents[i].sortPos) + "\n x = " + ofToString(blobEvents[i].headTop.x) + "\n y = " + ofToString(blobEvents[i].headTop.y) + "\n z = " + ofToString(blobEvents[i].headTop.z), _rect.x + blobEvents[i].baseRectangle2d.getCenter().x * xFactor, _rect.y + blobEvents[i].baseRectangle2d.getCenter().y * yFactor);
		}
        
    }
}

void BlobFinder::drawBodyBlobsBox(){
    for(int i = 0; i < blobEvents.size(); i++){
        //ofLog(OF_LOG_NOTICE, "blob[" + ofToString(i) + "] box =" + ofToString(blobEvents[i].bodyCenter));
        blobEvents[i].drawBodyBox();
    }
}

void BlobFinder::drawBodyBlobsHeadTop(){
    for(int i = 0; i < blobEvents.size(); i++){
        blobEvents[i].drawHeadTop();
    }
}

void BlobFinder::drawGazePoint(){
	if (useGazePoint.get()) {
		//gazePointer.setPosition(gazePoint.get());
		//gazePointer.ofNode::draw();
		ofDrawSphere(gazePoint.get().x, gazePoint.get().y, gazePoint.get().z, 50);
		ofDrawLine(gazePoint.get().x, gazePoint.get().y, 0, gazePoint.get().x, gazePoint.get().y, 3000);
	}
}

bool BlobFinder::hasParamUpdate(){
    if(parameterHasUpdated){
        parameterHasUpdated = false;
        return true;
    }
    return false;
}



void BlobFinder::updateSensorBox(int & value){
    sensorBox.clear();
    sensorBox.setMode(OF_PRIMITIVE_LINES);
    
    sensorBox.addVertex(ofPoint(sensorBoxLeft.get() * SCALE, sensorBoxFront.get() * SCALE, sensorBoxBottom.get() * SCALE));
    sensorBox.addVertex(ofPoint(sensorBoxRight.get() * SCALE, sensorBoxFront.get() * SCALE, sensorBoxBottom.get() * SCALE));
    
    sensorBox.addVertex(ofPoint(sensorBoxRight.get() * SCALE, sensorBoxFront.get() * SCALE, sensorBoxBottom.get() * SCALE));
    sensorBox.addVertex(ofPoint(sensorBoxRight.get() * SCALE, sensorBoxBack.get() * SCALE, sensorBoxBottom.get() * SCALE));
    
    sensorBox.addVertex(ofPoint(sensorBoxRight.get() * SCALE, sensorBoxBack.get() * SCALE, sensorBoxBottom.get() * SCALE));
    sensorBox.addVertex(ofPoint(sensorBoxLeft.get() * SCALE, sensorBoxBack.get() * SCALE, sensorBoxBottom.get() * SCALE));
    
    sensorBox.addVertex(ofPoint(sensorBoxLeft.get() * SCALE, sensorBoxBack.get() * SCALE, sensorBoxBottom.get() * SCALE));
    sensorBox.addVertex(ofPoint(sensorBoxLeft.get() * SCALE, sensorBoxFront.get() * SCALE, sensorBoxBottom.get() * SCALE));
    
    sensorBox.addVertex(ofPoint(sensorBoxLeft.get() * SCALE, sensorBoxFront.get() * SCALE, sensorBoxTop.get() * SCALE));
    sensorBox.addVertex(ofPoint(sensorBoxRight.get() * SCALE, sensorBoxFront.get() * SCALE, sensorBoxTop.get() * SCALE));
    
    sensorBox.addVertex(ofPoint(sensorBoxRight.get() * SCALE, sensorBoxFront.get() * SCALE, sensorBoxTop.get() * SCALE));
    sensorBox.addVertex(ofPoint(sensorBoxRight.get() * SCALE, sensorBoxBack.get() * SCALE, sensorBoxTop.get() * SCALE));
    
    sensorBox.addVertex(ofPoint(sensorBoxRight.get() * SCALE, sensorBoxBack.get() * SCALE, sensorBoxTop.get() * SCALE));
    sensorBox.addVertex(ofPoint(sensorBoxLeft.get() * SCALE, sensorBoxBack.get() * SCALE, sensorBoxTop.get() * SCALE));
    
    sensorBox.addVertex(ofPoint(sensorBoxLeft.get() * SCALE, sensorBoxBack.get() * SCALE, sensorBoxTop.get() * SCALE));
    sensorBox.addVertex(ofPoint(sensorBoxLeft.get() * SCALE, sensorBoxFront.get() * SCALE, sensorBoxTop.get() * SCALE));
    
    //captureCam.setPosition((sensorBoxLeft.get() * SCALE + sensorBoxRight.get() * SCALE)/2, (sensorBoxBack.get() * SCALE + sensorBoxBack.get() * SCALE)/2, sensorBoxTop.get() * SCALE);
    //captureCam.setPosition(5, 5, 0);
    //captureCam.
    parameterHasUpdated = true;
}
