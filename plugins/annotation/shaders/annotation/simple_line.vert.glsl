#version 330

layout(location = 0) in vec3 in_position;

uniform mat4 mvp;

void main(void) {
    gl_Position = mvp * vec4(in_position.x, in_position.y, in_position.z, 1.0);
}
