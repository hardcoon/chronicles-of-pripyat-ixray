function normal(shader,t_base,t_second,t_detail)
    shader:begin("deffer_model","st_watch_glass")
        :fog(false):zb(true,false)
        :blend(true,blend.srcalpha,blend.invsrcalpha):sorting(2,true)
    shader:dx10texture("s_base",t_base)
    shader:dx10sampler("smp_base")
end
