#include "ofApp.h"

#define RECONNECT_TIME 400

#define DEPTH_X_RES 640
#define DEPTH_Y_RES 480


//--------------------------------------------------------------
void ofApp::setup(){
	ofSetLogLevel(OF_LOG_VERBOSE);

	realSense = RSDevice::createUniquePtr();

	realSense->checkConnectedDialog();

	realSense->setVideoSize(REALSENSE_VIDEO_WIDTH, REALSENSE_VIDEO_HEIGHT);

	// we don't want to be running to fast
	//ofSetVerticalSync(true);
	//ofSetFrameRate(30);
    
    //create the socket and set to send to 127.0.0.1:11999
	//udpConnection.Create();
	//udpConnection.Connect("127.0.0.1",4653);
	//udpConnection.SetNonBlocking(true);

	//TCP.setMessageDelimiter("\n");
    
    iMainCamera = 0;
    
    previewCam.setUpAxis(glm::vec3(0, 0, 1));
    previewCam.setTranslationSensitivity(2., 2., 2.);
    
    /////////////////
    //BLOBFINDER   //
    /////////////////
    
    blobFinder.setup(gui);
    
    blobFinder.allocate();

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
    
    setupCalib = gui.addPanel();
    
    setupCalib->loadTheme("theme/theme_light.json");
    setupCalib->setName("Calibration Panel");
    
    setupCalib->add(captureVideo.set("use video", true));
    setupCalib->add(blobGrain.set("Grain", 2, 1, 4));

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

    ////////////////////
    //RealSense       // -> It needs to be after the GUI SETUP but before GUI DEVICE
    ////////////////////
    
	realSense->start();

	std:string kinectSerialID = "noID";
        
    ////////////////
    //GUI   DEVICE //
    ////////////////

	device = gui.addPanel();
    
	device->loadTheme("theme/theme_light.json");
	device->setName("RealSense Device");
	device->add<ofxGuiLabel>(kinectSerialID);

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

    updateMatrix();

    
    /////////////////

		
	// creating preview point cloud is bogging the system down, so switched off at startup
	bPreviewPointCloud = false;
    
    networkMng.setup(gui, kinectSerialID);
    
    int * val = 0;
    updateFrustumCone(*val);
 
    setupViewports();

    createHelp();
    
    capMesh.reSize(4);
    
    ////////////////
    //CAIRO RENDER //
    ////////////////
    /*
    opengl = ofGetGLRenderer();
    cairo = make_shared<ofCairoRenderer>();
    cairo->setupMemoryOnly(ofCairoRenderer::IMAGE);
    render.allocate(ofGetWidth() / 5., ofGetHeight() / 5., GL_RGBA);
     */
    
    ofSetLogLevel(OF_LOG_NOTICE);
    
    ofLogToFile("myLogFile.txt", true);
}


