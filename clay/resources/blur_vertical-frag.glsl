uniform sampler2D input_texture;
uniform float blurSize;

void main()
{
	
	float s = gl_TexCoord[0].s;
	float t = gl_TexCoord[0].t;

	vec4 col = vec4(0.0);

	col += texture2D(input_texture, vec2(s, t - 4.0*blurSize)) * 0.05;
	col += texture2D(input_texture, vec2(s, t - 3.0*blurSize)) * 0.09;
	col += texture2D(input_texture, vec2(s, t - 2.0*blurSize)) * 0.12;
	col += texture2D(input_texture, vec2(s, t - blurSize)) * 0.15;
	col += texture2D(input_texture, gl_TexCoord[0].st) * 0.18;
	col += texture2D(input_texture, vec2(s, t + blurSize)) * 0.15;
	col += texture2D(input_texture, vec2(s, t + 2.0*blurSize)) * 0.12;
	col += texture2D(input_texture, vec2(s, t + 3.0*blurSize)) * 0.09;
	col += texture2D(input_texture, vec2(s, t + 4.0*blurSize)) * 0.05;

	gl_FragColor = col;
    gl_FragColor.a = 1.0;
}

