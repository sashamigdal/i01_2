exchanges = dofile("data/exchanges.lua")

conf = {
   md = {
      exchanges = exchanges,
	universe = {
		symbol={

			["1"] = { fdo_symbol = 1748, pdp_symbol = "APC", cta_symbol = "APC" }
		} 
   	}
	}	
}
