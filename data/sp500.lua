exchanges = dofile("data/exchanges.lua")
sp500 = dofile("data/sp500_univ.lua")

conf = {
   md = {
      exchanges = exchanges,
      universe = sp500
   }
}
