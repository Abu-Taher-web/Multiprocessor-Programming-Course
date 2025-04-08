__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | 
      CLK_ADDRESS_CLAMP | CLK_FILTER_NEAREST; 

__kernel void simple_image(read_only image2d_t src_image,
                        write_only image2d_t dst_image) {
   /* Get pixel coordinates */
   int x = get_global_id(1); // width dimension
   int y = get_global_id(0); // height dimension
   int2 coord = (int2)(x, y);

   /* Read pixel value (using float for UNORM) */
   float4 pixel = read_imagef(src_image, sampler, coord);

   /* Simple operation (example: invert intensity) */
   pixel.x = 1.0f - pixel.x;

   /* Write new pixel value to output */
   write_imagef(dst_image, coord, pixel);
}