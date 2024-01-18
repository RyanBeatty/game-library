#version 330

// Input vertex attributes
in vec3 vertexPosition;

in mat4 instanceTransform;

// Input uniform values
uniform mat4 mvp;

// NOTE: Add here your custom variables

void main()
{
    // Compute MVP for current instance
    mat4 mvpi = mvp*instanceTransform;

    // Send vertex attributes to fragment shader

    // Calculate final vertex position
    gl_Position = mvpi*vec4(vertexPosition, 1.0);
}