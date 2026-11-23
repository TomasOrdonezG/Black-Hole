#include "ofApp.h"
#include "game/Setup.h"
#include <memory>

//--------------------------------------------------------------
void ofApp::setup() {
    mEngine.setup();
    mEngine.addObject(std::make_shared<Setup>());
}

//--------------------------------------------------------------
void ofApp::update() {
    mEngine.update();
}

//--------------------------------------------------------------
void ofApp::draw() {
    mEngine.draw();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {
    mEngine.keyPressed(key);
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key) {
    mEngine.keyReleased(key);
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y) {
    mEngine.mouseMoved(x, y);
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button) {
    mEngine.mouseDragged(x, y, button);
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button) {
    mEngine.mousePressed(x, y, button);
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button) {
    mEngine.mouseReleased(x, y, button);
}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y) {
    mEngine.mouseEntered(x, y);
}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y) {
    mEngine.mouseExited(x, y);
}

//--------------------------------------------------------------
void ofApp::mouseScrolled(int x, int y, float scrollX, float scrollY) {
    mEngine.mouseScrolled(x, y, scrollX, scrollY);
}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h) {
    mEngine.windowResized(w, h);
}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg) {
    mEngine.gotMessage(msg);
}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo) {
    mEngine.dragEvent(dragInfo);
}
