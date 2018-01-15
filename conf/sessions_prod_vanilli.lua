local time = require("time")

conf = {
fix8 = { globallogger = { path = "/tmp/fix8_global.log" } },
oe = {
sessions = {
    XNAS1 = {
        active = true,
        type = "OUCH42Session",
        mic = "XNAS",
        local_interface_name = "oehprnasdaq",

        mocloc_add_deadline_ns = time.hms_to_ns(15, 50),
        mocloc_mod_deadline_ns = time.hms_to_ns(15, 50),

        protocol = "SoupBinTCP",
        remote = { addr = "127.0.0.1", port = 8002 }, -- Carteret
        -- remote = { addr = "206.200.124.238", port = 8002 }, -- Carteret
        -- remote = { addr = "66.36.85.39", port = 26471 }, -- Ashburn
        username = "S15329",
        password = "FD192",
        heartbeat_grace_period = 5, -- seconds until disconnect
        firm_identifier = "    ", -- "ABCD"
    },
}
}
}
-- return sessions
