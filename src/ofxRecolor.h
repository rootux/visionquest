//
//  recolorManager.h
//  ofxFlowToolsExample
//
//  Created by Roee Shenberg on 7/7/15.
//
//

#ifndef ofxFlowToolsExample_recolorManager_h
#define ofxFlowToolsExample_recolorManager_h

#include <vector>
#include "ofMain.h"
#include "ofxColorize.h"
#include "ofxColorize2d.h"
#include "ofxColorize3d.h"


class ofxRecolor {
    std::vector<ofTexture> textures1d;
    std::vector<ofTexture> textures2d;
    
    unsigned int textureIndex1d;
    // current texture swapping process. We tie the animations for both 2d and 1d in the same way
    float swapStartTime;
    float swapEndTime;
    
    
    GLuint texture3d;
    static const int NOISE_3D_SIZE = 32;
    
    ofTexture defaultLut1d;
    ofImage temp;
    ofPixels lookup;
    ofxColorize colorize1d;
    ofxColorize2d colorize2d;
    ofxColorize3d colorize3d;
    
    static const int NOISE_SIZE = 128;
    constexpr static const double NOISE_SCALE = 32.0;
    ofPixels noise1d;
    ofTexture noise1dTexture;
    
    void updateNoise() {
        ofColor_<unsigned char> color;
        for(int i = 0; i < NOISE_SIZE; i++) {
            color.r = 255.999 * ofNoise(i*NOISE_SCALE/NOISE_SIZE, ofGetElapsedTimef()/9.5123, 0);
            color.g = 255.999 * ofNoise(i*NOISE_SCALE/NOISE_SIZE, ofGetElapsedTimef()/11.5123, 1);
            color.b = 255.999 * ofNoise(i*NOISE_SCALE/NOISE_SIZE, ofGetElapsedTimef()/13.6123, 2);
            noise1d.setColor(i, 0, color);
        }
        noise1dTexture.loadData(noise1d);
    }
    
    void onNextTemplate1d(int &newTexture1d) {
        // Only when user manually sets the template
        if(!animateTextures) {
            currentTexture1d = nextTexture1d = newTexture1d;
        }
    }
    
    // Make sure we're animating to the same texture
    void onAnimateTextureChanged(bool &isOn) {
        if(!isOn) {
            currentTexture1d = nextTexture1d;
        }
    }
    
    void onNextTemplate2d(int &newTexture2d) {
        // Only when user manually sets the template
        if(!animateTextures) {
			if(nextTexture2d)
            currentTexture2d = nextTexture2d = newTexture2d;
        }
    }
    
    ofTexture& getLastTexture1d() {
        if (!useNoise && (currentTexture1d < textures1d.size())) {
            return textures1d[currentTexture1d];
        }
        return noise1dTexture; //defaultLut1d;
    }
    ofTexture& getTexture1d() {
        if (!useNoise && (nextTexture1d < textures1d.size())) {
            return textures1d[nextTexture1d];
        }
        return noise1dTexture; // defaultLut1d;
    }
    
    void loadTextures(const std::string& path, std::vector<ofTexture>& target) {
        ofDirectory dir(path);
        dir.allowExt("png");
        //populate the directory object
        dir.listDir();
        
        ofLogNotice() << "loading LUT textures for dir " << path;
        ofImage loader;
        for(int i = 0; i < dir.numFiles(); i++){
            ofLogNotice() << (dir.getPath(i));
            loader.loadImage(dir.getPath(i));
            ofTexture toLoad;
            toLoad.allocate(loader.getPixelsRef(), false);
            toLoad.loadData(loader.getPixelsRef());
            toLoad.setTextureWrap(GL_MIRRORED_REPEAT, GL_MIRRORED_REPEAT);
            target.push_back(toLoad);
        }
    }
    
    // random walk in 0..1 wth wrapping around the edges
    double randomWalk(double current, double divisor, double speed) {
        current += (ofNoise(ofGetElapsedTimef()/divisor) - 0.5)*speed;
        return current - floor(current);
    }
    
