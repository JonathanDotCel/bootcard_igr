#include <sysclib.h>

#include "ioplib.h"
#include "irx_imports.h"
#include "module_debug.h"
#include "sio2regs.h"
#include "../sd2psxman_common.h"

IRX_ID(MODNAME, MAJOR, MINOR);

#define SD2PSXMAN_ID 0x8B
#define SD2PSXMAN_RESERVED 0xFF
#define SD2PSXMAN_REPLY_CONST 0xAA

#define SecrSetMcCommandHandler_Expnum 0x04
typedef int (*McCommandHandler_t)(int port, int slot, sio2_transfer_data_t *sio2_trans_data); // to hook into secrman setter
typedef void (*SecrSetMcCommandHandler_hook_t)(McCommandHandler_t handler);

SecrSetMcCommandHandler_hook_t ORIGINAL_SecrSetMcCommandHandler = NULL;
static McCommandHandler_t McCommandHandler = NULL;

static SifRpcDataQueue_t sd2psxman_queue;
static SifRpcServerData_t sd2psxman_server;

//TODO: adjust rpc buffer size
static u8 sd2psxman_rpc_buffer[268] __attribute__((__aligned__(4)));
static int RPCThreadID;


void HOOKED_SecrSetMcCommandHandler(McCommandHandler_t handler)
{
    DPRINTF("%s: handler ptr 0x%p\n",__FUNCTION__, handler);
    McCommandHandler = handler;
    ORIGINAL_SecrSetMcCommandHandler(handler); //we kept the original function to call it from here... else, SECRMAN wont auth cards...
}

static int sd2psxman_sio2_send_port0(u8 in_size, u8 *in_buf)
{
    int rv;
    sio2_transfer_data_t sio2packet;

    //clear port ctrl settings
    memset(sio2packet.port_ctrl1, 0, sizeof(sio2packet.port_ctrl1));
    memset(sio2packet.port_ctrl2, 0, sizeof(sio2packet.port_ctrl2));

    //configure baud and other common parameters
    //baud0 div = 0x2 (24MHz)
    //baud1 div = 0xff (~200KHz)
    sio2packet.port_ctrl1[0] = PCTRL0_ATT_LOW_PER(0x5)      |
                               PCTRL0_ATT_MIN_HIGH_PER(0x5) |
                               PCTRL0_BAUD0_DIV(0xFF)       |
                               PCTRL0_BAUD1_DIV(0xFF);
 
    sio2packet.port_ctrl2[0] = PCTRL1_ACK_TIMEOUT_PER(0x5) |
                               PCTRL1_INTER_BYTE_PER(0x5)  |
                               PCTRL1_UNK24(0x0)           |
                               PCTRL1_IF_MODE_SPI_DIFF(0x0);

    sio2packet.in_dma.addr = NULL;
    sio2packet.out_dma.addr = NULL;

    sio2packet.in_size = in_size;
    sio2packet.out_size = 0;

    sio2packet.in = in_buf; //MEM -> SIO2
    sio2packet.out = NULL;  //SIO2 -> MEM

    //Each byte is a separate transfer, this allows up to 15 bytes to be transferred while ignoring /ACK
    for (int i = 0; i < in_size; i++) {
        sio2packet.regdata[i] = TR_CTRL_PORT_NR(0)       |
                        TR_CTRL_PAUSE(0)            |
                        TR_CTRL_TX_MODE_PIO_DMA(0)  |
                        TR_CTRL_RX_MODE_PIO_DMA(0)  |
                        TR_CTRL_NORMAL_TR(1)        |
                        TR_CTRL_SPECIAL_TR(0)       |
                        TR_CTRL_BAUD_DIV(0)         |
                        TR_CTRL_WAIT_ACK_FOREVER(0) |
                        TR_CTRL_TX_DATA_SZ(1) |
                        TR_CTRL_RX_DATA_SZ(0);
    }
    sio2packet.regdata[in_size] = 0x0;

    //execute SIO2 transfer
    rv = McCommandHandler(0, 0, &sio2packet);
    
    if (rv == 0) {
        DPRINTF("McCommandHandler failed\n");
    }

    return rv;
}

