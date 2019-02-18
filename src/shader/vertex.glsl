#version 400 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

uniform mat4 mvp;
uniform mat4 mv;

out Attribute {
	vec3 position;
	vec3 normal;
} vertexOut;

void main() {
	gl_Position = mvp * vec4(position, 1.f);
	vertexOut.position = (mv * vec4(position, 1.f)).xyz;
	vertexOut.normal = (mv * vec4(normal, 0.f)).xyz;
	// vertexOut.normal = normal;
}
