
event_inherited();

oRollSpeed = variable_instance_get(id, "roll_speed");
rollSpeed = variable_instance_get(id, "roll_speed");
rollSpeedT = variable_instance_get(id, "roll_speed");
width = variable_instance_get(id, "width");
height = variable_instance_get(id, "height");
rollSpeedUpFactor = variable_instance_get(id, "roll_speedup_factor");
text = variable_instance_get(id, "text");
scl = variable_instance_get(id, "scl");
font = variable_instance_get(id, "font");

var nh = surface_get_height(application_surface);
var scalar = nh / BASE_RES_H;
surfW = width * scalar;
surfH = height * scalar;
creditsSurf = surface_create(surfW, surfH);

nowY = height + 50;
nowX = 10;