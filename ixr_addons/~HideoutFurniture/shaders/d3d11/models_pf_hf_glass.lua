-- Full-UV furniture glass; does not use the weapon reflex lens atlas.
function normal(shader, t_base, t_second, t_detail)
    shader:begin("models_pf_hf_glass", "models_pf_hf_glass")
        :fog(true):zb(true, false)
        :blend(true, blend.srcalpha, blend.invsrcalpha)
        :aref(true, 0):sorting(2, true):distort(false)
    shader:dx10texture("s_base", t_base)
    shader:dx10sampler("smp_base")
end
