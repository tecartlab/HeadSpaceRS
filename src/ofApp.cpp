#include "ofApp.h"

#define RECONNECT_TIME 400

#define DEPTH_X_RES 640
#define DEPTH_Y_RES 480


//--------------------------------------------------------------
void ofApp::setup(){
#ifdef TARGET_OPENGLES
	shader.load("shadersES2/shader");
#else
	if (ofIsGLProgrammableRenderer()) {
		shader.load("shadersGL3/shader");
	}
	else {
		shader.load("shadersGL2/shader");
	}
#endif

	ofLog(OF_LOG_NOTICE, "MainAPP: looking for RealSense Device...");

	ofSetLogLevel(OF_LOG_VERBOSE);

	realSense = RSDevice::createUniquePtr();

	realSense->checkConnectedDialog();

	//realSense->hardwareReset();

	realSense->setVideoSize(REALSENSE_VIDEO_WIDTH, REALSENSE_VIDEO_HEIGHT);

	ofLog(OF_LOG_NOTICE, "... RealSense Device found.");

	// we don't want to be running to fast
	//ofSetVerticalSync(true);
	//ofSetFrameRate(30);
    
	/////////////////////////////
	//     DEFINE VIEWPORTS    //
	/////////////////////////////

	float xOffset = VIEWGRID_WIDTH; //ofGetWidth() / 3;
	float yOffset = VIEWPORT_HEIGHT / N_CAMERAS;

	viewMain.x = xOffset;
	viewMain.y = 0;
	viewMain.width = ofGetWidth() - xOffset - MENU_WIDTH; //xOffset * 2;
	viewMain.height = VIEWPORT_HEIGHT;

	for (int i = 0; i < N_CAMERAS; i++) {

		viewGrid[i].x = 0;
		viewGrid[i].y = yOffset * i;
		viewGrid[i].width = xOffset;
		viewGrid[i].height = yOffset;
	}

    iMainCamera = 0;
    
    previewCam.setUpAxis(glm::vec3(0, 0, 1));
    previewCam.setTranslationSensitivity(2., 2., 2.);
	previewCam.setNearClip(0.001f);

	////////////////////
	//  BLOBFINDER    //
	////////////////////
	ofLog(OF_LOG_NOTICE, "MainAPP: setting up blobfinder");

	blobFinder.setup(gui);

	/////////////////////////////
	//   REALSENSE GUI   SETUP //
	/////////////////////////////
	ofLog(OF_LOG_NOTICE, "MainAPP: loading postprocessing GUI");

	post = gui.addPanel();
	post->loadTheme("theme/theme_light.json");
	post->setName("PostProcessing");
	post->add(realSense->param_usePostProcessing);
	post->add(realSense->param_filterDecimation);
	post->add(realSense->param_filterDecimation_mag);
	post->add(realSense->param_filterDisparities);
	post->add(realSense->param_filterSpatial);
	post->add(realSense->param_filterSpatial_smoothAlpha);
	post->add(realSense->param_filterSpatial_smoothDelta);
	post->add(realSense->param_filterSpatial_mag);
	post->add(realSense->param_filterTemporal);
	post->add(realSense->param_filterTemporal_smoothAlpha);
	post->add(realSense->param_filterTemporal_smoothDelta);
	post->add(realSense->param_filterTemporal_persistency);

	post->loadFromFile("postprocessing.xml");


    /////////////////////////////
    //CALIBRATION GUI   SETUP //
    ////////////////////////////
	ofLog(OF_LOG_NOTICE, "MainAPP: loading calibration settings");

    setupCalib = gui.addPanel();
    
    setupCalib->loadTheme("theme/theme_light.json");
    setupCalib->setName("Calibration Panel");
    
    setupCalib->add(blobGrain.set("Grain", 2, 1, 10));

    setupCalib->add(calibPoint_X.set("calibrationPoint_X", ofVec2f(REALSENSE_VIDEO_WIDTH / 2, REALSENSE_VIDEO_HEIGHT / 2), ofVec2f(0, 0), ofVec2f(REALSENSE_VIDEO_WIDTH, REALSENSE_VIDEO_HEIGHT)));
    setupCalib->add(calibPoint_Y.set("calibrationPoint_Y", ofVec2f(REALSENSE_VIDEO_WIDTH / 2, REALSENSE_VIDEO_HEIGHT / 2), ofVec2f(0, 0), ofVec2f(REALSENSE_VIDEO_WIDTH, REALSENSE_VIDEO_HEIGHT)));
    setupCalib->add(calibPoint_Z.set("calibrationPoint_Z", ofVec2f(REALSENSE_VIDEO_WIDTH / 2, REALSENSE_VIDEO_HEIGHT / 2), ofVec2f(0, 0), ofVec2f(REALSENSE_VIDEO_WIDTH, REALSENSE_VIDEO_HEIGHT)));
    
    nearFrustum.addListener(this, &ofApp::updateFrustumCone);
    farFrustum.addListener(this, &ofApp::updateFrustumCone);

    frustumGuiGroup.setName("frustumField");
    frustumGuiGroup.add(nearFrustum.set("nearFrustum", 400, 200, 2000));
    frustumGuiGroup.add(farFrustum.set("farFrustum", 4000, 2000, 10000));
    setupCalib->addGroup(frustumGuiGroup);
    
    //setupCalib->add(transformation.set("matrix rx ry tz", ofVec3f(0, 0, 0), ofVec3f(-90, -90, -6000), ofVec3f(90, 90, 6000)));
 
    setupCalib->loadFromFile("settings.xml");

	////////////////////////////
	//   GUI   Transfromation //
	////////////////////////////
	ofLog(OF_LOG_NOTICE, "MainAPP: loading transformation matrix");

	guitransform = gui.addPanel();

	guitransform->loadTheme("theme/theme_light.json");
	guitransform->setName("Transformation");

	transformationGuiGroup.setName("Matrix");
	transformationGuiGroup.add(transformation.set("Transform", ofMatrix4x4()));

	guitransform->addGroup(transformationGuiGroup);

	guitransform->loadFromFile("transformation.xml");

	bool invisible = false;

	guitransform->setVisible(invisible);

	updateMatrix();

	/////////////////////////////
	//   GUI   DEVICE PARAMS   //
	/////////////////////////////

	ofLog(OF_LOG_NOTICE, "MainAPP: loading Device Operation GUI");

	device = gui.addPanel();

	/////////////////////////////
	//   OPERATING GUI         //
	/////////////////////////////

	operating = gui.addPanel();
	operating->loadTheme("theme/theme_light.json");
	operating->setName("Operating");

	operatingModes.setName("Modes");
	operatingModes.add(mode0Capture.set("normal", false));
	operatingModes.add(mode1Record.set("recording", false));
	operatingModes.add(mode2Playback.set("playback", false));

	operatingToggles = operating->addGroup(operatingModes);
	operatingToggles->setExclusiveToggles(true);
	operatingToggles->setConfig(ofJson({ {"type", "radio"} }));

	operatingToggles->setActiveToggle(0);
	operatingToggles->getActiveToggleIndex().addListener(this, &ofApp::changeOperation);	

    ////////////////////////
    //    RealSense       // 
    ////////////////////////

	ofLog(OF_LOG_NOTICE, "MainAPP: starting attached Device...");

	// firing up the device, creating the GUI and loading the device parameters
	if (realSense->capture()) {
		createGUIDeviceParams();
	}

	ofLog(OF_LOG_NOTICE, "...starting attached Device done.");

    /////////////////
	// creating preview point cloud is bogging the system down, so switched off at startup
	bPreviewPointCloud = false;
    
	ofLog(OF_LOG_NOTICE, "MainAPP: setting up networking...");
	
	networkMng.setup(gui, realSense->getSerialNumber(-1));

	ofLog(OF_LOG_NOTICE, "...networking done.");

    int * val = 0;
    updateFrustumCone(*val);
 
    setupViewports();

    createHelp();
    
    capMesh.reSize(4);
    
    ofSetLogLevel(OF_LOG_NOTICE);
    
	ofSetLogLevel(OF_LOG_VERBOSE);
    //ofLogToFile("myLogFile.txt", true);

	if (ofIsGLProgrammableRenderer()) {
		ofLog(OF_LOG_NOTICE, "ofIsGLProgrammableRenderer() = " + ofToString(ofIsGLProgrammableRenderer()));
	}
}

