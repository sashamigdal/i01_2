ARCX = {
   symbol_mapping = "data/ARCASymbolMapping.xml",
   feeds = {
      INTXDP = {
         local_ipaddr = "127.0.0.1:0",
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
         }
      }
   }
}

return ARCX
