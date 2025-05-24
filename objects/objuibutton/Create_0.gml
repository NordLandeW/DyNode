
content = variable_instance_get(id, "content");
scale = variable_instance_get(id, "scale");

fontColor = c_white;
scriElement = scribble(cjk_prefix()+i18n_get(content))
    .starting_format("fDynamix16", fontColor)
    .transform(scale, scale)
    .align(fa_left, fa_top);
scriHeight = scriElement.get_height();

gradAlpha = 0;
gradMin = 0.05;
animTargetGradAlpha = 0;
animSpeed = 0.2;
shadowDistance = 3;

eventFunc = function () {
    announcement_play("按钮 "+content+" 被按下了 lol");
}