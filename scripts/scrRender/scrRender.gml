
/// @description This singleton handles note rendering under playback mode.
function NoteRenderer() constructor {

    // Init vertex format.
    vertex_format_begin();
    vertex_format_add_position();
    vertex_format_add_texcoord();
    vertex_format_add_color();
    vertFormat = vertex_format_end();
    
    // Create vertex buffer.
    cacheBuff = buffer_create(20 * 1024, buffer_fast, 1);
    vertBuff = vertex_create_buffer_from_buffer(cacheBuff, vertFormat);

    // Hold Particles timer.
    holdParticlesTimer = 0;
    tempAdditionSurface = -1;

    static render_state = function(state) {
        /// @type {Real} 
        var size = DyCore_render_active_notes(
            buffer_get_address(cacheBuff), objMain.nowTime, objMain.playbackSpeed, state);

		if(size < 0) return;

        buffer_set_used_size(cacheBuff, size);
        vertex_update_buffer_from_buffer(vertBuff, 0, cacheBuff, 0, size);

        var texture = texturegroup_get_textures("texNotes")[0];
        vertex_submit_ext(vertBuff, pr_trianglelist, texture, 0, size / 20);
    }

    static render = function() {
        gpu_push_state();
        gpu_set_tex_repeat(true);
        dyc_update_active_notes();

        var bound = DyCore_get_note_rendering_vertex_buffer_bound();
        if(buffer_get_size(cacheBuff) < bound) {
            buffer_resize(cacheBuff, bound);
        }

        // Render hold's bg
        render_state(1);

        // Render addition bg
        tempAdditionSurface = surface_checkate(tempAdditionSurface, 1920, 1080);
        surface_set_target(tempAdditionSurface);
        draw_clear_alpha(c_black, 0);
        gpu_set_blendmode_ext(bm_one, bm_zero);
        render_state(0);
        surface_reset_target();

        gpu_set_blendmode(bm_add);
        draw_surface(tempAdditionSurface, 0, 0);
        gpu_set_blendmode(bm_normal);

        // Render other parts.
        render_state(2);

        gpu_pop_state();

        // Emit holds particles.
        holdParticlesTimer += global.timeManager.get_delta() / 1000;
        holdParticlesTimer = min(holdParticlesTimer, 5 * PARTICLE_HOLD_DELAY);

        var partNum = floor(holdParticlesTimer / PARTICLE_HOLD_DELAY);
        holdParticlesTimer -= partNum * PARTICLE_HOLD_DELAY;

        if(partNum > 0) {
            var lastingHolds = dyc_get_lasting_holds();
            for(var i = 0, l = array_length(lastingHolds); i < l; i++) {
                var hold = dyc_get_note(lastingHolds[i]);
                note_emit_particles(PARTICLE_NOTE_LAST * partNum, hold, 1);
            }
        }
    }
}

global.noteRenderer = new NoteRenderer();