static int sd2psxman_sio2_send(int port, int slot, u8 in_size, u8 out_size, u8 *in_buf, u8 *out_buf)
{
    int rv;
    sio2_transfer_data_t sio2packet;

    //clear port ctrl settings
    memset(sio2packet.port_ctrl1, 0, sizeof(sio2packet.port_ctrl1));
    memset(sio2packet.port_ctrl2, 0, sizeof(sio2packet.port_ctrl2));

    //configure baud and other common parameters
    //baud0 div = 0x2 (24MHz)
    //baud1 div = 0xff (~200KHz), unused.
    sio2packet.port_ctrl1[port] = PCTRL0_ATT_LOW_PER(0x5)      |
                                  PCTRL0_ATT_MIN_HIGH_PER(0x5) |
                                  PCTRL0_BAUD0_DIV(0x2)        |
                                  PCTRL0_BAUD1_DIV(0xFF);
 
    //ACK_TIMEOUT should allow for around ~2.5ms of delay between bytes in a single transfer
    sio2packet.port_ctrl2[port] = PCTRL1_ACK_TIMEOUT_PER(0xffff) |
                                  PCTRL1_INTER_BYTE_PER(0x5)     |
                                  PCTRL1_UNK24(0x0)              |
                                  PCTRL1_IF_MODE_SPI_DIFF(0x0);

    sio2packet.in_dma.addr = NULL;
    sio2packet.out_dma.addr = NULL;

    sio2packet.in_size = in_size;
    sio2packet.out_size = out_size;

    sio2packet.in = in_buf;    //MEM -> SIO2
    sio2packet.out = out_buf;  //SIO2 -> MEM

    //configure single transfer (up to 255 bytes)
    sio2packet.regdata[0] = TR_CTRL_PORT_NR(port)       |
                            TR_CTRL_PAUSE(0)            |
                            TR_CTRL_TX_MODE_PIO_DMA(0)  |
                            TR_CTRL_RX_MODE_PIO_DMA(0)  |
                            TR_CTRL_NORMAL_TR(1)        |
                            TR_CTRL_SPECIAL_TR(0)       |
                            TR_CTRL_BAUD_DIV(0)         |
                            TR_CTRL_WAIT_ACK_FOREVER(0) |
                            TR_CTRL_TX_DATA_SZ(in_size) |
                            TR_CTRL_RX_DATA_SZ(out_size);
    sio2packet.regdata[1] = 0x0;

    //execute SIO2 transfer
    rv = McCommandHandler(port, slot, &sio2packet);
    
    if (rv == 0) {
        DPRINTF("McCommandHandler failed\n");
    }

    return rv;
}

static void sd2psxman_ping(void *data)
{
    sd2psxman_rpc_pkt_t *pkt = data;

    u8 wrbuf[0x3];
    u8 rdbuf[0x7];

    wrbuf[0x0] = SD2PSXMAN_ID;          //identifier
    wrbuf[0x1] = SD2PSXMAN_PING;        //command
    wrbuf[0x2] = SD2PSXMAN_RESERVED;    //reserved byte

    sd2psxman_sio2_send(pkt->port, pkt->slot, sizeof(wrbuf), sizeof(rdbuf), wrbuf, rdbuf);

    //bits 24-16: protocol ver
    //bits 16-8: product id
    //bits 8-0: revision id
    if (rdbuf[0x1] == SD2PSXMAN_REPLY_CONST) {
        pkt->ret = rdbuf[0x3] << 16 | rdbuf[0x4] << 8 | rdbuf[0x5];
    } else {
        DPRINTF("%s ERROR: Invalid response from card. Got 0x%x, Expected 0x%x\n", __func__, rdbuf[0x1], SD2PSXMAN_REPLY_CONST);
        pkt->ret = -2;
    }
}

static void sd2psxman_get_status(void *data)
{
    sd2psxman_rpc_pkt_t *pkt = data;

    u8 wrbuf[0x3];
    u8 rdbuf[0x6];

    wrbuf[0x0] = SD2PSXMAN_ID;          //identifier
    wrbuf[0x1] = SD2PSXMAN_GET_STATUS;  //command
    wrbuf[0x2] = SD2PSXMAN_RESERVED;    //reserved byte

    sd2psxman_sio2_send(pkt->port, pkt->slot, sizeof(wrbuf), sizeof(rdbuf), wrbuf, rdbuf);

    if (rdbuf[0x1] == SD2PSXMAN_REPLY_CONST) {
        pkt->ret = rdbuf[0x4];
    } else {
        DPRINTF("%s ERROR: Invalid response from card. Got 0x%x, Expected 0x%x\n", __func__, rdbuf[0x1], SD2PSXMAN_REPLY_CONST);
        pkt->ret = -2;
    }
}

