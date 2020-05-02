/*
	IDT HOOK ----By:kanren
	PS:处理函数自己写，只是写个框架
	注意事项:
	1.IDTR与IDTENTRY的结构体，WRK中有写，x86与x64不同，需注意结构体字节对齐 详细原因自行调试
	2.IDTENTRY属于只读页面，需Cr0 WP位置0才可以写入
	3.IDTENTRY中的成员说明:OffsetHigh(地址的高32位), OffsetMiddle(地址低32位的高16位), OffsetLow(地址的低16位)
	4.sidt获取当前CPU核心的IDTR,多核处理代码已经在下面了
	5.IDT属于PG保护范围，若想安全使用，自行准备PassPG
	6.编译环境:Win10 10.0.15063 SDK WDK10.0 VS2015
	源码仅供参考，如有问题，给我留言
*/

#include <ntifs.h>
#include <windef.h>
#include "Tools.h"

#define oprintf(...) DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, __VA_ARGS__)

IDTR idtr;
ULONG_PTR OldTrap0E = 0;
ULONG_PTR Trap0E_RET = 0;
VOID DriverUnload(IN PDRIVER_OBJECT pDriverObj)
{
	DbgPrint("DriverUnload\n");
}

KIRQL WPOFFx64()
{
	KIRQL  irql = KeRaiseIrqlToDpcLevel();
	UINT64  cr0 = __readcr0();
	cr0 &= 0xfffffffffffeffff;
	__writecr0(cr0);
	_disable();
	return  irql;
}

void WPONx64(KIRQL irql)
{
	UINT64 cr0 = __readcr0();
	cr0 |= 0x10000;
	_enable();
	__writecr0(cr0);
	KeLowerIrql(irql);
}



ULONG64 GetIdtAddr(ULONG64 IdtBaseAddr, UCHAR Index)
{
	PIDT_ENTRY Pidt = (PIDT_ENTRY)(IdtBaseAddr);
	Pidt = Pidt + Index;
	ULONG64 OffsetHigh, OffsetMiddle, OffsetLow, ret;

	OffsetHigh = Pidt->idt.OffsetHigh;
	OffsetHigh = OffsetHigh << 32;

	OffsetMiddle = Pidt->idt.OffsetMiddle;
	OffsetMiddle = OffsetMiddle << 16;



	OffsetLow = Pidt->idt.OffsetLow;
	ret = OffsetHigh + OffsetMiddle + OffsetLow;
	return ret;
}

ULONG64 SetIdtAddr(ULONG64 IdtBaseAddr, UCHAR Index, ULONG64 NewAddr)
{
	PIDT_ENTRY Pidt = (PIDT_ENTRY)(IdtBaseAddr);
	Pidt = Pidt + Index;
	ULONG64 OffsetHigh, OffsetMiddle, OffsetLow, ret;


	OffsetHigh = Pidt->idt.OffsetHigh;
	OffsetHigh = OffsetHigh << 32;
	OffsetMiddle = Pidt->idt.OffsetMiddle;
	OffsetMiddle = OffsetMiddle << 16;

	OffsetLow = Pidt->idt.OffsetLow;
	ret = OffsetHigh + OffsetMiddle + OffsetLow;
	Pidt->idt.OffsetHigh = NewAddr >> 32;
	Pidt->idt.OffsetMiddle = NewAddr << 32 >> 48;
	Pidt->idt.OffsetLow = NewAddr & 0xFFFF;
	return ret;
}

VOID HookJmp(UINT64 Addr, PVOID JmpAddr){

	*(UINT16*)Addr = 0xFEEB;//jmp self
	*(UINT32*)(Addr + 2) = 0;
	*(PVOID*)(Addr + 6) = JmpAddr;
	*(UINT16*)Addr = 0x25FF;
}

NTSTATUS DriverEntry(IN PDRIVER_OBJECT pDriverObj, IN PUNICODE_STRING pRegistryString)
{	
	oprintf("DriverEntry\n");
	pDriverObj->DriverUnload = DriverUnload;

	__sidt(&idtr);
	oprintf("IDT Base:%llX\n", idtr.Base);
	oprintf("Old IDT[0xE]:%llX\n", GetIdtAddr(idtr.Base, 0xE));

	OldTrap0E = GetIdtAddr(idtr.Base, 0xE);

	

	//inline hook 0E 

	/*Trap0E_RET = OldTrap0E + 0x10;
	KIRQL IRQL = WPOFFx64();
	HookJmp(OldTrap0E, Trap0E_Ori);
	WPONx64(IRQL);*/
	


	for (int i = 0; i < KeNumberProcessors; i++){//IDT HOOK配合shark大约在12-24小时左右收获蓝屏套餐，测试环境虚拟机win10 1709

		KeSetSystemAffinityThread((KAFFINITY)(1 << i));
		__sidt(&idtr);
		oprintf("CPU[%d] IDT Base:%llX\n", i, idtr.Base);
		KIRQL IRQL = WPOFFx64();
		//oprintf("CPU[%d] Old IDT[0xEA]:%llX\n", i, GetIdtAddr(idtr.Base, 0xE));

		oprintf("CPU[%d] Old IDT[0xE]:%llX\n", i, SetIdtAddr(idtr.Base, 0xE, (ULONG64)HookTrap0E));
		WPONx64(IRQL);
		KeRevertToUserAffinityThread();
	}

	return STATUS_SUCCESS;
}