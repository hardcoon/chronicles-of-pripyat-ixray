-- World-space editor overlay; shares the engine's existing vertex-colour shaders.
function normal(shader, t_base, t_second, t_detail)
    shader:begin("debug_draw", "debug_draw_nodepth")
        :zb(false, false)
        :blend(true, blend.srcalpha, blend.invsrcalpha)
end
