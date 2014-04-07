#include "projector.h"
extern float projCount;
extern float projWidth;
extern float projHeight;

/******************************************
 
 INIT
 
 ********************************************/

void Projector::init(int i){
    
    // defaults
    index = i;

    keyboard = false;
    mouse = false;
    xmlPrefix = "projectors/projector[";
    
    // intensity
    brightness = 1;
    contrast = 1;
    
    // color
    saturation = 1;
    
    // plane
    planePosition.set(0,0);
    planeDimensions.set(projWidth,projHeight);

    // camera
    cameraPosition.set(0,0,10);    // azi, ele, dis
    cameraOrientation.set(0,-20,0);    // roll, tilt, pan
    cameraFov = 40;
    cameraOffset.set(0,0);
    cameraScale.set(1,1);
    
    /* shear
     1,   xy,   xz,  0,
     yx,   1,   yz,  0,
     zx,  zy,   1,   0,
     0,    0,   0,   1
     */
    cameraShear.push_back(0); // 0=xy
    cameraShear.push_back(0); // 1=xz
    cameraShear.push_back(0); // 2=yx
    cameraShear.push_back(0); // 3=yz
    cameraShear.push_back(0); // 4=zx
    cameraShear.push_back(0); // 5=zy
    
    setup();
}




/******************************************
 
 SETUP
 
 ********************************************/

void Projector::setup() {
    
    // projection plane
    plane.setup(index);
    planePosition.x = plane.position[0];
    planePosition.y = plane.position[1];
    
    // create camera
    camera.setScale(1,-1,1); // legacy oF oddity
    camera.setNearClip(5);
    camera.setLensOffset(cameraOffset);
    camera.setFov(cameraFov);
    setCameraTransform();
    
    // create camera view
    view.setWidth(planeDimensions.x);
    view.setHeight(planeDimensions.y);
    
    // create camera fbo
    if (fbo.getWidth() != planeDimensions.x || fbo.getHeight() != planeDimensions.y) {
        fbo.allocate(planeDimensions.x, planeDimensions.y, GL_RGBA);
    }
    
    fbo.begin();
        ofClear(255);
    fbo.end();    
}



// camera transform
void Projector::setCameraTransform(){
    camera.resetTransform();

    camera.roll(cameraOrientation.x);
    camera.tilt(cameraOrientation.y);
    camera.pan(cameraOrientation.z+cameraPosition.x);
   
    // spherical coordinates: azi, ele, dis
    ofVec3f car = sphToCar(ofVec3f(cameraPosition.x, cameraPosition.y, cameraPosition.z));
    camera.setPosition(car);
}



/******************************************
 
 BEGIN
 
 ********************************************/

void Projector::begin() {
    ofMatrix4x4 mat = camera.getProjectionMatrix(view);
    ofMatrix4x4 transform;
 
    /*
    shear
        1,   xy,   xz,  0,
        yx,   1,   yz,  0,
        zx,  zy,   1,   0,
        0,    0,   0,   1
    */
    transform.set(
      cameraScale.x,   cameraShear[0],   cameraShear[1],   0,
      cameraShear[2],   cameraScale.y,   cameraShear[3],   0,
      cameraShear[4],   cameraShear[5],             1,   0,
             0,          0,          0,             1
    );

    ofMatrix4x4 m;
    m.makeFromMultiplicationOf(transform, mat);
    camera.setProjectionMatrix(m);
    
    fbo.begin();
        ofClear(0, 0, 0, 0);
        camera.begin(view);
}

void Projector::end() {
        camera.end();
    fbo.end();
}




/******************************************
 
 BIND
 
 ********************************************/

void Projector::bind() {
    if (active)
        mask.draw();
    fbo.getTextureReference().bind();
}

void Projector::unbind() {
    fbo.getTextureReference().unbind();
}



/******************************************
 
 DRAW
 
 ********************************************/

void Projector::draw() {
    glEnable(GL_CULL_FACE);
    glCullFace( GL_FRONT );
    plane.draw();
    glDisable(GL_CULL_FACE);
}

void Projector::drawPlaneConfig(){
   plane.drawConfig();
}

void Projector::drawKeystone(){
    plane.keystone.draw();
}

/******************************************
 
 MOUSE
 
 ********************************************/

vector<ofPoint> lastKey;
vector<ofVec3f> lastGrid;


