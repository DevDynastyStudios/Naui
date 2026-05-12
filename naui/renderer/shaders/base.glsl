@stage vertex

const vec2 positions[4] = vec2[](
    vec2(-1.0, -1.0),
    vec2(1.0, -1.0),
    vec2(-1.0, 1.0),
    vec2(1.0, 1.0)
);

const vec2 tex_coords[4] = vec2[](
    vec2(0.0, 0.0),
    vec2(1.0, 0.0),
    vec2(0.0, 1.0),
    vec2(1.0, 1.0)
);

layout(binding = 0) uniform BaseUniform
{
    vec4 rect;
    float rounding;
    float width;
};

out vec3 frag_color;

void main()
{
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    frag_color = color;
}

@stage fragment

layout(binding = 0) uniform sampler2D texture0;

in vec3 frag_color;
out vec4 out_color;

void main()
{
    out_color = vec4(frag_color, 1.0);
}