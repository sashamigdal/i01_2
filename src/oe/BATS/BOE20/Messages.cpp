#include <iomanip>

#include <i01_core/util.hpp>

#include "Messages.hpp"

namespace i01 { namespace OE { namespace BATS { namespace BOE20 { namespace Messages {

using i01::core::operator<<;

// template<typename FieldType>
// void output_helper(std::ostream &os, const char * prefix, const std::uint8_t *& p)
// {
//     os << prefix << *reinterpret_cast<const FieldType *>(p);
//     p += sizeof(FieldType);
// }

template<typename BitType, typename FieldType>
void print_if_present(std::ostream &os, const std::uint8_t *bf, const char *prefix, const BitType &bit, const FieldType &f)
{

    if (*bf & (std::uint8_t) bit) {
        os << prefix << f;
    }
}


MessageHeader MessageHeader::create(MessageType mt, SequenceNumber sn)
{
    MessageHeader mh;
    mh.start_of_message = 0xBABA;
    mh.message_length = sizeof(MessageHeader)-2;
    mh.message_type = mt;
    mh.matching_unit = 0;
    mh.sequence_number = sn;
    return mh;
}

std::ostream & operator<<(std::ostream &os, const MessageHeader &m)
{
    return os << "0x" << std::hex << (int) m.start_of_message << std::dec << ","
              << m.message_length << ","
              << "0x" << std::hex << (int) m.message_type << std::dec << ","
              << (int) m.matching_unit << ","
              << m.sequence_number;
}


std::ostream & operator<<(std::ostream &os, const UnitSequenceNumberSection &m)
{
    os << (int) m.number_of_units;
    for (auto i = 0U; i < m.number_of_units; i++) {
        os << "," << m.units[i];
    }
    return os;
}

std::ostream & operator<<(std::ostream &os, const UnitNumberSequencePair &m)
{
    return os << (int) m.unit_number << ","
              << m.unit_sequence;
}

std::ostream & operator<<(std::ostream &os, const ParamGroupHeader &m)
{
    return os << m.length << ","
              << std::hex << (int) m.type << std::dec;
}

std::ostream & operator<<(std::ostream &os, const UnitSequencesParamGroup &m)
{
    return os << "USPG," << m.header << ","
              << (char) m.no_unspecified_unit_replay << ","
              << m.unit_sequence_number;
}

std::ostream & operator<<(std::ostream &os, const ReturnBitfield &m)
{
    os << (int) m.num_bitfields;
    if (m.num_bitfields) {
        for (auto i = 0U; i < m.num_bitfields; i++) {
            os << "," << std::hex << (int) m.bitfields.u8[i] << std::dec;
        }
    }
    return os;
}

std::ostream & operator<<(std::ostream &os, const ReturnBitfieldsParamGroup &m)
{
    return os << "RBFPG," << m.header << ","
              << std::hex << "0x" << (int) m.message_type << std::dec << ","
              << m.bitfields;
}

std::ostream & operator<<(std::ostream &os, const VariableLengthBitfield &m)
{
    os << (int) m.num_bitfields;
    if (m.num_bitfields) {
        for (auto i = 0U; i < m.num_bitfields; i++) {
            os << ",0x" << std::hex << (int) m.bitfields[i] << std::dec;
        }
    }
    return os;
}

std::ostream & operator<<(std::ostream &os, const LoginRequest &m)
{
    os << m.header << ","
       << m.session_sub_id.arr << ","
       << m.username.arr << ","
       << m.password.arr<< ","
       << (int) m.num_param_groups;

    if (m.num_param_groups) {
        const std::uint8_t *pgbuf = &m.param_groups[0];
        auto i = 0U;
        while (i < m.num_param_groups) {
            const auto * pg = reinterpret_cast<const ParamGroup *>(pgbuf);
            os << "," << *pg;
            pgbuf += pg->header.length;
            i++;
        }
    }
    return os;
}

std::ostream & operator<<(std::ostream &os, const ParamGroup &m)
{
    switch (m.header.type) {
    case ParamGroupType::UNIT_SEQUENCES:
        {
            const auto *us = reinterpret_cast<const UnitSequencesParamGroup *>(&m);
            os << *us;
        }
        break;
    case ParamGroupType::RETURN_BITFIELDS:
        {
            const auto * rb = reinterpret_cast<const ReturnBitfieldsParamGroup *>(&m);
            os << *rb;
        }
        break;
    default:
        break;
    }
    return os;
}

std::ostream & operator<<(std::ostream &os, const LoginResponse &m)
{
    os << m.message_header << ","
       << (char) m.login_response_status << ","
       << m.login_response_text.arr << ","
       << m.no_unspecified_unit_replay << ","
       << m.last_received_sequence_number << ","
       << m.unit_sequence_number << ",";

    // now get the return param groups
    const auto *eom = (const std::uint8_t *)&m.unit_sequence_number.units[0] + m.unit_sequence_number.number_of_units*sizeof(UnitNumberSequencePair);

    const auto * npg = reinterpret_cast<const NumParamGroups *>(eom);
    os << (int) *npg;

    if (*npg) {
        // we have param groups...
        const std::uint8_t * pgbuf = eom + sizeof(NumParamGroups);
        auto i = 0U;
        while (i < *npg) {
            const auto * pg = reinterpret_cast<const ParamGroup *>(pgbuf);
            os << "," << *pg;
            pgbuf += pg->header.length;
            i++;
        }
    }
    return os;
}

std::ostream & operator<<(std::ostream &os, const ServerHeartbeat &m)
{
    return os << m.message_header;
}

std::ostream & operator<<(std::ostream &os, const ReplayComplete &m)
{
    return os << m.message_header;
}

std::ostream & operator<<(std::ostream &os, const Logout &m)
{
    return os << m.message_header << ","
              << (char) m.logout_reason << ","
              << m.logout_reason_text.arr << ","
              << m.last_received_sequence_number << ","
              << m.unit_sequence_number;
}

LogoutRequest LogoutRequest::create()
{
    LogoutRequest l;
    l.message_header = MessageHeader::create(MessageType::LOGOUT_REQUEST);
    return l;
}

std::ostream & operator<<(std::ostream &os, const LogoutRequest &m)
{
    return os << m.message_header;
}

ClientHeartbeat ClientHeartbeat::create()
{
    ClientHeartbeat hb;
    hb.message_header = MessageHeader::create(MessageType::CLIENT_HEARTBEAT);
    return hb;
}
std::ostream & operator<<(std::ostream &os, const ClientHeartbeat &m)
{
    return os << m.message_header;
}

std::ostream & operator<<(std::ostream &os, const NewOrder &m)
{
    os << m.message_header << ","
       << "CLORDID," << m.cl_ord_id << ","
       << m.side << ","
       << m.order_qty << ","
       << (int) m.num_bitfields;

    const auto * vbf = reinterpret_cast<const VariableLengthBitfield *>(&m.num_bitfields);
    os << ",BF," << *vbf;

    if (vbf->num_bitfields) {
        const std::uint8_t * p = (const std::uint8_t *) &vbf->bitfields[0] + vbf->num_bitfields;
        const std::uint8_t * bf = (const std::uint8_t *) &vbf->bitfields[0];
        const auto * opt = reinterpret_cast<const NewOrder::OptionalFields *>(p);

        if (vbf->num_bitfields >= 1) {
            print_if_present(os, bf, ",CLFIRM,", NewOrderBitfield1::CLEARING_FIRM, opt->clearing_firm);
            print_if_present(os, bf, ",CLACCT,", NewOrderBitfield1::CLEARING_ACCOUNT, opt->clearing_account);
            print_if_present(os, bf, ",PRICE,", NewOrderBitfield1::PRICE, opt->price);
            print_if_present(os, bf, ",EXECINST,", NewOrderBitfield1::EXEC_INST, (char) opt->exec_inst);
            print_if_present(os, bf, ",ORDTYPE,", NewOrderBitfield1::ORD_TYPE, opt->ord_type);
            print_if_present(os, bf, ",TIF,", NewOrderBitfield1::TIME_IN_FORCE, opt->time_in_force);
            print_if_present(os, bf, ",MINQTY,", NewOrderBitfield1::MIN_QTY, opt->min_qty);
            print_if_present(os, bf, ",MAXFLR,", NewOrderBitfield1::MAX_FLOOR, opt->max_floor);

        }
        if (vbf->num_bitfields >= 2) {
            bf++;
            print_if_present(os, bf, ",SYMBOL,", NewOrderBitfield2::SYMBOL, opt->symbol);
            print_if_present(os, bf, ",CAPACITY,", NewOrderBitfield2::CAPACITY, opt->capacity);
            print_if_present(os, bf, ",ROUTINST,", NewOrderBitfield2::ROUTING_INST, opt->routing_inst);

        }
        if (vbf->num_bitfields >= 3) {
            print_if_present(os, bf, ",ACCT,", NewOrderBitfield3::ACCOUNT, opt->account);
            print_if_present(os, bf, ",DISPIND,", NewOrderBitfield3::DISPLAY_INDICATOR, opt->display_indicator);
            print_if_present(os, bf, ",MAXRMPCT,", NewOrderBitfield3::MAX_REMOVE_PCT, (int) opt->max_remove_pct);
            print_if_present(os, bf, ",DISCAMT,", NewOrderBitfield3::DISCRETION_AMOUNT, opt->discretion_amount);
            print_if_present(os, bf, ",PEGDIFF,", NewOrderBitfield3::PEG_DIFFERENCE, opt->peg_difference);

            bf++;
        }
        if (vbf->num_bitfields >= 4) {
            bf++;
        }
        if (vbf->num_bitfields >= 5) {
            bf++;
            print_if_present(os, bf, ",ATTRQT,", NewOrderBitfield5::ATTRIBUTED_QUOTE, opt->attributed_quote);
            print_if_present(os, bf, ",EXTEXECINST,", NewOrderBitfield5::EXT_EXEC_INST, opt->ext_exec_inst);

        }
        if (vbf->num_bitfields >= 6) {
            bf++;
        }
    }
    return os;
}

std::ostream & operator<<(std::ostream &os, const CancelOrder &m)
{
    os << m.message_header << ","
       << m.orig_cl_ord_id << ","
       << (int) m.num_bitfields;

    const auto * vbf = reinterpret_cast<const VariableLengthBitfield *>(&m.num_bitfields);
    os << ",BF," << *vbf;

    if (vbf->num_bitfields) {
        const std::uint8_t * p = (const std::uint8_t *) &vbf->bitfields[0] + vbf->num_bitfields;
        const std::uint8_t * bf = (const std::uint8_t *) &vbf->bitfields[0];
        const auto * opt = reinterpret_cast<const CancelOrder::OptionalFields *>(p);

        if (vbf->num_bitfields >= 1) {
            print_if_present(os, bf, ",CLFIRM,", CancelOrderBitfield1::CLEARING_FIRM, opt->clearing_firm);
        }
    }
    return os;

}

std::ostream & operator<<(std::ostream &os, const ModifyOrder &m)
{
    os << m.message_header << ","
       << m.new_cl_ord_id << ","
       << m.orig_cl_ord_id << ","
       << (int) m.num_bitfields;

    const auto * vbf = reinterpret_cast<const VariableLengthBitfield *>(&m.num_bitfields);
    os << ",BF," << *vbf;

    if (vbf->num_bitfields) {
        const std::uint8_t * p = (const std::uint8_t *) &vbf->bitfields[0] + vbf->num_bitfields;
        const std::uint8_t * bf = (const std::uint8_t *) &vbf->bitfields[0];
        const auto * opt = reinterpret_cast<const ModifyOrder::OptionalFields *>(p);

        if (vbf->num_bitfields >= 1) {
            print_if_present(os, bf, ",CLFIRM,", ModifyOrderBitfield1::CLEARING_FIRM, opt->clearing_firm);
            print_if_present(os, bf, ",ORDQTY,", ModifyOrderBitfield1::ORDER_QTY, opt->order_qty);
            print_if_present(os, bf, ",PRICE,", ModifyOrderBitfield1::PRICE, opt->price);
            print_if_present(os, bf, ",ORDTYPE,", ModifyOrderBitfield1::ORD_TYPE, opt->ord_type);
            print_if_present(os, bf, ",CXLREJ,", ModifyOrderBitfield1::CANCEL_ORIG_ON_REJECT, opt->cancel_orig_on_reject);
            print_if_present(os, bf, ",EXECINST,", ModifyOrderBitfield1::EXEC_INST, opt->exec_inst);
            print_if_present(os, bf, ",SIDE,", ModifyOrderBitfield1::SIDE, opt->side);
        }
        if (vbf->num_bitfields >= 2) {
            bf++;
            print_if_present(os, bf, ",MAXFLR,", ModifyOrderBitfield2::MAX_FLOOR, opt->max_floor);
        }

    }
    return os;

}


std::ostream & operator<<(std::ostream &os, const OrderAcknowledgment &m)
{
    os << m.message_header << ","
       << m.transaction_time << ","
       << m.cl_ord_id << ","
       << m.order_id << ","
       << (int) m.number_of_return_bitfields;

    const auto * vbf = reinterpret_cast<const VariableLengthBitfield *>(&m.number_of_return_bitfields);
    os << ",BF," << *vbf;

    if (vbf->num_bitfields) {
        const std::uint8_t * p = (const std::uint8_t *) &vbf->bitfields[0] + vbf->num_bitfields;
        const std::uint8_t * bf = (const std::uint8_t *) &vbf->bitfields[0];
        const auto * opt = reinterpret_cast<const OrderAcknowledgment::OptionalFields *>(p);

        if (vbf->num_bitfields >= 1) {
            print_if_present(os, bf, ",SIDE,", OrderAcknowledgmentBitfield1::SIDE, opt->side);
            print_if_present(os, bf, ",PEGDIFF,", OrderAcknowledgmentBitfield1::PEG_DIFFERENCE, opt->peg_difference);
            print_if_present(os, bf, ",PRICE,", OrderAcknowledgmentBitfield1::PRICE, opt->price);
            print_if_present(os, bf, ",EXECINST,", OrderAcknowledgmentBitfield1::EXEC_INST, (char) opt->exec_inst);
            print_if_present(os, bf, ",ORDTYPE,", OrderAcknowledgmentBitfield1::ORD_TYPE, opt->ord_type);
            print_if_present(os, bf, ",TIF,", OrderAcknowledgmentBitfield1::TIME_IN_FORCE, opt->time_in_force);
            print_if_present(os, bf, ",MINQTY,", OrderAcknowledgmentBitfield1::MIN_QTY, opt->min_qty);
            print_if_present(os, bf, ",MAXRMPCT,", OrderAcknowledgmentBitfield1::MAX_REMOVE_PCT, opt->max_remove_pct);
        }
        if (vbf->num_bitfields >= 2) {
            bf++;
            print_if_present(os, bf, ",SYMBOL,", OrderAcknowledgmentBitfield2::SYMBOL, opt->symbol);
            print_if_present(os, bf, ",CAPACITY,", OrderAcknowledgmentBitfield2::CAPACITY, opt->capacity);
        }
        if (vbf->num_bitfields >= 3) {
            print_if_present(os, bf, ",ACCT,", OrderAcknowledgmentBitfield3::ACCOUNT, opt->account);
            print_if_present(os, bf, ",CLFIRM,", OrderAcknowledgmentBitfield3::CLEARING_FIRM, opt->clearing_firm);
            print_if_present(os, bf, ",CLACCT,", OrderAcknowledgmentBitfield3::CLEARING_ACCOUNT, opt->clearing_account);
            print_if_present(os, bf, ",DISPIND,", OrderAcknowledgmentBitfield3::DISPLAY_INDICATOR, opt->display_indicator);
            print_if_present(os, bf, ",MAXFLR,", OrderAcknowledgmentBitfield3::MAX_FLOOR, opt->max_floor);
            print_if_present(os, bf, ",DISCAMT,", OrderAcknowledgmentBitfield3::DISCRETION_AMOUNT, opt->discretion_amount);
            print_if_present(os, bf, ",ORDERQTY,", OrderAcknowledgmentBitfield3::ORDER_QTY, opt->order_qty);

            bf++;
        }
        if (vbf->num_bitfields >= 4) {
            bf++;
        }
        if (vbf->num_bitfields >= 5) {
            bf++;
            print_if_present(os, bf, ",LEAVES,", OrderAcknowledgmentBitfield5::LEAVES_QTY, opt->leaves_qty);
            print_if_present(os, bf, ",DISPPX,", OrderAcknowledgmentBitfield5::DISPLAY_PRICE, opt->display_price);
            print_if_present(os, bf, ",WORKPX,", OrderAcknowledgmentBitfield5::WORKING_PRICE, opt->working_price);
        }
        if (vbf->num_bitfields >= 6) {
            bf++;
            print_if_present(os, bf, ",ATTRQT,", OrderAcknowledgmentBitfield6::ATTRIBUTED_QUOTE, opt->attributed_quote);
            print_if_present(os, bf, ",EXTEXECINST,", OrderAcknowledgmentBitfield6::EXT_EXEC_INST, opt->ext_exec_inst);
        }
        if (vbf->num_bitfields >= 7) {
            bf++;
        }
        if (vbf->num_bitfields >= 8) {
            bf++;
            print_if_present(os, bf, ",ROUTINST,", OrderAcknowledgmentBitfield8::ROUTING_INST, opt->routing_inst);
        }
    }
    return os;
}

std::ostream & operator<<(std::ostream &os, const OrderRejected &m)
{
    os << m.message_header << ","
       << m.transaction_time << ","
       << m.cl_ord_id << ","
       << m.order_reject_reason << ","
       << m.text.arr << ","
       << (int) m.number_of_return_bitfields;

    const auto * vbf = reinterpret_cast<const VariableLengthBitfield *>(&m.number_of_return_bitfields);
    os << ",BF," << *vbf;

    if (vbf->num_bitfields) {
        const std::uint8_t * p = (const std::uint8_t *) &vbf->bitfields[0] + vbf->num_bitfields;
        const std::uint8_t * bf = (const std::uint8_t *) &vbf->bitfields[0];
        const auto * opt = reinterpret_cast<const OrderRejected::OptionalFields *>(p);

        if (vbf->num_bitfields >= 1) {
            print_if_present(os, bf, ",SIDE,", OrderRejectedBitfield1::SIDE, opt->side);
            print_if_present(os, bf, ",PEGDIFF,", OrderRejectedBitfield1::PEG_DIFFERENCE, opt->peg_difference);
            print_if_present(os, bf, ",PRICE,", OrderRejectedBitfield1::PRICE, opt->price);
            print_if_present(os, bf, ",EXECINST,", OrderRejectedBitfield1::EXEC_INST, (char) opt->exec_inst);
            print_if_present(os, bf, ",ORDTYPE,", OrderRejectedBitfield1::ORD_TYPE, opt->ord_type);
            print_if_present(os, bf, ",TIF,", OrderRejectedBitfield1::TIME_IN_FORCE, opt->time_in_force);
            print_if_present(os, bf, ",MINQTY,", OrderRejectedBitfield1::MIN_QTY, opt->min_qty);
            print_if_present(os, bf, ",MAXRMPCT,", OrderRejectedBitfield1::MAX_REMOVE_PCT, (int) opt->max_remove_pct);
        }
        if (vbf->num_bitfields >= 2) {
            bf++;
            print_if_present(os, bf, ",SYMBOL,", OrderRejectedBitfield2::SYMBOL, opt->symbol);
            print_if_present(os, bf, ",CAPACITY,", OrderRejectedBitfield2::CAPACITY, opt->capacity);
        }
        if (vbf->num_bitfields >= 3) {
            print_if_present(os, bf, ",ACCT,", OrderRejectedBitfield3::ACCOUNT, opt->account);
            print_if_present(os, bf, ",CLFIRM,", OrderRejectedBitfield3::CLEARING_FIRM, opt->clearing_firm);
            print_if_present(os, bf, ",CLACCT,", OrderRejectedBitfield3::CLEARING_ACCOUNT, opt->clearing_account);
            print_if_present(os, bf, ",DISPIND,", OrderRejectedBitfield3::DISPLAY_INDICATOR, opt->display_indicator);
            print_if_present(os, bf, ",MAXFLR,", OrderRejectedBitfield3::MAX_FLOOR, opt->max_floor);
            print_if_present(os, bf, ",DISCAMT,", OrderRejectedBitfield3::DISCRETION_AMOUNT, opt->discretion_amount);
            print_if_present(os, bf, ",ORDERQTY,", OrderRejectedBitfield3::ORDER_QTY, opt->order_qty);

            bf++;
        }
        if (vbf->num_bitfields >= 4) {
            bf++;
        }
        if (vbf->num_bitfields >= 5) {
            bf++;
        }
        if (vbf->num_bitfields >= 6) {
            bf++;
            print_if_present(os, bf, ",ATTRQT,", OrderRejectedBitfield6::ATTRIBUTED_QUOTE, opt->attributed_quote);
            print_if_present(os, bf, ",EXTEXECINST,", OrderRejectedBitfield6::EXT_EXEC_INST, opt->ext_exec_inst);
        }
        if (vbf->num_bitfields >= 7) {
            bf++;
        }
        if (vbf->num_bitfields >= 8) {
            bf++;
            print_if_present(os, bf, ",ROUTINST,", OrderRejectedBitfield8::ROUTING_INST, opt->routing_inst);
        }
    }
    return os;

}

std::ostream & operator<<(std::ostream &os, const OrderModified &m)
{
    os << m.message_header << ","
       << m.transaction_time << ","
       << m.cl_ord_id << ","
       << m.order_id << ","
       << (int) m.number_of_return_bitfields;

    const auto * vbf = reinterpret_cast<const VariableLengthBitfield *>(&m.number_of_return_bitfields);
    os << ",BF," << *vbf;

    if (vbf->num_bitfields) {
        const std::uint8_t * p = (const std::uint8_t *) &vbf->bitfields[0] + vbf->num_bitfields;
        const std::uint8_t * bf = (const std::uint8_t *) &vbf->bitfields[0];
        const auto * opt = reinterpret_cast<const OrderModified::OptionalFields *>(p);

        if (vbf->num_bitfields >= 1) {
            print_if_present(os, bf, ",SIDE,", OrderModifiedBitfield1::SIDE, opt->side);
            print_if_present(os, bf, ",PEGDIFF,", OrderModifiedBitfield1::PEG_DIFFERENCE, opt->peg_difference);
            print_if_present(os, bf, ",PRICE,", OrderModifiedBitfield1::PRICE, opt->price);
            print_if_present(os, bf, ",EXECINST,", OrderModifiedBitfield1::EXEC_INST, (char) opt->exec_inst);
            print_if_present(os, bf, ",ORDTYPE,", OrderModifiedBitfield1::ORD_TYPE, opt->ord_type);
            print_if_present(os, bf, ",TIF,", OrderModifiedBitfield1::TIME_IN_FORCE, opt->time_in_force);
            print_if_present(os, bf, ",MINQTY,", OrderModifiedBitfield1::MIN_QTY, opt->min_qty);
            print_if_present(os, bf, ",MAXRMPCT,", OrderModifiedBitfield1::MAX_REMOVE_PCT, (int) opt->max_remove_pct);
        }
        if (vbf->num_bitfields >= 2) {
            bf++;
        }
        if (vbf->num_bitfields >= 3) {
            print_if_present(os, bf, ",ACCT,", OrderModifiedBitfield3::ACCOUNT, opt->account);
            print_if_present(os, bf, ",CLFIRM,", OrderModifiedBitfield3::CLEARING_FIRM, opt->clearing_firm);
            print_if_present(os, bf, ",CLACCT,", OrderModifiedBitfield3::CLEARING_ACCOUNT, opt->clearing_account);
            print_if_present(os, bf, ",DISPIND,", OrderModifiedBitfield3::DISPLAY_INDICATOR, opt->display_indicator);
            print_if_present(os, bf, ",MAXFLR,", OrderModifiedBitfield3::MAX_FLOOR, opt->max_floor);
            print_if_present(os, bf, ",DISCAMT,", OrderModifiedBitfield3::DISCRETION_AMOUNT, opt->discretion_amount);
            print_if_present(os, bf, ",ORDERQTY,", OrderModifiedBitfield3::ORDER_QTY, opt->order_qty);

            bf++;
        }
        if (vbf->num_bitfields >= 4) {
            bf++;
        }
        if (vbf->num_bitfields >= 5) {
            bf++;
            print_if_present(os, bf, ",ORIGID,", OrderModifiedBitfield5::ORIG_CL_ORD_ID, opt->orig_cl_ord_id);
            print_if_present(os, bf, ",LEAVES,", OrderModifiedBitfield5::LEAVES_QTY, opt->leaves_qty);
            print_if_present(os, bf, ",DISPPX,", OrderModifiedBitfield5::DISPLAY_PRICE, opt->display_price);
            print_if_present(os, bf, ",WORKPX,", OrderModifiedBitfield5::WORKING_PRICE, opt->working_price);
        }
        if (vbf->num_bitfields >= 6) {
            bf++;
            print_if_present(os, bf, ",ATTRQT,", OrderModifiedBitfield6::ATTRIBUTED_QUOTE, opt->attributed_quote);
            print_if_present(os, bf, ",EXTEXECINST,", OrderModifiedBitfield6::EXT_EXEC_INST, opt->ext_exec_inst);
        }
        if (vbf->num_bitfields >= 7) {
            bf++;
        }
        if (vbf->num_bitfields >= 8) {
            bf++;
            print_if_present(os, bf, ",ROUTINST,", OrderModifiedBitfield8::ROUTING_INST, opt->routing_inst);
        }
    }
    return os;
}

std::ostream & operator<<(std::ostream &os, const OrderRestated &m)
{
    os << m.message_header << ","
       << m.transaction_time << ","
       << m.cl_ord_id << ","
       << m.order_id << ","
       << m.restatement_reason << ","
       << (int) m.number_of_return_bitfields;

    const auto * vbf = reinterpret_cast<const VariableLengthBitfield *>(&m.number_of_return_bitfields);
    os << ",BF," << *vbf;

    if (vbf->num_bitfields) {
        const std::uint8_t * p = (const std::uint8_t *) &vbf->bitfields[0] + vbf->num_bitfields;
        const std::uint8_t * bf = (const std::uint8_t *) &vbf->bitfields[0];
        const auto * opt = reinterpret_cast<const OrderRestated::OptionalFields *>(p);

        if (vbf->num_bitfields >= 1) {
            print_if_present(os, bf, ",SIDE,", OrderRestatedBitfield1::SIDE, opt->side);
            print_if_present(os, bf, ",PEGDIFF,", OrderRestatedBitfield1::PEG_DIFFERENCE, opt->peg_difference);
            print_if_present(os, bf, ",PRICE,", OrderRestatedBitfield1::PRICE, opt->price);
            print_if_present(os, bf, ",EXECINST,", OrderRestatedBitfield1::EXEC_INST, (char) opt->exec_inst);
            print_if_present(os, bf, ",ORDTYPE,", OrderRestatedBitfield1::ORD_TYPE, opt->ord_type);
            print_if_present(os, bf, ",TIF,", OrderRestatedBitfield1::TIME_IN_FORCE, opt->time_in_force);
            print_if_present(os, bf, ",MINQTY,", OrderRestatedBitfield1::MIN_QTY, opt->min_qty);
            print_if_present(os, bf, ",MAXRMPCT,", OrderRestatedBitfield1::MAX_REMOVE_PCT, (int) opt->max_remove_pct);
        }
        if (vbf->num_bitfields >= 2) {
            bf++;
            print_if_present(os, bf, ",SYMBOL,", OrderRestatedBitfield2::SYMBOL, opt->symbol);
            print_if_present(os, bf, ",CAPACITY,", OrderRestatedBitfield2::CAPACITY, opt->capacity);
        }
        if (vbf->num_bitfields >= 3) {
            print_if_present(os, bf, ",ACCT,", OrderRestatedBitfield3::ACCOUNT, opt->account);
            print_if_present(os, bf, ",CLFIRM,", OrderRestatedBitfield3::CLEARING_FIRM, opt->clearing_firm);
            print_if_present(os, bf, ",CLACCT,", OrderRestatedBitfield3::CLEARING_ACCOUNT, opt->clearing_account);
            print_if_present(os, bf, ",DISPIND,", OrderRestatedBitfield3::DISPLAY_INDICATOR, opt->display_indicator);
            print_if_present(os, bf, ",MAXFLR,", OrderRestatedBitfield3::MAX_FLOOR, opt->max_floor);
            print_if_present(os, bf, ",DISCAMT,", OrderRestatedBitfield3::DISCRETION_AMOUNT, opt->discretion_amount);
            print_if_present(os, bf, ",ORDERQTY,", OrderRestatedBitfield3::ORDER_QTY, opt->order_qty);

            bf++;
        }
        if (vbf->num_bitfields >= 4) {
            bf++;
        }
        if (vbf->num_bitfields >= 5) {
            bf++;
            print_if_present(os, bf, ",ORIGID,", OrderRestatedBitfield5::ORIG_CL_ORD_ID, opt->orig_cl_ord_id);
            print_if_present(os, bf, ",LEAVES,", OrderRestatedBitfield5::LEAVES_QTY, opt->leaves_qty);
            print_if_present(os, bf, ",DISPPX,", OrderRestatedBitfield5::DISPLAY_PRICE, opt->display_price);
            print_if_present(os, bf, ",WORKPX,", OrderRestatedBitfield5::WORKING_PRICE, opt->working_price);
        }
        if (vbf->num_bitfields >= 6) {
            bf++;
            print_if_present(os, bf, ",ATTRQT,", OrderRestatedBitfield6::ATTRIBUTED_QUOTE, opt->attributed_quote);
            print_if_present(os, bf, ",EXTEXECINST,", OrderRestatedBitfield6::EXT_EXEC_INST, opt->ext_exec_inst);
        }
        if (vbf->num_bitfields >= 7) {
            bf++;
        }
        if (vbf->num_bitfields >= 8) {
            bf++;
            print_if_present(os, bf, ",ROUTINST,", OrderRestatedBitfield8::ROUTING_INST, opt->routing_inst);
        }
    }
    return os;
}

std::ostream & operator<<(std::ostream &os, const UserModifyRejected &m)
{
    return os << m.message_header << ","
              << m.transaction_time << ","
              << m.cl_ord_id << ","
              << m.modify_reject_reason << ","
              << m.text.arr << ","
              << m.order_rejected_bitfields;
}

std::ostream & operator<<(std::ostream &os, const OrderCancelled &m)
{
    os << m.message_header << ","
       << m.transaction_time << ","
       << m.cl_ord_id << ","
       << m.cancel_reason << ","
       << (int) m.number_of_return_bitfields;

    const auto * vbf = reinterpret_cast<const VariableLengthBitfield *>(&m.number_of_return_bitfields);
    os << ",BF," << *vbf;

    if (vbf->num_bitfields) {
        const std::uint8_t * p = (const std::uint8_t *) &vbf->bitfields[0] + vbf->num_bitfields;
        const std::uint8_t * bf = (const std::uint8_t *) &vbf->bitfields[0];
        const auto * opt = reinterpret_cast<const OrderCancelled::OptionalFields *>(p);

        if (vbf->num_bitfields >= 1) {
            print_if_present(os, bf, ",SIDE,", OrderCancelledBitfield1::SIDE, opt->side);
            print_if_present(os, bf, ",PEGDIFF,", OrderCancelledBitfield1::PEG_DIFFERENCE, opt->peg_difference);
            print_if_present(os, bf, ",PRICE,", OrderCancelledBitfield1::PRICE, opt->price);
            print_if_present(os, bf, ",EXECINST,", OrderCancelledBitfield1::EXEC_INST, (char) opt->exec_inst);
            print_if_present(os, bf, ",ORDTYPE,", OrderCancelledBitfield1::ORD_TYPE, opt->ord_type);
            print_if_present(os, bf, ",TIF,", OrderCancelledBitfield1::TIME_IN_FORCE, opt->time_in_force);
            print_if_present(os, bf, ",MINQTY,", OrderCancelledBitfield1::MIN_QTY, opt->min_qty);
            print_if_present(os, bf, ",MAXRMPCT,", OrderCancelledBitfield1::MAX_REMOVE_PCT, (int) opt->max_remove_pct);
        }
        if (vbf->num_bitfields >= 2) {
            bf++;
            print_if_present(os, bf, ",SYMBOL,", OrderCancelledBitfield2::SYMBOL, opt->symbol);
            print_if_present(os, bf, ",CAPACITY,", OrderCancelledBitfield2::CAPACITY, opt->capacity);
        }
        if (vbf->num_bitfields >= 3) {
            print_if_present(os, bf, ",ACCT,", OrderCancelledBitfield3::ACCOUNT, opt->account);
            print_if_present(os, bf, ",CLFIRM,", OrderCancelledBitfield3::CLEARING_FIRM, opt->clearing_firm);
            print_if_present(os, bf, ",CLACCT,", OrderCancelledBitfield3::CLEARING_ACCOUNT, opt->clearing_account);
            print_if_present(os, bf, ",DISPIND,", OrderCancelledBitfield3::DISPLAY_INDICATOR, opt->display_indicator);
            print_if_present(os, bf, ",MAXFLR,", OrderCancelledBitfield3::MAX_FLOOR, opt->max_floor);
            print_if_present(os, bf, ",DISCAMT,", OrderCancelledBitfield3::DISCRETION_AMOUNT, opt->discretion_amount);
            print_if_present(os, bf, ",ORDERQTY,", OrderCancelledBitfield3::ORDER_QTY, opt->order_qty);

            bf++;
        }
        if (vbf->num_bitfields >= 4) {
            bf++;
        }
        if (vbf->num_bitfields >= 5) {
            bf++;
            print_if_present(os, bf, ",LEAVES,", OrderCancelledBitfield5::LEAVES_QTY, opt->leaves_qty);
        }
        if (vbf->num_bitfields >= 6) {
            bf++;
            print_if_present(os, bf, ",ATTRQT,", OrderCancelledBitfield6::ATTRIBUTED_QUOTE, opt->attributed_quote);
            print_if_present(os, bf, ",EXTEXECINST,", OrderCancelledBitfield6::EXT_EXEC_INST, opt->ext_exec_inst);
        }
        if (vbf->num_bitfields >= 7) {
            bf++;
        }
        if (vbf->num_bitfields >= 8) {
            bf++;
            print_if_present(os, bf, ",ROUTINST,", OrderCancelledBitfield8::ROUTING_INST, opt->routing_inst);
        }
    }
    return os;
}

std::ostream & operator<<(std::ostream &os, const CancelRejected &m)
{
    os << m.message_header << ","
       << m.transaction_time << ","
       << m.cl_ord_id << ","
       << m.cancel_reject_reason << ","
       << m.text.arr << ","
       << m.number_of_return_bitfields;
    const auto * vbf = reinterpret_cast<const VariableLengthBitfield *>(&m.number_of_return_bitfields);
    os << ",BF," << *vbf;

    if (vbf->num_bitfields) {
        const std::uint8_t * p = (const std::uint8_t *) &vbf->bitfields[0] + vbf->num_bitfields;
        const std::uint8_t * bf = (const std::uint8_t *) &vbf->bitfields[0];
        const auto * opt = reinterpret_cast<const CancelRejected::OptionalFields *>(p);

        if (vbf->num_bitfields >= 1) {
            print_if_present(os, bf, ",SIDE,", CancelRejectedBitfield1::SIDE, opt->side);
            print_if_present(os, bf, ",PEGDIFF,", CancelRejectedBitfield1::PEG_DIFFERENCE, opt->peg_difference);
            print_if_present(os, bf, ",PRICE,", CancelRejectedBitfield1::PRICE, opt->price);
            print_if_present(os, bf, ",EXECINST,", CancelRejectedBitfield1::EXEC_INST, (char) opt->exec_inst);
            print_if_present(os, bf, ",ORDTYPE,", CancelRejectedBitfield1::ORD_TYPE, opt->ord_type);
            print_if_present(os, bf, ",TIF,", CancelRejectedBitfield1::TIME_IN_FORCE, opt->time_in_force);
            print_if_present(os, bf, ",MINQTY,", CancelRejectedBitfield1::MIN_QTY, opt->min_qty);
            print_if_present(os, bf, ",MAXRMPCT,", CancelRejectedBitfield1::MAX_REMOVE_PCT, (int) opt->max_remove_pct);
        }
        if (vbf->num_bitfields >= 2) {
            bf++;
            print_if_present(os, bf, ",SYMBOL,", CancelRejectedBitfield2::SYMBOL, opt->symbol);
            print_if_present(os, bf, ",CAPACITY,", CancelRejectedBitfield2::CAPACITY, opt->capacity);
        }
    }
    return os;
}

std::ostream & operator<<(std::ostream &os, const OrderExecution &m)
{
    os << m.message_header << ","
       << m.transaction_time << ","
       << m.cl_ord_id << ","
       << m.exec_id << ","
       << m.last_shares << ","
       << m.last_px << ","
       << m.leaves_qty << ","
       << m.base_liquidity_indicator << ","
       << m.sub_liquidity_indicator << ","
       << (int) m.number_of_return_bitfields;

    const auto * vbf = reinterpret_cast<const VariableLengthBitfield *>(&m.number_of_return_bitfields);
    os << ",BF," << *vbf;

    if (vbf->num_bitfields) {
        const std::uint8_t * p = (const std::uint8_t *) &vbf->bitfields[0] + vbf->num_bitfields;
        const std::uint8_t * bf = (const std::uint8_t *) &vbf->bitfields[0];
        const auto * opt = reinterpret_cast<const OrderExecution::OptionalFields *>(p);

        if (vbf->num_bitfields >= 1) {
            print_if_present(os, bf, ",SIDE,", OrderExecutionBitfield1::SIDE, opt->side);
            print_if_present(os, bf, ",PEGDIFF,", OrderExecutionBitfield1::PEG_DIFFERENCE, opt->peg_difference);
            print_if_present(os, bf, ",PRICE,", OrderExecutionBitfield1::PRICE, opt->price);
            print_if_present(os, bf, ",EXECINST,", OrderExecutionBitfield1::EXEC_INST, (char) opt->exec_inst);
            print_if_present(os, bf, ",ORDTYPE,", OrderExecutionBitfield1::ORD_TYPE, opt->ord_type);
            print_if_present(os, bf, ",TIF,", OrderExecutionBitfield1::TIME_IN_FORCE, opt->time_in_force);
            print_if_present(os, bf, ",MINQTY,", OrderExecutionBitfield1::MIN_QTY, opt->min_qty);
            print_if_present(os, bf, ",MAXRMPCT,", OrderExecutionBitfield1::MAX_REMOVE_PCT, (int) opt->max_remove_pct);
        }
        if (vbf->num_bitfields >= 2) {
            bf++;
            print_if_present(os, bf, ",SYMBOL,", OrderExecutionBitfield2::SYMBOL, opt->symbol);
            print_if_present(os, bf, ",CAPACITY,", OrderExecutionBitfield2::CAPACITY, opt->capacity);
        }
        if (vbf->num_bitfields >= 3) {
            print_if_present(os, bf, ",ACCT,", OrderExecutionBitfield3::ACCOUNT, opt->account);
            print_if_present(os, bf, ",CLFIRM,", OrderExecutionBitfield3::CLEARING_FIRM, opt->clearing_firm);
            print_if_present(os, bf, ",CLACCT,", OrderExecutionBitfield3::CLEARING_ACCOUNT, opt->clearing_account);
            print_if_present(os, bf, ",DISPIND,", OrderExecutionBitfield3::DISPLAY_INDICATOR, opt->display_indicator);
            print_if_present(os, bf, ",MAXFLR,", OrderExecutionBitfield3::MAX_FLOOR, opt->max_floor);
            print_if_present(os, bf, ",DISCAMT,", OrderExecutionBitfield3::DISCRETION_AMOUNT, opt->discretion_amount);
            print_if_present(os, bf, ",ORDERQTY,", OrderExecutionBitfield3::ORDER_QTY, opt->order_qty);

            bf++;
        }
        if (vbf->num_bitfields >= 4) {
            bf++;
        }
        if (vbf->num_bitfields >= 5) {
            bf++;
        }
        if (vbf->num_bitfields >= 6) {
            bf++;
            print_if_present(os, bf, ",ATTRQT,", OrderExecutionBitfield6::ATTRIBUTED_QUOTE, opt->attributed_quote);
            print_if_present(os, bf, ",EXTEXECINST,", OrderExecutionBitfield6::EXT_EXEC_INST, opt->ext_exec_inst);
        }
        if (vbf->num_bitfields >= 7) {
            bf++;
        }
        if (vbf->num_bitfields >= 8) {
            bf++;
            print_if_present(os, bf, ",FEE,", OrderExecutionBitfield8::FEE_CODE, opt->fee_code);
            print_if_present(os, bf, ",ROUTINST,", OrderExecutionBitfield8::ROUTING_INST, opt->routing_inst);
        }
    }
    return os;
}

std::ostream & operator<<(std::ostream &os, const TradeCancelOrCorrect &m)
{
    os << m.message_header << ","
       << m.transaction_time << ","
       << m.cl_ord_id << ","
       << m.order_id << ","
       << m.exec_ref_id << ","
       << m.side << ","
       << m.base_liquidity_indicator << ","
       << m.clearing_firm << ","
       << m.clearing_account << ","
       << m.last_shares << ","
       << m.last_px << ","
       << m.corrected_price << ","
       << m.orig_time << ","
       << m.number_of_return_bitfields;
    const auto * vbf = reinterpret_cast<const VariableLengthBitfield *>(&m.number_of_return_bitfields);
    os << ",BF," << *vbf;

    if (vbf->num_bitfields) {
        const std::uint8_t * p = (const std::uint8_t *) &vbf->bitfields[0] + vbf->num_bitfields;
        const std::uint8_t * bf = (const std::uint8_t *) &vbf->bitfields[0];
        const auto * opt = reinterpret_cast<const TradeCancelOrCorrect::OptionalFields *>(p);

        if (vbf->num_bitfields >= 1) {
        }
        if (vbf->num_bitfields >= 2) {
            bf++;
            print_if_present(os, bf, ",SYMBOL,", TradeCancelOrCorrectBitfield2::SYMBOL, opt->symbol);
            print_if_present(os, bf, ",CAPACITY,", TradeCancelOrCorrectBitfield2::CAPACITY, opt->capacity);
        }
    }
    return os;
}

}}}}}
