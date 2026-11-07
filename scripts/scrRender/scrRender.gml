
/// @description This singleton handles note rendering under playback mode.
function NoteRenderer() constructor {

    // Init vertex format.
    vertex_format_begin();
    vertex_format_add_position();
    vertex_format_add_texcoord();
    vertex_format_add_color();
    // Position, UV/local coordinates, color, and mapping parameters: 28 bytes.
    vertex_format_add_custom(vertex_type_float2, vertex_usage_texcoord);
    vertFormat = vertex_format_end();
    
    // Create vertex buffer.
    vertexSize = 28;
    cacheBuff = buffer_create(vertexSize * 1024, buffer_fast, 1);
    vertBuff = vertex_create_buffer_from_buffer(cacheBuff, vertFormat);

    // Hold Particles timer.
    holdParticlesTimer = 0;
    tempAdditionSurface = -1;

    noteUv = []; chainUv = []; holdUv = []; edgeUv = [];
    noteSegments = []; chainSegments = []; edgeSegments = [];
    uniformNoteUv = shader_get_uniform(shd_notes, "u_note_uv");
    uniformChainUv = shader_get_uniform(shd_notes, "u_chain_uv");
    uniformHoldUv = shader_get_uniform(shd_notes, "u_hold_uv");
    uniformNoteSegments = shader_get_uniform(shd_notes, "u_note_segments");
    uniformChainSegments = shader_get_uniform(shd_notes, "u_chain_segments");
    uniformEdgeUv = shader_get_uniform(shd_notes, "u_edge_uv");
    uniformEdgeSegments = shader_get_uniform(shd_notes, "u_edge_segments");
    uniformMono = shader_get_uniform(shd_notes, "u_mono");

    /// @description Share the sprite mapping registered with DyCore.
    /// @param {Any} data Sprite metadata.
    static set_sprite_data = function(data) {
        var uv = [data.uv00, data.uv01, data.uv10, data.uv11];
        switch(data.name) {
            case "sprNote":
                noteUv = uv;
                noteSegments = [data.data[0], data.data[1], data.width];
                break;
            case "sprChain":
                chainUv = uv;
                chainSegments = [data.data[0], data.data[1], data.data[2], data.width];
                break;
            case "sprHold":
                holdUv = uv;
                break;
            case "sprHoldEdge":
                edgeUv = uv;
                edgeSegments = [data.data[0], data.data[1], data.width];
                break;
        }
    }

    static render_state = function(state) {
        /// @type {Real} 
        var size = DyCore_render_active_notes(
            buffer_get_address(cacheBuff), objMain.nowTime, objMain.playbackSpeed, state);

		if(size < 0) return;

        buffer_set_used_size(cacheBuff, size);
        vertex_update_buffer_from_buffer(vertBuff, 0, cacheBuff, 0, size);

        var texture = texturegroup_get_textures("texNotes")[0];
        vertex_submit_ext(vertBuff, pr_trianglelist, texture, 0, size / vertexSize);
    }

    static render = function() {
        gpu_push_state();
        gpu_set_tex_repeat(true);
        dyc_update_active_notes();

        var bound = DyCore_prepare_note_rendering();
        if(bound < 0) {
            gpu_pop_state();
            return;
        }
        if(buffer_get_size(cacheBuff) < bound) {
            var capacity = ceil(bound * 1.5 / 4096) * 4096;
            buffer_resize(cacheBuff, capacity);
        }

        var previousShader = shader_current();
        shader_set(shd_notes);
        shader_set_uniform_f_array(uniformNoteUv, noteUv);
        shader_set_uniform_f_array(uniformChainUv, chainUv);
        shader_set_uniform_f_array(uniformHoldUv, holdUv);
        shader_set_uniform_f_array(uniformNoteSegments, noteSegments);
        shader_set_uniform_f_array(uniformChainSegments, chainSegments);
        shader_set_uniform_f_array(uniformEdgeUv, edgeUv);
        shader_set_uniform_f_array(uniformEdgeSegments, edgeSegments);
        shader_set_uniform_f(uniformMono, previousShader == shd_mono);

        // Render hold's bg
        render_state(1);

        // Render addition bg
        tempAdditionSurface = surface_checkate(tempAdditionSurface, 1920, 1080);
        surface_set_target(tempAdditionSurface);
        draw_clear_alpha(c_black, 0);
        gpu_set_blendmode_ext(bm_one, bm_zero);
        render_state(0);
        surface_reset_target();
        // Surface compositing keeps the caller's shader and its theme behavior.
        if(previousShader == -1) shader_reset();
        else shader_set(previousShader);

        gpu_set_blendmode(bm_add);
        draw_surface(tempAdditionSurface, 0, 0);
        gpu_set_blendmode(bm_normal);

        // Render other parts.
        shader_set(shd_notes);
        render_state(2);
        if(previousShader == -1) shader_reset();
        else shader_set(previousShader);

        gpu_pop_state();

        // Emit holds particles.
        holdParticlesTimer += global.timeManager.get_delta() / 1000;
        holdParticlesTimer = min(holdParticlesTimer, 5 * PARTICLE_HOLD_DELAY);

        var partNum = floor(holdParticlesTimer / PARTICLE_HOLD_DELAY);
        holdParticlesTimer -= partNum * PARTICLE_HOLD_DELAY;

        if(partNum > 0 && objMain.nowPlaying && global.particleEffects == 1 &&
            !(part_particles_count(objMain.partSysNote) > MAX_PARTICLE_COUNT)) {
            var lastingHolds = dyc_get_lasting_holds();
            for(var i = 0, l = array_length(lastingHolds); i < l; i++) {
                if(part_particles_count(objMain.partSysNote) > MAX_PARTICLE_COUNT) break;
                var hold = dyc_get_note(lastingHolds[i]);
                note_emit_particles(PARTICLE_NOTE_LAST * partNum, hold, 1);
            }
        }
    }

    static cleanup = function() {
        if(surface_exists(tempAdditionSurface)) surface_free(tempAdditionSurface);
        tempAdditionSurface = -1;
        if(buffer_exists(cacheBuff)) buffer_delete(cacheBuff);
        cacheBuff = -1;
        if(vertBuff != -1) vertex_delete_buffer(vertBuff);
        vertBuff = -1;
        if(vertFormat != -1) vertex_format_delete(vertFormat);
        vertFormat = -1;
    }
}

global.noteRenderer = new NoteRenderer();