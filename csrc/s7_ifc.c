/*
* @Author: lorenzo
* @Date:   2018-01-26 09:35:34
* @Last Modified by:   Lorenzo
* @Last Modified time: 2018-06-11 17:31:19
*/

#define ZERYNTH_PRINTF
#define ZERYNTH_SOCKETS
#include "zerynth.h"

#include "snap7.h"

S7Object Client;

C_NATIVE(s7_Cli_Create) {
    C_NATIVE_UNWARN();

    RELEASE_GIL();
    Client = Cli_Create();
    ACQUIRE_GIL();

    return ERR_OK;
}

C_NATIVE(s7_Cli_Connect) {
    C_NATIVE_UNWARN();

    int32_t Rack, Slot;
    int32_t Requested, Negotiated;
    uint8_t* addr;
    int32_t addrlen;

    if (parse_py_args("sii", nargs, args, &addr, &addrlen, &Rack, &Slot) != 3)
        return ERR_TYPE_EXC;
    
    uint8_t* addr_s = gc_malloc(sizeof(uint8_t) * (addrlen + 1));
    memcpy(addr_s, addr, addrlen);
    addr_s[addrlen] = 0;
  
    RELEASE_GIL();
    if (Cli_ConnectTo(Client, addr_s, Rack,Slot)) {
        ACQUIRE_GIL();
        gc_free(addr_s);
        return ERR_IOERROR_EXC;
    };
    Cli_GetPduLength(Client, &Requested, &Negotiated);
    ACQUIRE_GIL();

    *res = ptuple_new(2, NULL);
    PTUPLE_SET_ITEM(*res, 0, PSMALLINT_NEW(Requested));
    PTUPLE_SET_ITEM(*res, 1, PSMALLINT_NEW(Negotiated));

    gc_free(addr_s);
    return ERR_OK;
}

C_NATIVE(s7_Cli_GetCpuInfo) {
    C_NATIVE_UNWARN();

    TS7CpuInfo Info;

    // RELEASE_GIL();    
    // if(Cli_GetCpuInfo(Client, &Info)) {
    //     ACQUIRE_GIL();
    //     return ERR_IOERROR_EXC;
    // }
    // ACQUIRE_GIL();

    strcpy(Info.ModuleTypeName, "");
    strcpy(Info.SerialNumber, "");
    strcpy(Info.ASName, "");
    strcpy(Info.Copyright, "");
    strcpy(Info.ModuleName, "");

    // printf("tuple...\n");
    // printf("%s %i\n", Info.ModuleTypeName, strlen(Info.ModuleTypeName));
    // printf("%s %i\n", Info.SerialNumber, strlen(Info.SerialNumber));
    // printf("%s %i\n", Info.ASName, strlen(Info.ASName));
    // printf("%s %i\n", Info.Copyright, strlen(Info.Copyright));
    // printf("%s %i\n", Info.ModuleName, strlen(Info.ModuleName));

    *res = ptuple_new(5, NULL);

    PTUPLE_SET_ITEM(*res, 0, pstring_new(strlen(Info.ModuleTypeName), &(Info.ModuleTypeName)));
    PTUPLE_SET_ITEM(*res, 1, pstring_new(strlen(Info.SerialNumber), &(Info.SerialNumber)));
    PTUPLE_SET_ITEM(*res, 2, pstring_new(strlen(Info.ASName), &(Info.ASName)));
    PTUPLE_SET_ITEM(*res, 3, pstring_new(strlen(Info.Copyright), &(Info.Copyright)));
    PTUPLE_SET_ITEM(*res, 4, pstring_new(strlen(Info.ModuleName), &(Info.ModuleName)));


    // printf("return...\n");

    return ERR_OK;

    // TODO: fixme... (Cli_GetCpuInfo seems to work fine but then everythin crashes)
}

// Data I/O main functions

static inline int32_t area_to_wordlen(int32_t Area) {
    if (Area >= S7AreaPE && Area <= S7AreaDB) {
        return S7WLByte;
    }
    if (Area >= S7AreaCT && Area <= S7AreaTM) {
        return S7WLWord; // should work instead of S7WLCounter and S7WLTimer, check!
    }    
    return -1;
}

