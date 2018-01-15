local _M = {}

function _M.genconf(dt)
    local default_data_timeout_seconds = 5
    local bats_data_timeout_seconds = default_data_timeout_seconds
    local baty_data_timeout_seconds = default_data_timeout_seconds
    local edgx_data_timeout_seconds = default_data_timeout_seconds
    local edga_data_timeout_seconds = default_data_timeout_seconds
    local arcx_data_timeout_seconds = default_data_timeout_seconds
    local xnys_ob_data_timeout_seconds = default_data_timeout_seconds
    local xnys_imb_data_timeout_seconds = 40
    local xnas_data_timeout_seconds = default_data_timeout_seconds
    local xbos_data_timeout_seconds = default_data_timeout_seconds
    local xpsx_data_timeout_seconds = default_data_timeout_seconds
    local bats_poller = "batspoller"
    local baty_poller = bats_poller
    local edgx_poller = bats_poller
    local edga_poller = bats_poller

    local xnas_poller = "nasdaqpoller"
    local xbos_poller = xnas_poller
    local xpsx_poller = xnas_poller

    local arcx_poller = "sftipoller"
    local xnys_poller = arcx_poller

    local simon_mdedge = "10.9.49.37"
    local simon_mdnasdaq = "10.9.49.53"
    local simon_mdsfti = "10.9.49.85"

    local garfunkel_mdedge = "10.9.49.36"
    local garfunkel_mdnasdaq = "10.9.49.52"
    local garfunkel_mdsfti = "10.9.49.84"

    local milli_mdbats = "10.8.49.52"
    local milli_mdedge = "10.8.49.68"
    local milli_mdnasdaq = "10.8.49.36"
    local milli_mdsfti = "10.8.49.84"

    local vanilli_mdbats = "10.8.49.53"
    local vanilli_mdedge = "10.8.49.69"
    local vanilli_mdnasdaq = "10.8.49.37"
    local vanilli_mdsfti = "10.8.49.85"

    local lemmy_mdbats = "10.14.49.52"
    local lemmy_mdedge = "10.14.49.68"
    local lemmy_mdnasdaq = "10.14.49.84"
    local lemmy_mdsfti = "10.14.49.36"

    local zeppelin_mdbats = "10.150.196.198"
    local zeppelin_mdedge = "10.105.116.38"
    local zeppelin_mdnasdaq = "10.49.85.118"
    local zeppelin_mdsfti = "10.148.250.118"

    if (dt == nil or dt == 0) then
        local dt = os.time()
    end
    -- local dt = os.time{year=2015, month=08, day=05}
    local arcx_symbol_mapping     = os.date("/storage/i01/data/ref/arca/%Y/%m/%d/ARCASymbolMapping.xml", dt)
    local xnys_symbol_mapping     = os.date("/storage/i01/data/ref/nyse/%Y/%m/%d/OpenBook/SymbolMap.xml", dt)
    local xnys_trd_symbol_mapping = os.date("/storage/i01/data/ref/nyse/%Y/%m/%d/NYSESymbolMapping_NMS.xml", dt)
    
    local md = {
          eventpollers = {
             batspoller = { affinity = 7 },
             nasdaqpoller = { affinity = 8 },
             sftipoller = { affinity = 9 },
             mdsys = { affinity = 6 },
          },
          exchanges = {
             BATS = {
                feeds = {
                   PITCH = {
                      local_ipaddr = { simon = simon_mdedge,
                                       garfunkel = garfunkel_mdedge,
                                       milli = milli_mdbats,
                                       vanilli = vanilli_mdbats,
                                       lemmy = lemmy_mdedge,
                                       zeppelin = zeppelin_mdedge
                      },
		      data_timeout_seconds = bats_data_timeout_seconds,
                      eventpoller = bats_poller,
                      units = {
                         ["1"] = {
                            A = { address = "224.0.130.128:30001" },
                            B = { address = "233.209.92.128:30001" }
                         },
                         ["2"] = {
                            A = { address = "224.0.130.128:30002" },
                            B = { address = "233.209.92.128:30002" }
                         },
                         ["3"] = {
                            A = { address = "224.0.130.128:30003" },
                            B = { address = "233.209.92.128:30003" }
                         },
                         ["4"] = {
                            A = { address = "224.0.130.128:30004" },
                            B = { address = "233.209.92.128:30004" }
                         },
                         ["5"] = {
                            A = { address = "224.0.130.129:30005" },
                            B = { address = "233.209.92.129:30005" }
                         },
                         ["6"] = {
                            A = { address = "224.0.130.129:30006" },
                            B = { address = "233.209.92.129:30006" }
                         },
                         ["7"] = {
                            A = { address = "224.0.130.129:30007" },
                            B = { address = "233.209.92.129:30007" }
                         },
                         ["8"] = {
                            A = { address = "224.0.130.129:30008" },
                            B = { address = "233.209.92.129:30008" }
                         },
                         ["9"] = {
                            A = { address = "224.0.130.130:30009" },
                            B = { address = "233.209.92.130:30009" }
                         },
                         ["10"] = {
                            A = { address = "224.0.130.130:30010" },
                            B = { address = "233.209.92.130:30010" }
                         },
                         ["11"] = {
                            A = { address = "224.0.130.130:30011" },
                            B = { address = "233.209.92.130:30011" }
                         },
                         ["12"] = {
                            A = { address = "224.0.130.130:30012" },
                            B = { address = "233.209.92.130:30012" }
                         },
                         ["13"] = {
                            A = { address = "224.0.130.131:30013" },
                            B = { address = "233.209.92.131:30013" }
                         },
                         ["14"] = {
                            A = { address = "224.0.130.131:30014" },
                            B = { address = "233.209.92.131:30014" }
                         },
                         ["15"] = {
                            A = { address = "224.0.130.131:30015" },
                            B = { address = "233.209.92.131:30015" }
                         },
                         ["16"] = {
                            A = { address = "224.0.130.131:30016" },
                            B = { address = "233.209.92.131:30016" }
                         },
                         ["17"] = {
                            A = { address = "224.0.130.132:30017" },
                            B = { address = "233.209.92.132:30017" }
                         },
                         ["18"] = {
                            A = { address = "224.0.130.132:30018" },
                            B = { address = "233.209.92.132:30018" }
                         },
                         ["19"] = {
                            A = { address = "224.0.130.132:30019" },
                            B = { address = "233.209.92.132:30019" }
                         },
                         ["20"] = {
                            A = { address = "224.0.130.132:30020" },
                            B = { address = "233.209.92.132:30020" }
                         },
                         ["21"] = {
                            A = { address = "224.0.130.133:30021" },
                            B = { address = "233.209.92.133:30021" }
                         },
                         ["22"] = {
                            A = { address = "224.0.130.133:30022" },
                            B = { address = "233.209.92.133:30022" }
                         },
                         ["23"] = {
                            A = { address = "224.0.130.133:30023" },
                            B = { address = "233.209.92.133:30023" }
                         },
                         ["24"] = {
                            A = { address = "224.0.130.133:30024" },
                            B = { address = "233.209.92.133:30024" }
                         },
                         ["25"] = {
                            A = { address = "224.0.130.134:30025" },
                            B = { address = "233.209.92.134:30025" }
                         },
                         ["26"] = {
                            A = { address = "224.0.130.134:30026" },
                            B = { address = "233.209.92.134:30026" }
                         },
                         ["27"] = {
                            A = { address = "224.0.130.134:30027" },
                            B = { address = "233.209.92.134:30027" }
                         },
                         ["28"] = {
                            A = { address = "224.0.130.134:30028" },
                            B = { address = "233.209.92.134:30028" }
                         },
                         ["29"] = {
                            A = { address = "224.0.130.135:30029" },
                            B = { address = "233.209.92.135:30029" }
                         },
                         ["30"] = {
                            A = { address = "224.0.130.135:30030" },
                            B = { address = "233.209.92.135:30030" }
                         },
                         ["31"] = {
                            A = { address = "224.0.130.135:30031" },
                            B = { address = "233.209.92.135:30031" }
                         },
                         ["32"] = {
                            A = { address = "224.0.130.135:30032" },
                            B = { address = "233.209.92.135:30032" }
                         },
             }}}},
             BATY = {
                feeds = {
                   PITCH = {
                      local_ipaddr = { simon = simon_mdedge,
                                       garfunkel = garfunkel_mdedge,
                                       milli = milli_mdbats,
                                       vanilli = vanilli_mdbats,
                                       lemmy = lemmy_mdedge,
                                       zeppelin = zeppelin_mdedge
                      },
                      eventpoller = baty_poller,
                      data_timeout_seconds = baty_data_timeout_seconds,
		      units = {
                         ["1"] = {
                            A = { address = "224.0.130.192:30201" },
                            B = { address = "233.209.92.192:30201" }
                         },
                         ["2"] = {
                            A = { address = "224.0.130.192:30202" },
                            B = { address = "233.209.92.192:30202" }
                         },
                         ["3"] = {
                            A = { address = "224.0.130.192:30203" },
                            B = { address = "233.209.92.192:30203" }
                         },
                         ["4"] = {
                            A = { address = "224.0.130.192:30204" },
                            B = { address = "233.209.92.192:30204" }
                         },
                         ["5"] = {
                            A = { address = "224.0.130.193:30205" },
                            B = { address = "233.209.92.193:30205" }
                         },
                         ["6"] = {
                            A = { address = "224.0.130.193:30206" },
                            B = { address = "233.209.92.193:30206" }
                         },
                         ["7"] = {
                            A = { address = "224.0.130.193:30207" },
                            B = { address = "233.209.92.193:30207" }
                         },
                         ["8"] = {
                            A = { address = "224.0.130.193:30208" },
                            B = { address = "233.209.92.193:30208" }
                         },
                         ["9"] = {
                            A = { address = "224.0.130.194:30209" },
                            B = { address = "233.209.92.194:30209" }
                         },
                         ["10"] = {
                            A = { address = "224.0.130.194:30210" },
                            B = { address = "233.209.92.194:30210" }
                         },
                         ["11"] = {
                            A = { address = "224.0.130.194:30211" },
                            B = { address = "233.209.92.194:30211" }
                         },
                         ["12"] = {
                            A = { address = "224.0.130.194:30212" },
                            B = { address = "233.209.92.194:30212" }
                         },
                         ["13"] = {
                            A = { address = "224.0.130.195:30213" },
                            B = { address = "233.209.92.195:30213" }
                         },
                         ["14"] = {
                            A = { address = "224.0.130.195:30214" },
                            B = { address = "233.209.92.195:30214" }
                         },
                         ["15"] = {
                            A = { address = "224.0.130.195:30215" },
                            B = { address = "233.209.92.195:30215" }
                         },
                         ["16"] = {
                            A = { address = "224.0.130.195:30216" },
                            B = { address = "233.209.92.195:30216" }
                         },
                         ["17"] = {
                            A = { address = "224.0.130.196:30217" },
                            B = { address = "233.209.92.196:30217" }
                         },
                         ["18"] = {
                            A = { address = "224.0.130.196:30218" },
                            B = { address = "233.209.92.196:30218" }
                         },
                         ["19"] = {
                            A = { address = "224.0.130.196:30219" },
                            B = { address = "233.209.92.196:30219" }
                         },
                         ["20"] = {
                            A = { address = "224.0.130.196:30220" },
                            B = { address = "233.209.92.196:30220" }
                         },
                         ["21"] = {
                            A = { address = "224.0.130.197:30221" },
                            B = { address = "233.209.92.197:30221" }
                         },
                         ["22"] = {
                            A = { address = "224.0.130.197:30222" },
                            B = { address = "233.209.92.197:30222" }
                         },
                         ["23"] = {
                            A = { address = "224.0.130.197:30223" },
                            B = { address = "233.209.92.197:30223" }
                         },
                         ["24"] = {
                            A = { address = "224.0.130.197:30224" },
                            B = { address = "233.209.92.197:30224" }
                         },
                         ["25"] = {
                            A = { address = "224.0.130.198:30225" },
                            B = { address = "233.209.92.198:30225" }
                         },
                         ["26"] = {
                            A = { address = "224.0.130.198:30226" },
                            B = { address = "233.209.92.198:30226" }
                         },
                         ["27"] = {
                            A = { address = "224.0.130.198:30227" },
                            B = { address = "233.209.92.198:30227" }
                         },
                         ["28"] = {
                            A = { address = "224.0.130.198:30228" },
                            B = { address = "233.209.92.198:30228" }
                         },
                         ["29"] = {
                            A = { address = "224.0.130.199:30229" },
                            B = { address = "233.209.92.199:30229" }
                         },
                         ["30"] = {
                            A = { address = "224.0.130.199:30230" },
                            B = { address = "233.209.92.199:30230" }
                         },
                         ["31"] = {
                            A = { address = "224.0.130.199:30231" },
                            B = { address = "233.209.92.199:30231" }
                         },
                         ["32"] = {
                            A = { address = "224.0.130.199:30232" },
                            B = { address = "233.209.92.199:30232" }
                         },
             }}}},
             EDGX ={
                feeds = {
                   PITCH = {
                      local_ipaddr = { simon = simon_mdedge,
                                       garfunkel = garfunkel_mdedge,
                                       milli = milli_mdbats,
                                       vanilli = vanilli_mdbats,
                                       lemmy = lemmy_mdedge,
                                       zeppelin = zeppelin_mdedge
                      },
		      data_timeout_seconds = edgx_data_timeout_seconds,
                      eventpoller = edgx_poller,
                      units = {
                         ["1"] = {
                            A = { address = "224.0.130.64:30401" },
                            B = { address = "233.209.92.64:30401" }
                         },
                         ["2"] = {
                            A = { address = "224.0.130.64:30402" },
                            B = { address = "233.209.92.64:30402" }
                         },
                         ["3"] = {
                            A = { address = "224.0.130.64:30403" },
                            B = { address = "233.209.92.64:30403" }
                         },
                         ["4"] = {
                            A = { address = "224.0.130.64:30404" },
                            B = { address = "233.209.92.64:30404" }
                         },
                         ["5"] = {
                            A = { address = "224.0.130.65:30405" },
                            B = { address = "233.209.92.65:30405" }
                         },
                         ["6"] = {
                            A = { address = "224.0.130.65:30406" },
                            B = { address = "233.209.92.65:30406" }
                         },
                         ["7"] = {
                            A = { address = "224.0.130.65:30407" },
                            B = { address = "233.209.92.65:30407" }
                         },
                         ["8"] = {
                            A = { address = "224.0.130.65:30408" },
                            B = { address = "233.209.92.65:30408" }
                         },
                         ["9"] = {
                            A = { address = "224.0.130.66:30409" },
                            B = { address = "233.209.92.66:30409" }
                         },
                         ["10"] = {
                            A = { address = "224.0.130.66:30410" },
                            B = { address = "233.209.92.66:30410" }
                         },
                         ["11"] = {
                            A = { address = "224.0.130.66:30411" },
                            B = { address = "233.209.92.66:30411" }
                         },
                         ["12"] = {
                            A = { address = "224.0.130.66:30412" },
                            B = { address = "233.209.92.66:30412" }
                         },
                         ["13"] = {
                            A = { address = "224.0.130.67:30413" },
                            B = { address = "233.209.92.67:30413" }
                         },
                         ["14"] = {
                            A = { address = "224.0.130.67:30414" },
                            B = { address = "233.209.92.67:30414" }
                         },
                         ["15"] = {
                            A = { address = "224.0.130.67:30415" },
                            B = { address = "233.209.92.67:30415" }
                         },
                         ["16"] = {
                            A = { address = "224.0.130.67:30416" },
                            B = { address = "233.209.92.67:30416" }
                         },
                         ["17"] = {
                            A = { address = "224.0.130.68:30417" },
                            B = { address = "233.209.92.68:30417" }
                         },
                         ["18"] = {
                            A = { address = "224.0.130.68:30418" },
                            B = { address = "233.209.92.68:30418" }
                         },
                         ["19"] = {
                            A = { address = "224.0.130.68:30419" },
                            B = { address = "233.209.92.68:30419" }
                         },
                         ["20"] = {
                            A = { address = "224.0.130.68:30420" },
                            B = { address = "233.209.92.68:30420" }
                         },
                         ["21"] = {
                            A = { address = "224.0.130.69:30421" },
                            B = { address = "233.209.92.69:30421" }
                         },
                         ["22"] = {
                            A = { address = "224.0.130.69:30422" },
                            B = { address = "233.209.92.69:30422" }
                         },
                         ["23"] = {
                            A = { address = "224.0.130.69:30423" },
                            B = { address = "233.209.92.69:30423" }
                         },
                         ["24"] = {
                            A = { address = "224.0.130.69:30424" },
                            B = { address = "233.209.92.69:30424" }
                         },
                         ["25"] = {
                            A = { address = "224.0.130.70:30425" },
                            B = { address = "233.209.92.70:30425" }
                         },
                         ["26"] = {
                            A = { address = "224.0.130.70:30426" },
                            B = { address = "233.209.92.70:30426" }
                         },
                         ["27"] = {
                            A = { address = "224.0.130.70:30427" },
                            B = { address = "233.209.92.70:30427" }
                         },
                         ["28"] = {
                            A = { address = "224.0.130.70:30428" },
                            B = { address = "233.209.92.70:30428" }
                         },
                         ["29"] = {
                            A = { address = "224.0.130.71:30429" },
                            B = { address = "233.209.92.71:30429" }
                         },
                         ["30"] = {
                            A = { address = "224.0.130.71:30430" },
                            B = { address = "233.209.92.71:30430" }
                         },
                         ["31"] = {
                            A = { address = "224.0.130.71:30431" },
                            B = { address = "233.209.92.71:30431" }
                         },
                         ["32"] = {
                            A = { address = "224.0.130.71:30432" },
                            B = { address = "233.209.92.71:30432" }
                         },
             }}}},
             EDGA ={
                feeds = {
                   PITCH = {
                      local_ipaddr = { simon = simon_mdedge,
                                       garfunkel = garfunkel_mdedge,
                                       milli = milli_mdbats,
                                       vanilli = vanilli_mdbats,
                                       lemmy = lemmy_mdedge,
                                       zeppelin = zeppelin_mdedge
                      },
		      data_timeout_seconds = edga_data_timeout_seconds,
                      eventpoller = edga_poller,
                      units = {
                         ["1"] = {
                            A = { address = "224.0.130.0:30301" },
                            B = { address = "233.209.92.0:30301" }
                         },
                         ["2"] = {
                            A = { address = "224.0.130.0:30302" },
                            B = { address = "233.209.92.0:30302" }
                         },
                         ["3"] = {
                            A = { address = "224.0.130.0:30303" },
                            B = { address = "233.209.92.0:30303" }
                         },
                         ["4"] = {
                            A = { address = "224.0.130.0:30304" },
                            B = { address = "233.209.92.0:30304" }
                         },
                         ["5"] = {
                            A = { address = "224.0.130.1:30305" },
                            B = { address = "233.209.92.1:30305" }
                         },
                         ["6"] = {
                            A = { address = "224.0.130.1:30306" },
                            B = { address = "233.209.92.1:30306" }
                         },
                         ["7"] = {
                            A = { address = "224.0.130.1:30307" },
                            B = { address = "233.209.92.1:30307" }
                         },
                         ["8"] = {
                            A = { address = "224.0.130.1:30308" },
                            B = { address = "233.209.92.1:30308" }
                         },
                         ["9"] = {
                            A = { address = "224.0.130.2:30309" },
                            B = { address = "233.209.92.2:30309" }
                         },
                         ["10"] = {
                            A = { address = "224.0.130.2:30310" },
                            B = { address = "233.209.92.2:30310" }
                         },
                         ["11"] = {
                            A = { address = "224.0.130.2:30311" },
                            B = { address = "233.209.92.2:30311" }
                         },
                         ["12"] = {
                            A = { address = "224.0.130.2:30312" },
                            B = { address = "233.209.92.2:30312" }
                         },
                         ["13"] = {
                            A = { address = "224.0.130.3:30313" },
                            B = { address = "233.209.92.3:30313" }
                         },
                         ["14"] = {
                            A = { address = "224.0.130.3:30314" },
                            B = { address = "233.209.92.3:30314" }
                         },
                         ["15"] = {
                            A = { address = "224.0.130.3:30315" },
                            B = { address = "233.209.92.3:30315" }
                         },
                         ["16"] = {
                            A = { address = "224.0.130.3:30316" },
                            B = { address = "233.209.92.3:30316" }
                         },
                         ["17"] = {
                            A = { address = "224.0.130.4:30317" },
                            B = { address = "233.209.92.4:30317" }
                         },
                         ["18"] = {
                            A = { address = "224.0.130.4:30318" },
                            B = { address = "233.209.92.4:30318" }
                         },
                         ["19"] = {
                            A = { address = "224.0.130.4:30319" },
                            B = { address = "233.209.92.4:30319" }
                         },
                         ["20"] = {
                            A = { address = "224.0.130.4:30320" },
                            B = { address = "233.209.92.4:30320" }
                         },
                         ["21"] = {
                            A = { address = "224.0.130.5:30321" },
                            B = { address = "233.209.92.5:30321" }
                         },
                         ["22"] = {
                            A = { address = "224.0.130.5:30322" },
                            B = { address = "233.209.92.5:30322" }
                         },
                         ["23"] = {
                            A = { address = "224.0.130.5:30323" },
                            B = { address = "233.209.92.5:30323" }
                         },
                         ["24"] = {
                            A = { address = "224.0.130.5:30324" },
                            B = { address = "233.209.92.5:30324" }
                         },
                         ["25"] = {
                            A = { address = "224.0.130.6:30325" },
                            B = { address = "233.209.92.6:30325" }
                         },
                         ["26"] = {
                            A = { address = "224.0.130.6:30326" },
                            B = { address = "233.209.92.6:30326" }
                         },
                         ["27"] = {
                            A = { address = "224.0.130.6:30327" },
                            B = { address = "233.209.92.6:30327" }
                         },
                         ["28"] = {
                            A = { address = "224.0.130.6:30328" },
                            B = { address = "233.209.92.6:30328" }
                         },
                         ["29"] = {
                            A = { address = "224.0.130.7:30329" },
                            B = { address = "233.209.92.7:30329" }
                         },
                         ["30"] = {
                            A = { address = "224.0.130.7:30330" },
                            B = { address = "233.209.92.7:30330" }
                         },
                         ["31"] = {
                            A = { address = "224.0.130.7:30331" },
                            B = { address = "233.209.92.7:30331" }
                         },
                         ["32"] = {
                            A = { address = "224.0.130.7:30332" },
                            B = { address = "233.209.92.7:30332" }
                         },
             }}}},
             ARCX = {
                symbol_mapping = arcx_symbol_mapping,
                feeds = {
                   INTXDP = {
                      local_ipaddr = { simon = simon_mdsfti,
                                       garfunkel = garfunkel_mdsfti,
                                       milli = milli_mdsfti,
                                       vanilli = vanilli_mdsfti,
                                       lemmy = lemmy_mdsfti,
                                       zeppelin = zeppelin_mdsfti
                      },
		      data_timeout_seconds = arcx_data_timeout_seconds,
                      eventpoller = arcx_poller,
                      units = {
                         INT_1 = {
                            A = { address = "224.0.59.64:11064" },
                            B = { address = "224.0.59.192:11192" }
                         },
                         INT_2 = {
                            A = { address = "224.0.59.65:11065" },
                            B = { address = "224.0.59.193:11193" }
                         },
                         INT_3 = {
                            A = { address = "224.0.59.66:11066" },
                            B = { address = "224.0.59.194:11194" }
                         },
                         INT_4 = {
                            A = { address = "224.0.59.67:11067" },
                            B = { address = "224.0.59.195:11195" }
                         }
             }}}},
             XNYS={
                symbol_mapping = xnys_symbol_mapping,
                feeds = {
                   OB = {
                      -- protocol = "PDP",
                      local_ipaddr = { simon = simon_mdsfti,
                                       garfunkel = garfunkel_mdsfti,
                                       milli = milli_mdsfti,
                                       vanilli = vanilli_mdsfti,
                                       lemmy = lemmy_mdsfti,
                                       zeppelin = zeppelin_mdsfti
                      },
	  	      data_timeout_seconds = xnys_ob_data_timeout_seconds,
                      eventpoller = xnys_poller,
                      units = {
                         OB_AA = {
                            A = { address = "233.75.215.96:60096" },
                            B= { address = "233.75.215.224:60224" }
                         },
                         OB_BB = {
                            A = { address = "233.75.215.97:60097" },
                            B= { address = "233.75.215.225:60225" }
                         },
                         OB_CC = {
                            A = { address = "233.75.215.98:60098" },
                            B= { address = "233.75.215.226:60226" }
                         },
                         OB_DD = {
                            A = { address = "233.75.215.99:60099" },
                            B= { address = "233.75.215.227:60227" }
                         },
                         OB_EE = {
                            A = { address = "233.75.215.100:60100" },
                            B= { address = "233.75.215.228:60228" }
                         },
                         OB_FF = {
                            A = { address = "233.75.215.101:60101" },
                            B= { address = "233.75.215.229:60229" }
                         },
                         OB_GG = {
                            A = { address = "233.75.215.102:60102" },
                            B= { address = "233.75.215.230:60230" }
                         },
                         OB_HH = {
                            A = { address = "233.75.215.103:60103" },
                            B= { address = "233.75.215.231:60231" }
                         },
                         OB_IJ = {
                            A = { address = "233.75.215.104:60104" },
                            B= { address = "233.75.215.232:60232" }
                         },
                         OB_KK = {
                            A = { address = "233.75.215.105:60105" },
                            B= { address = "233.75.215.233:60233" }
                         },
                         OB_LL = {
                            A = { address = "233.75.215.106:60106" },
                            B= { address = "233.75.215.234:60234" }
                         },
                         OB_MM = {
                            A = { address = "233.75.215.107:60107" },
                            B= { address = "233.75.215.235:60235" }
                         },
                         OB_NN = {
                            A = { address = "233.75.215.108:60108" },
                            B= { address = "233.75.215.236:60236" }
                         },
                         OB_OO = {
                            A = { address = "233.75.215.109:60109" },
                            B= { address = "233.75.215.237:60237" }
                         },
                         OB_PQ = {
                            A = { address = "233.75.215.110:60110" },
                            B= { address = "233.75.215.238:60238" }
                         },
                         OB_RR = {
                            A = { address = "233.75.215.111:60111" },
                            B= { address = "233.75.215.239:60239" }
                         },
                         OB_SS = {
                            A = { address = "233.75.215.112:60112" },
                            B= { address = "233.75.215.240:60240" }
                         },
                         OB_TT = {
                            A = { address = "233.75.215.113:60113" },
                            B= { address = "233.75.215.241:60241" }
                         },
                         OB_UV = {
                            A = { address = "233.75.215.114:60114" },
                            B= { address = "233.75.215.242:60242" }
                         },
                         OB_WZ = {
                            A = { address = "233.75.215.115:60115" },
                            B= { address = "233.75.215.243:60243" }
                         }
                      }
                   },
                   IMB = {
                      -- protocol = "PDP",
                      local_ipaddr = { simon = simon_mdsfti,
                                       garfunkel = garfunkel_mdsfti,
                                       milli = milli_mdsfti,
                                       vanilli = vanilli_mdsfti,
                                       lemmy = lemmy_mdsfti,
                                       zeppelin = zeppelin_mdsfti
                      },
		      data_timeout_seconds = xnys_imb_data_timeout_seconds,
                      eventpoller = xnys_poller,
                      units = {
                         IMB = {
                            A = { address = "233.75.215.44:60044" },
                            B= { address = "233.75.215.172:60172" }
                         }
                      }
                   },
                   TRD = {
                      symbol_mapping = xnys_trd_symbol_mapping,
                      -- protocol = "XDP",
                      local_ipaddr = { simon = simon_mdsfti,
                                       garfunkel = garfunkel_mdsfti,
                                       milli = milli_mdsfti,
                                       vanilli = vanilli_mdsfti,
                                       lemmy = lemmy_mdsfti,
                                       zeppelin = zeppelin_mdsfti
                      },
                      eventpoller = xnys_poller,
                      units = {
                         TRD = {
                            A = { address = "233.75.215.40:8040" },
                            B = { address = "233.75.215.168:8168" }
                         }
             }}}},
             XNAS = {
                feeds = {
                   ITCH5 = {
                      local_ipaddr = { simon = simon_mdnasdaq,
                                       garfunkel = garfunkel_mdnasdaq,
                                       milli = milli_mdnasdaq,
                                       vanilli = vanilli_mdnasdaq,
                                       lemmy = lemmy_mdnasdaq,
                                       zeppelin = zeppelin_mdnasdaq
                      },
		      data_timeout_seconds = xnas_data_timeout_seconds,
                      eventpoller = xnas_poller,
                      units = {
                         ITCH5 = {
                            A = { address = "233.54.12.111:26477" },
                            B = { address = "233.49.196.111:26477" }
                         }
             }}}},
             XBOS = {
                feeds = {
                   ITCH5 = {
                      local_ipaddr = { simon = simon_mdnasdaq,
                                       garfunkel = garfunkel_mdnasdaq,
                                       milli = milli_mdnasdaq,
                                       vanilli = vanilli_mdnasdaq,
                                       lemmy = lemmy_mdnasdaq,
                                       zeppelin = zeppelin_mdnasdaq
                      },
		      data_timeout_seconds = xbos_data_timeout_seconds,
                      eventpoller = xbos_poller,
                      units = {
                         ITCH5 = {
                            A = { address = "233.54.12.40:25475"},
                            B = { address = "233.49.196.40:25475"}
                         }
             }}}},
             XPSX = {
                feeds = {
                   ITCH5 = {
                      local_ipaddr = { simon = simon_mdnasdaq,
                                       garfunkel = garfunkel_mdnasdaq,
                                       milli = milli_mdnasdaq,
                                       vanilli = vanilli_mdnasdaq,
                                       lemmy = lemmy_mdnasdaq,
                                       zeppelin = zeppelin_mdnasdaq
                      },
		      data_timeout_seconds = xpsx_data_timeout_seconds,
                      eventpoller = xpsx_poller,
                      units = {
                         ITCH5 = {
                            A = { address = "233.54.12.45:26477" },
                            B = { address = "233.49.196.45:26477"}
                         }
                      }
                   }
                }
             }
          }
       }
    return md
end

return _M
