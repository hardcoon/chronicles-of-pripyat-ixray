-- Use the accepted watch's depth/velocity pass and native HUD emissive pass.
-- The mesh remains on the same l_forearm mount; no independent world transform.
function normal(shader,t_base,t_second,t_detail)
    shader:begin("deffer_model","st_watch")
        :fog(false):emissive(true)
    shader:dx10texture("s_base","pf_exo\\display\\atlas")
    shader:dx10sampler("smp_base")
    shader:dx10texture("s_watch_sdf","st_watch\\font_sdf")
    shader:dx10sampler("smp_rtlinear")
    shader:dx10stencil(true,cmp_func.always,255,127,stencil_op.keep,stencil_op.replace,stencil_op.keep)
    shader:dx10stencil_ref(1)
end
function l_special(shader,t_base,t_second,t_detail)
    shader:begin("deffer_model","pf_exo_status")
        :zb(true,false):fog(false):emissive(true)
    shader:dx10texture("s_base","pf_exo\\display\\atlas")
    shader:dx10sampler("smp_base")
    shader:dx10texture("s_watch_sdf","st_watch\\font_sdf")
    shader:dx10sampler("smp_rtlinear")
end
