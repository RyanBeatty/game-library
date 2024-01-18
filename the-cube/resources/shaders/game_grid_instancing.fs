#version 330

// Input vertex attributes (from vertex shader)
uniform vec4 colDiffuse;

// Output fragment color
out vec4 finalColor;

void main()
{
    finalColor = colDiffuse;
}