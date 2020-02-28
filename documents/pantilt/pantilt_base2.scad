$fn = 180; 

module make_hole(x, y, diam)
{
    difference()
    {
        children();
        translate([x, y, -5]) cylinder(10, diam/2, diam/2, false);
    }    
}

yoffs = 3.0;
hole1 = 7.0; 
hole2 = 2.5; //2.6;
hole3 = 2.5; //;
holeWidth = 15;
xwidth = 53.34;
ywidth = 58.0;
thickness = 2.5;
yoffs2 = 21;
xoffs = (xwidth - 53.34)/2;

module fillet(x, y, r)
{
    translate([x, y, -5])
    union()
    {
        children();
        difference()
        {
            translate([-r/2, -r/2, 0]) cube([r, r, 10]);
            cylinder(10, r, r, false);          
        }
    }
  
}

module plate()
{
translate([-25, -25, 0])
{
    make_hole(xwidth/2, yoffs2, hole1)
    make_hole(xwidth/2+holeWidth/2, yoffs2, hole2)
    make_hole(xwidth/2-holeWidth/2, yoffs2, hole2)
    make_hole(xwidth/2, ywidth/2, hole1)
    make_hole(xwidth/2+holeWidth/2, ywidth/2, hole2)
    make_hole(xwidth/2-holeWidth/2, ywidth/2, hole2)
make_hole(7.6 + xoffs, yoffs, hole3)
make_hole(35.5 + xoffs, yoffs, hole3)
make_hole(2.5 + xoffs, 52.1 + yoffs, hole3)
make_hole(50.7 + xoffs, 50.8 + yoffs, hole3)
    
make_hole(xwidth + xoffs - 7.6, yoffs, hole3)
make_hole(xwidth + xoffs - 7.6, ywidth - yoffs, hole3)
make_hole(7.6 + xoffs, ywidth - yoffs, hole3)
cube([xwidth, ywidth, thickness]);
}
}

if (0)
    projection(cut = true) plate();
else
    plate();