void ofApp::changeOperation(int& _index) {

	switch (_index) {
	case 0:
		if (realSense->capture()) {
			createGUIDeviceParams();
			setupViewports();
		}
		break;
	case 1:
		if (realSense->record()) {
			createGUIDeviceParams();
			setupViewports();
		}
		break;
	case 2:
		if (realSense->playback()) {
		}
		break;
	}
}

void ofApp::createGUIDeviceParams() {
	device->clear();
	device->loadTheme("theme/theme_light.json");
	device->setName("RealSense Device");
	device->add<ofxGuiLabel>(realSense->getSerialNumber(-1));

	intrinsicGuiGroup.clear();
	intrinsicGuiGroup.setName("Settings");
	intrinsicGuiGroup.add(realSense->param_deviceLaser);
	intrinsicGuiGroup.add(realSense->param_deviceLaser_mag);
	intrinsicGuiGroup.add(realSense->param_deviceAutoExposure);
	intrinsicGuiGroup.add(realSense->param_deviceExposure_mag);
	intrinsicGuiGroup.add(realSense->param_deviceGain_mag);
	intrinsicGuiGroup.add(realSense->param_deviceFrameQueSize_mag);
	intrinsicGuiGroup.add(realSense->param_deviceAsicTemparature);
	intrinsicGuiGroup.add(realSense->param_deviceProjectorTemparature);

	device->addGroup(intrinsicGuiGroup);

	device->loadFromFile(realSense->getSerialNumber(-1) + ".xml");
}

