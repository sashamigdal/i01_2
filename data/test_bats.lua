local units = {}

units[1] = "A"
units[2] = "AH"
units[3] = "AS"

univ = dofile("data/sp500_univ.lua")

local res = nil
for esi,vals in pairs(univ.symbol) do
   for i=1,32 do
      if units[i] and vals.pdp_symbol < units[i] then
         univ.symbol[esi].bats_unit = i - 1
         break
      end
   end
end


conf = { universe = univ }
