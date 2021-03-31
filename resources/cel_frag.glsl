#version 120

uniform vec3 lightPos1;
uniform vec3 lightPos2;
uniform vec3 lightColor1;
uniform vec3 lightColor2;
uniform vec3 ka;
uniform vec3 kd;
uniform vec3 ks;
uniform float s;
uniform int celLevels;

varying vec3 vertPos; 
varying vec3 norm;

float quantify(float c) {
	float range = 1.0f / celLevels;
	for (int i = 1; i < celLevels; i++) {
		if (c < i * range)
			return (i - 1) * range;
	}
	return 1.0f - range;
}

void main()
{
	vec3 n = normalize(norm);
	vec3 eye = normalize(vec3(0) - vertPos);
	float n_eye = dot(n, eye);
	if (n_eye < 0.3f && n_eye > -0.3f)
		gl_FragColor = vec4(vec3(0.0f), 1.0f);
	else {
		vec3 color = vec3(0.0f);
		vec3 l_1 = normalize(lightPos1 - vertPos);
		vec3 l_2 = normalize(lightPos2 - vertPos);
		vec3 h_1 = normalize(l_1 + eye);
		vec3 h_2 = normalize(l_2 + eye);

		vec3 ca = ka;
		vec3 cd_1 = kd * max(0, dot(l_1, n));
		vec3 cd_2 = kd * max(0, dot(l_2, n));
		vec3 cs_1 = ks * pow(max(0, dot(h_1, n)), s);
		vec3 cs_2 = ks * pow(max(0, dot(h_2, n)), s);
		color.r = lightColor1.r * (ca.r + cd_1.r + cs_1.r) + lightColor2.r * (ca.r + cd_2.r + cs_2.r);
		color.g = lightColor1.g * (ca.g + cd_1.g + cs_1.g) + lightColor2.g * (ca.g + cd_2.g + cs_2.g);
		color.b = lightColor1.b * (ca.b + cd_1.b + cs_1.b) + lightColor2.b * (ca.b + cd_2.b + cs_2.b);
		color.r = quantify(color.r);
		color.g = quantify(color.g);
		color.b = quantify(color.b);
		gl_FragColor = vec4(color, 1.0);
	};

}
