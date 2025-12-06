
colorTransition += global.timeManager.get_delta() / (colorTransitionTime * 1000000);
colorTransition = min(colorTransition, 1.0);

var animY = AnimcurveTween(0, 1, acEaseInOut, colorTransition);
for(var i=0; i<3; i++) {
    colors[i] = merge_color(origColors[i], targetColors[i], animY);
}

trianglify_step(triStruct);