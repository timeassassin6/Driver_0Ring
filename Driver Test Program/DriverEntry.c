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
    UNICODE_STRING uDeviceName = { 0 };     // �豸��
    UNICODE_STRING uSymbolicLinkName = { 0 };       // ����������

    // ��ʼ���ַ���
    RtlInitUnicodeString(&uDeviceName, DEVICE_NAME);
    RtlInitUnicodeString(&uSymbolicLinkName, SYMBOLIC_LINK_NAME);

    // �����豸����
    PDEVICE_OBJECT pDeviceObject = NULL;
    ntStatus = IoCreateDevice(
        pDriverObject,          // ��������
        0,
        &uDeviceName,           // �豸��
        FILE_DEVICE_UNKNOWN,    // �豸����
        0,
        FALSE,
        &pDeviceObject);        // �豸����ָ��
    if (!NT_SUCCESS(ntStatus))
    {
        DbgPrint("�����豸ʧ��!\n status:%x", ntStatus);
        return ntStatus;
    }

    // ����ͨѶ��ʽ
    pDeviceObject->Flags |= DO_BUFFERED_IO;     // ϵͳ���巽ʽ

    // ������������
    ntStatus = IoCreateSymbolicLink(&uSymbolicLinkName, &uDeviceName);
    if (!NT_SUCCESS(ntStatus))
    {
        IoDeleteDevice(pDeviceObject);
        DbgPrint("������������ʧ��! ntStatus:%X\n", ntStatus);
        return ntStatus;
    }

    // ������ǲ����
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

    // ����ж�غ���
    pDriverObject->DriverUnload = DriverUnload;

    DbgPrint("Driver Loaded! \n");

    return STATUS_SUCCESS;
}

VOID DriverUnload(PDRIVER_OBJECT pDriverObject)
{
    UNICODE_STRING uLinkName = { 0 };   // ����������

    // ��ʼ���ַ���
    RtlInitUnicodeString(&uLinkName, SYMBOLIC_LINK_NAME);

    // ɾ����������
    IoDeleteSymbolicLink(&uLinkName);
    // ɾ���豸����
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
    PVOID pBuffer = pIrp->AssociatedIrp.SystemBuffer;       // ϵͳBuffer
    ULONG uReadLength = pStack->Parameters.Read.Length;     // ��ȡ����


    // ������д�뵽Buffer
    UCHAR pSrouce[] = { 7,7,7,7,8,8,8,8,8 };
    ULONG uOutputSize = uReadLength < sizeof(pSrouce) ? uReadLength : sizeof(pSrouce);
    RtlMoveMemory(pBuffer, pSrouce, uOutputSize);

    pIrp->IoStatus.Status = STATUS_SUCCESS;
    pIrp->IoStatus.Information = uOutputSize;       // �������
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS DispatchWrite(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{

    UNREFERENCED_PARAMETER(pDeviceObject);

    DbgPrint("DispatchWrite\n");

    PVOID pWriteBuffer = pIrp->AssociatedIrp.SystemBuffer;          // ϵͳBuffer
    PIO_STACK_LOCATION pStack = IoGetCurrentIrpStackLocation(pIrp);

    ULONG uWriteLength = pStack->Parameters.Write.Length;           // д�볤��

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

    PVOID pSystemBuffer = pIrp->AssociatedIrp.SystemBuffer;     // ��ȡ������(��������Ļ�����һ��.)

    ULONG uIoControlCode = pStack->Parameters.DeviceIoControl.IoControlCode;    // ������
    ULONG uInLength = pStack->Parameters.DeviceIoControl.InputBufferLength;     // Ring3���볤��
    ULONG uOutLength = pStack->Parameters.DeviceIoControl.OutputBufferLength;   // Ring3���ڽ���������ݵ�Buffer��С

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

        // ��ȡBuffer�е�����, ����ӡ
        for (ULONG i = 0; i < uInLength; i++)
        {
            DbgPrint("%02X ", ((PUCHAR)pSystemBuffer)[i]);
        }
        DbgPrint("\n");

        ULONG var = 0x12345678;

        ULONG min = uOutLength < sizeof(var) ? uOutLength : sizeof(var);
        pIrp->IoStatus.Information = min;   // ʵ���������
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


