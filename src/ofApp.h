#pragma once

#include "ofMain.h"
#include "ofxNetwork.h"
#include "ofxGuiExtended.h"
#include "BlobFinder.h"
#include "Planef.h"
#include "Linef.h"
#include "Grid.h"
#include "TrackingNetworkManager.h"
#include "Frustum.h"
#include "CaptureMeshArray.h"

#include "ofxRealSenseTwo.h"
#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API 

#include <ofMatrix4x4.h>

#define N_CAMERAS 6

#define VIEWGRID_WIDTH  132
#define MENU_WIDTH      1000
#define VIEWPORT_HEIGHT 480

#define REALSENSE_DEPTH_WIDTH   848
#define REALSENSE_DEPTH_HEIGHT  480

#define REALSENSE_VIDEO_WIDTH   848
#define REALSENSE_VIDEO_HEIGHT  480

#define N_MEASURMENT_CYCLES 10

using namespace std;
using namespace ofxRealSenseTwo;

//helpfull links during development:
// https://github.com/openframeworks/openFrameworks/issues/3817

class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();
        void exit();
        
		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);		

        vector <string> storeText;
		
        //ofxUDPManager udpConnection;

		ofTrueTypeFont  mono;
		ofTrueTypeFont  monosm;
		vector<ofPoint> stroke;
    

    bool bShowVisuals = false;

    //////////////////
    //    NETWORK   //
    //////////////////

    TrackingNetworkManager networkMng;
    
    //////////////////
    //OPENGL CAMERAS//
    //////////////////

    //viewports
    void setupViewports();
    
    ofRectangle viewMain;
    ofRectangle viewGrid[N_CAMERAS];

    //camera pointers
    ofCamera * cameras[N_CAMERAS];
    int iMainCamera;

    ofEasyCam cam;
    
    grid mainGrid;
    
    shared_ptr<ofBaseGLRenderer> opengl;
    shared_ptr<ofCairoRenderer> cairo;
    ofTexture render;
    
    /////////////
    //RealSense//
    /////////////
        
	RSDevicePtr realSense;

    ofMatrix4x4 unprojection;
    
    #ifdef USE_TWO_KINECTS
        ofxKinect kinect2;
    #endif

    bool dispRaw;

    bool bPreviewPointCloud;
    
    ofVboMesh previewmesh;//, capturemesh;
    
    CaptureMeshArray capMesh;
    
    Frustum realSenseFrustum;
	
	/**
	Changes operation of application
	@param _index 0=normal, 1= recording to file, 2=playback from file
	*/
	void changeOperation(int& _index);

    void drawPreview();
    void drawCapturePointCloud(bool _mask);

	void createGUIDeviceParams();

    void createFrustumCone();
    void updateFrustumCone(int & value);

    /////////////////
    //COLOR CONTOUR//
    /////////////////
    
    BlobFinder blobFinder;
            
    // used for viewing the point cloud
    ofEasyCam previewCam;
    
	ofShader shader;

    ///////////////
    //CALCULATION//
    ///////////////
    void updateCalc();
    void updateMatrix();
    void measurementCycleRaw();
    void measurementCycleFine();

    void drawCalibrationPoints();
	glm::vec3 calcPlanePoint(ofParameter<ofVec2f> & cpoint, int _size, int _step);
    
    bool bUpdateCalc = false;
    bool bUpdateMeasurment = false;
	bool bUpdateMeasurmentFine = false;
	bool bUpdateImageMask = false;
	char  bUpdateSetMesurmentPoint = -1;
    
    int cycleCounter = 0;
   
    ofVec3f planePoint1Meas[N_MEASURMENT_CYCLES];
    ofVec3f planePoint2Meas[N_MEASURMENT_CYCLES];
    ofVec3f planePoint3Meas[N_MEASURMENT_CYCLES];
    
    ofVec3f planePoint_X;
    ofVec3f planePoint_Y;
    ofVec3f planePoint_Z;

    ofVec3f planeCenterPoint;

    ofSpherePrimitive sphere_X;
    ofSpherePrimitive sphere_Y;
    ofSpherePrimitive sphere_Z;
    
    ofSpherePrimitive frustumCenterSphere;
    ofSpherePrimitive frustumTopSphere;

    ofVboMesh geometry;
        
    ofMatrix4x4 deviceTransform;

    string calcdata;
    
    bool bShowCalcData;

    //////////////
    //PROPERTIES//
    //////////////
    ofxGui gui;
    
    ofxGuiPanel *setupCalib;
	ofxGuiPanel *device;
	ofxGuiPanel *post;
	ofxGuiPanel *guitransform;
	ofxGuiPanel *operating;

	//mode panel
	ofxGuiGroup *operatingToggles;

	ofParameterGroup operatingModes;
	ofParameter<bool> mode0Capture;
	ofParameter<bool> mode1Record;
	ofParameter<bool> mode2Playback;

    ofParameter<ofVec2f> calibPoint_X;
    ofParameter<ofVec2f> calibPoint_Y;
    ofParameter<ofVec2f> calibPoint_Z;
 
	ofParameterGroup transformationGuiGroup;

    ofParameter<ofMatrix4x4> transformation;
    
    ofParameterGroup frustumGuiGroup;

    ofParameter<int> nearFrustum;
    ofParameter<int> farFrustum;

    ofParameterGroup intrinsicGuiGroup;

    ofParameter<float> depthCorrectionBase;
    ofParameter<float> depthCorrectionDivisor;
    ofParameter<float> pixelSizeCorrector;

    ofParameter<int> blobGrain;

    ofParameter<bool> captureVideo;

    //////////
    // HELP //
    //////////
    string help;

    bool bShowHelp = true;

    void createHelp();

};

