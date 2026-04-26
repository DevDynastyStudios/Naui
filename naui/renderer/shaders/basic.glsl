@vs vs
in vec2 in_position;
in vec3 in_color;

out vec3 frag_color;

void main()
{
    gl_Position = vec4(in_position, 0.0, 1.0);
    frag_color = in_color;
}
@end

@fs fs
out vec4 out_color;
in vec3 frag_color;

void main()
{
    out_color = vec4(frag_color, 1.0);
}
@end

@program basic vs fs