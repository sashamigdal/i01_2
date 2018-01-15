xnys = dofile("data/xnys.lua")
arcx = dofile("data/arcx_int.lua")
bats = dofile("data/bats.lua")
baty = dofile("data/baty.lua")
edgx = dofile("data/edgx.lua")
edga = dofile("data/edga.lua")
xnas = dofile("data/xnas.lua")
xbos = dofile("data/xbos.lua")
xpsx = dofile("data/xpsx.lua")

exchanges = {
   XNYS = xnys,
   ARCX = arcx,
   BATS = bats,
   BATY = baty,
   EDGX = edgx,
   EDGA = edga,
   XNAS = xnas,
   XBOS = xbos,
   XPSX = xpsx
}

return exchanges
