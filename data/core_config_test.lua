
conf = {
    dingus = {
        name = "my example feed",
        size = 42,
    },
}

for k,v in pairs(current_conf) do
    if (k == "dingus.size") then
        conf.dingus.prev_size = v
    end
end
