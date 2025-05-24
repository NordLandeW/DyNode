
if(!instance_exists(objUIInputManager))
    instance_create(0, 0, objUIInputManager);

inputID = variable_instance_get(id, "inputID");
title = variable_instance_get(id, "title");

objUIInputManager.register_input(inputID, id);

input = "";
fontColor = c_white;
scriElement = scribble(cjk_prefix()+i18n_get(title))
    .starting_format("fDynamix16", fontColor)
    .align(fa_left, fa_top);
scriHeight = scriElement.get_height();
maxWidth = 300;

gradAlpha = 0;
animTargetGradAlpha = 0;
gradMin = 0.05;
animSpeed = 0.2;
shadowDistance = 3;
lineShootDistance = 30;