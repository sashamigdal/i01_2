export engine_date=20150824
# The system-wide LuaJIT is not built with Valgrind support built in (and uses
# MAP_32BIT and an internal malloc replacement for allocations).
./build.sh -d -- -DI01_ENABLE_LUAJIT=ON && \
$@ .build.gcc.Debug.$HOSTNAME/bin/engine \
    --engine.id 1 \
    --engine.date 20150824 \
    --engine.plugins-path-prefix .build.gcc.Debug.$HOSTNAME/lib \
    --engine.stop-at 16:31:00 \
    --lua-file conf/strategy_aleph.lua \
    --conf 'md.hist.search_path=/srv/data/src/i01-marketdata/pcap-sorted/%Y/%m/%d/' 
    #--conf 'md.hist.search_path=/media/fdo_xc/storage-xc/i01/data/pcap-sorted/%Y/%m/%d/' \
    #--lua-file data/sp500.lua \
    #--lua-file data/full.lua \
    #--lua-file conf/sessions_prod_vanilli.lua \
    #--lua-file data/manual.lua \
    #--lua-file conf/etb_locates20150504.lua \
    #--lua-file data/risk_prod_test.lua \
