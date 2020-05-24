#include "StdAfx.h"

#define DEVICE_NAME L"\\Device\\basic_nt"
#define SYMBOLIC_LINK_NAME L"\\??\\basic_nt"

#define OPER1 CTL_CODE(FILE_DEVICE_UNKNOWN,0x800,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define OPER2 CTL_CODE(FILE_DEVICE_UNKNOWN,0x900,METHOD_BUFFERED,FILE_ANY_ACCESS)

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegPath);
VOID DriverUnload(PDRIVER_OBJECT pDriverObject);

NTSTATUS DispatchCommon(PDEVICE_OBJECT pDeviceObject, PIRP pIrp);
NTSTATUS DispatchCreate(PDEVICE_OBJECT pDeviceObject, PIRP pIrp);
NTSTATUS DispatchRead(PDEVICE_OBJECT pDeviceObject, PIRP pIrp);
NTSTATUS DispatchWrite(PDEVICE_OBJECT pDeviceObject, PIRP pIrp);
NTSTATUS DispatchIoDeviceControl(PDEVICE_OBJECT pDeviceObject, PIRP pIrp);
NTSTATUS DispatchCleanup(PDEVICE_OBJECT pDeviceObject, PIRP pIrp);
NTSTATUS DispatchClose(PDEVICE_OBJECT pDeviceObject, PIRP pIrp);


NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegPath)
{
    UNREFERENCED_PARAMETER(pRegPath);
    NTSTATUS ntStatus = 0;
    UNICODE_STRING uDeviceName = { 0 };     // 设备名
    UNICODE_STRING uSymbolicLinkName = { 0 };       // 符号链接名

    // 初始化字符串
    RtlInitUnicodeString(&uDeviceName, DEVICE_NAME);
    RtlInitUnicodeString(&uSymbolicLinkName, SYMBOLIC_LINK_NAME);

    // 创建设备对象
    PDEVICE_OBJECT pDeviceObject = NULL;
    ntStatus = IoCreateDevice(
        pDriverObject,          // 驱动对象
        0,
        &uDeviceName,           // 设备名
        FILE_DEVICE_UNKNOWN,    // 设备类型
        0,
        FALSE,
        &pDeviceObject);        // 设备对象指针
    if (!NT_SUCCESS(ntStatus))
    {
        DbgPrint("创建设备失败!\n status:%x", ntStatus);
        return ntStatus;
    }

    // 设置通讯方式
    pDeviceObject->Flags |= DO_BUFFERED_IO;     // 系统缓冲方式

    // 创建符号链接
    ntStatus = IoCreateSymbolicLink(&uSymbolicLinkName, &uDeviceName);
    if (!NT_SUCCESS(ntStatus))
    {
        IoDeleteDevice(pDeviceObject);
        DbgPrint("创建符号链接失败! ntStatus:%X\n", ntStatus);
        return ntStatus;
    }

    // 设置派遣函数
    for (size_t i = 0; i < IRP_MJ_MAXIMUM_FUNCTION + 1; i++)
    {
        pDriverObject->MajorFunction[i] = DispatchCommon;
    }
    pDriverObject->MajorFunction[IRP_MJ_CREATE] = DispatchCreate;
    pDriverObject->MajorFunction[IRP_MJ_READ] = DispatchRead;
    pDriverObject->MajorFunction[IRP_MJ_WRITE] = DispatchWrite;
    pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchIoDeviceControl;
    pDriverObject->MajorFunction[IRP_MJ_CLEANUP] = DispatchCleanup;
    pDriverObject->MajorFunction[IRP_MJ_CLOSE] = DispatchClose;

    // 设置卸载函数
    pDriverObject->DriverUnload = DriverUnload;

    DbgPrint("Driver Loaded! \n");

    return STATUS_SUCCESS;
}

VOID DriverUnload(PDRIVER_OBJECT pDriverObject)
{
    UNICODE_STRING uLinkName = { 0 };   // 符号链接名

    // 初始化字符串
    RtlInitUnicodeString(&uLinkName, SYMBOLIC_LINK_NAME);

    // 删除符号链接
    IoDeleteSymbolicLink(&uLinkName);
    // 删除设备对象
    IoDeleteDevice(pDriverObject->DeviceObject);

    DbgPrint("Driver Unloaded! \n");
}