//--------------------------------------------------------------
void ofApp::setupViewports(){
	//call here whenever we resize the window
 
	device->setWidth(MENU_WIDTH / 4);
	post->setWidth(MENU_WIDTH / 4);
	setupCalib->setWidth(MENU_WIDTH / 4);
	blobFinder.panel->setWidth(MENU_WIDTH / 4);
	networkMng.panel->setWidth(MENU_WIDTH / 4);

	device->setPosition(ofGetWidth() - MENU_WIDTH, 20);
	post->setPosition(ofGetWidth() - MENU_WIDTH, 400);
	setupCalib->setPosition(ofGetWidth() - MENU_WIDTH / 4 * 3, 20);
	blobFinder.panel->setPosition(ofGetWidth() - MENU_WIDTH / 4 * 2, 20);
	networkMng.panel->setPosition(ofGetWidth() - MENU_WIDTH / 4, 20);
    //ofLog(OF_LOG_NOTICE, "ofGetWidth()" + ofToString(ofGetWidth()));

	//--
	// Define viewports
    
	float xOffset = VIEWGRID_WIDTH; //ofGetWidth() / 3;
	float yOffset = VIEWPORT_HEIGHT / N_CAMERAS;
    
	viewMain.x = xOffset;
	viewMain.y = 0;
	viewMain.width = ofGetWidth() - xOffset - MENU_WIDTH; //xOffset * 2;
	viewMain.height = VIEWPORT_HEIGHT;
    
	for(int i = 0; i < N_CAMERAS; i++){
        
		viewGrid[i].x = 0;
		viewGrid[i].y = yOffset * i;
		viewGrid[i].width = xOffset;
		viewGrid[i].height = yOffset;
	}
    
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
        
    sphere_X.setPosition(planePoint_X);
    sphere_Y.setPosition(planePoint_Y);
    sphere_Z.setPosition(planePoint_Z);

    sphere_X.setRadius(25);
    sphere_Y.setRadius(25);
    sphere_Z.setRadius(25);

    
    Planef REL_floorPlane = Planef(planePoint_X, planePoint_Y, planePoint_Z);
    if(REL_floorPlane.getNormal().z < 0.f){ // if it points downwards
        REL_floorPlane = Planef(planePoint_Z, planePoint_Y, planePoint_X);
    }
    
    Linef KINECT_Z_axis = Linef(ofVec3f(0, 0, 0), ofVec3f(0, 0, -1));
    
    //Y-Z plane
    Planef KINECT_vertical_Plane = Planef(ofVec3f(0, 0, 0), ofVec3f(0, 0, -1), ofVec3f(0, 1, -1));
    //X-Z plane
    Planef KINECT_horizontal_Plane = Planef(ofVec3f(0, 0, 0), ofVec3f(0, 0, -1), ofVec3f(1, 0, -1));
    
    Linef ABS_Y_Axis;
    if(KINECT_vertical_Plane.intersects(REL_floorPlane))
       ABS_Y_Axis = KINECT_vertical_Plane.getIntersection(REL_floorPlane);
    
    ofVec3f ABS_frustumCenterPoint = REL_floorPlane.getIntersection(KINECT_Z_axis);
    
    ofVec3f ABS_Z_Axis = ofVec3f(REL_floorPlane.normal).scale(1000);
    ofVec3f ABS_X_Axis = ofVec3f(ABS_Y_Axis.direction).cross(ABS_Z_Axis);
    
    ofVec3f HELPER_X_Axis = ofVec3f(ABS_frustumCenterPoint).cross(ofVec3f(ABS_Y_Axis.direction).scale(100));
    ofVec3f HELPER_Z_Axis = ofVec3f(HELPER_X_Axis).cross(ofVec3f(ABS_Y_Axis.direction).scale(100));

    float kinectRransform_xAxisRot = ABS_frustumCenterPoint.angle(ofVec3f(HELPER_Z_Axis).scale(-1.));
    float kinectRransform_yAxisRot = -HELPER_X_Axis.angle(ABS_X_Axis);
    
    float kinectRransform_zTranslate = ABS_frustumCenterPoint.length();

    ofMatrix4x4 zTranMatrix = ofMatrix4x4();
    zTranMatrix.translate(0, 0, kinectRransform_zTranslate);
    zTranMatrix.rotate(kinectRransform_xAxisRot, 1, 0, 0);
    zTranMatrix.rotate(kinectRransform_yAxisRot, 0, 1, 0);
    
    transformation.set(ofVec3f(kinectRransform_xAxisRot, kinectRransform_yAxisRot, zTranMatrix.getTranslation().z));
    
    //ofLog(OF_LOG_NOTICE, "zpos: " + ofToString(kinectTransform.getTranslation().z));
    
    ofMatrix4x4 centerMatrix = ofMatrix4x4();
    centerMatrix.rotate(kinectRransform_xAxisRot, 1, 0, 0);
    centerMatrix.rotate(kinectRransform_yAxisRot, 0, 1, 0);
    centerMatrix.translate(0, 0, zTranMatrix.getTranslation().z);
    
    planeCenterPoint = centerMatrix.preMult(ABS_frustumCenterPoint);
    //planeCenterPoint.rotate(kinectRransform_xAxisRot, ofVec3f(1, 0, 0));


    geometry.clear();
    geometry.setMode(OF_PRIMITIVE_LINES);
    geometry.addColor(ofColor::blueSteel);
    geometry.addVertex(ofVec3f(0, 0, 0));
    geometry.addColor(ofColor::blueSteel);
    geometry.addVertex(ABS_frustumCenterPoint);
    
    geometry.addColor(ofColor::green);
    geometry.addVertex(ABS_frustumCenterPoint);
    geometry.addColor(ofColor::green);
    geometry.addVertex(ABS_frustumCenterPoint + ofVec3f(ABS_Y_Axis.direction).scale(1000));
    
    geometry.addColor(ofColor::red);
    geometry.addVertex(ABS_frustumCenterPoint);
    geometry.addColor(ofColor::red);
    geometry.addVertex(ABS_frustumCenterPoint + ABS_X_Axis);
    
    geometry.addColor(ofColor::blue);
    geometry.addVertex(ABS_frustumCenterPoint);
    geometry.addColor(ofColor::blue);
    geometry.addVertex(ABS_frustumCenterPoint + ABS_Z_Axis);

    geometry.addColor(ofColor::blueViolet);
    geometry.addVertex(ABS_frustumCenterPoint);
    geometry.addColor(ofColor::blueViolet);
    geometry.addVertex(ABS_frustumCenterPoint + HELPER_X_Axis);

    geometry.addColor(ofColor::greenYellow);
    geometry.addVertex(ABS_frustumCenterPoint);
    geometry.addColor(ofColor::greenYellow);
    geometry.addVertex(ABS_frustumCenterPoint + ofVec3f(HELPER_Z_Axis).scale(1000));
    
    calcdata = string("distance to plane center point: " + ofToString(ABS_frustumCenterPoint.length()) + "\n");
	calcdata += "position point X: " + ofToString(planePoint_X) + "\n";
	calcdata += "position point Y: " + ofToString(planePoint_Y) + "\n";
	calcdata += "position point Z: " + ofToString(planePoint_Z) + "\n";
	calcdata += "distance to X: " + ofToString(planePoint_X.length()) + "\n";
	calcdata += "distance to Y: " + ofToString(planePoint_Y.length()) + "\n";
    calcdata += "distance to Z: " + ofToString(planePoint_Z.length()) + "\n";
	calcdata += "distance X to Z: " + ofToString(ofVec3f(planePoint_X - planePoint_Z).length()) + "\n";
	calcdata += "distance Y to Z: " + ofToString(ofVec3f(planePoint_Y - planePoint_Z).length()) + "\n";


    //ofLog(OF_LOG_NOTICE, "planeCenterPoint.x" + ofToString(planeCenterPoint.x));
    //ofLog(OF_LOG_NOTICE, "planeCenterPoint.y" + ofToString(planeCenterPoint.y));
    //ofLog(OF_LOG_NOTICE, "planeCenterPoint.z" + ofToString(planeCenterPoint.z));
    //ofLog(OF_LOG_NOTICE, "yAxisRot" + ofToString(kinectRransform_yAxisRot));

    frustumCenterSphere.setRadius(20);
    
    bUpdateCalc = false;
    
 //   ofLog(OF_LOG_NOTICE, "updating... ");

    updateMatrix();
}