//--------------------------------------------------------------
void ofApp::setupViewports(){
	//call here whenever we resize the window
 
	device->setWidth(MENU_WIDTH / 4);
	post->setWidth(MENU_WIDTH / 4);
	setupCalib->setWidth(MENU_WIDTH / 4);
	blobFinder.panel->setWidth(MENU_WIDTH / 4);
	networkMng.panel->setWidth(MENU_WIDTH / 4);
	operating->setWidth(MENU_WIDTH / 4);

	device->setPosition(ofGetWidth() - MENU_WIDTH, 20);
	post->setPosition(ofGetWidth() - MENU_WIDTH, 400);
	operating->setPosition(ofGetWidth() - MENU_WIDTH, 800);
	setupCalib->setPosition(ofGetWidth() - MENU_WIDTH / 4 * 3, 20);
	blobFinder.panel->setPosition(ofGetWidth() - MENU_WIDTH / 4 * 2, 20);
	networkMng.panel->setPosition(ofGetWidth() - MENU_WIDTH / 4, 20);
    //ofLog(OF_LOG_NOTICE, "ofGetWidth()" + ofToString(ofGetWidth()));

    
	//
	//--
}

void ofApp::updateFrustumCone(int & value){
    if(realSense != NULL && realSense->isRunning()){
		double ref_pix_size = 1;// kinect.getZeroPlanePixelSize();
		double ref_distance = 1;// kinect.getZeroPlaneDistance();
        
        realSenseFrustum.near1 = nearFrustum.get();
        realSenseFrustum.far1 = farFrustum.get();
        
        double factorNear = 2 * ref_pix_size * realSenseFrustum.near1 / ref_distance;
        double factorFar = 2 * ref_pix_size * realSenseFrustum.far1 / ref_distance;
        
        //ofVec3f((x - DEPTH_X_RES/2) *factor, (y - DEPTH_Y_RES/2) *factor, raw[y * w + x]));
        
        realSenseFrustum.left = (0 - DEPTH_X_RES/2) *factorNear;
        realSenseFrustum.right = (640 - DEPTH_X_RES/2) *factorNear;
        realSenseFrustum.top = (0 - DEPTH_Y_RES/2) *factorNear;
        realSenseFrustum.bottom = (480 - DEPTH_Y_RES/2) *factorNear;
        
        realSenseFrustum.leftFar = (0 - DEPTH_X_RES/2) *factorFar;
        realSenseFrustum.rightFar = (640 - DEPTH_X_RES/2) *factorFar;
        realSenseFrustum.topFar = (0 - DEPTH_Y_RES/2) *factorFar;
        realSenseFrustum.bottomFar = (480 - DEPTH_Y_RES/2) *factorFar;
        
        realSenseFrustum.update();
        //createFrustumCone();
    }
    
}

void ofApp::measurementCycleRaw(){
    if(cycleCounter < N_MEASURMENT_CYCLES){
        planePoint1Meas[cycleCounter] = calcPlanePoint(calibPoint_X, 0, 1);
        planePoint2Meas[cycleCounter] = calcPlanePoint(calibPoint_Y, 0, 1);
        planePoint3Meas[cycleCounter] = calcPlanePoint(calibPoint_Z, 0, 1);
        cycleCounter++;
    } else {
        planePoint_X = ofVec3f();
        planePoint_Y = ofVec3f();
        planePoint_Z = ofVec3f();
        for(int y = 0; y < N_MEASURMENT_CYCLES; y++){
            planePoint_X += planePoint1Meas[y];
            planePoint_Y += planePoint2Meas[y];
            planePoint_Z += planePoint3Meas[y];
        }
        planePoint_X /= N_MEASURMENT_CYCLES;
        planePoint_Y /= N_MEASURMENT_CYCLES;
        planePoint_Z /= N_MEASURMENT_CYCLES;
        bUpdateMeasurment = false;
        bUpdateMeasurmentFine = true;
        cycleCounter = 0;
    }
}

