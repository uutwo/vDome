#pragma once
#include "model.h"
#include "projector.h"
#include "menu.h"

#ifdef TARGET_OSX
#include "ofxSyphon.h"
#endif

#ifdef TARGET_WIN32
#include "spoutR.h"
#endif

#include "input.h"

namespace vd {
class vdome;
    
class Window : public ofBaseApp {

public:
    Window();

    void    init();
    void    setup();
    void    update();
    void    draw();
    
    void    loadXML(ofXml &xml);
    void    saveXML(ofXml &xml);

    void    keyPressed(int key);
    void    keyReleased(int key);
    
    void    mousePressed(ofMouseEventArgs& mouseArgs);
    void    mouseDragged(ofMouseEventArgs& mouseArgs);
    void    mouseReleased(ofMouseEventArgs& mouseArgs);
    
    ofPoint getPosition();
    void    setPosition(int x, int y);
    
    ofPoint getResolution();
    void    setResolution(int w, int h);
    
    void    setVSync(bool val);    
    void    setFrameRate(int val);
    
    int     index;
    Input   *input;
    Menu    menu;
    Model   model;
    int     projectorStartingIndex;

    enum    sources {MEDIA, CAPTURE, SYPHON, SPOUT, GRID, BLACK, WHITE, GREY, COLOR};
    
    static ofEvent<int> keyPressEvent;
    static ofEvent<int> keyReleaseEvent;

    vector<Projector>   projectors;

#ifdef TARGET_OSX
    ofxSyphonClient syphon;
#endif
    
#ifdef TARGET_WIN32
    SpoutR          spout;
#endif
    
private:
    int     x;
    int     y;
    int     width;
    int     height;
	bool    fullscreen;
    
    float   ratio;
    int     maxHistory;
    
    ofShader    shader;
};

}
