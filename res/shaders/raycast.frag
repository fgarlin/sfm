#version 450 core

const vec3 AMBIENT_COLOR = vec3(0.1);
const vec3 SURFACE_COLOR = vec3(0.8);

uniform sampler3D tsdf_tex;
uniform sampler3D color_tex;

uniform vec3      volume_dims;
uniform vec3      camera_pos_tex_space; // Camera position in texture space
uniform float     step_size;
uniform int       display_mode;

in  vec3 v_texCoord;
out vec4 fragColor;


vec3 calculateNormal(vec3 p)
{
    ivec3 v = ivec3(floor(p * volume_dims));
    vec3 n;
    n.x = texelFetch(tsdf_tex, ivec3(v.x + 1, v.yz), 0).r
        - texelFetch(tsdf_tex, ivec3(v.x - 1, v.yz), 0).r;
    n.y = texelFetch(tsdf_tex, ivec3(v.x, v.y + 1, v.z), 0).r
        - texelFetch(tsdf_tex, ivec3(v.x, v.y - 1, v.z), 0).r;
    n.z = texelFetch(tsdf_tex, ivec3(v.xy, v.z + 1), 0).r
        - texelFetch(tsdf_tex, ivec3(v.xy, v.z - 1), 0).r;
    n = normalize(n);
    return n;
}

vec4 phongShading(vec3 p)
{
    vec3 normal = calculateNormal(p);

    // The light is placed at the camera
    vec3 lightDir = normalize(camera_pos_tex_space - p);

    vec3 diffuse = vec3(max(dot(normal, lightDir), 0.0));
    vec3 result = (AMBIENT_COLOR + diffuse) * SURFACE_COLOR;

    return vec4(result, 1.0);
}

vec2 rayVolumeIntersect(vec3 origin, vec3 dir)
{
    const vec3 box_min = vec3(0.0);
    const vec3 box_max = vec3(1.0);
    vec3 inv_dir = 1.0 / dir;
    vec3 tmin_tmp = (box_min - origin) * inv_dir;
    vec3 tmax_tmp = (box_max - origin) * inv_dir;
    vec3 tmin = min(tmin_tmp, tmax_tmp);
    vec3 tmax = max(tmin_tmp, tmax_tmp);
    float t0 = max(tmin.x, max(tmin.y, tmin.z));
    float t1 = min(tmax.x, min(tmax.y, tmax.z));
    return vec2(t0, t1);
}

void main()
{
    // Calculate the view ray direction in texture space. We are going to traverse
    // the volume front-to-back (the camera is the origin) in 3D texture space.
    vec3 rayDir = normalize(v_texCoord - camera_pos_tex_space);

    vec2 intersection = rayVolumeIntersect(camera_pos_tex_space, rayDir);
    if (intersection.x > intersection.y)
        discard;

    // Do not sample voxels behind the camera
    float t1 = max(intersection.x, 0.0);
    float t2 = intersection.y;
    float dt = step_size;

    bool found = false;
    float prev_tsdf = 0.0;
    vec3 surface_point;

    for (float t = t1; t < t2; t += dt) {
        vec3 p = camera_pos_tex_space + rayDir * t;

        float tsdf = texture(tsdf_tex, p).r;
        if (tsdf < 0.0) {
            // Linearly interpolate the surface
            float surface_t = mix(t, t - dt, prev_tsdf / (prev_tsdf - tsdf));
            surface_point = camera_pos_tex_space + rayDir * surface_t;
            found = true;
            break;
        }

        prev_tsdf = tsdf;
    }

    vec4 color;
    if (found) {
        if (display_mode == 0) {
            color = vec4(texture(color_tex, surface_point).rgb, 1.0);
        } else if (display_mode == 1) {
            color = vec4(calculateNormal(surface_point), 1.0);
        } else if (display_mode == 2) {
            color = phongShading(surface_point);
        }
    } else {
        discard;
    }

    fragColor = color;
}