void ofApp::measurementCycleFine(){
    if(cycleCounter < N_MEASURMENT_CYCLES){
        ofVec3f p1meas = calcPlanePoint(calibPoint_X, 0, 1);
        ofVec3f p2meas = calcPlanePoint(calibPoint_Y, 0, 1);
        ofVec3f p3meas = calcPlanePoint(calibPoint_Z, 0, 1);
        if(planePoint_X.z / 1.05 < p1meas.z &&
           p1meas.z < planePoint_X.z * 1.05 &&
           planePoint_Y.z / 1.05 < p2meas.z &&
           p2meas.z < planePoint_Y.z * 1.05 &&
           planePoint_Z.z / 1.05 < p3meas.z &&
           p3meas.z < planePoint_Z.z * 1.05){
            planePoint1Meas[cycleCounter] = p1meas;
            planePoint2Meas[cycleCounter] = p2meas;
            planePoint3Meas[cycleCounter] = p3meas;
            cycleCounter++;
        }
    } else {
        planePoint_X = ofVec3f();
        planePoint_Y = ofVec3f();
        planePoint_Z = ofVec3f();
        for(int y = 0; y < N_MEASURMENT_CYCLES; y++){
            planePoint_X += planePoint1Meas[y];
            planePoint_Y += planePoint2Meas[y];
            planePoint_Z += planePoint3Meas[y];
        }
        planePoint_X /= N_MEASURMENT_CYCLES;
        planePoint_Y /= N_MEASURMENT_CYCLES;
        planePoint_Z /= N_MEASURMENT_CYCLES;
        bUpdateMeasurmentFine = false;
        cycleCounter = 0;
        updateCalc();
    }
}

//--------------------------------------------------------------
void ofApp::updateCalc(){

	// This algorithm calculates the transformation matrix to 
	// transform from the camera centered coordinate system to the
	// calibration points defined coordinate system, where
	//   point z represents the coordinate center
	//   point x represents the x - axis from the coordinate center
	//   point y represents the y - axis from the coordinate center

	// translation vector to new coordinate system
	glm::vec3 translate = glm::vec3(planePoint_Z);

	glm::vec3 newXAxis = glm::normalize(glm::vec3(planePoint_X - planePoint_Z));
	glm::vec3 newYAxis = glm::normalize(glm::vec3(planePoint_Y - planePoint_Z));
	glm::vec3 newZAxis = glm::cross(newXAxis, newYAxis);

	// we calculate the Y axis from the Z axis to make sure all the vectors are perpendicular to each other
	// CAREFULL: It could be disabled because:
	//   Using nonperpendicular axis inspired from the point cloud data seems to 
	//   correct some of point cloud distortions....
	newYAxis = glm::cross(newZAxis, newXAxis);

	// the following solution was inspired by this post: https://stackoverflow.com/questions/34391968/how-to-find-the-rotation-matrix-between-two-coordinate-systems
	// however: it uses a 4x4 matrix and puts translation data as follows:
	//{ x.x x.y x.z 0 y.x y.y y.z 0 z.x z.y z.z 0 t.x t.y t.z 1 }

	float mat[16] = {
		newXAxis.x,
		newXAxis.y,
		newXAxis.z,
		0,
		newYAxis.x,
		newYAxis.y,
		newYAxis.z,
		0,
		newZAxis.x,
		newZAxis.y,
		newZAxis.z,
		0,
		translate.x,
		translate.y,
		translate.z,
		1
	};

	// and what we need at the end is the inverse of this:
	glm::mat4 transform = glm::inverse(glm::make_mat4x4(mat));

    geometry.clear();
    geometry.setMode(OF_PRIMITIVE_LINES);

	geometry.addColor(ofColor::red);
	geometry.addVertex(translate);
	geometry.addColor(ofColor::red);
	geometry.addVertex(translate + newXAxis);

	geometry.addColor(ofColor::green);
    geometry.addVertex(translate);
    geometry.addColor(ofColor::green);
    geometry.addVertex(translate + newYAxis); 
    
    geometry.addColor(ofColor::blue);
    geometry.addVertex(translate);
    geometry.addColor(ofColor::blue);
    geometry.addVertex(translate + newZAxis);

    calcdata = string("distance to new coordinate center point: " + ofToString(glm::length(translate)) + "\n");
	calcdata += "position point X: " + ofToString(planePoint_X) + "\n";
	calcdata += "position point Y: " + ofToString(planePoint_Y) + "\n";
	calcdata += "position point Z: " + ofToString(planePoint_Z) + "\n";
	calcdata += "distance to X: " + ofToString(planePoint_X.length()) + "\n";
	calcdata += "distance to Y: " + ofToString(planePoint_Y.length()) + "\n";
    calcdata += "distance to Z: " + ofToString(planePoint_Z.length()) + "\n";
	calcdata += "distance X to Z: " + ofToString(ofVec3f(planePoint_X - planePoint_Z).length()) + "\n";
	calcdata += "distance Y to Z: " + ofToString(ofVec3f(planePoint_Y - planePoint_Z).length()) + "\n";

    frustumCenterSphere.setRadius(20);
    
    bUpdateCalc = false;
    
 //   ofLog(OF_LOG_NOTICE, "updating... ");

	transformation.set(ofMatrix4x4(transform));

	updateMatrix();
}