void Projector::mousePressed(ofMouseEventArgs& mouseArgs) {
    if (editMode == CORNERPIN || editMode == GRID) {
        if (editMode == CORNERPIN) {
            lastKey = getKeystonePoints();
        }
        else if (editMode == GRID) {
            lastGrid = getGridPoints();
        }
        plane.onMousePressed(mouseArgs);
    }
    else if (editMode == BRUSH_SCALE || editMode == BRUSH_OPACITY) {
        mask.mousePressed(mouseArgs);
    }
}

void Projector::mouseDragged(ofMouseEventArgs& mouseArgs) {
    if (editMode == CORNERPIN || editMode == GRID) {
        plane.onMouseDragged(mouseArgs);
    }
    else if (editMode == BRUSH_SCALE || editMode == BRUSH_OPACITY) {
        mask.mouseDragged(mouseArgs);
    }
}

void Projector::mouseReleased(ofMouseEventArgs& mouseArgs) {
    if (editMode == CORNERPIN || editMode == GRID) {
        if (editMode == CORNERPIN) {
            vector<ofPoint> value = plane.getKeystonePoints();
            history.execute( new SetKeystonePoints(*this, value, lastKey) );
        }
        else if (editMode == GRID) {
            vector<ofPoint> value = plane.getGridPoints();
            history.execute( new SetGridPoints(*this, value, lastGrid) );
        }
        plane.onMouseReleased(mouseArgs);
    }
    else if (editMode == BRUSH_SCALE || editMode == BRUSH_OPACITY) {
        mask.mouseReleased(mouseArgs);
    }
}

/******************************************
 
 KEYBOARD
 
 ********************************************/

