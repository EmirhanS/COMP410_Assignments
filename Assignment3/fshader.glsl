#version 410
in  vec4 color;
in  vec2 texCoord;
out vec4 fcolor;

uniform sampler2D tex;
uniform int TextureFlag;

void main() 
{ 
    if (TextureFlag == 1) {
        // Sample texture and combine with lighting
        fcolor = texture(tex, texCoord);
    }
    else {
        fcolor = color;
    }
} 

