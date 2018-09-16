//
//  Frustum.cpp
//  kinectTCPServer
//
//  Created by maybites on 14.02.14.
//
//

#include "Frustum.h"

Frustum::Frustum(){
}

void Frustum::draw(){
    frustum.draw();
}

void Frustum::drawWireframe(){
    frustum.drawWireframe();
}

void Frustum::update(){
    frustum.clear();
    frustum.setMode(OF_PRIMITIVE_LINES);
    frustum.addVertex(ofPoint(left, -top, -near1));
    frustum.addVertex(ofPoint(leftFar, -topFar, -far1));
    
    frustum.addVertex(ofPoint(right, -top, -near1));
    frustum.addVertex(ofPoint(rightFar, -topFar, -far1));
    
    frustum.addVertex(ofPoint(right, -bottom, -near1));
    frustum.addVertex(ofPoint(rightFar, -bottomFar, -far1));
    
    frustum.addVertex(ofPoint(left, -bottom, -near1));
    frustum.addVertex(ofPoint(leftFar, -bottomFar, -far1));
    
    
    frustum.addVertex(ofPoint(left, -top, -near1));
    frustum.addVertex(ofPoint(right, -top, -near1));
    
    frustum.addVertex(ofPoint(right, -top, -near1));
    frustum.addVertex(ofPoint(right, -bottom, -near1));
    
    frustum.addVertex(ofPoint(right, -bottom, -near1));
    frustum.addVertex(ofPoint(left, -bottom, -near1));
    
    frustum.addVertex(ofPoint(left, -bottom, -near1));
    frustum.addVertex(ofPoint(left, -top, -near1));
    
    
    frustum.addVertex(ofPoint(leftFar, -topFar, -far1));
    frustum.addVertex(ofPoint(rightFar, -topFar, -far1));
    
    frustum.addVertex(ofPoint(rightFar, -topFar, -far1));
    frustum.addVertex(ofPoint(rightFar, -bottomFar, -far1));
    
    frustum.addVertex(ofPoint(rightFar, -bottomFar, -far1));
    frustum.addVertex(ofPoint(leftFar, -bottomFar, -far1));
    
    frustum.addVertex(ofPoint(leftFar, -bottomFar, -far1));
    frustum.addVertex(ofPoint(leftFar, -topFar, -far1));

}