#version 330

// Input vertex attributes (from vertex shader)
in vec3 fragPosition;
in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 fragNormal;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// Output fragment color
out vec4 finalColor;

// NOTE: Add here your custom variables

uniform vec4 ambient;
uniform vec3 viewPos;

void main()
{
    vec4 diffuse = colDiffuse;
    if (fragPosition.y <= -0.9) {
        diffuse = vec4(0.3, 0.3, 0.3, 1);
    }

    vec4 texelColor = texture(texture0, fragTexCoord);
    finalColor = texelColor * diffuse;
}