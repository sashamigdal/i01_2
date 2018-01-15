local time = require("time")

conf = {
fix8 = { globallogger = { path = "/tmp/fix8_global.log" } },
oe = {
sessions = {
    BATS1 = {
        active = true,
        type = "BOE20Session",
        mic = "BATS",
        local_interface_name = "oehprbats",

        mocloc_add_deadline_ns = time.hms_to_ns(15, 55),
        mocloc_mod_deadline_ns = time.hms_to_ns(16, 0),

        -- remote = { addr = "208.90.210.72", port = 12152 }, -- NJ2
        -- remote = { addr = "174.136.177.41", port = 12152 }, -- Chicago
        remote = { addr = "127.0.0.1", port = 5555 },
        username = "PICO",
        session_sub_id = "0654",
        password = "FD191",
        trading_group = "",
        clearing_firm = "UBSS",
        clearing_account = "    ", -- required by HPR but not anyone else
        mpid = "UBSS",
    },
}
}
}
-- return sessions