NTSTATUS DispatchCommon(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
    UNREFERENCED_PARAMETER(pDeviceObject);

    pIrp->IoStatus.Status = STATUS_SUCCESS;
    pIrp->IoStatus.Information = 0;

    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS DispatchCreate(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
    UNREFERENCED_PARAMETER(pDeviceObject);

    DbgPrint("DispatchCreate!\n");

    pIrp->IoStatus.Status = STATUS_SUCCESS;
    pIrp->IoStatus.Information = 0;

    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS DispatchRead(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
    UNREFERENCED_PARAMETER(pDeviceObject);

    DbgPrint("DispatchRead\n");

    PIO_STACK_LOCATION pStack = IoGetCurrentIrpStackLocation(pIrp);
    PVOID pBuffer = pIrp->AssociatedIrp.SystemBuffer;       // 系统Buffer
    ULONG uReadLength = pStack->Parameters.Read.Length;     // 读取长度


    // 将数据写入到Buffer
    UCHAR pSrouce[] = { 7,7,7,7,8,8,8,8,8 };
    ULONG uOutputSize = uReadLength < sizeof(pSrouce) ? uReadLength : sizeof(pSrouce);
    RtlMoveMemory(pBuffer, pSrouce, uOutputSize);

    pIrp->IoStatus.Status = STATUS_SUCCESS;
    pIrp->IoStatus.Information = uOutputSize;       // 输出长度
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS DispatchWrite(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{

    UNREFERENCED_PARAMETER(pDeviceObject);

    DbgPrint("DispatchWrite\n");

    PVOID pWriteBuffer = pIrp->AssociatedIrp.SystemBuffer;          // 系统Buffer
    PIO_STACK_LOCATION pStack = IoGetCurrentIrpStackLocation(pIrp);

    ULONG uWriteLength = pStack->Parameters.Write.Length;           // 写入长度

    for (size_t i = 0; i < uWriteLength; i++)
    {
        DbgPrint("%02X ", ((PUCHAR)pWriteBuffer)[i]);
    }
    DbgPrint("\n");

    pIrp->IoStatus.Status = STATUS_SUCCESS;
    pIrp->IoStatus.Information = uWriteLength;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}


NTSTATUS DispatchClose(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
    UNREFERENCED_PARAMETER(pDeviceObject);

    DbgPrint("DispatchClose\n");

    pIrp->IoStatus.Status = STATUS_SUCCESS;
    pIrp->IoStatus.Information = 0;

    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}


NTSTATUS DispatchIoDeviceControl(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
    UNREFERENCED_PARAMETER(pDeviceObject);

    DbgPrint("DispatchDeivceControl\n");

    PIO_STACK_LOCATION pStack = IoGetCurrentIrpStackLocation(pIrp);

    PVOID pSystemBuffer = pIrp->AssociatedIrp.SystemBuffer;     // 获取缓冲区(输入输出的缓冲区一致.)

    ULONG uIoControlCode = pStack->Parameters.DeviceIoControl.IoControlCode;    // 控制码
    ULONG uInLength = pStack->Parameters.DeviceIoControl.InputBufferLength;     // Ring3输入长度
    ULONG uOutLength = pStack->Parameters.DeviceIoControl.OutputBufferLength;   // Ring3用于接收输出数据的Buffer大小

    switch (uIoControlCode)
    {
    case OPER1:
        DbgPrint("DeviceControl OPER1  \n");

        pIrp->IoStatus.Information = 0;
        pIrp->IoStatus.Status = STATUS_SUCCESS;

        break;

    case OPER2:
    {
        DbgPrint("DeviceControl OPER2 InputLength: %d \n", uInLength);
        DbgPrint("DeviceControl OPER2 OutputLength: %d \n", uOutLength);

        // 读取Buffer中的数据, 并打印
        for (ULONG i = 0; i < uInLength; i++)
        {
            DbgPrint("%02X ", ((PUCHAR)pSystemBuffer)[i]);
        }
        DbgPrint("\n");

        ULONG var = 0x12345678;

        ULONG min = uOutLength < sizeof(var) ? uOutLength : sizeof(var);
        pIrp->IoStatus.Information = min;   // 实际输出长度
        pIrp->IoStatus.Status = STATUS_SUCCESS;

        break;
    }
    default:
    {
        pIrp->IoStatus.Information = 0;
        pIrp->IoStatus.Status = -1;
        break;
    }
    }

    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}


NTSTATUS DispatchCleanup(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
    UNREFERENCED_PARAMETER(pDeviceObject);

    DbgPrint("DispatchCleanup\n");

    pIrp->IoStatus.Status = STATUS_SUCCESS;
    pIrp->IoStatus.Information = 0;

    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}


