uniform float K1,K2,K3;
uniform float mvt1, mvt2, mvt3;

uniform sampler2D texture;

uniform vec2 offset;

void main (void)
{
	vec3 noiseVec1;
	vec3 noiseVec2;
	vec3 noiseVec3;
	vec2 displacement1;
	vec2 displacement2;
	vec2 displacement3;
	vec4 color;
	float a,b,c;

	vec2 texture1 = (gl_TexCoord[0].st * 0.5);

	displacement1 = (gl_TexCoord[0].st * 0.5) + vec2(0.0,0.5);
	displacement2 = (gl_TexCoord[0].st * 0.5) + vec2(0.5,0.0);
	displacement3 = (gl_TexCoord[0].st * 0.5) + vec2(0.5,0.5);



	noiseVec1 = normalize(texture2D(texture, displacement1.xy).xyz);
	noiseVec1 = (noiseVec1 * 2.0 - 1.0) * 0.01 * K1;
	a = cos(mvt1)*0.5 +1.;
	b = cos(mvt1+2.0944)*0.5 +1.;
	c = cos(mvt1+4.1888)*0.5 +1.;
	noiseVec1 = vec3 ( a * noiseVec1.x + b * noiseVec1.y + c * noiseVec1.z, b * noiseVec1.x + c * noiseVec1.y + a * noiseVec1.z, 0) ;

	noiseVec2 = normalize(texture2D(texture, displacement2.xy).xyz);
	noiseVec2 = (noiseVec2 * 2.0 - 1.0) * 0.01 * K2;
	a = cos(mvt2)*0.5 +1.;
	b = cos(mvt2+2.0944)*0.5 +1.;
	c = cos(mvt2+4.1888)*0.5 +1.;
	noiseVec2 = vec3 ( a * noiseVec2.x + b * noiseVec2.y + c * noiseVec2.z, b * noiseVec2.x + c * noiseVec2.y + a * noiseVec2.z, 0) ;

	noiseVec3 = normalize(texture2D(texture, displacement3.xy).xyz);
	noiseVec3 = (noiseVec3 * 2.0 - 1.0) * 0.01 * K3;
	a = cos(mvt3)*0.5 +1.;
	b = cos(mvt3+2.0944)*0.5 +1.;
	c = cos(mvt3+4.1888)*0.5 +1.;
	noiseVec3 = vec3 ( a * noiseVec3.x + b * noiseVec3.y + c * noiseVec3.z, b * noiseVec3.x + c * noiseVec3.y + a * noiseVec3.z, 0) ;
	
//	gl_FragColor = texture2D(texture, texture1 + noiseVec1.xy + noiseVec2.xy + noiseVec3.xy );
	color = texture2D(texture, texture1 + noiseVec1.xy + noiseVec2.xy + noiseVec3.xy );
//	color.a = 0.5;
	gl_FragColor = color;
}
