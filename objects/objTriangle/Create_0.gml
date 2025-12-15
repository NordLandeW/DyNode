
persistent = true;

colors = variable_instance_get(id, "colors");
triStruct = trianglify_generate(BASE_RES_W, BASE_RES_H, [0.05, 0.12], 200);

colorTransition = 1.0;
colorTransitionTime = 1;      // in seconds
origColors = SnapDeepCopy(colors);
targetColors = SnapDeepCopy(colors);
origRoom = room;
roomColors = SnapDeepCopy(colors);

color_transist = function(to_color) {
    origColors = colors;
    targetColors = to_color;
    colorTransition = 0.0;
}

if(instance_number(objTriangle) > 1) {
    with(objTriangle) {
        if(id != other.id) {
            color_transist(other.roomColors);
        }
    }
    instance_destroy();
}