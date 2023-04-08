#version 430

out vec4 color;
uniform int vaoIndex;


void main(void)
{
	if(vaoIndex == 0)
		color = vec4(1,0,0,0);
	if(vaoIndex == 1)
		color = vec4(0,1,0,0);
	if(vaoIndex == 2)
		color = vec4(0,0,1,0);
}