local time = require("time")

conf = {
oe = {
sessions = {
    UAT1 = {
        active = false,
        type = "OUCH42Session",
        mic = "XNAS",
        local_interface_name = "oehprnasdaq",

        mocloc_add_deadline_ns = time.hms_to_ns(15, 50),
        mocloc_mod_deadline_ns = time.hms_to_ns(15, 50),

        protocol = "SoupBinTCP",
        -- for UFO: protocol = "UDP",
        remote = { addr = "10.7.48.12", port = 8007 },
        -- direct: remote = { addr = "206.200.20.52", port = 8007, },
        username = "S152T3",
        password = "testonly",
        heartbeat_grace_period = 5, -- seconds until disconnect
        firm_identifier = "", -- "ABCD"
    },
    UAT2 = {
        active = false,
        type = "NYSEAlgoSession",
        mic = "XNYS",
        local_interface_name = "oehprsfti",

        mocloc_add_deadline_ns = time.hms_to_ns(15, 59, 50),
        mocloc_mod_deadline_ns = time.hms_to_ns(15, 59, 59, 999),

        fix8_session_xml = "NYSEAlgoUAT.sessions.xml",
        bag_session_name = "BAG_UAT",
        cbs_session_name = "CBS_UAT",
        bag_max_message_per_second = 100,
        cbs_max_message_per_second = 100,
        cbs_parent_mnemonic_filter = "PICO",
        cbs_parent_agency_filter = "1A", -- prod: 8U
        nyse_algo_name = "Artemis",
    },
    UAT3 = {
        active = true,
        type = "BOE20Session",
        mic = "BATS",
        local_interface_name = "oehprbats",

        mocloc_add_deadline_ns = time.hms_to_ns(15, 55),
        mocloc_mod_deadline_ns = time.hms_to_ns(16, 0),

        -- remote = { addr = "174.136.174.68", port = 10443 },
        remote = { addr = "127.0.0.1", port = 5555 },
        username = "PICO",
        session_sub_id = "0013",
        password = "bz11pico",
        trading_group = "",
        mpid = "PQTZ", -- TODO: allowed_mpids = { A, B, C, ... },
        clearing_firm = "PQTZ",
        clearing_account = "TEST",
    },
    UAT4 = {
        active = false,
        type = "BOE20Session",
        mic = "BATY",
        local_interface_name = "oehprbats",

        mocloc_add_deadline_ns = time.hms_to_ns(15, 55),
        mocloc_mod_deadline_ns = time.hms_to_ns(16, 0),

        remote = { addr = "174.136.174.132", port = 10352 },
        username = "PICO",
        sendersubid = "0013",
        password = "by11pico",
        trading_group = "",
        mpid = "PQTZ", -- TODO: allowed_mpids = { A, B, C, ... },
    },
    UAT5 = {
        active = false,
        type = "BOE20Session",
        mic = "EDGX",
        local_interface_name = "oehpredge",

        mocloc_add_deadline_ns = time.hms_to_ns(15, 55),
        mocloc_mod_deadline_ns = time.hms_to_ns(16, 0),

        remote = { addr = "174.136.174.36", port = 10273 },
        username = "PICO",
        sendersubid = "0008",
        password = "bx8pico",
        trading_group = "",
        mpid = "PQTZ",
    },
    UAT6 = {
        active = false,
        type = "BOE20Session",
        mic = "EDGA",
        local_interface_name = "oehpredge",

        mocloc_add_deadline_ns = time.hms_to_ns(15, 55),
        mocloc_mod_deadline_ns = time.hms_to_ns(16, 0),

        remote = { addr = "174.136.174.4", port = 10285 },
        username = "PICO",
        sendersubid = "0008",
        password = "ba8pico",
        trading_group = "",
        mpid = "PQTZ",
    },
    UAT7 = {
        active = false,
        type = "UTPDirectSession",
        mic = "XNYS",
        local_interface_name = "oehprsfti",

        mocloc_add_deadline_ns = time.hms_to_ns(15, 45),
        mocloc_mod_deadline_ns = time.hms_to_ns(15, 58),

        remote = { addr = "162.68.216.65", port = 54207 },
        sendercompid = "PCQT-11",
        onbehalfofcompid = "",
        -- optional: sendersubid = "",
        -- optional: clearingfirm = "",
        -- optional: account = "",
    },
    UAT8 = {
        active = false,
        type = "ArcaDirectSession",
        mic = "ARCX",
        local_interface_name = "oehprsfti",

        mocloc_add_deadline_ns = time.hms_to_ns(15, 59),
        mocloc_mod_deadline_ns = time.hms_to_ns(15, 59),

        remote = { addr = "162.68.216.151", port = 52201 },
        logon_id = "PCO10",
        group_id = "ATEST",
    },
    UAT9 = {
        active = false,
        type = "COUCHSession",
        mic = "CAES",
        local_interface_name = "oehprnasdaq",

        -- mocloc_add_deadline_ns = time.hms_to_ns(0, 0),
        -- mocloc_mod_deadline_ns = time.hms_to_ns(0, 0),

        remote = { addr = "169.33.18.7", port = 10350 },
        username = "FDOUAT",
        password = "TEST123",
    },

}
}
}
-- return sessions
