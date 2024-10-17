/* _    _  _ _  _ _ ____ ____ _ ___
 * |    |  |  \/  | | __ |__/ | |  \
 * |___ |__| _/\_ | |__] |  \ | |__/
 * =================================
 * Luxigrid - LED Matrix Diffuser
 * Copyright (c) 2024 OverScore Media - MIT License
 *
 * MIT LICENSE:
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


/* [LED Matrix size settings] */
// The pitch of the panel, in millimetres (P4 is 4, P3 is 3, and so on)
pixel_pitch = 4;

// The number of rows (horizontal) of LEDs
number_of_rows = 64;
// The number of columns (vertical) of LEDs
number_of_columns = 32;

// The size of half the matrix (assuming this can't be printed in one piece)
grid_width = (number_of_rows * pixel_pitch) / 2;
grid_height = number_of_columns * pixel_pitch;

/* [Diffuser Grid settings] */
// The height, in mm, of the grid
wall_height = 2;
// The width, in mm, of the gridlines
wall_width = 0.5;

/* [Bezel Settings] */
// The width of the bezel around the diffuser grid
bezel_width = 10;

// Distance from the first layer to the bottom of the bezel
bezel_height = 13.5;
distance_to_bracket =  bezel_height + wall_height;

// The width of the margin from the grid to the bezel
bezel_margin = 1.25;

// How thick the bezel should be at its thinnest points (i.e., away from the screw holes)
thin_bezel_width = 3;

// Distance from the side wall to the start of the top/bottom wall cutouts
top_bottom_wall_cutout_margin_wall = 15;
// Distance from the open end to the start of the top/bottom wall cutouts
top_bottom_wall_cutout_margin_open = 35;
// Distance from the corner to the start of the side wall cutout
side_wall_cutout_margin = 20;

/* [Screw Hole Settings] */

// The diameter of the screw holes in the bezel
screw_hole_diameter = 4.0; // 4.0 for M3 OD4.5 L6 Heated Inserts

// The distance from the corners of the bezel to the middle of the screw holes
screw_hole_offset = 6;

// Note how the calculation for the actual offset involves subtracting the radius of the screw holes
screw_hole_corner_offset = screw_hole_offset - (screw_hole_diameter / 2);

// The horizontal offset from the middle of the matrix to the interior screw holes
interior_screw_horizontal_offset = 15;
// The horizontal offset from the inner edge of the bezel to the interior screw holes
interior_screw_vertical_offset = 1.5;
// The depth, in mm, that the screws protrude into the walls of the bezel
screw_hole_depth = 7; // 7 for M3 OD4.5 L6 Heated Inserts

// The extra radius of the cylinders extending from the screw holes on the corners
corner_screw_hole_supports_offset = 3;

// For the "fillets" on the tops of the screw holes, to aid in the placement of heated inserts
screw_hole_fillet_size = 7; // 7 for M3 OD4.5 L6 Heated Inserts

/* [First Layer Settings] */
// Height of the first layer
first_layer_height = 0.2;

// Set this to 0 if you don't want a lip
// You'll want to print one diffuser with a first layer lip, and one without one for best results
// First layer overhang/lip amount so there's less of a seam between the diffuser panels
first_layer_overhang = 0.5; // Generally 0.5 works

// First Layer
translate([-grid_width / 2 - (bezel_width), (-grid_height / 2) + (wall_width / 2) - bezel_width, 0])
    color([1, 0, 0])
    cube([grid_width + (bezel_width * 2), grid_height + bezel_width + first_layer_overhang, first_layer_height]);


// Diffuser Grid
for(x = [0 : pixel_pitch : grid_width - pixel_pitch + wall_width]) {
    for(y = [0 : pixel_pitch : grid_height - pixel_pitch + wall_width]) {
        // Vertical grid walls
        translate([(x - grid_width / 2 - wall_width / 2), (y - grid_height / 2 - wall_width / 2), first_layer_height]) {
            cube([wall_width, wall_width + pixel_pitch, wall_height]);
        }

        // Horizontal grid walls
        translate([(x - grid_width / 2 - wall_width / 2), (y - grid_height / 2 - wall_width / 2), first_layer_height]) {
            cube([wall_width + pixel_pitch, wall_width, wall_height]);
        }
    }
}


