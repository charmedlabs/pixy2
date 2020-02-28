$fn = 180;

twidth = 3.5; //2.1;
tlength = 1.9; //1.1;

// size is the XY plane size, height in Z
module hexagon(size, height) {
  boxWidth = size/1.75;
  for (r = [-60, 0, 60]) rotate([0,0,r]) cube([boxWidth, size, height], true);
}

module make_hole(x, y, diam)
{
    difference()
    {
        children();
        translate([x, y, -5]) cylinder(10, diam/2, diam/2, false);
    }    
}

module make_tie_hole(x, y)
{
    width = 2.6; //2.1;
    length = 1.6; //1.1;
    
    difference()
    {
        children();
        translate([x-width/2, y-length/2, -5]) cube([width, length, 10]);
    }
}


module pan_servo(x, y, angle)
{
    swidth = 12.5; // width of servo
    slength = 24.0; // length of servo
    sthickness = 2.0; // thickness of servo tab
    soffs = 5.5; // servo axis offset from edge
    shoffs = 2.4; // servo hole offset
    shole = 2.3;  // servo mounting hole

    difference()
    {
        children();
        rotate([0, 0, angle]) 
        translate([x-soffs, y-swidth/2, -5])      
        union()
        {
            cube([slength, swidth, 10]);
            //translate([-soffs-shoffs, swidth/2, -5])
            translate([-shoffs, swidth/2, -1])
            cylinder(10, shole/2, shole/2, false);
            translate([slength+shoffs, swidth/2, -1])
            cylinder(10, shole/2, shole/2, false);
       }
    }
}

module tilt_servo(x, y)
{
    swidth = 11.5; // width of servo
    sthickness = 1.5; // thickness of servo tab

    difference()
    {
        children();
        translate([x, y, -.001]) 
        union()
        {
            cube([sthickness, swidth, 5]);
            translate([-11-twidth/2, -tlength/2-0.6, -5]) cube([twidth, tlength, 10]);
            translate([-11-twidth/2, -tlength/2+swidth+0.5, -5]) cube([twidth, tlength, 10]);
            translate([-3.25-twidth/2, -tlength/2-0.6, -5]) cube([twidth, tlength, 10]);
            translate([-3.25-twidth/2, -tlength/2+swidth+0.5, -5]) cube([twidth, tlength, 10]);
            
        }
    }
}

module body()
{
    extraWidth = 6;
    hole440 = 3.2; //2.7;
    swidth = 12.5; //8.3; // width of servo
    slength = 24.0; // 20; // length of servo
    sthickness = 2.0; //0.90; // thickness of servo tab
    soffs = 5.5; // servo axis offset from edge
    shoffs = 1.8; // servo hole offset
    shole = 1.6; //2.1; // servo mounting hole
    thickness = 3.2;
    depth = 46.5+extraWidth/2;
    cwidth = 23; // chunk width
    cdepth = 7.0; // chunk depth
    ewidth = 34.5; // spacing between bracket holes
    hwidth = 3.15; // halfwidth of bracket
    backend = 0;
    width = ewidth + 2*hwidth+extraWidth;
    
    pan_servo(0, -5, 180)
    tilt_servo(-1.7, -swidth-swidth/2+4)
    make_hole(-width/2+(width-ewidth)/2, depth-width/2-hwidth, hole440)
    make_hole(width/2-(width-ewidth)/2, depth-width/2-hwidth, hole440)
    difference()
    {
        union()
        {
            difference()
            {
                cylinder(thickness, width/2+backend, width/2+backend,               false);
                union()
                {
                    translate([-width/2, 0, -.001])                 cube([               width, depth, thickness+               .002]);
                    translate([width/2, -50, -1]) cube([20, 100, 10]);
                    translate([-width/2-20, -50, -1]) cube([20, 100, 10]);
                   
                }
            }
            translate([-width/2, 0, 0]) cube([width, depth-width/2-.001, thickness]);
        }
        union()
        {
            translate([-width/2+(width-cwidth)/2, depth-width/2-cdepth, -.001]) cube([cwidth, cdepth, thickness+               1]);
    if (1) // cable ties
    {
            translate([14, -6, -5]) rotate([0, 0, 90]) cube([twidth, tlength, 10]);
            translate([20, -6, -5]) rotate([0, 0, 90]) cube([twidth, tlength, 10]);            
            translate([14, 8.0, -5]) rotate([0, 0, 90]) cube([twidth, tlength, 10]);
            translate([20, 8.0, -5]) rotate([0, 0, 90]) cube([twidth, tlength, 10]);
    }
      }
    }
}

if (0)
{
    projection(cut = true) body();
}
else
{
    body();
}
