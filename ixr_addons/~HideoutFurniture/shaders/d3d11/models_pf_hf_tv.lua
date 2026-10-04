-- World TV screen only. Theora textures contain V/U/Y bytes on IX-Ray R4.
function normal(shader, t_base, t_second, t_detail)
    local screen = string.find(string.lower(t_base), "pf_hf_expansion", 1, true)
        and "models_pf_hf_tv_crt" or "models_pf_hf_tv"
    shader:begin("models_pf_hf_glass", screen)
        :fog(true):zb(true, true)
        :blend(false, blend.one, blend.zero):sorting(2, true):distort(false)
    shader:dx10texture("s_base", "pf_hf_video\\tv_broadcast_clean")
    shader:dx10sampler("smp_base")
end
