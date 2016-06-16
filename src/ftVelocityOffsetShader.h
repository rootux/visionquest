
#pragma once

#include "ofMain.h"
#include "ftShader.h"

namespace flowTools {
	
	class ftVelocityOffsetShader : public ftShader {
	public:
		ftVelocityOffsetShader() {
			
			if (ofIsGLProgrammableRenderer())
				glThree();
			else
				glTwo();
		}
		
	protected:
		void glTwo() {
			vertexShader = GLSL120(
                                   uniform sampler2DRect fieldTexture;
                                   uniform vec2 texResolution;
                                   uniform float vectorSize;
                                   uniform float maxSize;

                                   void main() {
                                       vec4 lineStart = gl_PositionIn[0];
                                       vec2 uv = lineStart.xy * texResolution;
                                       vec2 line = texture2DRect(fieldTexture, uv).xy * vectorSize;
                                       
                                       if (length(line) > maxSize)
                                           line = normalize(line) * maxSize;

                                       gl_Position = gl_ModelViewProjectionMatrix * (lineStart + vec4(line, 0, 0));
                                       gl_FrontColor = gl_Color;
                                   }
								);
			
			fragmentShader = GLSL120(
								  void main() {
									  gl_FragColor = gl_Color;
								  }
								  );
						
			shader.setupShaderFromSource(GL_VERTEX_SHADER, vertexShader);
			shader.setupShaderFromSource(GL_FRAGMENT_SHADER, fragmentShader);
			shader.linkProgram();

		}
		
		void glThree() {			
			vertexShader = GLSL150(
								uniform mat4 modelViewProjectionMatrix;
                                uniform sampler2DRect fieldTexture;
                                uniform vec2 texResolution;
                                uniform float vectorSize;
                                uniform float maxSize;
                                   
								in vec4 position;
								in vec2	texcoord;
								in vec4	color;
								
								out vec2 texCoordVarying;
								out vec4 colorVarying;
								
                                   float sigmoid(float t) {
                                       return 1./(1. + exp(- t + 3.5));
                                   }
								void main()
								{
									colorVarying = color;
                                    vec4 lineStart = position;
                                    vec2 uv = lineStart.xy * texResolution;
                                    vec2 line = texture(fieldTexture, uv).xy * vectorSize;
                                    /*
                                    if (length(line) > maxSize)
                                        line = normalize(line) * maxSize;
                                    */
                                    float len = length(line);
                                    line = line * (sigmoid(len)/len);
									gl_Position = modelViewProjectionMatrix * (position + vec4(line, 0, 0));
                                    
                                    texCoordVarying = uv * 4;// hack for differing texture size
								}
								
								);
			
			
			fragmentShader = GLSL150(
                                     uniform sampler2DRect colorTexture;
                                     in vec2 texCoordVarying;
								  out vec4 fragColor;
								  
								  void main()
								  {
                                      
									  fragColor = texture(colorTexture, texCoordVarying);
								  }
								  );
			
			shader.setupShaderFromSource(GL_VERTEX_SHADER, vertexShader);
			shader.setupShaderFromSource(GL_FRAGMENT_SHADER, fragmentShader);
			shader.bindDefaults();
			shader.linkProgram();
		}
	
	public:	
		void update(ofVboMesh& _fieldMesh, ofTexture& _floatTexture, float _vectorSize, float _maxSize, ofTexture& _colorMap, int resetIndex) {
			int width = _floatTexture.getWidth();
			int height = _floatTexture.getHeight();
            ofPushStyle();
            glEnable(GL_PRIMITIVE_RESTART);
            glPrimitiveRestartIndex(resetIndex); // set restart index
            ofDisableDepthTest();
            glDepthMask(GL_FALSE);
			shader.begin();
			shader.setUniformTexture("fieldTexture", _floatTexture,0);
            shader.setUniformTexture("colorTexture", _colorMap, 1);
			shader.setUniform2f("texResolution", width, height);
			shader.setUniform1f("vectorSize", _vectorSize / width * 10.0);
			shader.setUniform1f("maxSize", _maxSize);
			_fieldMesh.draw();
			shader.end();
            glDisable(GL_PRIMITIVE_RESTART);
            ofPopStyle();
		}
	};
}