conf = {
   oe = {
      universe = {
            default = {
               order_size_limit = 300
               , order_price_limit = 900
               , order_value_limit = 15000
               , order_rate_limit = 10
               , position_limit = 1000
               , position_value_limit = 20000
               , lot_size = 100
            }
         }

      , risk = {
         firm = {
              realized_loss_limit = 5000,
              unrealized_loss_limit = 15000,
              gross_notional_limit = 100000,
              net_notional_limit = 50000,
              long_open_exposure_limit = 20000,
              short_open_exposure_limit = 20000,
              gross_open_exposure_limit = 40000
         }
      }
   }
}