void Projector::keyPressed(int key) {
    
    if (editMode == CORNERPIN || editMode == GRID)
        plane.keyPressed(key);

    switch (key) {
        case 122:
            history.undo();
            break;
        case 121:
            history.redo();
            break;
            
        case 114: // reset
            switch (editMode) {
                    
                case 1: // projector intensity
                    history.execute( new SetContrast(*this, 1) );
                    history.execute( new SetBrightness(*this, 1) );
                    break;
                    
                case 2: // projector color
                    history.execute( new SetSaturation(*this, 1) );
                    break;
                    
                case 3:
                    // projector keystone
                    plane.resetKeystone();
                    break;
                    
                case 4:
                    // projector grid
                    plane.resetGrid();
                    break;
                    
                case 5: // projector position
                    history.execute( new SetCameraPosition(*this, 0,0,5) );
                    break;
                    
                case 6: // projector orientation
                    history.execute( new SetCameraOrientation(*this, 0,0,0) );
                    break;
                    
                case 7: // projector fov
                    history.execute( new SetCameraFov(*this, 33) );
                    break;
                    
                case 8: // projector offset
                    history.execute( new SetCameraOffset(*this, 0, 0) );
                    break;
                    
                case 9: // projector scale
                    history.execute( new SetCameraScale(*this, 1, 1) );
                    break;
                    
                case 10: // projector shear 1
                    cameraShear[3] = 0;
                    cameraShear[4] = 0;
                    cameraShear[1] = 0;
                    history.execute( new SetCameraShear(*this, cameraShear ) );
                    break;
                    
                case 11: // projector shear 2
                    cameraShear[5] = 0;
                    cameraShear[2] = 0;
                    cameraShear[0] = 0;
                    history.execute( new SetCameraShear(*this, cameraShear ) );
                    break;
            }
            break;
            
            
        case OF_KEY_RIGHT:  // up = switch on mode
            switch (editMode) {
                case NONE:
                    break;
                    
                case BRIGHTNESS:
                    history.execute( new SetBrightness(*this, brightness + value * .1) );
                    break;
                case CONTRAST:
                    history.execute( new SetContrast(*this, contrast + value * .1) );
                    break;
                case BRUSH_SCALE:
                    history.execute( new SetBrushScale(*this, mask.brushScale + value * .1) );
                    break;
                case BRUSH_OPACITY:
                    history.execute( new SetBrushOpacity(*this, mask.brushOpacity + value) );
                    break;
                    
                case SATURATION:
                    history.execute( new SetSaturation(*this, saturation + value * .1) );
                    break;
                    
                case CORNERPIN:
                    break;
                case GRID:
                    break;
                    
                case AZIMUTH:
                    history.execute( new SetCameraPosition(*this, cameraPosition.x + value, cameraPosition.y, cameraPosition.z) );
                    break;
                case ELEVATION:
                    history.execute( new SetCameraPosition(*this, cameraPosition.x, cameraPosition.y + value, cameraPosition.z) );
                    break;
                case DISTANCE:
                    history.execute( new SetCameraPosition(*this, cameraPosition.x, cameraPosition.y, cameraPosition.z + value) );
                    break;
 
                case ROLL:
                    history.execute( new SetCameraOrientation(*this, cameraOrientation.x + value, cameraOrientation.y, cameraOrientation.z) );
                    break;
                case TILT:
                    history.execute( new SetCameraOrientation(*this, cameraOrientation.x, cameraOrientation.y + value, cameraOrientation.z) );
                    break;
                case PAN:
                    history.execute( new SetCameraOrientation(*this, cameraOrientation.x, cameraOrientation.y, cameraOrientation.z + value) );
                    break;
                    
                case FOV: 
                    history.execute( new SetCameraFov(*this, cameraFov + value) );
                    break;

                case SCALE: 
                    history.execute( new SetCameraScale(*this, cameraScale.x + value * .1, cameraScale.y + value * .1) );
                    break;
                case SCALE_X:
                    history.execute( new SetCameraScale(*this, cameraScale.x + value * .1, cameraScale.y) );
                    break;
                case SCALE_Y:
                    history.execute( new SetCameraScale(*this, cameraScale.x, cameraScale.y + value * .1) );
                    break;
                    
                case SHEAR_XY:
                    cameraShear[0] += value *.1;
                    history.execute( new SetCameraShear(*this, cameraShear ) );
                    break;
                case SHEAR_XZ:
                    cameraShear[1] += value *.1;
                    history.execute( new SetCameraShear(*this, cameraShear ) );
                    break;
                case SHEAR_YX:
                    cameraShear[2] += value *.1;
                    history.execute( new SetCameraShear(*this, cameraShear ) );
                    break;
                case SHEAR_YZ:
                    cameraShear[3] += value *.1;
                    history.execute( new SetCameraShear(*this, cameraShear ) );
                    break;
                case SHEAR_ZX:
                    cameraShear[4] += value *.1;
                    history.execute( new SetCameraShear(*this, cameraShear ) );
                    break;
                case SHEAR_ZY:
                    cameraShear[5] += value *.1;
                    history.execute( new SetCameraShear(*this, cameraShear ) );
                    break;
            }
            break;
            

            
        case OF_KEY_LEFT:  // up = switch on mode
            switch (editMode) {
                case NONE:
                    break;
                    
                case BRIGHTNESS:
                    history.execute( new SetBrightness(*this, brightness - value * .1) );
                    break;
                case CONTRAST:
                    history.execute( new SetContrast(*this, contrast - value * .1) );
                    break;
                case BRUSH_SCALE:
                    history.execute( new SetBrushScale(*this, mask.brushScale - value * .1) );
                    break;
                case BRUSH_OPACITY:
                    history.execute( new SetBrushOpacity(*this, mask.brushOpacity - value) );
                    break;
                    
                case SATURATION:
                    history.execute( new SetSaturation(*this, saturation - value * .1) );
                    break;
                    
                case CORNERPIN:
                    break;
                case GRID:
                    break;
                    
                case AZIMUTH:
                    history.execute( new SetCameraPosition(*this, cameraPosition.x - value, cameraPosition.y, cameraPosition.z) );
                    break;
                case ELEVATION:
                    history.execute( new SetCameraPosition(*this, cameraPosition.x, cameraPosition.y - value, cameraPosition.z) );
                    break;
                case DISTANCE:
                    history.execute( new SetCameraPosition(*this, cameraPosition.x, cameraPosition.y, cameraPosition.z - value) );
                    break;
                    
                case ROLL:
                    history.execute( new SetCameraOrientation(*this, cameraOrientation.x - value, cameraOrientation.y, cameraOrientation.z) );
                    break;
                case TILT:
                    history.execute( new SetCameraOrientation(*this, cameraOrientation.x, cameraOrientation.y - value, cameraOrientation.z) );
                    break;
                case PAN:
                    history.execute( new SetCameraOrientation(*this, cameraOrientation.x, cameraOrientation.y, cameraOrientation.z - value) );
                    break;
                    
                case FOV:
                    history.execute( new SetCameraFov(*this, cameraFov - value) );
                    break;
                    
                case SCALE:
                    history.execute( new SetCameraScale(*this, cameraScale.x - value * .1, cameraScale.y - value * .1) );
                    break;
                case SCALE_X:
                    history.execute( new SetCameraScale(*this, cameraScale.x - value * .1, cameraScale.y) );
                    break;
                case SCALE_Y:
                    history.execute( new SetCameraScale(*this, cameraScale.x, cameraScale.y - value * .1) );
                    break;
                    
                case SHEAR_XY:
                    cameraShear[0] -= value *.1;
                    history.execute( new SetCameraShear(*this, cameraShear ) );
                    break;
                case SHEAR_XZ:
                    cameraShear[1] -= value *.1;
                    history.execute( new SetCameraShear(*this, cameraShear ) );
                    break;
                case SHEAR_YX:
                    cameraShear[2] -= value *.1;
                    history.execute( new SetCameraShear(*this, cameraShear ) );
                    break;
                case SHEAR_YZ:
                    cameraShear[3] -= value *.1;
                    history.execute( new SetCameraShear(*this, cameraShear ) );
                    break;
                case SHEAR_ZX:
                    cameraShear[4] -= value *.1;
                    history.execute( new SetCameraShear(*this, cameraShear ) );
                    break;
                case SHEAR_ZY:
                    cameraShear[5] -= value *.1;
                    history.execute( new SetCameraShear(*this, cameraShear ) );
                    break;
            }
            break;
            
        default:
            break;
    } 
}