static inline int32_t prepare_read_sequence(int32_t Area, int32_t Amount, int32_t* WordLen, uint8_t *sequencetype, PObject **seq) {
    *WordLen = area_to_wordlen(Area);
    if (*WordLen < 0) {
        return -1;
    }

    *sequencetype = ((*WordLen == S7WLByte) ? PBYTES : PSHORTS);
    *seq = (PObject*) psequence_new(*sequencetype, (*sequencetype == PBYTES) ? Amount : 2*Amount);

    return 0;
}

C_NATIVE(s7_Cli_ReadArea) {
    C_NATIVE_UNWARN();

    int32_t Area, DBNumber, Start, Amount, WordLen, i;
    uint8_t sequencetype;

    uint16_t swap_val;

    if (parse_py_args("iiii", nargs, args, &Area, &DBNumber, &Start, &Amount) != 4)
        return ERR_TYPE_EXC;

    if (prepare_read_sequence(Area, Amount, &WordLen, &sequencetype, res) < 0) {
        return ERR_UNSUPPORTED_EXC;
    }

    // TODO: db may not be needed
    RELEASE_GIL();
    int status = Cli_ReadArea(Client, Area, DBNumber, Start, Amount, WordLen, (void*) ((PBytes*) *res)->seq);
    if (status == -1){
        return ERR_IOERROR_EXC;
    }
    if (sequencetype == PSHORTS) {
        uint16_t *short_seq = ((uint16_t*) ((PBytes*) *res)->seq);
        for (i = 0; i<Amount; i++) {
            swap_val =  short_seq[i];
            swap_val = (swap_val >> 8) | (swap_val << 8); // s7 big endian to mcu little endian...
            short_seq[i] = swap_val;
        }
    }
    ACQUIRE_GIL();

    return ERR_OK;
}


static inline int32_t eval_write_amount(int32_t Area, int32_t *WordLen, int32_t *Amount, int data_type, uint32_t data_len) {
    int32_t ExpWordLen;
    uint8_t wordsize;

    ExpWordLen = area_to_wordlen(Area);

    if (ExpWordLen < 0) {
        return ERR_UNSUPPORTED_EXC;
    }

    if (IS_BYTE_PSEQUENCE_TYPE(data_type)) {
        *WordLen = S7WLByte;
    }
    else {
        if (IS_SHORT_PSEQUENCE_TYPE(data_type)) {
            *WordLen = S7WLWord;
        }
        else {
            return ERR_TYPE_EXC;
        }
    }

    if (ExpWordLen != *WordLen) {
        return ERR_TYPE_EXC;
    }

    wordsize = ((*WordLen == S7WLByte) ? 1 : 2);
    *Amount = (data_len/wordsize);

    return 0; 
}

C_NATIVE(s7_Cli_WriteArea) {
    C_NATIVE_UNWARN();

    int32_t Area, DBNumber, Start, Amount, WordLen;
    void *pUsrData;
    uint32_t pUsrData_len;

    Area = PSMALLINT_VALUE(args[0]);
    DBNumber = PSMALLINT_VALUE(args[1]);
    Start  = PSMALLINT_VALUE(args[2]);

    pUsrData = ((PBytes*) args[3])->seq;
    pUsrData_len = PSEQUENCE_ELEMENTS((PSequence*) args[3]);

    int32_t exc_code = eval_write_amount(Area, &WordLen, &Amount, PTYPE(args[3]), pUsrData_len);
    if (exc_code != 0) {
        return exc_code;
    }

    // TODO: db may not be needed
    RELEASE_GIL();
    Cli_WriteArea(Client, Area, DBNumber, Start, Amount, WordLen, pUsrData);
    ACQUIRE_GIL();
    return ERR_OK;
}


