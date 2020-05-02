
#pragma pack(1)
typedef struct _IDTR
{
	USHORT limit;
	ULONG64 Base;
}IDTR, *PIDTR;

typedef union _IDT_ENTRY
{
	struct kidt
	{
		USHORT OffsetLow;
		USHORT Selector;
		USHORT IstIndex : 3;
		USHORT Reserved0 : 5;
		USHORT Type : 5;
		USHORT Dpl : 2;
		USHORT Present : 1;
		USHORT OffsetMiddle;
		ULONG OffsetHigh;
		ULONG Reserved1;
	}idt;
	UINT64 Alignment;
}IDT_ENTRY, *PIDT_ENTRY;
#pragma pack()

extern void HookTrap0E();
extern void Trap0E_Ori();
extern ULONG_PTR OldTrap0E;
extern ULONG_PTR Trap0E_RET;