-- Textured, depth-tested ghost. It never writes scene depth.
function normal(shader, t_base, t_second, t_detail)
    shader:begin("script_preview_model", "script_preview_model")
        :fog(false)
        :zb(false, false)
        :blend(true, blend.srcalpha, blend.invsrcalpha)
    shader:dx10texture("s_base", t_base)
    shader:dx10texture("s_position", "$user$position")
    shader:dx10sampler("smp_base")
    shader:dx10sampler("smp_nofilter")
end
