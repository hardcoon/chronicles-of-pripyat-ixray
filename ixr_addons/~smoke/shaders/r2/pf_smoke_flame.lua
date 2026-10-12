function normal(shader, t_base, t_second, t_detail)
    shader:begin("deffer_model", "pf_smoke_flame")
        :fog(false)
        :zb(true, false)
        :blend(true, blend.srcalpha, blend.one)
        :sorting(3, true)
end