//--------------------------------------------------------------
void ofApp::updateMatrix(){
    kinectRransform = ofMatrix4x4();
    
    kinectRransform.rotate(transformation.get().x, 1, 0, 0);
    kinectRransform.rotate(transformation.get().y, 0, 1, 0);

    kinectRransform.translate(0, 0, transformation.get().z);
    
    blobFinder.kinectPos = ofVec3f(0, 0, transformation.get().z);    
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
	if(realSense->update(ofxRSSDK::PointCloud::INFRALEFT)) {

        if(bUpdateMeasurment){
            measurementCycleRaw();
        }
        if(bUpdateMeasurmentFine){
            measurementCycleFine();
        }
		/*
		updatePointCloud(capMesh.update(), blobGrain.get(), true, false);
		if(bPreviewPointCloud) {
		updatePointCloud(previewmesh, blobGrain.get() + 1, false, true);
		}
		*/
    
        //////////////////////////////////
        // Cature captureCloud to FBO
        //////////////////////////////////
        
		/*
        blobFinder.captureBegin();
        drawCapturePointCloud();
        blobFinder.captureEnd();
		*/

        //////////////////////////////////
        // BlobFinding on the captured FBO
        /////////////////////////////////////
        //blobFinder.update();
	}
    
    //networkMng.update(blobFinder, realSenseFrustum, transformation.get());
}