static void sd2psxman_get_card(void *data)
{
    sd2psxman_rpc_pkt_t *pkt = data;

    u8 wrbuf[0x3];
    u8 rdbuf[0x6];

    wrbuf[0x0] = SD2PSXMAN_ID;          //identifier
    wrbuf[0x1] = SD2PSXMAN_GET_CARD;    //command
    wrbuf[0x2] = SD2PSXMAN_RESERVED;    //reserved byte
    
    sd2psxman_sio2_send(pkt->port, pkt->slot, sizeof(wrbuf), sizeof(rdbuf), wrbuf, rdbuf);

    if (rdbuf[0x1] == SD2PSXMAN_REPLY_CONST) {
        pkt->ret = rdbuf[0x3] << 8 | rdbuf[0x4];
    } else {
        DPRINTF("%s ERROR: Invalid response from card. Got 0x%x, Expected 0x%x\n", __func__, rdbuf[0x1], SD2PSXMAN_REPLY_CONST);
        pkt->ret = -2;
    }
}

static void sd2psxman_set_card(void *data)
{
    sd2psxman_rpc_pkt_t *pkt = data;

    u8 wrbuf[0x8];
    u8 rdbuf[0x2];

    wrbuf[0x0] = SD2PSXMAN_ID;          //identifier
    wrbuf[0x1] = SD2PSXMAN_SET_CARD;    //command
    wrbuf[0x2] = SD2PSXMAN_RESERVED;    //reserved byte
    wrbuf[0x3] = pkt->type;             //card type (0 = regular, 1 = boot)
    wrbuf[0x4] = pkt->mode;             //set mode (num, next, prev)
    wrbuf[0x5] = pkt->cnum >> 8;        //card number upper 8 bits
    wrbuf[0x6] = pkt->cnum & 0xFF;      //card number lower 8 bits

    sd2psxman_sio2_send(pkt->port, pkt->slot, sizeof(wrbuf), sizeof(rdbuf), wrbuf, rdbuf);

    if (rdbuf[0x1] == SD2PSXMAN_REPLY_CONST) {
        pkt->ret = 0;
    } else {
        DPRINTF("%s ERROR: Invalid response from card. Got 0x%x, Expected 0x%x\n", __func__, rdbuf[0x1], SD2PSXMAN_REPLY_CONST);
        pkt->ret = -2;
    }
}

static void sd2psxman_get_channel(void *data)
{
    sd2psxman_rpc_pkt_t *pkt = data;

    u8 wrbuf[0x3];
    u8 rdbuf[0x6];

    wrbuf[0x0] = SD2PSXMAN_ID;          //identifier
    wrbuf[0x1] = SD2PSXMAN_GET_CHANNEL; //command
    wrbuf[0x2] = SD2PSXMAN_RESERVED;    //reserved byte

    sd2psxman_sio2_send(pkt->port, pkt->slot, sizeof(wrbuf), sizeof(rdbuf), wrbuf, rdbuf);
    
    if (rdbuf[0x1] == SD2PSXMAN_REPLY_CONST) {
        pkt->ret = rdbuf[0x3] << 8 | rdbuf[0x4];
    } else {
        DPRINTF("%s ERROR: Invalid response from card. Got 0x%x, Expected 0x%x\n", __func__, rdbuf[0x1], SD2PSXMAN_REPLY_CONST);
        pkt->ret = -2;
    }
}

static void sd2psxman_set_channel(void *data)
{
    sd2psxman_rpc_pkt_t *pkt = data;

    u8 wrbuf[0x7];
    u8 rdbuf[0x2];

    wrbuf[0x0] = SD2PSXMAN_ID;          //identifier
    wrbuf[0x1] = SD2PSXMAN_SET_CHANNEL; //command
    wrbuf[0x2] = SD2PSXMAN_RESERVED;    //reserved byte
    wrbuf[0x3] = pkt->mode;             //set mode (num, next, prev)
    wrbuf[0x4] = pkt->cnum >> 8;        //channel number upper 8 bits    
    wrbuf[0x5] = pkt->cnum & 0xFF;      //channel number lower 8 bits

    sd2psxman_sio2_send(pkt->port, pkt->slot, sizeof(wrbuf), sizeof(rdbuf), wrbuf, rdbuf);
    
    if (rdbuf[0x1] == SD2PSXMAN_REPLY_CONST) {
        pkt->ret = 0;
    } else {
        DPRINTF("%s ERROR: Invalid response from card. Got 0x%x, Expected 0x%x\n", __func__, rdbuf[0x1], SD2PSXMAN_REPLY_CONST);
        pkt->ret = -2;
    }
}

