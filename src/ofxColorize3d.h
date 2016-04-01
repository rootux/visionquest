//
//  ofxColorize3d.h
//  ofxFlowToolsExample
//
//  Created by Roee Shenberg on 9/28/15.
//
//

#ifndef ofxFlowToolsExample_ofxColorize3d_h
#define ofxFlowToolsExample_ofxColorize3d_h


#include "ofMain.h"
#include "ftShader.h"

class ofxColorize3d : public flowTools::ftShader {
    ofTexture defaultLut;
public:
    ofxColorize3d() {
        
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
                                 uniform sampler3D lut;
                                 uniform sampler3D lut2;
                                 
                                 uniform float cutoff;
                                 uniform vec3 offset;
                                 uniform vec3 rotation;
                                 uniform float blendFactor;
                                 uniform vec2 size; // size of the source texture
                                 uniform float scale;
                                 
                                 const vec2 xyToZ = vec2(1.01447, 0.789809);
                                 const float texToDepth = 65.536; // 65536/1000
                                 
                                 
                                 in vec2 texCoordVarying;
                                 out vec4 fragColor;
                                 
                                 void main(){
                                     //vec4 color = texture(source, texCoordVarying);
                                     float normalizedZ = texture(source, texCoordVarying).r;
                                     float z = normalizedZ*texToDepth;
                                     vec3 realWorld = vec3((texCoordVarying/size - 0.5) * xyToZ * z, z*2);
                                     
                                       // cutoffs
                                     if ((normalizedZ == 0) || (normalizedZ > cutoff)) {
                                        fragColor = vec4(0.0);
                                     } else {
                                         float ratio = 1.0 - normalizedZ/cutoff;
                                        fragColor = texture(lut, realWorld*scale + offset) * (1.0-blendFactor)
                                           + texture(lut2, realWorld + offset) * blendFactor;
                                         fragColor *= ratio;
                                     }
                                     //fragColor = vec4(realWorld, 1.0);
                                 }
                                 );
        
        
        shader.setupShaderFromSource(GL_VERTEX_SHADER, vertexShader);
        shader.setupShaderFromSource(GL_FRAGMENT_SHADER, fragmentShader);
        shader.bindDefaults();
        shader.linkProgram();
    }
    
public:
    
    void update(ofFbo& _buffer, ofTexture& _scalarTexture, GLuint _lut, GLuint _lut2, bool _mirror, float _blendFactor, float _scale = 0.5, float _offsetX = 0, float _offsetY = 0, float _cutoff = 1.0, float _dx = 1.0, float _dy = 0.0){
        _buffer.begin();
        shader.begin();
        shader.setUniformTexture("source", _scalarTexture, 0);
        shader.setUniformTexture("lut", GL_TEXTURE_3D, _lut, 1);
        shader.setUniformTexture("lut2", GL_TEXTURE_3D, _lut2, 2);
        shader.setUniform2f("position", _offsetX, _offsetY);
        shader.setUniform2f("direction", _dx*_scale/_cutoff, _dy*_scale/_cutoff);
        shader.setUniform1f("cutoff", _cutoff);
        shader.setUniform1f("blendFactor", _blendFactor);
        shader.setUniform1f("scale", _scale);
        shader.setUniform2f("size", _scalarTexture.getWidth(), _scalarTexture.getHeight());
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
