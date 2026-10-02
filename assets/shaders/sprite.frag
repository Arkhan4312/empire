#version 430 core

in vec2 vUV;
in vec4 vColor;

out vec4 FragColor;

uniform sampler2D uTexture;

void main() {
FragColor = texture(uTexture, vUV) * vColor;
}