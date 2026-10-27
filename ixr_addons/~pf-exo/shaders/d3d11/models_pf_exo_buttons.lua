-- Inspection-only HUD overlay: no distortion queue or angle-dependent alpha.
function normal(shader,t_base,t_second,t_detail)
    shader:begin("deffer_model","pf_exo_buttons")
        :fog(false):zb(true,false)
        :blend(true,blend.srcalpha,blend.invsrcalpha):sorting(2,true)
    shader:dx10texture("s_base",t_base)
    shader:dx10sampler("smp_base")
end
