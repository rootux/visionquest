#pragma once
#include "ofMain.h"
#include "ftShader.h"
#include "ftFbo.h"

namespace flowTools {
	class ftDrawMaskedShader : public ftShader {
public:
	ftDrawMaskedShader() {

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
#if 0
		vertexShader = GLSL150(
			uniform mat4 modelViewProjectionMatrix;
		uniform float vectorSize;
		uniform float maxSize;

		in vec4 position;
		in vec2	texcoord;
		in vec4	color;

		out vec2 texCoordVarying;
		out vec4 colorVarying;

		float sigmoid(float t) {
			return 1. / (1. + exp(-t + 3.5));
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
			line = line * (sigmoid(len) / len);
			gl_Position = modelViewProjectionMatrix * (position + vec4(line, 0, 0));

			texCoordVarying = uv * 4;// hack for differing texture size
		}

		);
#endif

		fragmentShader = GLSL150(
			uniform sampler2DRect colorTexture;
		uniform sampler2DRect maskTexture;
		uniform float maxValid;

		in vec2 texCoordVarying;
		out vec4 fragColor;

		void main()
		{
			if (texture(maskTexture, texCoordVarying).r <= maxValid) {
				fragColor = texture(colorTexture, texCoordVarying);
			}
			else {
				fragColor = vec4(0, 0, 0, 1);
			}
		}
		);

		shader.setupShaderFromSource(GL_VERTEX_SHADER, vertexShader);
		shader.setupShaderFromSource(GL_FRAGMENT_SHADER, fragmentShader);
		shader.bindDefaults();
		shader.linkProgram();
	}

public:
	void update(ofFbo& dest, ofTexture& _colorTexture, ofTexture& _maskTexture, int _bodyCount) {
		ofPushStyle();
		dest.begin();
		shader.begin();
		shader.setUniformTexture("colorTexture", _colorTexture, 0);
		shader.setUniformTexture("maskTexture", _maskTexture, 1);
		shader.setUniform1f("maxValid", (_bodyCount / 255.0f) +  0.5f/255.f);
		_colorTexture.draw(0, 0, dest.getWidth(), dest.getHeight());
		shader.end();
		dest.end();
		ofPopStyle();
	}
};
}
