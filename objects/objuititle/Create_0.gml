
title = variable_instance_get(id, "title");
scriElement = scribble("[scale,2.25]DyNode[/scale] [scale,0.8]"+VERSION+"[/scale]\n[c_ltgrey]"+"[sprMsdfNotoSans][scale, 1.5]"+i18n_get(title))
                .starting_format("mDynamix", c_white)
                .msdf_shadow(c_black, 0.3, 0, 3, 3)
                .align(fa_left, fa_top);
scriTypist = scribble_typist().in(1, 10);

placeX = 0.05;
placeY = 0.05;