void Projector::keyReleased(int key) {
    if (keyboard)
        plane.keyReleased(key);
}

/******************************************
 
 SETTINGS
 
 ********************************************/

void Projector::loadXML(ofXml &xml) {
    string str;
    float val;
    string pre = xmlPrefix + ofToString(index);
    
    
    // intensity
    if (xml.exists(pre + "][@brightness]")) {
        val = ofToFloat( xml.getAttribute(pre + "][@brightness]") );
        brightness = val;
    }
    if (xml.exists(pre + "][@contrast]")) {
        val = ofToFloat( xml.getAttribute(pre + "][@contrast]") );
        contrast = val;
    }
    
    
    // color
    if (xml.exists(pre + "][@saturation]")) {
        val = ofToFloat( xml.getAttribute(pre + "][@saturation]") );
        saturation = val;
    }
    
    
    // plane position
    /*
     if (xml.exists(pre + "][@x]")) {
     val = ofToFloat( xml.getAttribute(pre + "][@x]") );
     history.execute( new SetPlanePosition(*this, val, getPlanePosition().y) );
     }
     if (xml.exists(pre + "][@y]")) {
     val = ofToFloat( xml.getAttribute(pre + "][@y]") );
     history.execute( new SetPlanePosition(*this, getPlanePosition().x, val) );
     }*/
    
    
    // plane dimensions - global    ---> now setting with global vars
    /*if (xml.exists("projectors[@dimensions]")) {
        str = xml.getAttribute("projectors[@dimensions]");
        float w  = ofToFloat(ofSplitString(str, ",")[0]);
        float h  = ofToFloat(ofSplitString(str, ",")[1]);
        setPlaneDimensions(w, h);
    }
    // plane dimensions - local    
    else if (xml.exists(pre + "][@dimensions]")) {
        str = xml.getAttribute(pre + "][@dimensions]");
        float w  = ofToFloat(ofSplitString(str, ",")[0]);
        float h  = ofToFloat(ofSplitString(str, ",")[1]);
        setPlaneDimensions(w, h);
    }*/
    
    
    
    plane.load(xml);
    
    
    
    
    // camera position
    if (xml.exists(pre + "][@position]")) {
        str = xml.getAttribute(pre + "][@position]");
        float azi  = ofToFloat(ofSplitString(str, ",")[0]);
        float ele  = ofToFloat(ofSplitString(str, ",")[1]);
        float dis  = ofToFloat(ofSplitString(str, ",")[2]);
        setCameraPosition(azi, ele, dis);
    }
    
    // camera orientation
    if (xml.exists(pre + "][@orientation]")) {
        str = xml.getAttribute(pre + "][@orientation]");
        float roll = ofToFloat(ofSplitString(str, ",")[0]);
        float tilt = ofToFloat(ofSplitString(str, ",")[1]);
        float pan = ofToFloat(ofSplitString(str, ",")[2]);
        setCameraOrientation(roll, tilt, pan);
    }
    
    // camera lens fov
    if (xml.exists(pre + "][@fov]")) {
        val = ofToFloat( xml.getAttribute(pre + "][@fov]") );
        setCameraFov(val);
    }
    
    //camera lens offset
    if (xml.exists(pre + "][@offset]")) {
        str = xml.getAttribute(pre + "][@offset]");
        float offX  = ofToFloat(ofSplitString(str, ",")[0]);
        float offY  = ofToFloat(ofSplitString(str, ",")[1]);
        setCameraOffset(offX, offY);
    }
    
    // camera scale
    if (xml.exists(pre + "][@scale]")) {
        str = xml.getAttribute(pre + "][@scale]");
        float sx  = ofToFloat(ofSplitString(str, ",")[0]);
        float sy  = ofToFloat(ofSplitString(str, ",")[1]);
        setCameraScale(sx, sy);
    }
    
    mask.load(index);
}

