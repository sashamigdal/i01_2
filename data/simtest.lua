-- local time = require("time")
function hms_to_ns(h, m, s, ms)
    s = s or 0
    ms = ms or 0
    return ((((((h * 60) + m) * 60) + s) * 1000) + ms) * 1000 * 1000
end

conf = {
   ts = {
      strategies = {
         sampler = {
            type = "simtest"
         }
      }
   },

   oe = {
      sessions = {
         UAT3 = {
            active = true,
            type = "BOE20Session",
            mic = "BATS",
            local_interface_name = "oehprbats",

            mocloc_add_deadline_ns = hms_to_ns(15, 55),
            mocloc_mod_deadline_ns = hms_to_ns(16, 0),

            -- remote = { addr = "174.136.174.68", port = 10443 },
            remote = { addr = "127.0.0.1", port = 5555 },
            username = "PICO",
            session_sub_id = "0013",
            password = "bz11pico",
            trading_group = "",
            mpid = "PQTZ", -- TODO: allowed_mpids = { A, B, C, ... },
            clearing_firm = "PQTZ",
            clearing_account = "TEST",
         }
      }
   }
}
