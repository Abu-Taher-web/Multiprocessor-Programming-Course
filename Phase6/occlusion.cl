__kernel void occlusion_filling(
    __global uchar* input_disparity,  // Original disparity map with holes
    __global uchar* filled_disparity, // Output filled disparity map
    uint image_width,                // Width of the disparity map
    uint image_height,               // Height of the disparity map
    uint max_search_radius         // Maximum neighborhood radius to search
) {
    const int col = get_global_id(0);  // Current pixel row (Y-coordinate)
    const int row = get_global_id(1);  // Current pixel column (X-coordinate)
    
    // Immediate neighbor offsets for searching
    int row_offset, col_offset;
    int neighbor_index;
    int current_radius;
    bool found_non_zero;

    // Initialize output with input value
    filled_disparity[row * image_width + col] = input_disparity[row * image_width + col];
    
    // Only process pixels with missing disparity (0 values)
    if(input_disparity[row * image_width + col] == 0) {
        found_non_zero = false;
        
        // Expand search radius until valid pixel found or max radius reached
        for (current_radius = 1; 
             (current_radius <= max_search_radius) && (!found_non_zero); 
             current_radius++) {
            
            // Search in concentric square neighborhoods
            for (col_offset = -current_radius; 
                 (col_offset <= current_radius) && (!found_non_zero); 
                 col_offset++) {
                
                for (row_offset = -current_radius; 
                     (row_offset <= current_radius) && (!found_non_zero); 
                     row_offset++) {
                    
                    // Skip out-of-bounds coordinates and center pixel
                    if (row + row_offset < 0 || row + row_offset >= image_height ||
                        col + col_offset < 0 || col + col_offset >= image_width ||
                        (row_offset == 0 && col_offset == 0)) {
                        continue;
                    }

                    neighbor_index = (row + row_offset) * image_width + (col + col_offset);
                    
                    // Copy first valid neighbor found
                    if(input_disparity[neighbor_index] != 0) {
                        filled_disparity[row * image_width + col] = input_disparity[neighbor_index];
                        found_non_zero = true;
                        break;
                    }
                }
            }
        }
    }
}