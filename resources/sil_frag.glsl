#version 120

varying vec3 vertPos; 
varying vec3 norm;

void main()
{
	vec3 n = normalize(norm);
	vec3 eye = normalize(vec3(0) - vertPos);
	float n_eye = dot(n, eye);
	if (n_eye < 0.3f && n_eye > -0.3f)
		gl_FragColor = vec4(vec3(0.0f), 1.0f);
	else
		gl_FragColor = vec4(vec3(1.0f), 1.0f);

}
