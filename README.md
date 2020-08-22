# Real-time Structure from Motion from RGB-D images using OpenGL compute shaders

This project implements a dense Structure from Motion (SfM) technique that
reconstructs a 3D environment from 2D RGB-D images in real time. OpenGL compute
shaders are used to take advantage of parallel hardware architectures, allowing
real time performance on commodity hardware. A stream of input RGB-D images
(either from a live camera or a dataset) is sequentially integrated into a 3D
volume representation of the environment. A greater number of input images
provides further refinement of the point cloud data and consequently more
surface detail. The volume resolution and size can also be customized to suit a
particular use-case, enabling the use of this technique for a wide variety of
situations: from interior scenes to urban-scale environments.

Every frame, every voxel in the volumetric grid is projected to the image plane
of the camera, and its depth is compared to the actual depth from the sensor
data. The difference between the two values is known as the Truncated Signed
Distance Function (TSDF), and indicates the distance from that particular voxel
to the surface inside the volume. The color of the voxel is also stored. The
algorithm gives less importance to newer frames by using a weight function, so
the point cloud data stabilizes as more frames are integrated into the volume.
The point cloud is then raycasted so it can be displayed to the user.

![True Colors](https://github.com/fgarlin/sfm/blob/master/gallery/office_color.png?raw=true)
![Normals](https://github.com/fgarlin/sfm/blob/master/gallery/office_normals.png?raw=true)
![Phong Shading](https://github.com/fgarlin/sfm/blob/master/gallery/office_phong.png?raw=true)