static void sd2psxman_get_gameid(void *data)
{
    sd2psxman_gameid_rpc_pkt_t *pkt = data;

    u8 wrbuf[0x3];
    u8 rdbuf[0xFF]; //fixed packet size of 255 bytes

    wrbuf[0x0] = SD2PSXMAN_ID;          //identifier
    wrbuf[0x1] = SD2PSXMAN_GET_GAMEID;  //command
    wrbuf[0x2] = SD2PSXMAN_RESERVED;    //reserved byte

    sd2psxman_sio2_send(pkt->port, pkt->slot, sizeof(wrbuf), sizeof(rdbuf), wrbuf, rdbuf);
    
    if (rdbuf[0x1] == SD2PSXMAN_REPLY_CONST) {
        pkt->ret = 0;
        pkt->gameid_len = rdbuf[0x3];
        char* str = &rdbuf[0x4];
        strcpy(pkt->gameid, str);
    } else {
        DPRINTF("%s ERROR: Invalid response from card. Got 0x%x, Expected 0x%x\n", __func__, rdbuf[0x1], SD2PSXMAN_REPLY_CONST);
        pkt->ret = -2;
    }
}

static void sd2psxman_set_gameid(void *data)
{
    sd2psxman_gameid_rpc_pkt_t *pkt = data;

    u8 wrbuf[0xFF];
    u8 rdbuf[0x2];

    wrbuf[0x0] = SD2PSXMAN_ID;          //identifier
    wrbuf[0x1] = SD2PSXMAN_SET_GAMEID;  //command
    wrbuf[0x2] = SD2PSXMAN_RESERVED;    //reserved byte
    wrbuf[0x3] = pkt->gameid_len;       //gameid length

    char* str = &wrbuf[0x4];
    strcpy(str, pkt->gameid);

    sd2psxman_sio2_send(pkt->port, pkt->slot, (pkt->gameid_len + 5), sizeof(rdbuf), wrbuf, rdbuf);
    sd2psxman_sio2_send_port0((pkt->gameid_len + 5), wrbuf);
    
    if (rdbuf[0x1] == SD2PSXMAN_REPLY_CONST) {
        pkt->ret = 0;
    } else {
        DPRINTF("%s ERROR: Invalid response from card. Got 0x%x, Expected 0x%x\n", __func__, rdbuf[0x1], SD2PSXMAN_REPLY_CONST);
        pkt->ret = -2;
    }
}

static void sd2psxman_unmount_bootcard(void *data)
{
    sd2psxman_rpc_pkt_t *pkt = data;

    u8 wrbuf[0x3];
    u8 rdbuf[0x4];

    wrbuf[0x0] = SD2PSXMAN_ID;               //identifier
    wrbuf[0x1] = SD2PSXMAN_UNMOUNT_BOOTCARD; //command
    wrbuf[0x2] = SD2PSXMAN_RESERVED;         //reserved byte

    sd2psxman_sio2_send(pkt->port, pkt->slot, sizeof(wrbuf), sizeof(rdbuf), wrbuf, rdbuf);

    if (rdbuf[0x1] == SD2PSXMAN_REPLY_CONST) {
        pkt->ret = 0;
    } else {
        DPRINTF("%s ERROR: Invalid response from card. Got 0x%x, Expected 0x%x\n", __func__, rdbuf[0x1], SD2PSXMAN_REPLY_CONST);
        pkt->ret = -2;
    }
}

static void sd2psxman_send_raw_payload(void *data)
{
    sd2psxman_raw_rpc_pkt_t *pkt = data;

    sd2psxman_sio2_send(pkt->port, pkt->slot, pkt->payload_size, pkt->payload_size, pkt->payload, pkt->payload);
}

static void *sd2psxman_rpc_handler(unsigned int CMD, void *rpcBuffer, int size)
{
    if (McCommandHandler == NULL)
    {
        DPRINTF("ERROR: CANNOT SEND COMMANDS IF 'McCommandHandler' HAS NOT BEEN INTERCEPTED, PLEASE LOAD MCMAN BEFORE USING SD2PSXMAN FEATURES\n");
        return rpcBuffer;
    }
   
    switch(CMD)
	{
        case SD2PSXMAN_PING:
            sd2psxman_ping(rpcBuffer);
            break;
        case SD2PSXMAN_GET_STATUS:
            sd2psxman_get_status(rpcBuffer);
            break;
        case SD2PSXMAN_GET_CARD:
            sd2psxman_get_card(rpcBuffer);
            break;
        case SD2PSXMAN_SET_CARD:
            sd2psxman_set_card(rpcBuffer);
            break;
        case SD2PSXMAN_GET_CHANNEL:
            sd2psxman_get_channel(rpcBuffer);
            break;
        case SD2PSXMAN_SET_CHANNEL:
            sd2psxman_set_channel(rpcBuffer);
            break;
        case SD2PSXMAN_GET_GAMEID:
            sd2psxman_get_gameid(rpcBuffer);
            break;
        case SD2PSXMAN_SET_GAMEID:
            sd2psxman_set_gameid(rpcBuffer);
            break;
        case SD2PSXMAN_UNMOUNT_BOOTCARD:
            sd2psxman_unmount_bootcard(rpcBuffer);
            break;
        case SD2PSXMAN_SEND_RAW_PAYLOAD:
            sd2psxman_send_raw_payload(rpcBuffer);
            break;
        default:
            printf(MODNAME": Unknown CMD (%d) called!\n", CMD);
    }

    return rpcBuffer;
}

