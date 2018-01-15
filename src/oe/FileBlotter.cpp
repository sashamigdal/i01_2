#include <i01_core/Config.hpp>

#include <i01_oe/FileBlotter.hpp>
#include <i01_oe/OrderLogFormat.hpp>
#include <i01_oe/OrderManager.hpp>
#include <i01_oe/OrderSession.hpp>


namespace olf = i01::OE::OrderLogFormat;

namespace i01 { namespace OE {


FileBlotter::FileBlotter(OrderManager& om, const std::string& path)
    : Blotter(om)
    , m_orderlog(path)
{
}


FileBlotter::~FileBlotter()
{
    if (m_started) {
        olf::Message<olf::FileTrailer> m(
                olf::MessageType::END_OF_LOG
              , Timestamp::now()
              , olf::FileTrailer{ .terminator = 0xDEADBEEF });
        m_orderlog.write((const char*)(&m), sizeof(m));
    }
}

void FileBlotter::log_start()
{
    olf::Message<olf::FileHeader> m(
        olf::MessageType::START_OF_LOG
      , Timestamp::now());
    m.body.reset();
    m_orderlog.write((const char*)(&m), sizeof(m));
    m_started = true;
}

void FileBlotter::log_new_order(const Order* order_p)
{
    if (UNLIKELY(order_p == nullptr)) {
        std::cerr << "FileBlotter: log_new_order received nullptr." << std::endl;
        return;
    }
    // TODO: separate NewOrderBody from SENT and LOCAL_REJECT...
    //olf::Message<olf::NewOrderBody> m(
    //        olf::MessageType::NEW_ORDER
    //      , order_p->creation_time()
    //      , olf::NewOrderBody{
    //          .instrument_esi = order_p->instrument() ? order_p->instrument()->esi() : olf::ESI(0),
    //          .market_mic = order_p->market().market(),
    //          .local_account = order_p->local_account(),
    //          .local_id = order_p->localID(),
    //          .orig_price = order_p->original_price(),
    //          .size = order_p->size(),
    //          .side = order_p->side(),
    //          .tif = order_p->tif(),
    //          .type = order_p->type(),
    //          .user_data = order_p->userdata(),
    //          .session_name = order_p->session() ? olf::string_to_session_name(order_p->session()->name().c_str()) : 0
    //        } );
    //m_orderlog.write((const char*)(&m), sizeof(m));
}

void FileBlotter::log_order_sent(const Order* order_p)
{
    if (UNLIKELY(order_p == nullptr)) {
        std::cerr << "FileBlotter: log_order_sent received nullptr." << std::endl;
        return;
    }

    olf::Message<olf::OrderSentBody> m(
            olf::MessageType::ORDER_SENT
          , order_p->sent_time()
          , olf::OrderSentBody{ .order = olf::NewOrderBody{
              .instrument_esi = order_p->instrument() ? order_p->instrument()->esi() : olf::ESI(0),
              .market_mic = order_p->market().market(),
              .oid = olf::OrderIdentifier {
                        .local_account = order_p->local_account(),
                        .local_id = order_p->localID(),
                        .session_name = order_p->session() ? olf::string_to_session_name(order_p->session()->name().c_str()) : olf::SessionName{0}
                    },
              .client_order_id = order_p->client_order_id(),
              .orig_price = order_p->original_price(),
              .size = order_p->size(),
              .side = order_p->side(),
              .tif = order_p->tif(),
              .type = order_p->type(),
              .user_data = order_p->userdata(),

            } } );
    m_orderlog.write((const char*)(&m), sizeof(m));
}

void FileBlotter::log_local_reject(const Order* order_p)
{
    if (UNLIKELY(order_p == nullptr)) {
        std::cerr << "FileBlotter: log_local_reject received nullptr." << std::endl;
        return;
    }

    olf::Message<olf::OrderLocalRejectBody> m(
            olf::MessageType::ORDER_LOCAL_REJECT
          , order_p->creation_time()
          , olf::OrderLocalRejectBody{ .order = olf::NewOrderBody{
              .instrument_esi = order_p->instrument() ? order_p->instrument()->esi() : olf::ESI(0),
              .market_mic = order_p->market().market(),
              .oid = olf::OrderIdentifier{
                        .local_account = order_p->local_account(),
                        .local_id = order_p->localID(),
                        .session_name = order_p->session() ? olf::string_to_session_name(order_p->session()->name().c_str()) : olf::SessionName{0}
                    },
              .client_order_id = order_p->client_order_id(),
              .orig_price = order_p->original_price(),
              .size = order_p->size(),
              .side = order_p->side(),
              .tif = order_p->tif(),
              .type = order_p->type(),
              .user_data = order_p->userdata(),
            } } );
    m_orderlog.write((const char*)(&m), sizeof(m));
}

void FileBlotter::log_pending_cancel(const Order* order_p, Size new_qty)
{
    if (UNLIKELY(order_p == nullptr)) {
        std::cerr << "FileBlotter: log_pending_cancel received nullptr." << std::endl;
        return;
    }
    olf::Message<olf::OrderCxlReqBody> m(
            olf::MessageType::ORDER_CANCEL_REQUEST
          , order_p->last_request_time()
          , olf::OrderCxlReqBody{
                .oid = olf::OrderIdentifier{
                    .local_account = order_p->local_account()
                  , .local_id = order_p->localID()
                  , .session_name = olf::string_to_session_name(order_p->session()->name())
                }
              , .new_qty = new_qty
            });
    m_orderlog.write((const char*)(&m), sizeof(m));
}

void FileBlotter::log_destroy(const Order* order_p)
{
    if (UNLIKELY(order_p == nullptr)) {
        std::cerr << "FileBlotter: log_destroy received nullptr." << std::endl;
        return;
    }
    olf::Message<olf::OrderDestroyBody> m(
            olf::MessageType::ORDER_DESTROY
          , order_p->last_request_time()
          , olf::OrderDestroyBody{
                .oid = olf::OrderIdentifier{
                    .local_account = order_p->local_account()
                  , .local_id = order_p->localID()
                  , .session_name = (order_p->session() ? olf::string_to_session_name(order_p->session()->name()) : olf::SessionName())
                } });
    m_orderlog.write((const char*)(&m), sizeof(m));
}

void FileBlotter::log_acknowledged(const Order* order_p)
{
    if (UNLIKELY(order_p == nullptr)) {
        std::cerr << "FileBlotter: ack received nullptr." << std::endl;
        return;
    }
    olf::Message<olf::OrderAckBody> m(
            olf::MessageType::ORDER_ACKNOWLEDGEMENT
          , order_p->last_response_time()
          , olf::OrderAckBody{
                .oid = olf::OrderIdentifier{
                    .local_account = order_p->local_account()
                  , .local_id = order_p->localID()
                  , .session_name = olf::string_to_session_name(order_p->session()->name())
                }
          , .open_size = order_p->open_size()
          , .exchange_ts = order_p->last_exchange_time()});
    m_orderlog.write((const char*)(&m), sizeof(m));
}

void FileBlotter::log_rejected(const Order* order_p)
{
    if (UNLIKELY(order_p == nullptr)) {
        std::cerr << "FileBlotter: reject received nullptr." << std::endl;
        return;
    }
    olf::Message<olf::OrderRejectBody> m(
            olf::MessageType::ORDER_REMOTE_REJECT
          , order_p->last_response_time()
          , olf::OrderRejectBody{
                .oid = olf::OrderIdentifier{
                    .local_account = order_p->local_account()
                  , .local_id = order_p->localID()
                  , .session_name = olf::string_to_session_name(order_p->session()->name())
                }
          , .exchange_ts = order_p->last_exchange_time() });
    m_orderlog.write((const char*)(&m), sizeof(m));
}

void FileBlotter::log_filled(
        const Order* order_p
      , const Size fillSize
      , const Price fillPrice
      , const Timestamp& ts
      , const FillFeeCode fill_fee_code)
{
    if (UNLIKELY(order_p == nullptr)) {
        std::cerr << "FileBlotter: fill received nullptr." << std::endl;
        return;
    }
    olf::Message<olf::OrderFillBody> m(
            order_p->state() == OrderState::FILLED ? olf::MessageType::ORDER_FILL : olf::MessageType::ORDER_PARTIAL_FILL
          , order_p->last_response_time()
          , olf::OrderFillBody{
                .oid = olf::OrderIdentifier{
                    .local_account = order_p->local_account()
                  , .local_id = order_p->localID()
                  , .session_name = olf::string_to_session_name(order_p->session()->name())
                  }
              , .filled_qty = fillSize
              , .filled_price = fillPrice
              , .filled_fee_code = fill_fee_code
              , .exchange_ts = order_p->last_exchange_time()
            });
    m_orderlog.write((const char*)(&m), sizeof(m));
}

void FileBlotter::log_cancel_rejected(const Order* order_p)
{
    if (UNLIKELY(order_p == nullptr)) {
        std::cerr << "FileBlotter: cxlrej received nullptr." << std::endl;
        return;
    }
    olf::Message<olf::OrderCxlRejectBody> m(
            olf::MessageType::ORDER_CANCEL_REJECT
          , order_p->last_response_time()
          , olf::OrderCxlRejectBody{
                .oid = olf::OrderIdentifier{
                    .local_account = order_p->local_account()
                  , .local_id = order_p->localID()
                  , .session_name = olf::string_to_session_name(order_p->session()->name())
                  }
          , .exchange_ts = order_p->last_exchange_time()
            });
    m_orderlog.write((const char*)(&m), sizeof(m));
}

void FileBlotter::log_cancelled(const Order* order_p, Size cxled_qty)
{
    if (UNLIKELY(order_p == nullptr)) {
        std::cerr << "FileBlotter: cxl received nullptr." << std::endl;
        return;
    }
    olf::Message<olf::OrderCxlBody> m(
            olf::MessageType::ORDER_CANCEL
          , order_p->last_response_time()
          , olf::OrderCxlBody{
                .oid = olf::OrderIdentifier{
                    .local_account = order_p->local_account()
                  , .local_id = order_p->localID()
                  , .session_name = olf::string_to_session_name(order_p->session()->name())
                  }
              , .cxled_qty = cxled_qty
              , .exchange_ts = order_p->last_exchange_time()
            });
    m_orderlog.write((const char*)(&m), sizeof(m));
}

void FileBlotter::log_add_session(const OrderSession* osp)
{
    if (UNLIKELY(osp == nullptr)) {
        std::cerr << "FileBlotter: add_session received nullptr." << std::endl;
        return;
    }
    olf::Message<olf::NewSessionBody> m(
            olf::MessageType::NEW_SESSION
          , Timestamp::now()
          , olf::NewSessionBody{
                .name = olf::string_to_session_name(osp->name())
              , .market_mic = osp->market().market()
            });
    m_orderlog.write((const char*)(&m), sizeof(m));
}

void FileBlotter::log_position(const EngineID src, const Instrument* ins, Quantity qty, Price avgpx, Price markpx)
{
    if (UNLIKELY(ins == nullptr)) {
        std::cerr << "FileBlotter: position received nullptr." << std::endl;
        return;
    }
    olf::Message<olf::PositionBody> m(
            olf::MessageType::POSITION
          , Timestamp::now()
          , olf::PositionBody{
                .source = src
              , .instrument = olf::NewInstrumentBody{
                    .esi = ins->esi()
                  , .listing_market = ins->listing_market().market()
                }
              , .qty = qty
              , .avg_price = avgpx
              , .mark_price = markpx
            });
    ::strncpy((char*)&m.body.instrument.symbol[0], ins->symbol().c_str(), sizeof(m.body.instrument.symbol));
    // TODO: add Bloomberg BBGID/FIGI support to instrument.
    ::memset(&m.body.instrument.bloomberg_gid_figi[0], 0, sizeof(m.body.instrument.bloomberg_gid_figi));
    m_orderlog.write((const char*)(&m), sizeof(m));

}
    
void FileBlotter::log_add_strategy(const std::string& name, const OE::LocalAccount& la)
{
    if (UNLIKELY(la == 0)) {
        std::cerr << "FileBlotter: invalid LocalAccount for strategy " << name << std::endl;
        return;
    }
    olf::Message<olf::NewAccountBody> m(
            olf::MessageType::NEW_ACCOUNT
          , Timestamp::now()
          , olf::NewAccountBody{
                .la = la
          });
    ::memset(&m.body.name[0], 0, sizeof(m.body.name));
    ::strncpy((char*)&m.body.name[0], name.c_str(), sizeof(m.body.name));
    m_orderlog.write((const char*)(&m), sizeof(m));
}

} }