//--------------------------------------------------------------
void ofApp::updateMatrix(){

	sphere_X.setPosition(planePoint_X);
	sphere_Y.setPosition(planePoint_Y);
	sphere_Z.setPosition(planePoint_Z);

	sphere_X.setRadius(0.05);
	sphere_Y.setRadius(0.05);
	sphere_Z.setRadius(0.05);

    //deviceTransform = ofMatrix4x4();
    
	deviceTransform = transformation.get();

    //blobFinder.kinectPos = ofVec3f(0, 0, transformation.get().z);    
}

//--------------------------------------------------------------
glm::vec3 ofApp::calcPlanePoint(ofParameter<ofVec2f> & cpoint, int _size, int _step){
	glm::vec3 ppoint;

	int width = realSense->getDepthWidth();
    int height = realSense->getDepthHeight();
   
    int size = _size;
    int step = _step;
    float factor;
    int counter = 0;
    
    int minX = ((cpoint.get().x - size) >= 0)?(cpoint.get().x - 1): 0;
    int minY = ((cpoint.get().y - size) >= 0)?(cpoint.get().y - 1): 0;
    int maxY = ((cpoint.get().y + size) < cpoint.getMax().y)?(cpoint.get().y + size): cpoint.getMax().y - 1;
    int maxX = ((cpoint.get().x + size) < cpoint.getMax().x)?(cpoint.get().x + size): cpoint.getMax().x - 1;
      
    float corrDistance;

	glm::vec3 coord;
    
    for(int y = minY; y <= maxY; y = y + step) {
        for(int x = minX; x <= maxX; x = x + step) {
 			coord = realSense->getSpacePointFromInfraLeftFrameCoord(glm::vec2(x, y));
            if(coord.z > 0) {
				ppoint += coord;
                counter++;
            }
        }
    }
    ppoint /= counter;
  
    return ppoint;
    
}


//--------------------------------------------------------------
void ofApp::update(){
	
	ofBackground(100, 100, 100);
    	
	// there is a new frame and we are connected
	if(realSense->update(ofxRealSenseTwo::PointCloud::INFRALEFT)) {

        if(bUpdateMeasurment){
            measurementCycleRaw();
        }
        if(bUpdateMeasurmentFine){
            measurementCycleFine();
        }

		if (bUpdateImageMask) {
			blobFinder.captureMaskBegin();
			drawCapturePointCloud(true);
			blobFinder.captureMaskEnd();
		}
		else {
			//////////////////////////////////
			// Cature captureCloud to FBO
			//////////////////////////////////

			blobFinder.captureBegin();
			drawCapturePointCloud(false);
			blobFinder.captureEnd();

			//////////////////////////////////
			// BlobFinding on the captured FBO
			/////////////////////////////////////
			blobFinder.update();

			networkMng.update(blobFinder, realSenseFrustum, transformation.get());
		}
    
	}
}

