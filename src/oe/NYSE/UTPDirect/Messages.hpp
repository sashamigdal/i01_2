#pragma once

#include <cstdint>

#include <i01_core/macro.hpp>

#include "Types.hpp"

namespace i01 { namespace OE { namespace NYSE { namespace UTPDirect { namespace Messages {
    using namespace i01::OE::NYSE::UTPDirect::Types;

    /* SESSION MESSAGES */

    struct Logon {
        MessageType                       message_type;
        MsgLength                         msg_length;
        MsgSeqNum                         msg_seqnum;
        MsgSeqNum                         last_seqnum;
        LoginID                           sender_comp_id;
        MessageVersionProfile             message_version_profile;
        CancelOnDisconnect                cancel_on_disconnect;
        std::uint8_t                      filler[3];
    } __attribute__((packed));
    I01_ASSERT_SIZE(Logon, 60);

    typedef Logon LogonAccepted;

    struct LogonReject {
        MessageType                       message_type;
        MsgLength                         msg_length;
        MsgSeqNum                         msg_seqnum;
        MsgSeqNum                         last_seqnum_client_to_gw;
        MsgSeqNum                         last_seqnum_gw_to_client;
        LogonRejectType                   logon_reject_type;
        std::uint8_t                      text[40];
        std::uint8_t                      filler[2];
    } __attribute__((packed));
    I01_ASSERT_SIZE(LogonReject, 60);

    struct TestRequest {
        MessageType                       message_type;
        MsgLength                         msg_length;
        MsgSeqNum                         msg_seqnum;
    } __attribute__((packed));
    I01_ASSERT_SIZE(TestRequest, 8);

    struct HeartbeatMessage {
        MessageType                       message_type;
        MsgLength                         msg_length;
        MsgSeqNum                         msg_seqnum;
    } __attribute__((packed));
    I01_ASSERT_SIZE(HeartbeatMessage, 8);


    struct NewOrder1 {
        MessageType                       message_type;
        MsgLength                         msg_length;
        MsgSeqNum                         msg_seqnum;
        Quantity                          order_qty;
        Quantity                          max_floor_qty;
        Price                             price;
        PriceScale                        price_scale;
        Symbol                            symbol;
        ExecInst                          exec_inst;
        Side                              side;
        OrderType                         order_type;
        TimeInForce                       time_in_force;
        Capacity                          capacity;
        RoutingInstruction                routing_instruction;
        DOTReserve                        dot_reserve;
        CompID                            on_behalf_of_compid;
        SubID                             sender_sub_id; /* userdata... */
        ClearingFirm                      clearing_firm;
        Account                           account;
        ClientOrderID                     client_order_id;
        std::uint8_t                      filler[3];
    } __attribute__((packed));
    I01_ASSERT_SIZE(NewOrder1, 84);

    // TODO: NewOrder D2 variant 2 and beyond...

} } } } }

