@stage vertex

in vec2 in_position;
in vec2 in_uv;
in vec4 in_color;
in int in_texture_id;

out vec4 frag_color;
out vec2 frag_uv;
out float frag_texture_id;

layout(binding = 0) uniform ViewData {
    vec2 u_resolution;
};

void main()
{
    vec2 ndc = (in_position / u_resolution) * 2.0 - 1.0;
    gl_Position = vec4(ndc, 0.0, 1.0);

    frag_uv = in_uv;
    frag_color = in_color;
    frag_texture_id = float(in_texture_id);
}

@stage fragment

layout(binding = 0) uniform sampler2D texture0;

in vec4 frag_color;
in vec2 frag_uv;
in float frag_texture_id;

out vec4 out_color;

void main()
{
    const int texture_id = int(frag_texture_id);
    if (texture_id == -1)
        out_color = frag_color;
    else
        out_color = texture(texture0, frag_uv);
}