//--------------------------------------------------------------
void ofApp::draw(){

	ofSetColor(255, 255, 255);

    //ofLogNotice() << "draw next frame";
    if(bShowVisuals){
        //Draw viewport previews
		realSense->drawDepthStream(viewGrid[0]);
		realSense->drawInfraLeftStream(viewGrid[1]);

        //blobFinder.captureFBO.draw(viewGrid[2]);
        //blobFinder.contourFinder.draw(viewGrid[3]);
        //blobFinder.contourEyeFinder.draw(viewGrid[4]);

        
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
                //blobFinder.captureFBO.draw(viewMain);
                break;
            case 3:
                ofSetColor(255, 0, 0, 255);
                //blobFinder.contourFinder.draw(viewMain);

                ofNoFill();
                ofSetColor(255, 0, 255, 255);
                //blobFinder.drawBodyBlobs2d(viewMain);
                
               break;
            case 4:
                //blobFinder.contourEyeFinder.draw(viewMain);

                ofNoFill();
                ofSetColor(255, 0, 255, 255);
                //blobFinder.drawBodyBlobs2d(viewMain);
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

        //blobFinder.contourEyeFinder.draw(viewMain);

        ofNoFill();
        ofSetColor(255, 0, 255, 255);
        //blobFinder.drawBodyBlobs2d(viewMain);
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

void ofApp::updatePointCloud(ofVboMesh & mesh, int step, bool useFrustumCone, bool useVideoColor) {
	/*
	
    int w = 640;
	int h = 480;
    //	ofMesh mesh;
    mesh.clear();
	mesh.setMode(OF_PRIMITIVE_POINTS);
    
    double ref_pix_size = 2 * kinect.getZeroPlanePixelSize() * pixelSizeCorrector.get();
    double ref_distance = kinect.getZeroPlaneDistance();
    ofShortPixelsRef raw = kinect.getRawDepthPixels();
    double factor = 0;
    double corrDistance;
    
    int minRaw = 10000;
    int maxRaw = 0;
    
    ofVec3f vertex;
    
    float sensorFieldFront = blobFinder.sensorBoxFront.get();
    float sensorFieldBack = blobFinder.sensorBoxBack.get();
    float sensorFieldLeft = blobFinder.sensorBoxLeft.get();
    float sensorFieldRight = blobFinder.sensorBoxRight.get();
    float sensorFieldTop = blobFinder.sensorBoxTop .get();
    float sensorFieldBottom = blobFinder.sensorBoxBottom.get();
    
	for(int y = 0; y < h; y += step) {
		for(int x = 0; x < w; x += step) {
            vertex.z = 0;
            corrDistance = (float)raw[y * w + x] * (depthCorrectionBase.get() + (float)raw[y * w + x] / depthCorrectionDivisor.get());
            factor = ref_pix_size * corrDistance / ref_distance;
            if(useFrustumCone){
                if(nearFrustum.get() < corrDistance && corrDistance < farFrustum.get()) {
                    vertex = ofVec3f((x - DEPTH_X_RES/2) *factor, -(y - DEPTH_Y_RES/2) *factor, -corrDistance);
                    vertex = kinectRransform.preMult(vertex);
                }
            } else {
                vertex = ofVec3f((x - DEPTH_X_RES/2) *factor, -(y - DEPTH_Y_RES/2) *factor, -corrDistance);
                vertex = kinectRransform.preMult(vertex);
            }
            if(vertex.z != 0){
                if(sensorFieldLeft < vertex.x && vertex.x < sensorFieldRight &&
                   sensorFieldFront < vertex.y && vertex.y < sensorFieldBack &&
                   sensorFieldBottom < vertex.z && vertex.z < sensorFieldTop){
                    mesh.addColor((vertex.z - sensorFieldBottom) / (sensorFieldTop - sensorFieldBottom));
                } else {
                    if(useVideoColor)
                        mesh.addColor(kinect.getColorAt(x,y));
                    else
                        mesh.addColor(ofColor::black);
                }
                mesh.addVertex(vertex);
			}
  		}
	}
	*/

}

void ofApp::drawPreview() {
	glPointSize(4);
	ofPushMatrix();

    //This moves the crossingpoint of the kinect center line and the plane to the center of the stage
    ofTranslate(-planeCenterPoint.x, -planeCenterPoint.y, 0);
	if (bPreviewPointCloud) {
		realSense->draw();
	}
	ofPopMatrix();

	ofPushMatrix();

    ofSetColor(255, 255, 0);
    blobFinder.sensorBox.draw();
    
    ofNoFill();
    ofSetColor(255, 100, 255);
    blobFinder.drawBodyBlobsBox();
    blobFinder.drawBodyBlobsHeadTop();
    ofSetColor(100, 100, 255);
    blobFinder.drawHeadBlobs();
    ofSetColor(0, 0, 255);
    blobFinder.drawEyeCenters();

    ofFill();
    ofSetColor(255, 100, 100);
    blobFinder.drawGazePoint();

	glEnable(GL_DEPTH_TEST);
    
    ofMultMatrix(kinectRransform);

    ofFill();
	/*
    ofSetColor(255, 0, 0);
    sphere_X.draw();
    sphere_Y.draw();
    sphere_Z.draw();
    frustumCenterSphere.draw();
	*/

    geometry.draw();

    ofSetColor(0, 0, 255);
    realSenseFrustum.drawWireframe();
    

	glDisable(GL_DEPTH_TEST);
	ofPopMatrix();
    
}

void ofApp::drawCapturePointCloud() {
    glEnable(GL_DEPTH_TEST);
    glPointSize(blobGrain.get() * 2);
	ofPushMatrix();
	ofScale(0.01, 0.01, 0.01);
    capMesh.draw();
	ofPopMatrix();
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
    help += "press p -> to show pointcloud\n";
    help += "press k -> to update the calculation\n";
    help += "press h -> to show help \n";
    help += "press r -> to show calculation results \n";
	help += "press s -> to save current settings.\n";
	help += "press l -> to load last saved settings\n";
	help += "press 1 - 6 -> to change the viewport\n";
	help += "press x|y|z + mouse-release -> to change the calibration points in viewport 1\n";
    
	help += "press t -> to terminate the connection, connection is: " + ofToString(realSense->isRunning()) + "\n";
	help += "press o -> to open the connection again\n";
    help += "ATTENTION: Setup-Settings (ServerID and Video) will only apply after restart\n";
 	help += "Broadcasting ip: "+networkMng.broadcastIP.get()+" port: "+ofToString(networkMng.broadcastPort.get())+" serverID: "+ofToString(networkMng.kinectID)+" \n";
 	help += "Correction Distance Math -> corrected distance = distance * (Base + distance / Divisor)\n";
	help += "Correction pixel Site    -> corrected pixel size = pixel size * Factor\n";
    /*
     help += "using opencv threshold = " + ofToString(bThreshWithOpenCV) + " (press spacebar)\n";
     help += "set near threshold " + ofToString(nearThreshold) + " (press: + -)\n";
     help += "set far threshold " + ofToString(farThreshold) + " (press: < >) num blobs found " + ofToString(contourFinder.nBlobs) + "\n";
     */
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
	switch (key) {
		case ' ':
			break;
			
		case'p':
			bPreviewPointCloud = !bPreviewPointCloud;
            break;
            
		case'v':
			bShowVisuals = !bShowVisuals;
            break;
            
		case 'o':
			//kinect.open();
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
			networkMng.panel->saveToFile("broadcast.xml");
			post->saveToFile("postprocessing.xml");
			device->saveToFile(realSense->getSerialNumber(-1) + ".xml");
            break;

        case 'l':
            setupCalib->loadFromFile("settings.xml");
            blobFinder.panel->loadFromFile("trackings.xml");
            networkMng.panel->loadFromFile("broadcast.xml");
			post->loadFromFile("postprocessing.xml");
			device->loadFromFile(realSense->getSerialNumber(-1) + ".xml");
            break;

		case 'm':
			if(cam.getMouseInputEnabled()) cam.disableMouseInput();
			else cam.enableMouseInput();
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
			
		case 'w':
			//kinect.enableDepthNearValueWhite(!kinect.isDepthNearValueWhite());
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
    if(iMainCamera == 0 || iMainCamera == 1) {
        if(ofGetKeyPressed('x')){
            int posX = (x - VIEWGRID_WIDTH) / viewMain.width * REALSENSE_DEPTH_WIDTH;
            int posY = y;
            if(0 <= posX && posX < REALSENSE_DEPTH_WIDTH &&
               0 <= posY && posY < REALSENSE_DEPTH_HEIGHT)
                calibPoint_X.set(glm::vec2(posX, posY));
        }else if(ofGetKeyPressed('y')){
            int posX = (x - VIEWGRID_WIDTH) / viewMain.width * REALSENSE_DEPTH_WIDTH;
            int posY = y;
            if(0 <= posX && posX < REALSENSE_DEPTH_WIDTH &&
               0 <= posY && posY < REALSENSE_DEPTH_HEIGHT)
                calibPoint_Y.set(glm::vec2(posX, posY));
        }else if(ofGetKeyPressed('z')){
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


