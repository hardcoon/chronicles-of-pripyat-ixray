function normal(shader, t_base, t_second, t_detail)
    shader:begin("pf_pda_screen", "pf_pda_screen")
        :fog(false)
        :zb(true, false)
        :blend(true, blend.srcalpha, blend.invsrcalpha)
        :sorting(2, true)
    shader:dx10texture("s_pdascreen", "$user$pf_pda")
    shader:dx10sampler("smp_pda")
end
