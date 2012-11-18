varying vec2 coord;
uniform vec2 center;
uniform float zoom;

void main(void)
{
	coord = zoom * (gl_Vertex.xy) + center;
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}