//--------------------------------------------------------------
void ofApp::draw(){

	ofSetColor(255, 255, 255);

    //ofLogNotice() << "draw next frame";
    if(bShowVisuals){
        //Draw viewport previews
		realSense->drawDepthStream(viewGrid[0]);
		realSense->drawInfraLeftStream(viewGrid[1]);

        blobFinder.grayImage.draw(viewGrid[2]);
        blobFinder.contourFinder.draw(viewGrid[3]);
        blobFinder.maskFbo.draw(viewGrid[4]);

        
        switch (iMainCamera) {
            case 0:
				realSense->drawDepthStream(viewMain);
                drawCalibrationPoints();
                break;
            case 1:
				realSense->drawInfraLeftStream(viewMain);
                drawCalibrationPoints();
                break;
            case 2:
                blobFinder.grayImage.draw(viewMain);
				ofSetColor(255, 0, 0, 255);
				blobFinder.contourFinder.draw(viewMain);
				
				break;
            case 3:
				blobFinder.fbo.draw(viewMain);
				ofSetColor(255, 0, 0, 255);
				blobFinder.contourFinder.draw(viewMain);

                ofNoFill();
                ofSetColor(255, 0, 255, 255);
                blobFinder.drawBodyBlobs2d(viewMain);
                
               break;
            case 4:
				blobFinder.maskFbo.draw(viewMain);

                ofNoFill();
                ofSetColor(255, 0, 255, 255);
                blobFinder.drawBodyBlobs2d(viewMain);
                break;
            case 5:
                previewCam.begin(viewMain);
                mainGrid.drawPlane(5., 5, false);
                drawPreview();
                previewCam.end();
                break;
            default:
                break;
        }
        
        //Draw opengl viewport previews (ofImages dont like opengl calls before they are drawn
        if(iMainCamera != 5){ // make sure the camera is drawn only once (so the interaction with the mouse works)
            previewCam.begin(viewGrid[5]);
            mainGrid.drawPlane(5., 5, false);
            drawPreview();
            previewCam.end();
        }

        glDisable(GL_DEPTH_TEST);
        ofPushStyle();
        // Highlight background of selected camera
        ofSetColor(255, 0, 255, 255);
        ofNoFill();
        ofSetLineWidth(3);
        ofDrawRectangle(viewGrid[iMainCamera]);
    } else {

        blobFinder.contourEyeFinder.draw(viewMain);

        ofNoFill();
        ofSetColor(255, 0, 255, 255);
        blobFinder.drawBodyBlobs2d(viewMain);
    }

    //--
    

	// draw instructions
	ofSetColor(255, 255, 255);
    
    if(bShowHelp) {
        if(bShowCalcData){
            ofDrawBitmapString(calcdata, 20 ,VIEWPORT_HEIGHT + 20);
        } else {
            ofDrawBitmapString(help, 20 ,VIEWPORT_HEIGHT + 20);
        }
    }

    ofDrawBitmapString("fps: " + ofToString(ofGetFrameRate()), ofGetWidth() - 200, 10);

    ofPopStyle();
}

void ofApp::drawPreview() {
	glPointSize(4);
	glEnable(GL_DEPTH_TEST);

	ofPushMatrix();

    //This moves the crossingpoint of the kinect center line and the plane to the center of the stage
    //ofTranslate(-planeCenterPoint.x, -planeCenterPoint.y, 0);
	ofMultMatrix(deviceTransform);
	if (bPreviewPointCloud) {
		realSense->draw();
	}
	ofFill();
	ofSetColor(255, 0, 0);
	sphere_X.draw();
	sphere_Y.draw();
	sphere_Z.draw();
	/*
	frustumCenterSphere.draw();
	*/

	geometry.draw();

	//ofSetColor(0, 0, 255);
	//realSenseFrustum.drawWireframe();
	
	ofPopMatrix();

	ofPushMatrix();

    ofSetColor(255, 255, 0);
    blobFinder.drawSensorBox();

    ofNoFill();
    ofSetColor(255, 100, 255);
    blobFinder.drawBodyBlobsBox();
    blobFinder.drawBodyBlobsHeadTop();

    ofFill();
    ofSetColor(255, 100, 100);
    blobFinder.drawGazePoint();

    
	glDisable(GL_DEPTH_TEST);
	ofPopMatrix();
    
}

