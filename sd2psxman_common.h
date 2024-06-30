#ifndef SD2PSXMAN_COMMON_H
#define SD2PSXMAN_COMMON_H

#include <tamtypes.h>

#define SD2PSXMAN_IRX 0xB0355 // 'S' + 'S' + '2' + 'P' + 'S' + 'X' + 'M' + 'A' + 'N' + 0xB00B5

//RPC functions and CMDs share the same IDs
enum SD2PSXMAN_RPC_ID {
    SD2PSXMAN_PING = 0x1,
    SD2PSXMAN_GET_STATUS,
    SD2PSXMAN_GET_CARD,
    SD2PSXMAN_SET_CARD,
    SD2PSXMAN_GET_CHANNEL,
    SD2PSXMAN_SET_CHANNEL,
    SD2PSXMAN_GET_GAMEID,
    SD2PSXMAN_SET_GAMEID,
    SD2PSXMAN_UNMOUNT_BOOTCARD = 0x30,
    SD2PSXMAN_SEND_RAW_PAYLOAD,
};

enum SD2PSXMAN_SET_MODE {
    SD2PSXMAN_MODE_NUM,
    SD2PSXMAN_MODE_NEXT,
    SD2PSXMAN_MODE_PREV
};

enum SD2PSXMAN_PORTS{
    PORT_MEMCARD1 = 2,
    PORT_MEMCARD2
};

enum SD2PSXMAN_CARD_TYPE{
    SD2PSXMAN_CARD_TYPE_REGULAR = 0,
    SD2PSXMAN_CARD_TYPE_BOOT
};

typedef struct sd2psxman_rpc_pkt_t
{
    u8 slot;    //used for multitap
    u8 port;    //0 = ctrl1, 1 = ctrl2, 2 = memcard1, 3 = memcard2
    u8 type;    //regular or boot card
    u8 mode;    //num, next, or prev for commands set chan/card
    u16 cnum;   //card or channel number depending on cmd
    int ret;    //return value
}sd2psxman_rpc_pkt_t;

typedef struct sd2psxman_gameid_rpc_pkt_t
{
    u8 slot;    //used for multitap
    u8 port;    //0 = ctrl1, 1 = ctrl2, 2 = memcard1, 3 = memcard2
    u8 gameid_len;
    char gameid[251];
    int ret;
}sd2psxman_gameid_rpc_pkt_t;

typedef struct sd2psxman_raw_rpc_pkt_t
{
    u8 slot;    //used for multitap
    u8 port;    //0 = ctrl1, 1 = ctrl2, 2 = memcard1, 3 = memcard2
    u8 payload[255];
    u8 payload_size;
    int ret;
}sd2psxman_raw_rpc_pkt_t;

#define SD2PSXMAN_RPC_PKT_SIZE sizeof(sd2psxman_rpc_pkt_t)
#define SD2PSXMAN_GAMEID_RPC_PKT_SIZE sizeof(sd2psxman_gameid_rpc_pkt_t)
#define SD2PSXMAN_RAW_RPC_PKT_SIZE sizeof(sd2psxman_raw_rpc_pkt_t)

#endif