static void threadRpcFunction(void *arg)
{
	(void)arg;

	DPRINTF("RPC Thread Started\n");

	SifSetRpcQueue( &sd2psxman_queue , GetThreadId() );
	SifRegisterRpc( &sd2psxman_server, SD2PSXMAN_IRX, (void *)sd2psxman_rpc_handler,(u8 *)&sd2psxman_rpc_buffer,NULL,NULL, &sd2psxman_queue );
	SifRpcLoop( &sd2psxman_queue );
}

int __stop(int argc, char *argv[]);
int __start(int argc, char *argv[]);

int _start(int argc, char *argv[])
{
    if (argc >= 0) 
        return __start(argc, argv);
    else
        return __stop(-argc, argv);
}

int __start(int argc, char *argv[])
{
    printf("SD2PSX Manager v%d.%d by El_isra and qnox32\n", MAJOR, MINOR);

    if (ioplib_getByName("mcman") != NULL)
    {
        DPRINTF("MCMAN FOUND. Must be loaded after this module to intercept the McCommandHandler\n");
        goto quit;
    }

	iop_thread_t T;
	T.attr = TH_C;
	T.option = 0;
	T.thread = &threadRpcFunction;
	T.stacksize = 0x800;
	T.priority = 0x1e;

	DPRINTF("Creating RPC thread.\n");
	RPCThreadID = CreateThread(&T);
	if (RPCThreadID < 0)
	{
		DPRINTF("CreateThread failed. (%i)\n", RPCThreadID);
        goto quit;
	}
	else
	{
#ifdef DEBUG
		int TSTAT = 
#endif
        StartThread(RPCThreadID, NULL);
        DPRINTF("Thread started (%d)\n", TSTAT);
	}
    iop_library_t * SECRMAN = ioplib_getByName("secrman");
    if (SECRMAN == NULL)
    {
        DPRINTF("SECRMAN not found\n");
        goto quit_and_stop_thread;
    }
    DPRINTF("Found SECRMAN. version 0x%X\n", SECRMAN->version);

    ORIGINAL_SecrSetMcCommandHandler = (SecrSetMcCommandHandler_hook_t)ioplib_hookExportEntry(SECRMAN, SecrSetMcCommandHandler_Expnum, HOOKED_SecrSetMcCommandHandler);
    if (ORIGINAL_SecrSetMcCommandHandler == NULL)
    {
        DPRINTF("Error hooking into SecrSetMcCommandHandler\n");
        goto quit_and_stop_thread;
    } else {
        DPRINTF("Hooked SecrSetMcCommandHandler (new one:0x%p, old one 0x%p)\n", HOOKED_SecrSetMcCommandHandler, ORIGINAL_SecrSetMcCommandHandler);
    }

    iop_library_t * lib_modload = ioplib_getByName("modload");
    if (lib_modload != NULL) {
        DPRINTF("modload 0x%x detected\n", lib_modload->version);
        if (lib_modload->version > 0x102) //IOP is running a MODLOAD version wich supports unloading IRX Modules
            return MODULE_REMOVABLE_END; // and we do support getting unloaded...
    } else {
        DPRINTF("modload not detected!\n");
    }

    return MODULE_RESIDENT_END;
    quit_and_stop_thread:
    DeleteThread(RPCThreadID);
    quit:
    return MODULE_NO_RESIDENT_END;
}

int __stop(int argc, char *argv[])
{
    DPRINTF("Unloading module\n");
    DPRINTF("Stopping RPC Thread\n");
    DeleteThread(RPCThreadID);
    DPRINTF("Restoring SECRMAN callback setter\n");
    iop_library_t * SECRMAN = ioplib_getByName("secrman");
    ioplib_hookExportEntry(SECRMAN, SecrSetMcCommandHandler_Expnum, ORIGINAL_SecrSetMcCommandHandler);
    return MODULE_NO_RESIDENT_END;
}
