local _M = {}

function _M.hms_to_ns(h, m, s, ms) 
    s = s or 0
    ms = ms or 0
    return ((((((h * 60) + m) * 60) + s) * 1000) + ms) * 1000 * 1000
end

function _M.hms_to_ms(h, m, s, ms) 
    s = s or 0
    ms = ms or 0
    return ((((((h * 60) + m) * 60) + s) * 1000) + ms)
end

return _M
