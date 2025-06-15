/// @description Using custom draw pipeline :)

// qaqaqaqaq drawing ducks...
// If instance count is getting too big, fall back to the native draw pipelines.
if(instance_count > INSTANCE_OPTI_THRESHOLD)
    draw_event();