void ofApp::drawCapturePointCloud(bool _mask) {
    glEnable(GL_DEPTH_TEST);

	shader.begin();

	float lowerLimit = blobFinder.sensorBoxBottom.get() / 1000.f;
	float upperLimit = blobFinder.sensorBoxTop.get() / 1000.f;

	if (_mask) {
		//ofClear(255, 255, 255, 255);
		shader.setUniform1i("mask", 1);
		glPointSize(blobGrain.get() * 4);
	}
	else {
		shader.setUniform1i("mask", 0);
		glPointSize(blobGrain.get() * 2);
	}
	shader.setUniform1f("lowerLimit", lowerLimit);
	shader.setUniform1f("upperLimit", upperLimit);
	shader.setUniformMatrix4f("viewMatrixInverse", glm::inverse(ofGetCurrentViewMatrix()));

	ofPushMatrix();
	ofMultMatrix(deviceTransform);
	realSense->draw();
	ofPopMatrix();
	
	shader.end();
	
	glDisable(GL_DEPTH_TEST);

}

void ofApp::drawCalibrationPoints(){
    glDisable(GL_DEPTH_TEST);
    ofPushStyle();
    ofSetColor(255, 0, 0);
    ofNoFill();
    ofDrawBitmapString("x", calibPoint_X.get().x/REALSENSE_DEPTH_WIDTH*viewMain.width + VIEWGRID_WIDTH + 5, calibPoint_X.get().y -5);
    ofDrawBitmapString("y", calibPoint_Y.get().x/REALSENSE_DEPTH_WIDTH*viewMain.width + VIEWGRID_WIDTH + 5, calibPoint_Y.get().y -5);
    ofDrawBitmapString("z", calibPoint_Z.get().x/REALSENSE_DEPTH_WIDTH*viewMain.width + VIEWGRID_WIDTH + 5, calibPoint_Z.get().y -5);
    ofDrawCircle(calibPoint_X.get().x/REALSENSE_DEPTH_WIDTH*viewMain.width + VIEWGRID_WIDTH, calibPoint_X.get().y, 2);
    ofDrawCircle(calibPoint_Y.get().x/REALSENSE_DEPTH_WIDTH*viewMain.width + VIEWGRID_WIDTH, calibPoint_Y.get().y, 2);
    ofDrawCircle(calibPoint_Z.get().x/REALSENSE_DEPTH_WIDTH*viewMain.width + VIEWGRID_WIDTH, calibPoint_Z.get().y, 2);
    ofPopStyle();
    glEnable(GL_DEPTH_TEST);
}

//--------------------------------------------------------------
void ofApp::exit() {
    ofLog(OF_LOG_NOTICE, "exiting application...");

	realSense->stop();
	
}