void Projector::saveXML(ofXml &xml) {
    string pre = xmlPrefix + ofToString(index);
    
    xml.setAttribute(pre + "][@brightness]", ofToString(brightness));
    xml.setAttribute(pre + "][@contrast]", ofToString(contrast));
    xml.setAttribute(pre + "][@saturation]", ofToString(saturation));
    
    //camera
    xml.setAttribute(pre + "][@position]", ofToString(cameraPosition.x) +  "," + ofToString(cameraPosition.y) +  "," + ofToString(cameraPosition.z) );
    xml.setAttribute(pre + "][@orientation]", ofToString(cameraOrientation.x) +  "," + ofToString(cameraOrientation.y) +  "," + ofToString(cameraOrientation.z) );
    xml.setAttribute(pre + "][@fov]", ofToString(cameraFov));
    xml.setAttribute(pre + "][@offset]", ofToString(cameraOffset.x) +  "," + ofToString(cameraOffset.y) );
    xml.setAttribute(pre + "][@scale]", ofToString(cameraScale.x) +  "," + ofToString(cameraScale.y) );
    
    // plane
    //xml.setAttribute(pre + "][@dimensions]", ofToString(planeDimensions.x) +  "," + ofToString(planeDimensions.y) );
    
    
    plane.save(xml);
    mask.save(index);
}



/******************************************
 
 ACCESSORS
 
 ********************************************/


// plane
ofVec2f Projector::getPlanePosition(){
    return planePosition;
}
void Projector::setPlanePosition(float x, float y){
    planePosition.set(x,y);
}

ofVec2f Projector::getPlaneDimensions(){
    return planeDimensions;
}
void Projector::setPlaneDimensions(float x, float y){
    planeDimensions.set(x,y);
    setup();
}

// camera
ofVec3f Projector::getCameraPosition(){
    return cameraPosition;
}
void Projector::setCameraPosition(float azi, float ele, float dis){
    cameraPosition.set(azi, ele, dis);
    setCameraTransform();
}

ofVec3f Projector::getCameraOrientation(){
    return cameraOrientation;
}
void Projector::setCameraOrientation(float roll, float tilt, float pan){
    cameraOrientation.set(roll, tilt, pan);
    setCameraTransform();
}

float Projector::getCameraFov(){
    return cameraFov;
}
void Projector::setCameraFov(float v){
    cameraFov = v;
    camera.setFov(v);
}

ofVec2f Projector::getCameraOffset(){
    return cameraOffset;
}
void Projector::setCameraOffset(float x, float y){
    cameraOffset.set(x,y);
    camera.setLensOffset(cameraOffset);
}

ofVec2f Projector::getCameraScale(){
    return cameraScale;
}
void Projector::setCameraScale(float x, float y){
    cameraScale.set(x,y);
}

vector<float> Projector::getCameraShear(){
    return cameraShear;
}
void Projector::setCameraShear(vector<float> v){
    cameraShear = v;
}

// scalar value
void Projector::setValue(float v) {
    value = v;
    plane.value = value;
    plane.keystone.value = value;
}

// keystone
bool Projector::getKeystoneActive() {
    return plane.keystoneActive;
}
void Projector::setKeystoneActive(bool v) {
    plane.keystoneActive = v;
}

vector<ofPoint> Projector::getKeystonePoints() {
    return plane.getKeystonePoints();
}
void Projector::setKeystonePoints(vector<ofPoint> pts) {
    plane.setKeystonePoints(pts);
}

// grid active
bool Projector::getGridActive() {
    return plane.gridActive;
}
void Projector::setGridActive(bool v) {
    plane.gridActive = v;
}

vector<ofVec3f> Projector::getGridPoints(){
    return plane.getGridPoints();
}
void Projector::setGridPoints(vector<ofVec3f> v){
    plane.setGridPoints(v);
}


// texture
ofTexture& Projector::getTextureReference(){
	return fbo.getTextureReference();
}


/******************************************
 
 SPHERICAL TO CARTESIAN COORDINATES
 
 ********************************************/

ofVec3f Projector::sphToCar(ofVec3f t) {
    float azi, ele, dis;
    float x, y, z;
    azi = ofDegToRad(t.x);
    ele = ofDegToRad(t.y+90);
    dis = t.z;
    x = sin(azi) * sin(ele) * dis;
    y = cos(ele) * dis;
    z = cos(azi) * sin(ele) * dis;
    return ofVec3f(x,y,z);
};

