uniform sampler2D source;
uniform float time;
uniform float intensity;

void main()
{
    vec2 texCoord = gl_TexCoord[0].xy;
    
    //Multiple sine waves for more chaotic shake
    float shakeX = sin(texCoord.y * 15.0 + time * 80.0) * intensity;
    shakeX += sin(texCoord.y * 25.0 - time * 60.0) * intensity * 0.5;
    
    float shakeY = cos(texCoord.x * 15.0 + time * 70.0) * intensity;
    shakeY += cos(texCoord.x * 20.0 - time * 50.0) * intensity * 0.5;
    
    vec2 offset = vec2(shakeX, shakeY);
    gl_FragColor = texture2D(source, texCoord + offset);
}