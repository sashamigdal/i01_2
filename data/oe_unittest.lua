conf = {
    oe = {
        sessions = {
            SimTest1 = { type = "L2SimSession", mic = "XNYS" },
            SimTest2 = { type = "L2SimSession", mic = "BATS" },
            SimTest3 = { type = "L2SimSession", mic = "XNAS" },
            SimTest4 = { type = "L2SimSession", mic = "ARCX" },
            SimTest5 = { type = "L2SimSession", mic = "XNYS" },
            SimTest6 = { type = "L2SimSession", mic = "XNYS" },
            BOETest1 = { type = "BOE20Session",
                                    mic = "BATS",
                                    active = "true",
                                    remote = { addr = "127.0.0.1", port = "5555" },
                                    session_sub_id = "0013",
                                    username = "PICO",
                                    password = "bz11pico",
                                    clearing_firm = "PQTZ",
                                    clearing_account = "TEST"
            }
        },
        universe = {
           default = {
              order_size_limit = 10000,
              order_price_limit = 1000,
              order_value_limit = 10000,
              order_rate_limit = 100,
              position_limit = 10000,
              position_value_limit = 20000,
              lot_size = 100,
              locate_size = 10000
           }
        },
        risk = {
           firm = {
              realized_loss_limit = 5000,
              unrealized_loss_limit = 15000,
              gross_notional_limit = 100000,
              net_notional_limit = 100000,
              long_open_exposure_limit = 200000,
              short_open_exposure_limit = 200000,
              gross_open_exposure_limit = 400000
           }
        }
    }
}