void ofApp::createHelp(){
    help = string("press v -> to show visualizations\n");
	help += "press 1 - 6 -> to change the viewport\n";
	help += "press p -> to show pointcloud\n";
    help += "press h -> to show help \n";
    help += "press s -> to save current settings.\n";
	help += "press l -> to load last saved settings\n";
	help += "press m -> to update mask image CAREFULL: press m again to stop updating (" + ofToString(bUpdateImageMask) + ")\n";
	help += "press c -> to clear mask image\n";
	help += "press x|y|z and then mouse-click -> to change the calibration points in viewport 1\n";
	help += "press k -> to update the calculation\n";
	help += "press r -> to show calculation results \n";
	help += "press t -> to terminate the connection, connection is: " + ofToString(realSense->isRunning()) + "\n";
	help += "press o -> to open the connection again\n";
    help += "ATTENTION: Setup-Settings (ServerID and Video) will only apply after restart\n";
 	help += "Broadcasting ip: "+networkMng.broadcastIP.get()+" port: "+ofToString(networkMng.broadcastPort.get())+" serverID: "+ofToString(networkMng.mServerID)+" \n";
    /*
     help += "using opencv threshold = " + ofToString(bThreshWithOpenCV) + " (press spacebar)\n";
     help += "set near threshold " + ofToString(nearThreshold) + " (press: + -)\n";
     help += "set far threshold " + ofToString(farThreshold) + " (press: < >) num blobs found " + ofToString(contourFinder.nBlobs) + "\n";
     */
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
	bUpdateSetMesurmentPoint = -1;
	switch (key) {
		case ' ':
			break;
			
		case'p':
			bPreviewPointCloud = !bPreviewPointCloud;
            break;
            
		case'v':
			bShowVisuals = !bShowVisuals;
            break;
            
		case 'c':
			blobFinder.clearMask();
			break;
			
		case 't':
			//kinect.close();
			break;
            
        case 'r':
            bShowCalcData = !bShowCalcData;
            break;
            
        case 'k':
            bUpdateMeasurment = true;
            break;
 
        case 's':
            setupCalib->saveToFile("settings.xml");
            blobFinder.panel->saveToFile("trackings.xml");
			blobFinder.saveMask();
			networkMng.panel->saveToFile("broadcast.xml");
			post->saveToFile("postprocessing.xml");
			device->saveToFile(realSense->getSerialNumber(-1) + ".xml");
			guitransform->saveToFile("transformation.xml");
			break;

        case 'l':
            setupCalib->loadFromFile("settings.xml");
            blobFinder.panel->loadFromFile("trackings.xml");
			blobFinder.loadMask();
            networkMng.panel->loadFromFile("broadcast.xml");
			post->loadFromFile("postprocessing.xml");
			device->loadFromFile(realSense->getSerialNumber(-1) + ".xml");
			guitransform->loadFromFile("transformation.xml");
			break;
           
		case 'h':
			bShowHelp = !bShowHelp;
            if (bShowHelp) {
                createHelp();
            }
			break;
            
		case '>':
		case '.':
			//farThreshold ++;
			//if (farThreshold > 255) farThreshold = 255;
			break;
			
		case '<':
		case ',':
			//farThreshold --;
			//if (farThreshold < 0) farThreshold = 0;
			break;
			
		case '+':
		case '=':
			//nearThreshold ++;
			//if (nearThreshold > 255) nearThreshold = 255;
			break;
			
		case '-':
			//nearThreshold --;
			//if (nearThreshold < 0) nearThreshold = 0;
			break;
			
		case 'm':
			bUpdateImageMask = !bUpdateImageMask;
			if (bUpdateImageMask) {
				blobFinder.clearMask();
			}
			break;
						
		case 'x':
			bUpdateSetMesurmentPoint = key;
			break;

		case 'y':
			bUpdateSetMesurmentPoint = key;
			break;

		case 'z':
			bUpdateSetMesurmentPoint = key;
			break;

		case '0':
			//kinect.setLed(ofxKinect::LED_OFF);
			break;
            
		case '1':
            iMainCamera = 0;
			//kinect.setLed(ofxKinect::LED_GREEN);
			break;
			
		case '2':
            iMainCamera = 1;
			//kinect.setLed(ofxKinect::LED_YELLOW);
			break;
			
		case '3':
            iMainCamera = 2;
			//kinect.setLed(ofxKinect::LED_RED);
			break;
			
		case '4':
            iMainCamera = 3;
			//kinect.setLed(ofxKinect::LED_BLINK_GREEN);
			break;
			
		case '5':
            iMainCamera = 4;
			//kinect.setLed(ofxKinect::LED_BLINK_YELLOW_RED);
			break;
            
		case '6':
            iMainCamera = 5;
			//kinect.setLed(ofxKinect::LED_BLINK_YELLOW_RED);
			break;
						            
	}

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){


}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){
	stroke.push_back(ofPoint(x,y));
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
    if(iMainCamera == 0 || iMainCamera == 1 && bUpdateSetMesurmentPoint != -1) {
        if(bUpdateSetMesurmentPoint == 'x'){
            int posX = (x - VIEWGRID_WIDTH) / viewMain.width * REALSENSE_DEPTH_WIDTH;
            int posY = y;
            if(0 <= posX && posX < REALSENSE_DEPTH_WIDTH &&
               0 <= posY && posY < REALSENSE_DEPTH_HEIGHT)
                calibPoint_X.set(glm::vec2(posX, posY));
        }else if(bUpdateSetMesurmentPoint == 'y'){
            int posX = (x - VIEWGRID_WIDTH) / viewMain.width * REALSENSE_DEPTH_WIDTH;
            int posY = y;
            if(0 <= posX && posX < REALSENSE_DEPTH_WIDTH &&
               0 <= posY && posY < REALSENSE_DEPTH_HEIGHT)
                calibPoint_Y.set(glm::vec2(posX, posY));
        }else if(bUpdateSetMesurmentPoint == 'z'){
            int posX = (x - VIEWGRID_WIDTH) / viewMain.width * REALSENSE_DEPTH_WIDTH;
            int posY = y;
            if(0 <= posX && posX < REALSENSE_DEPTH_WIDTH &&
               0 <= posY && posY < REALSENSE_DEPTH_HEIGHT)
                calibPoint_Z.set(glm::vec2(posX, posY));
        }
    }
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){
}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){
	/*
    cairo->setupMemoryOnly(ofCairoRenderer::IMAGE,
                           false, false,
                           ofRectangle(0, 0, w, h));
    render.allocate(w, h, GL_RGBA);
	*/
	setupViewports();
}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}


