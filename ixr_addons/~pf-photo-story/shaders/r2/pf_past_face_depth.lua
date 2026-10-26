-- Native shader VM material, scoped to the ghost head depth copies.
function normal(shader,t_base,t_second,t_detail)
    shader:begin("deffer_model","pf_past_depth")
        :sorting(2,false)
        :fog(false)
        :zb(true,true)
        :blend(false,blend.one,blend.zero)
        :color_write_enable(false,false,false,false)
end
