$fn=100;

module case() {

    th=2;
    w=100;
    h=120;
    
    xd = 26; // desplazamiento display
    xe = -20; // desplazamiento encoder

    translate([0,0,th/2])
    difference() {
        cube([w,h,th],center=true);
        {
            // taladro display
            translate([0,xd,0])
                cube([72,25,10],center=true);

            // taladro encoder
            translate([0,xe,0])
                cylinder(d=7,h=10, center=true);

        }
        
    }

    // soporte lcd
    translate([0,xd+1.5,0])
    color("red") 
    {
        d1=4.5;
        d2=2.7;
        h=4.5;
        
        
        v = [ 
            [ -37.5,-15.5,h/2+th],
            [ -37.5, 15.5,h/2+th],
            [37.5, 15.5,h/2+th],
            [37.5,-15.5,h/2+th]
        ];
        
        for ( i = v ) {
            translate(i)   
                difference() {
                 cylinder(d=d1,h=h, center=true);    
                 cylinder(d=d2,h=h+5, center=true);  
                } 
        } 
    }

    // soporte encoder
    color("red") 
    {
            translate([-6.5-1,xe,2.5+2])
            cube([2,10,5],center=true);

            translate([6.5+1,xe,2.5+2])
            cube([2,10,5],center=true);    
    }
    
    
    color("green")
    translate([50-th,-60,th])
    walls();
    

}

module prism(l, w, h){
    polyhedron(
       points=[[0,0,0], [l,0,0], [l,w,0], [0,w,0], [0,w,h], [l,w,h]],
       faces=[[0,1,2,3],[5,4,3,2],[0,4,5,1],[0,3,4],[5,2,1]]
       );
}


module walls() {
    th=2;
    h=120;
    w=100;
    
    he = 25; // altura pared externa
    hi = 15; // altura pared interna
    
    


    {
        // pared derecha
        difference() {
            cube([th,h,he]);
            
            // taladro boton reset
            translate([0,th+3,th+4])
            rotate([0,90,0])
            cylinder(d=4,h=5, center=true); 
        
            // taladros para tapa    
            translate([0,h/2,he-7])
            rotate([0,90,0])
            cylinder(d=2.7,h=30,center=true);  
        }
        
        // pared izquierda
        difference() {
            translate([-w+th,0,0])
            cube([th,h,he]);
         
            // taladros para tapa    
            translate([-w,,h/2,he-7])
            rotate([0,90,0])
            cylinder(d=2.7,h=30,center=true);  
            
        }
        
        // pared inferior
        difference() {
            translate([-w+th,0,0])
            cube([w,th,he]);
            
            // taladro conector usb
            translate([-13-4,-3,th+4])
            cube([13,th+6,6]);
            
                       
            // taladros para tapa    
            translate([-w/2+th,0,he-7])
            rotate([90,0,0])
            cylinder(d=2.7,h=30,center=true);  
            }
        
        // pared superior
        difference() {
            translate([-w+th,h-th,0])
            cube([w,th,he]); 
            

            // taladros para tapa    
            translate([-w/2+th,h,he-7])
            rotate([90,0,0])
            cylinder(d=2.7,h=30,center=true);                 
        }  
        
        //pared interior izquierda
        difference() {
            translate([-28-th,0,0])
            cube([th,th+35+th+26+th,hi]);
              
            // taladro relay
            translate([-28-th-5,th+35+th,2])
            cube([10,26,8]);
        }    
            
        translate([-28,0,0])
        cube([th,35+th,hi]);
          
        // division wemos relay
        translate([-28,th+35,0])
        cube([28,th,hi]);
        
        // division relay
        translate([-28,th+35+28,0])
        cube([28,th,hi]);
        
        // pared izquierda fuente
        translate([-w+2*th+34,0,0])
        cube([th,52+2*th,hi]);
        
        // pared superior fuente
        translate([-w+2*th,th+52,0])
        cube([34,th,hi]);
    };   
    
    // soportes tapa
   
    l = 30; // support len
    st = 4 ; // ancho del soporte
    
    // superiores
    translate([-w+2*th,h-th-st,he-th])
    prism(l,st,-st);

    translate([-l,h-th-st,he-th])
    prism(l,st,-st);
    
    
    // inferiores
    translate([-w+2*th,st+th,he-th])
    prism(l,-st,-st);

    translate([-l,st+th,he-th])
    prism(l,-st,-st); 
    
    // izquierda
    translate([-w+st+2*th,0,he-th])
    rotate([0,0,90])
    prism(l,st,-st);
    
    translate([-w+st+2*th,h-l,he-th])
    rotate([0,0,90])
    prism(l,st,-st);
    
    
    // derecha
    translate([-st,0,he-th])
    rotate([0,0,90])
    prism(l,-st,-st);
    
    translate([-st,h-l,he-th])
    rotate([0,0,90])
    prism(l,-st,-st);
}


module tapa_tornillo() {
   th=2; 
   h=8;
   l=6;
   
   translate([0,0,h/2]) 
   difference() {
       cube([l,2*th,h], center=true);
       translate([0,0,1])
       rotate([90,0,0])
        cylinder(d=2.7,h=30,center=true);
   }
}


module tapa() {
    
    th=2;
    w=100;
    h=120;
    
    l = 30; // len fijar tornille

    difference() {
        translate([0,0,th/2])
        cube([w-2*th,h-2*th,th],center=true);
        
        cylinder(d=30,h=20, center=true);
    }
    
    // inferior
    translate([0,-h/2+2*th,th])
    tapa_tornillo();
    
    // superior
    translate([0,h/2-2*th,th])
    tapa_tornillo();
    
    translate([-w/2+4,0,th])
    rotate([0,0,90])
    tapa_tornillo();
    
    translate([w/2-4,0,th])
    rotate([0,0,90])
    tapa_tornillo();
    
    
}




case();


color("red")
translate([0,0,27])
rotate([180,0,0])
tapa();



