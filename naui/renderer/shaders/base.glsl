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
layout(binding = 1) uniform sampler2D texture1;
layout(binding = 2) uniform sampler2D texture2;
layout(binding = 3) uniform sampler2D texture3;
layout(binding = 4) uniform sampler2D texture4;

in vec4 frag_color;
in vec2 frag_uv;
in float frag_texture_id;
out vec4 out_color;

float sdf_alpha(float dist)
{
    float edge = 128.0 / 255.0;
    float w = fwidth(dist);
    return smoothstep(edge - w, edge + w, dist);
}

void sdf_font(sampler2D tex)
{
    float d = texture(tex, frag_uv).r;
    out_color = vec4(frag_color.rgb, frag_color.a * sdf_alpha(d));
}

void main()
{
    const int texture_id = int(frag_texture_id);
    switch (texture_id)
    {
    case -1: out_color = frag_color; return;
    case 0:  out_color = texture(texture0, frag_uv) * frag_color; return;

    case 1: sdf_font(texture1); return;
    case 2: sdf_font(texture2); return;
    case 3: sdf_font(texture3); return;
    case 4: sdf_font(texture4); return;
    }
}