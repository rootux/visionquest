
#pragma once

#include "ofMain.h"
#include "ftVelocityOffsetShader.h"

namespace flowTools {
	
	class ftVelocityOffset {
	public:
		
		void	allocate(int _width, int _height){
			width = _width;
			height = _height;
            
            resetIndex = (width + 1) * (height + 1);
			
			fieldMesh.setMode(OF_PRIMITIVE_LINE_STRIP);
			float xStep = 1. / width;
			float yStep = 1. / height;
            int index = 0;
            for (int y=0; y<height; y++) {
                for (int x=0; x<width; x++) {
                    fieldMesh.addVertex(ofVec3f((x + 0.5) * xStep, (y + 0.5) * yStep, 0));
                    fieldMesh.addIndex(index);
                    index++;
					//fieldMesh.addVertex(ofVec3f((x + 1.5) * xStep, (y + 0.5) * yStep, 0));
				}
                fieldMesh.addIndex(resetIndex);
			}
            fieldMesh.setUsage(GL_STATIC_DRAW);
            
            // TODO: fix size hack
            feedbackBuffer.allocate(_width*16, _height*16);
            feedbackBuffer.clear();
			
			
			parameters.setName("velocity lines");
			parameters.add(vectorSize.set("vector Size", 1, 0, 2));
			parameters.add(maxSize.set("maxSize", 1, 0, 1));
			parameters.add(lineSmooth.set("line smooth", false));
		};
		
		void    draw(int _x, int _y, int _width, int _height) {
			
			ofPushMatrix();
			ofPushStyle();
			
            feedbackBuffer.begin();
            
			ofEnableAlphaBlending();
            
            ofSetColor(0, 10);
            
            ofDrawRectangle(0, 0, feedbackBuffer.getWidth(), feedbackBuffer.getHeight());
            ofSetColor(255, 255);
            
			//ofDisableAntiAliasing();
			
			if (lineSmooth.get()) {
				glEnable(GL_LINE_SMOOTH);
			}
			// TODO: fit into 
			ofScale(feedbackBuffer.getWidth(), feedbackBuffer.getHeight());
//			fieldMesh.draw();
//			fieldVbo.draw(GL_POINTS, 0, vectorSize.get());
            if (colorTexture != NULL) {
                velocityOffsetShader.update(fieldMesh, *floatTexture, vectorSize.get(), maxSize.get(), *colorTexture, resetIndex);
            } else {
                velocityOffsetShader.update(fieldMesh, *floatTexture, vectorSize.get(), maxSize.get(), *floatTexture, resetIndex);
            }
			
			if (lineSmooth.get()) {
				glDisable(GL_LINE_SMOOTH);
			}
            
			
            feedbackBuffer.end();
            feedbackBuffer.draw(_x, _y, _width, _height);
            
			//ofEnableAntiAliasing();
			ofPopStyle();
			ofPopMatrix();
		}
		
		void	setSource(ofTexture& tex)	{ floatTexture = &tex; }
		void	setVectorSize(float _value)	{ vectorSize.set(_value); }
		void	setMaxSize(float _value)	{ maxSize.set(_value); }
		void	setLineSmooth(bool _value)	{ lineSmooth.set(_value); }
        void    setColorMap(ofTexture& tex) { colorTexture = &tex; }
		
		float	getVectorSize()	{ return vectorSize.get(); }
		float	getMaxSize()	{ return maxSize.get(); }
		bool	getLineSmooth() { return lineSmooth.get(); }
		int		getWidth()		{ return width; }
		int		getHeight()		{ return height; }
		
		ofParameterGroup	parameters;
		
	protected:
		int		width;
		int		height;
		
		ofParameter<float>	vectorSize;
		ofParameter<float>	maxSize;
		ofParameter<bool>	lineSmooth;
		
		//ofMesh		fieldMesh;
		ofTexture*	floatTexture;
		ofVboMesh	fieldMesh;
        ofTexture*  colorTexture;
                                            
        int resetIndex;
		
		ftVelocityOffsetShader velocityOffsetShader;
        
        ftFbo feedbackBuffer; //TODO: make global
		
	};
}

