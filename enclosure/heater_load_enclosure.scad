//$fs=0.1;
$fn=100;

ENCLOSURE_BASE_OUTER_DIM=[121, 155, 40];
ENCLOSURE_BASE_WALL_THICKNESS=4;

module enclosure_base_mount_points(){
  MOUNT_POINT_RADIUS=3;
  X_OFFSET=5;
  union(){
    translate([ENCLOSURE_BASE_WALL_THICKNESS+X_OFFSET, ENCLOSURE_BASE_OUTER_DIM.y*0.8,0]) cylinder(h=ENCLOSURE_BASE_WALL_THICKNESS+5, r=MOUNT_POINT_RADIUS);
    translate([ENCLOSURE_BASE_OUTER_DIM.x-ENCLOSURE_BASE_WALL_THICKNESS-X_OFFSET, ENCLOSURE_BASE_OUTER_DIM.y*0.8,0]) cylinder(h=ENCLOSURE_BASE_WALL_THICKNESS+5, r=MOUNT_POINT_RADIUS);
  }
}

module enclosure_base_wire_hole(){
  translate([ENCLOSURE_BASE_OUTER_DIM.x/2,-1,ENCLOSURE_BASE_OUTER_DIM.z/2]) rotate([-90,0,0]) cylinder(h=ENCLOSURE_BASE_WALL_THICKNESS+10, r=7);
}

module enclosure_base_switch_hole(){
  SWITCH_DIM=[ENCLOSURE_BASE_WALL_THICKNESS+10,31,11];
  translate([-1,ENCLOSURE_BASE_OUTER_DIM.y/2,ENCLOSURE_BASE_OUTER_DIM.z/2]) cube(SWITCH_DIM, center=true);
}

module outer_walls(){
  INNER_SPACE_DIM=ENCLOSURE_BASE_OUTER_DIM-[ENCLOSURE_BASE_WALL_THICKNESS*2, ENCLOSURE_BASE_WALL_THICKNESS*2, 0];
  difference(){
    cube(ENCLOSURE_BASE_OUTER_DIM);
    translate([ENCLOSURE_BASE_WALL_THICKNESS,ENCLOSURE_BASE_WALL_THICKNESS,ENCLOSURE_BASE_WALL_THICKNESS]) cube(INNER_SPACE_DIM);
    enclosure_base_mount_points();
    enclosure_base_wire_hole();
    enclosure_base_switch_hole();
  };
}

module corner_support_one(){
  CORNER_SUPPORT_RADIUS=10;
  CORNER_SUPPORT_SCREW_HOLE_RADIUS=3;
  CORNER_SUPPORT_SCREW_DEPTH=20;
  difference(){
    translate([CORNER_SUPPORT_RADIUS, CORNER_SUPPORT_RADIUS, 0]) cylinder(h=ENCLOSURE_BASE_OUTER_DIM.z, r=CORNER_SUPPORT_RADIUS);
    translate([CORNER_SUPPORT_RADIUS, CORNER_SUPPORT_RADIUS, ENCLOSURE_BASE_OUTER_DIM.z-CORNER_SUPPORT_SCREW_DEPTH+1]) cylinder(h=CORNER_SUPPORT_SCREW_DEPTH, r=CORNER_SUPPORT_SCREW_HOLE_RADIUS);
  }
}

CORNER_SUPPORT_RADIUS=6;
CORNER_SUPPORT_OFFSET=[
  [CORNER_SUPPORT_RADIUS,CORNER_SUPPORT_RADIUS,0],
  [ENCLOSURE_BASE_OUTER_DIM.x-CORNER_SUPPORT_RADIUS,CORNER_SUPPORT_RADIUS,0],
  [CORNER_SUPPORT_RADIUS,ENCLOSURE_BASE_OUTER_DIM.y-CORNER_SUPPORT_RADIUS,0],
  [ENCLOSURE_BASE_OUTER_DIM.x-CORNER_SUPPORT_RADIUS,ENCLOSURE_BASE_OUTER_DIM.y-CORNER_SUPPORT_RADIUS,0],
];
module corner_supports(){
  CORNER_SUPPORT_INSERT_RADIUS=5/2;
  CORNER_SUPPORT_SCREW_DEPTH=20;
  for(i=[0:1:3]){
    translate(CORNER_SUPPORT_OFFSET[i]) difference(){
      cylinder(h=ENCLOSURE_BASE_OUTER_DIM.z, r=CORNER_SUPPORT_RADIUS);
      translate([0, 0, ENCLOSURE_BASE_OUTER_DIM.z-CORNER_SUPPORT_SCREW_DEPTH+1]) cylinder(h=CORNER_SUPPORT_SCREW_DEPTH, r=CORNER_SUPPORT_INSERT_RADIUS);
    }
  }
}


module enclosure_base() {
  outer_walls();
  corner_supports();
  board_supports();
}

BOARD_SUPPORT_OFFSET=[
  [1.5,129,0],
  [90,129,0],
  [1.5,6,0],
  [90,6,0],
];
module board_supports(){
  BOARD_SUPPORT_RADIUS=5;
  translate([14.5,7.5,0]) for(i=[0:1:3]){
    translate([BOARD_SUPPORT_OFFSET[i].x,BOARD_SUPPORT_OFFSET[i].y,0]) difference(){
      cylinder(h=10, r=BOARD_SUPPORT_RADIUS);
      translate([0,0,3]) cylinder(h=8, r=BOARD_SUPPORT_RADIUS/2);
    }
  }
}
module enclosure_cap(){
  SCREW_HOLE_RADIUS=3/2;
  color("blue", alpha=0.8) difference(){
    cube([ENCLOSURE_BASE_OUTER_DIM.x,ENCLOSURE_BASE_OUTER_DIM.y,ENCLOSURE_BASE_WALL_THICKNESS]);
    for(i=[0:1:3]){
      translate(CORNER_SUPPORT_OFFSET[i])
      cylinder(h=ENCLOSURE_BASE_WALL_THICKNESS+1, r=SCREW_HOLE_RADIUS);
    }
  }
}

module vitamin_board(){
  HOLE_RADIUS=3/2;
  HOLE_OFFSET_X=[1,90];
  HOLE_OFFSET_Y=[6.5,129];
  
  VITAMIN_BOARD_POSITION=[ENCLOSURE_BASE_WALL_THICKNESS+9,ENCLOSURE_BASE_WALL_THICKNESS+3, ENCLOSURE_BASE_WALL_THICKNESS+7];
  translate(VITAMIN_BOARD_POSITION) difference(){
    color("green", alpha=0.5) cube([95,140,30]);
    translate([HOLE_RADIUS+HOLE_OFFSET_X[0],HOLE_OFFSET_Y[0],0]) cylinder(h=50, r=HOLE_RADIUS);
    translate([HOLE_RADIUS+HOLE_OFFSET_X[1],HOLE_OFFSET_Y[0],0]) cylinder(h=50, r=HOLE_RADIUS);
    translate([HOLE_RADIUS+HOLE_OFFSET_X[0],HOLE_OFFSET_Y[1],0]) cylinder(h=50, r=HOLE_RADIUS);
    translate([HOLE_RADIUS+HOLE_OFFSET_X[1],HOLE_OFFSET_Y[1],0]) cylinder(h=50, r=HOLE_RADIUS);
  }
}

enclosure_base();
//enclosure_cap();
//vitamin_board();