    void initTexture3d() {
        glEnable(GL_TEXTURE_3D);
        glGenTextures(1, &texture3d);
        
        glBindTexture(GL_TEXTURE_3D, texture3d);
        
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        unsigned char * texData = new unsigned char[NOISE_3D_SIZE*NOISE_3D_SIZE*NOISE_3D_SIZE*3];
        int pos = 0;
        for(int x = 0; x < NOISE_3D_SIZE; x++) {
            for(int y = 0; y < NOISE_3D_SIZE; y++) {
                for(int z = 0; z < NOISE_3D_SIZE; z++) {
                    texData[pos++] = ofNoise(x,y,z,0)*255;
                    texData[pos++] = ofNoise(x,y,z,1)*255;
                    texData[pos++] = ofNoise(x,y,z,2)*255;
                }
            }
        }
        
        glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB8, NOISE_3D_SIZE, NOISE_3D_SIZE, NOISE_3D_SIZE, 0, GL_RGB, GL_UNSIGNED_BYTE, texData);
        
        delete[] texData;
    }
    
    
    // animation state for offset - it's a wrap-around variable so we treat it special
    // 1d
    double offsetState;
    double externalOffset;
    // 2d
    double offsetXState, offsetYState, rotateState;
    
public:
    ofParameterGroup parameters;
    ofParameter<bool> active;
    ofParameter<int> noiseDimension;
    ofParameter<bool> useNoise;
    ofParameter<float> cutoff;
    ofParameter<float> offset;
    ofParameter<float> scale;
    ofParameter<bool> animateOffset;
    ofParameter<bool> animateScale;
    ofParameter<bool> animateRotation;
    ofParameter<float> externalOffsetScale;
    ofParameter<float> externalOffsetSmooth;
    ofParameter<int> currentTexture1d;
    ofParameter<int> currentTexture2d;
    ofParameter<int> nextTexture1d;
    ofParameter<int> nextTexture2d;
    ofParameter<bool> animateTextures;
    
    ofxRecolor() {
        textureIndex1d = 0;
        currentTexture1d = nextTexture1d = currentTexture2d = nextTexture2d = 0;
    }
    
    void setup() {
        // create 1d lookup table

        lookup.allocate(1024, 1, OF_IMAGE_COLOR);
        for(int i = 0; i < 1024; i++) {
            int r, g, b;
            if (i < 1024/3) {
                r = ofMap(i, 1024/3, 0, 256, 0);
                g = 0;
                b = ofMap(i, 1024/3, 0, 0, 255);
            } else if (i < 1024*2/3) {
                r = ofMap(i, 1024*2/3, 1024/3, 0, 255);
                g = ofMap(i, 1024/3, 1024*2/3, 0, 256);
                b = 0;
                
            } else {
                r = 0;
                g = ofMap(i, 1024, 1024*2/3, 0, 255);
                b = ofMap(i, 1024*2/3, 1024, 0, 255);
            }
            lookup[i*3] = r;
            lookup[i*3 + 1] = g;
            lookup[i*3 + 2] = b;
        }
        defaultLut1d.allocate(lookup, false);
        defaultLut1d.loadData(lookup);
        //temp.loadImage("color1d/1.png");
        //defaultLut1d.allocate(temp.getPixelsRef(), false);
        //defaultLut1d.loadData(temp.getPixelsRef());
        defaultLut1d.setTextureWrap(GL_MIRRORED_REPEAT, GL_MIRRORED_REPEAT);
        
        noise1d.allocate(NOISE_SIZE, 1, 3);
        noise1dTexture.allocate(noise1d, false);
        noise1dTexture.setTextureWrap(GL_MIRRORED_REPEAT, GL_MIRRORED_REPEAT);
        
        loadTextures("color1d/", textures1d);
        loadTextures("color2d/", textures2d);
        
        initTexture3d();
        
        parameters.setName("recolor");
        parameters.add(active.set("Active", true));
        parameters.add(nextTexture1d.set("Current Texture 1d", 0, 0, textures1d.size() - 1));
        nextTexture1d.addListener(this, &ofxRecolor::onNextTemplate1d);
        parameters.add(nextTexture2d.set("Current Texture 2d", 0, 0, textures2d.size() - 1));
		nextTexture2d.addListener(this, &ofxRecolor::onNextTemplate2d);
        parameters.add(animateTextures.set("Animate Textures", true));
        animateTextures.addListener(this, &ofxRecolor::onAnimateTextureChanged);
        //parameters.add(use2d.set("2D recoloring", false));
        parameters.add(noiseDimension.set("Noise Dimension", 1, 1, 3));
        parameters.add(useNoise.set("Use Noise", false));
        parameters.add(cutoff.set("Cutoff", 0.15, 0.0, 1.0));
        parameters.add(offset.set("Offset", 0.0, 0.0, 1.0));
        parameters.add(scale.set("Scale", 5, 0.0, 10.0));
        parameters.add(animateScale.set("Animate Scale", false));
        parameters.add(animateOffset.set("Animate Offset", false));
        parameters.add(animateRotation.set("Animate Rotation (2D)", false));
        parameters.add(externalOffsetScale.set("Depth Scale", 1, -1, 1));
        parameters.add(externalOffsetSmooth.set("Depth Smoothing", 0.85, 0.0, 1.0));
    }
    
    void update(ofFbo& _buffer, ofTexture& _source, bool _mirror, float _externalOffset = 0) {
        if (active) {
            // advance our noise - the random variable is the speed
            //offsetState += (ofNoise(ofGetElapsedTimef()/31.5246) - 0.5)*0.1;
            //offsetState = offsetState - floor(offsetState);
            offsetState = randomWalk(offsetState, 31.5246, 0.1);
            offsetXState = randomWalk(offsetXState, 61.9237, 0.001);
            offsetYState = randomWalk(offsetYState, 71.2231, 0.001);
            rotateState = randomWalk(rotateState, 41.5131, 0.01);
            
            float offsetAnimation = 0;
            float offsetXAnimation = 0;
            float offsetYAnimation = 0;
            if (animateOffset) {
                // offsetstate is 0..1, offset range for a full cycle is 0..2 due to MIRRORED_REPEAT
                offsetAnimation = 2*offsetState;
                offsetXAnimation = 2*offsetXState;
                offsetYAnimation = 2*offsetYState;
            }
            
            externalOffset = externalOffset*externalOffsetSmooth + _externalOffset*externalOffsetScale*(1 - externalOffsetSmooth);
            offsetAnimation += externalOffset;
            
            float scaleAnimation = 1;
            if (animateScale) {
                // scale animation is multiplicative, we'll exponentiate -1..1
                // noise to get a range from 1/e to e which is close to the range we have
                scaleAnimation = exp((ofNoise(ofGetElapsedTimef()/17.62357) - 0.5)*2);
            }
            
            float rotateAnimation = 0;
            if (animateRotation) {
                rotateAnimation = rotateState*PI*2;
            }
            
            if (ofGetElapsedTimef() >= swapEndTime) {
                swapStartTime = ofGetElapsedTimef();
                swapEndTime = swapStartTime + ofRandom(5,15);
                if(animateTextures) {
                    currentTexture1d = nextTexture1d;
                    currentTexture2d = nextTexture2d;
                    nextTexture2d = (int)ofRandom(0, textures2d.size() - 0.001);
                    nextTexture1d = (int)ofRandom(0, textures1d.size() - 0.001);
                }
            }
            
            updateNoise();

            switch(noiseDimension) {
                case 1:
                    colorize2d.update(_buffer, _source, getLastTexture1d(), getTexture1d(), _mirror, ofMap(ofGetElapsedTimef(), swapStartTime, swapEndTime, 0,1), scale*scaleAnimation, offset + offsetAnimation, 0, cutoff);
                    break;

                case 2:
                    colorize2d.update(_buffer, _source, textures2d[currentTexture2d], textures2d[nextTexture2d], _mirror, ofMap(ofGetElapsedTimef(), swapStartTime, swapEndTime, 0,1), scale*scaleAnimation, offsetXAnimation, offsetYAnimation, cutoff, cos(rotateAnimation), sin(rotateAnimation));
                    break;
                case 3:
                    colorize3d.update(_buffer, _source, texture3d, texture3d, _mirror, 0, scale*scaleAnimation);
                    break;
            }
        } else {
            _buffer.begin();
            if (_mirror)
                _source.draw(_buffer.getWidth(), 0, -_buffer.getWidth(), _buffer.getHeight());  // Flip Horizontal
            else
                _source.draw(0, 0, _buffer.getWidth(), _buffer.getHeight());
            // NOTE: was useful debugging
            //defaultLut1d.draw(0,500,_buffer.getWidth(),10);
            _buffer.end();
        }
    }
};

#endif
