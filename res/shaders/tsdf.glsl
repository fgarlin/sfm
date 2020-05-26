#version 450
layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

layout(binding = 0, r16f)  uniform image3D tsdf_tex;
layout(binding = 1, rgba8) uniform image3D color_tex;
layout(binding = 2, r16ui) uniform uimage3D weight_tex;
layout(binding = 3)        uniform usampler2D frame_depth_tex;
layout(binding = 4)        uniform sampler2D  frame_color_tex;

uniform mat4 texture_to_model;
uniform mat4 model;
uniform mat4 extrinsic;
uniform mat3 intrinsic;
uniform float trunc_margin;


void main()
{
    ivec3 coords = ivec3(gl_GlobalInvocationID);
    // Normalized texture coordinates [0,1]
    vec3 normCoords = (vec3(coords) + 0.5) / // Sample the voxel mid point
        vec3(gl_NumWorkGroups * gl_WorkGroupSize);
    // Invert the y and z coordinates to correct for the model being upside down
    normCoords.yz = 1.0 - normCoords.yz;

    // Transform the voxel position from 3D texture coordinates to 2D image coordinates
    vec4 voxelPosCameraSpace = extrinsic * model * texture_to_model *
        vec4(normCoords, 1.0);
    vec3 voxelPosImageSpace = intrinsic * voxelPosCameraSpace.xyz;
    // Perspective division
    voxelPosImageSpace.xy /= voxelPosImageSpace.z;
    voxelPosImageSpace.xy = round(voxelPosImageSpace.xy);

    // Sample the real depth at this voxel in millimeters
    unsigned int depth_mm =
        texelFetch(frame_depth_tex, ivec2(voxelPosImageSpace.xy), 0).r;
    // Invalid depth values are set to 65535, ignore them
    if (depth_mm == 65535) depth_mm = 0;

    vec2 texSize = textureSize(frame_depth_tex, 0);

    // Check that:
    // 1. It's a valid depth value
    // 2. The voxel is in front of the camera
    // 3. The voxel image coordinates are within the texture borders
    if ( depth_mm != 0                                       &&
         voxelPosCameraSpace.z > 0.0                         &&
         all(greaterThan(voxelPosImageSpace.xy, vec2(0.0)) ) &&
         all(lessThan   (voxelPosImageSpace.xy, texSize  ) ) )
    {
        float depth = float(depth_mm) / 1000.0; // To meters
        // Calculate the signed distance function of this voxel
        float sdf = depth - voxelPosCameraSpace.z;
        if (sdf >= -trunc_margin) {
            float dist = min(1.0, sdf / trunc_margin);

            unsigned int prev_weight = imageLoad(weight_tex, coords).r;
            unsigned int new_weight = prev_weight + 1;
            imageStore(weight_tex, coords, uvec4(new_weight, 0, 0, 0));

            float prev_tsdf = imageLoad(tsdf_tex, coords).r;
            float avg_tsdf = (prev_tsdf * prev_weight + dist) / new_weight;
            imageStore(tsdf_tex, coords, vec4(dist, 0.0, 0.0, 0.0));

            vec3 color =
                texelFetch(frame_color_tex, ivec2(voxelPosImageSpace.xy), 0).rgb;
            vec3 prev_color = imageLoad(color_tex, coords).rgb;
            vec3 avg_color = (prev_color * prev_weight + color) / new_weight;
            imageStore(color_tex, coords, vec4(avg_color, 1.0));
        }
    }
}
