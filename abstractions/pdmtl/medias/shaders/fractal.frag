uniform float maxIterations;

varying vec2 coord;	// texture coordinates

void main ()
{
	vec2 c = coord;
	vec2 z = c;
	gl_FragColor = vec4(1.0,1.,0.,1.);
	for (float i = 0.; i < maxIterations ; i += 1.0)
	{
		z = vec2(z.x*z.x - z.y*z.y, 2.0*z.x*z.y) + c;
		if (dot(z, z) > 4.0)
		{
			gl_FragColor *=	(i / maxIterations);
			break;
		}
	}

}
