#include "ofApp.h"
#include "game/Setup.h"
#include <memory>

//--------------------------------------------------------------
void ofApp::setup() {
    mEngine.setup();
    mEngine.addObject(std::make_shared<Setup>());
}

//--------------------------------------------------------------
void ofApp::update() { mEngine.update(); }

//--------------------------------------------------------------
void ofApp::draw() { mEngine.draw(); }

//--------------------------------------------------------------
void ofApp::keyPressed(int key) { mEngine.keyPressed(key); }

//--------------------------------------------------------------
void ofApp::keyReleased(int key) {}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y) {}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button) { mEngine.mouseDragged(x, y, button); }

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button) { mEngine.mousePressed(x, y, button); }

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button) {}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y) {}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y) {}

//--------------------------------------------------------------
void ofApp::mouseScrolled(int x, int y, float scrollX, float scrollY) { mEngine.mouseScrolled(x, y, scrollX, scrollY); }

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h) { mEngine.resize(w, h); }

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg) {}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo) {}