C_NATIVE(s7_Cli_ReadMultiVars) {
    C_NATIVE_UNWARN();

    if (nargs != 1)
        return ERR_TYPE_EXC;

    int32_t Area, DBNumber, Start, Amount, WordLen;

    uint8_t sequencetype;

    uint32_t items_num = PSEQUENCE_ELEMENTS(args[0]);
    uint32_t i;
    TS7DataItem *Items;
    PObject **item_descs = PSEQUENCE_OBJECTS(args[0]);

    Items = gc_malloc(sizeof(TS7DataItem) * items_num);
    *res = ptuple_new(items_num, NULL);
    for (i=0; i < items_num; i++) {

        PSequence *item_desc;
        item_desc = (PSequence*) item_descs[i];

        if (parse_py_args("iiii", PSEQUENCE_ELEMENTS(item_desc), PSEQUENCE_OBJECTS(item_desc), &Area, &DBNumber, &Start, &Amount) != 4) {
            gc_free(Items);
            return ERR_TYPE_EXC;
        }

        PSequence *buf;
        if (prepare_read_sequence(Area, Amount, &WordLen, &sequencetype, &buf) < 0) {
            gc_free(Items);
            return ERR_UNSUPPORTED_EXC;
        }

        PTUPLE_SET_ITEM(*res, i, buf);

        // TODO: db may not be needed
        Items[i].Area     = Area;
        Items[i].WordLen  = WordLen;
        Items[i].DBNumber = DBNumber;
        Items[i].Start    = Start;
        Items[i].Amount   = Amount;
        Items[i].pdata    = ((PBytes *) buf)->seq;
    }

    RELEASE_GIL();
    Cli_ReadMultiVars(Client, &Items[0], items_num);
    ACQUIRE_GIL();

    gc_free(Items);

    return ERR_OK;
}


C_NATIVE(s7_Cli_WriteMultiVars) {
    C_NATIVE_UNWARN();

    if (nargs != 1)
        return ERR_TYPE_EXC;

    int32_t Area, DBNumber, Start, Amount, WordLen;
    void *pUsrData;
    uint32_t pUsrData_len;

    uint32_t items_num = PSEQUENCE_ELEMENTS(args[0]);
    uint32_t i;
    TS7DataItem *Items;
    PObject **item_descs = PSEQUENCE_OBJECTS(args[0]);

    Items = gc_malloc(sizeof(TS7DataItem) * items_num);

    for (i=0; i < items_num; i++) {

        PSequence *item_desc;
        item_desc = (PSequence*) item_descs[i];

        PObject **item_desc_objs = PSEQUENCE_OBJECTS(item_desc);

        Area = PSMALLINT_VALUE(item_desc_objs[0]);
        DBNumber = PSMALLINT_VALUE(item_desc_objs[1]);
        Start  = PSMALLINT_VALUE(item_desc_objs[2]);

        pUsrData = ((PBytes*) item_desc_objs[3])->seq;
        pUsrData_len = PSEQUENCE_ELEMENTS((PSequence*) item_desc_objs[3]);

        int32_t exc_code = eval_write_amount(Area, &WordLen, &Amount, PTYPE(item_desc_objs[3]), pUsrData_len);
        if (exc_code != 0) {
            gc_free(Items);
            return exc_code;
        }

        // TODO: db may not be needed
        Items[i].Area     = Area;
        Items[i].WordLen  = WordLen;
        Items[i].DBNumber = DBNumber;
        Items[i].Start    = Start;
        Items[i].Amount   = Amount;
        Items[i].pdata    = pUsrData;
    }

    RELEASE_GIL();
    Cli_WriteMultiVars(Client, &Items[0], items_num);
    ACQUIRE_GIL();

    gc_free(Items);

    return ERR_OK;
}


// Data I/O Lean functions
// int S7API Cli_DBRead(S7Object Client, int DBNumber, int Start, int Size, void *pUsrData);
// int S7API Cli_DBWrite(S7Object Client, int DBNumber, int Start, int Size, void *pUsrData);
// int S7API Cli_MBRead(S7Object Client, int Start, int Size, void *pUsrData);
// int S7API Cli_MBWrite(S7Object Client, int Start, int Size, void *pUsrData);
// int S7API Cli_EBRead(S7Object Client, int Start, int Size, void *pUsrData);
// int S7API Cli_EBWrite(S7Object Client, int Start, int Size, void *pUsrData);
// int S7API Cli_ABRead(S7Object Client, int Start, int Size, void *pUsrData);
// int S7API Cli_ABWrite(S7Object Client, int Start, int Size, void *pUsrData);
// int S7API Cli_TMRead(S7Object Client, int Start, int Amount, void *pUsrData);
// int S7API Cli_TMWrite(S7Object Client, int Start, int Amount, void *pUsrData);
// int S7API Cli_CTRead(S7Object Client, int Start, int Amount, void *pUsrData);
// int S7API Cli_CTWrite(S7Object Client, int Start, int Amount, void *pUsrData);
