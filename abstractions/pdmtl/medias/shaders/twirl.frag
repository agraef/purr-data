uniform float twirl; 
uniform float size;
uniform float radius;
uniform vec2 origin;

uniform sampler2DRect tex0;

varying vec2 texcoord0;
varying vec2 texdim0;

void main (void) 
{ 
		
	vec2 point = texcoord0/texdim0;	
	
	vec2 normCoord = vec2(2.0) * point - vec2(1.0);

	normCoord += origin;

	float r = length(normCoord);
	float phi = atan(normCoord.y, normCoord.x); 


	phi = phi + (1.0 - smoothstep(-size, size,r)) * twirl;

	normCoord.x = r* cos(phi);
	normCoord.y = r* sin(phi);
	
	normCoord -= origin;
	
	vec2 texCoord0 = (normCoord / 2.0 + 0.5) * texdim0;
	gl_FragColor = texture2DRect(tex0, texCoord0);
	
} 
