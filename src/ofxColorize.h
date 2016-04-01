//
//  ofxColorize.h
//  ofxFlowToolsExample
//
//  Created by Roee Shenberg on 7/7/15.
//
//

#ifndef ofxFlowToolsExample_ofxColorize_h
#define ofxFlowToolsExample_ofxColorize_h

#include "ofMain.h"
#include "ftShader.h"

class ofxColorize : public flowTools::ftShader {
public:
    ofxColorize() {

        ofLogVerbose("init ftToScalarShader");
        if (ofIsGLProgrammableRenderer())
            glThree();
        else
            glTwo();
    }
    
protected:
    void glTwo() {
        fragmentShader = GLSL120(
                                 uniform sampler2DRect ScalarTexture;
                                 uniform float Scale;
                                 void main(){
                                     vec4	velocity = texture2DRect(normalTexture, gl_TexCoord[0].st);
                                     velocity.xyz -= vec3(Scale * 0.5);
                                     velocity.xyz *= vec3(Scale);
                                     velocity.w = 0.0;
                                     gl_FragColor = velocity;
                                 }
                                 );
        
        shader.setupShaderFromSource(GL_FRAGMENT_SHADER, fragmentShader);
        shader.linkProgram();
    }
    
    void glThree() {
        fragmentShader = GLSL150(
                                 uniform sampler2DRect source;
                                 uniform sampler2D lut;
                                 uniform sampler2D lut2;
                                 
                                 uniform float cutoff;
                                 uniform float offset;
                                 uniform float scale;
                                 uniform float blendFactor;
                                 
                                 in vec2 texCoordVarying;
                                 out vec4 fragColor;
                                 
                                 void main(){
                                     //vec4 color = texture(source, texCoordVarying);
                                     float textureCoord = texture(source, texCoordVarying).r;
                                     // cutoffs
                                     if ((textureCoord == 0) || (textureCoord > cutoff)) {
                                         fragColor = vec4(0.0);
                                     } else {
                                         fragColor = texture(lut, vec2(textureCoord*scale + offset,0)) * (1.0-blendFactor)
                                                    +texture(lut2, vec2(textureCoord*scale + offset,0)) * blendFactor;
                                     }
                                 }
                                 );
        
        
        shader.setupShaderFromSource(GL_VERTEX_SHADER, vertexShader);
        shader.setupShaderFromSource(GL_FRAGMENT_SHADER, fragmentShader);
        shader.bindDefaults();
        shader.linkProgram();
    }
    
public:
    
    void update(ofFbo& _buffer, ofTexture& _scalarTexture, ofTexture& _lut, ofTexture& _lut2, bool _mirror, float _blendFactor, float _scale = 0.5, float _offset = 0, float _cutoff = 1.0){
        _buffer.begin();
        shader.begin();
        shader.setUniformTexture("source", _scalarTexture, 0);
        shader.setUniformTexture("lut", _lut, 1);
        shader.setUniformTexture("lut2", _lut2, 2);
        shader.setUniform1f("scale", _scale/_cutoff);
        shader.setUniform1f("offset", _offset);
        shader.setUniform1f("cutoff", _cutoff);
        shader.setUniform1f("blendFactor", _blendFactor);
        if (!_mirror) {
            renderFrame(_buffer.getWidth(), _buffer.getHeight(), _scalarTexture.getWidth(), _scalarTexture.getHeight());
        } else {
            renderFrameMirrored(_buffer.getWidth(), _buffer.getHeight(), _scalarTexture.getWidth(), _scalarTexture.getHeight());
        }
        shader.end();
        //_lut.draw(0,500,_buffer.getWidth(),10);
        _buffer.end();
    }
};

#endif
