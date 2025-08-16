
if(mouse_isclick_l() && mouse_inbound(x, y, x+maxWidth, y+scriHeight*2)) {
    if(fType == "Open") {
        input = dyc_get_open_filename(filter, fname, dir, fTitle);
    }
    else if(fType == "Save") {
        input = dyc_get_save_filename(filter, fname, dir, fTitle);
    }
}

event_inherited();