// Additional vertical grid wall at the end (otherwise grid is cut-off at the top)
translate([(grid_width / 2 - wall_width / 2), (-grid_height / 2 + wall_width / 2), first_layer_height]) {
    cube([wall_width, grid_height, wall_height]);
}


// Additional horizontal grid wall at the end
translate([(-grid_width / 2), (grid_height / 2), first_layer_height]) {
    cube([grid_width, wall_width / 2, wall_height]);
}


// The walls around the grid, protruding down to the bracket on the other side of the LED matrix
module walls() {
    // Wall 1
    translate([-grid_height / 2 - bezel_width, -grid_width / 2 - bezel_width + wall_width / 2, 0]) {
        cube([bezel_width - wall_width / 2, bezel_width + grid_width, distance_to_bracket]);
    }

    // Wall 2
    translate([grid_height / 2 + wall_width/2, -grid_width / 2 - bezel_width + wall_width / 2, 0]) {
        cube([bezel_width - wall_width/2, bezel_width + grid_width, distance_to_bracket]);
    }

    // Wall 3
    translate([-grid_width / 2 - bezel_width / 2, -grid_width / 2 - bezel_width + wall_width / 2, 0]) {
        cube([bezel_width + grid_width, bezel_width - wall_width, distance_to_bracket]);
    }
}


// This cube represents a volume that will be subtracted from the sides of the walls, so there will be a margin between the walls and the LED matrix
module wall_margins() {
    translate([-grid_width / 2 - wall_width / 2 - bezel_margin, -grid_height / 2 - wall_width / 2 - bezel_margin, wall_height + first_layer_height]) {
        color([1, 0.6, 1]) {
            cube([grid_width + wall_width + bezel_margin * 2, grid_height + wall_width + bezel_margin + 0.01, distance_to_bracket - wall_height]);
        }
    }
}


// The screw holes in the walls around the grid
module screw_holes() {
    rotate([0, 180, 0]) {
    
        // The 0.01's are to prevent z fighting in the difference() with the walls
        translate([(grid_width / 2 + bezel_width - screw_hole_diameter / 2 - screw_hole_corner_offset), -(grid_width / 2 + bezel_width - screw_hole_diameter / 2 - wall_width / 2 - screw_hole_corner_offset), -distance_to_bracket - 0.01]) {
            cylinder(h=screw_hole_depth + 0.01, r=screw_hole_diameter / 2, $fn=100);
        }

        translate([(-grid_width / 2 - bezel_width + screw_hole_diameter / 2 + screw_hole_corner_offset), -(grid_width / 2 + bezel_width - screw_hole_diameter / 2 - wall_width / 2 - screw_hole_corner_offset), -distance_to_bracket - 0.01]) {
            cylinder(h=screw_hole_depth + 0.01, r=screw_hole_diameter / 2, $fn=100);
        }
        
        
        translate([(grid_width / 2 + bezel_width - screw_hole_diameter / 2 - screw_hole_corner_offset) + interior_screw_vertical_offset, (grid_width / 2 - screw_hole_diameter / 2 - wall_width / 2 - screw_hole_corner_offset) - interior_screw_horizontal_offset, -distance_to_bracket - 0.01]) {
            cylinder(h=screw_hole_depth + 0.01, r=screw_hole_diameter / 2, $fn=100);
        }

        translate([(-grid_width / 2 - bezel_width + screw_hole_diameter / 2 + screw_hole_corner_offset) - interior_screw_vertical_offset, (grid_width / 2 - screw_hole_diameter / 2 - wall_width / 2 - screw_hole_corner_offset) - interior_screw_horizontal_offset, -distance_to_bracket - 0.01]) {
            cylinder(h=screw_hole_depth + 0.01, r=screw_hole_diameter / 2, $fn=100);
        }
    }
}

