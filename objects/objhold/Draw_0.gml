
// If instance count is getting too big, fall back to the native draw pipelines.
if(instance_count > INSTANCE_OPTI_THRESHOLD) {
    draw_event(false);
    draw_event(true);
}