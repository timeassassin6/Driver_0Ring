#include "StdAfx.h"
// TypeIndex        : 0x7进程

typedef struct _HANDLE_TABLE
{
    ULONG TableCode;
    ULONG QuotaProcess;
    ULONG UniqueProcessId;
    ULONG HandleLock;
    UQUAD HandleTableList;
    ULONG HandleContentionEvent;
    ULONG DebugInfo;
    ULONG ExtraInfoPages;
    ULONG Flags;
    ULONG StrictFIFO;
    ULONG FirstFreeHandle;
    ULONG LastFreeHandleEntry;
    ULONG HandleCount;
    ULONG NextHandleNeedingPool;
    ULONG HandleCountHighWatermark;
}GlobalHandleTable, * PGlobalHandleTable;

PGlobalHandleTable pHandleTable;

PULONG GetGlobalHandleTableAddress();
PULONG FindPspCidTable();
VOID EnumHandleTable();

PULONG FindPspCidTable()//利用特征码从ntoskrl的导出函数中查找PspCidTable变量
{
    PULONG  CidTableAddr = 0;
    UNICODE_STRING FuncName;
    RtlInitUnicodeString(&FuncName, L"PsLookupProcessByProcessId");
    PUCHAR startAddr = (PUCHAR)MmGetSystemRoutineAddress(&FuncName);
    for (ULONG i = 0; i <= 100; i++)
    {
        if (*(startAddr + i) == 0xFF &&
            *(startAddr + i + 1) == 0x75 &&
            *(startAddr + i + 2) == 0x08)
            //特征码
            //FF75 08           push [ebp+8]
            //8B3D XXXXXXXX     mov edi,PspCidTable
        {
            CidTableAddr = *(PULONG)(startAddr + i + 5);
            DbgPrint("CidTableAddr: % p\n", CidTableAddr);
            break;
        }
    }
    return CidTableAddr;
}

PULONG GetGlobalHandleTableAddress()//得到全局句柄表地址
{
    PULONG PspCidTable = *(PULONG)FindPspCidTable();
    DbgPrint("PspCidTable: % p\n", PspCidTable);
    pHandleTable = (PGlobalHandleTable)PspCidTable;
    DbgPrint("HandleTableAddr: % p\n", pHandleTable->TableCode & 0xFFFFFFFC);
    return (PULONG)(pHandleTable->TableCode & 0xFFFFFFFC);
}

VOID EnumHandleTable()
{
    USHORT count0 = 0, count1 = 0;
    PULONG HandleTableAddress = (PULONG)GetGlobalHandleTableAddress();
    switch (pHandleTable->TableCode & 0x3)//判断句柄表级别
    {
        case 0://0级句柄表
        {
            ULONGLONG HandleTableValue0 = *HandleTableAddress;
            ULONG KernelObjectAddress = (ULONG)(HandleTableValue0 & 0xFFFFFFF8);
            while (HandleTableValue0)
            {
                if(*(PCHAR)KernelObjectAddress -0x0C == 7)
                {
                    DbgPrint("The Program Name is:%s", *(PSTR)KernelObjectAddress + 0x16C);
                }
                HandleTableAddress++;
                HandleTableValue0 = *HandleTableAddress;
                KernelObjectAddress = (ULONG)(HandleTableValue0 & 0xFFFFFFF8);
            }
            break;
        };
        case 1://1级句柄表
        {
            PULONG pHandleTableValue0 = *(PULONG)HandleTableAddress;
            ULONGLONG(*HandleTableValue1)[512];
            HandleTableValue1 = pHandleTableValue0;
            ULONG KernelObjectAddress;

            while (pHandleTableValue0 != 0)
            {
                DbgPrint("The pHandleTableValue0 is:%X\n", pHandleTableValue0);
                DbgPrint("The (*HandleTableValue1)[0] is:%016I64x\n", (*HandleTableValue1)[count0]);
                while (count0 < 512)
                {
                    KernelObjectAddress = (ULONG)((*HandleTableValue1)[count0] & 0xFFFFFFF8);
                    DbgPrint("The KernelObjectAddress is:%08I32x\n", KernelObjectAddress);
                    if (KernelObjectAddress != 0)
                    {
                        DbgPrint("TypeIndex is:%d\n", *(PUCHAR)(KernelObjectAddress - 0x0C));
                        if (*(PUCHAR)(KernelObjectAddress - 0x0C) == (UCHAR)0x07)
                        {

                            DbgPrint("The Program Name is:%s", (PSTR)(KernelObjectAddress + 0x16C));
                            DbgPrint("ok");
                        }
                    }
                    count0++;
                }
                HandleTableAddress++;
                DbgPrint("The HandleTableAddress is:%X\n", HandleTableAddress);
                pHandleTableValue0 = *(PULONG)HandleTableAddress;
                HandleTableValue1 = pHandleTableValue0;
                count0 = 0;
            }
            break;
        };
        case 2://2级句柄表
        {
            PULONG HandleTableValue0 = *(PULONG)HandleTableAddress;
            PULONG HandleTableValue1 = (PULONG)*HandleTableValue0;
            ULONGLONG HandleTableValue2 = *HandleTableValue1;
            ULONG KernelObjectAddress = (ULONG)(HandleTableValue2 & 0xFFFFFFF8);
            while (HandleTableValue0!=0)
            {
                while (HandleTableValue1 && count0 < 1024)
                {
                    while (HandleTableValue2 && count1 < 512)
                    {
                        if (*(PCHAR)KernelObjectAddress - 0x0C == 7)
                        {
                            DbgPrint("The Program Name is:%s", *(PSTR)KernelObjectAddress + 0x16C);
                        }
                        HandleTableValue2++;
                        count1++;
                        KernelObjectAddress = (ULONG)(HandleTableValue2 & 0xFFFFFFF8);
                    }
                    HandleTableValue0++;
                    HandleTableValue1 = HandleTableValue0;
                    count0++;
                }
                HandleTableValue0++;
                HandleTableValue1 = (PULONG)*HandleTableValue0;
            }
            break;
        };
    }
}