// Poor man's fillets around the screw holes
// Basically cylinders with one smaller end and one larger end, subtracted from the tops of the screw holes
module screw_hole_fillets() {
    rotate([0, 180, 0]) {
        // The 0.01's are to prevent z fighting in the difference() with the walls
        translate([(grid_width / 2 + bezel_width - screw_hole_diameter / 2 - screw_hole_corner_offset), -(grid_width / 2 + bezel_width - screw_hole_diameter / 2 - wall_width / 2 - screw_hole_corner_offset), -distance_to_bracket - 0.01]) {
            cylinder(h=1 + 0.01, r1=screw_hole_fillet_size / 2, r2=screw_hole_diameter / 2, $fn=100);
        }

        translate([(-grid_width / 2 - bezel_width + screw_hole_diameter / 2 + screw_hole_corner_offset), -(grid_width / 2 + bezel_width - screw_hole_diameter / 2 - wall_width / 2 - screw_hole_corner_offset), -distance_to_bracket - 0.01]) {
            cylinder(h=1 + 0.01, r1=screw_hole_fillet_size / 2, r2=screw_hole_diameter / 2, $fn=100);
        }
        
        
        translate([(grid_width / 2 + bezel_width - screw_hole_diameter / 2 - screw_hole_corner_offset) + interior_screw_vertical_offset, (grid_width / 2 - screw_hole_diameter / 2 - wall_width / 2 - screw_hole_corner_offset) - interior_screw_horizontal_offset, -distance_to_bracket - 0.01]) {
            cylinder(h=1 + 0.01, r1=screw_hole_fillet_size / 2, r2=screw_hole_diameter / 2, $fn=100);
        }

        translate([(-grid_width / 2 - bezel_width + screw_hole_diameter / 2 + screw_hole_corner_offset) - interior_screw_vertical_offset, (grid_width / 2 - screw_hole_diameter / 2 - wall_width / 2 - screw_hole_corner_offset) - interior_screw_horizontal_offset, -distance_to_bracket - 0.01]) {
            cylinder(h=1 + 0.01, r1=screw_hole_fillet_size / 2, r2=screw_hole_diameter / 2, $fn=100);
        }
    }
}

// The cut-outs from the walls, so they use less plastic to print
// Only the space around the screw holes really needs to be that thick
module wall_cutouts() {
    rotate([0, 180, 0]) {
        // Top/bottom wall cutouts
        translate([-grid_width / 2 - bezel_width + thin_bezel_width, -grid_width / 2 - bezel_width + wall_width / 2 + top_bottom_wall_cutout_margin_wall, -bezel_height - wall_height - 0.01]) {
            color([1, 1, 0.5])
                        
            cube([grid_width + (bezel_width * 2) - (thin_bezel_width * 2), grid_width - top_bottom_wall_cutout_margin_open, bezel_height - first_layer_height + 0.01]);
        }
        
        // Side wall cutout
        translate([(-grid_width / 2 - bezel_width) + side_wall_cutout_margin, -grid_width / 2 - bezel_width + wall_width / 2 + thin_bezel_width, -bezel_height - wall_height - 0.01]) {
            color([0.5, 1, 1])
            
            cube([(grid_width + bezel_width * 2) - (side_wall_cutout_margin * 2), bezel_width - thin_bezel_width, bezel_height - first_layer_height + 0.01]);
        }
        
    }
}

// Cylinders extending from the screw holes on the corners
// Makes the inside corners a little rounder, so there's more material around the holes
module corner_screw_hole_supports() {
    rotate([0, 180, 0]) {
    
        // The 0.01's are to prevent z fighting in the difference() with the walls
        translate([(grid_width / 2 + bezel_width - screw_hole_diameter / 2 - screw_hole_corner_offset), -(grid_width / 2 + bezel_width - screw_hole_diameter / 2 - wall_width / 2 - screw_hole_corner_offset), -distance_to_bracket]) {
            cylinder(h=distance_to_bracket, r=(screw_hole_diameter / 2) + corner_screw_hole_supports_offset, $fn=100);
        }

        translate([(-grid_width / 2 - bezel_width + screw_hole_diameter / 2 + screw_hole_corner_offset), -(grid_width / 2 + bezel_width - screw_hole_diameter / 2 - wall_width / 2 - screw_hole_corner_offset), -distance_to_bracket]) {
            cylinder(h=distance_to_bracket, r=(screw_hole_diameter / 2) + corner_screw_hole_supports_offset, $fn=100);
        }
    }
}

difference() {
    union() {
        difference () {
            walls();
            
            union() {
                wall_margins();
                wall_cutouts();
            }
        }

        corner_screw_hole_supports();
    }
    
    union() {
        screw_holes();
        screw_hole_fillets();
    }
}