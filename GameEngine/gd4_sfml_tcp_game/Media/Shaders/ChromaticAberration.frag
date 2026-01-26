uniform sampler2D source;
uniform float intensity;

void main()
{
    vec2 texCoord = gl_TexCoord[0].xy;
    
    //Offset from center (0.5, 0.5)
    vec2 direction = texCoord - vec2(0.5);
    
    //Sample RGB channels at different offsets
    float r = texture2D(source, texCoord + direction * intensity).r;
    float g = texture2D(source, texCoord).g; //Green stays centered
    float b = texture2D(source, texCoord - direction * intensity).b;
    
    gl_FragColor = vec4(r, g, b, 1.0);
}