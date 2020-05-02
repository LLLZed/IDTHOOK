include macamd64.inc


EXTERN OldTrap0E:DQ;
EXTERN Trap0E_RET:DQ;


.Code

ReadMSR	Proc
	rdmsr	; MSR[ecx] --> edx:eax
	shl	rdx, 32
	or	rax, rdx
	ret
ReadMSR	EndP

DisableWP 	Proc
		cli
		mov	rax,cr0
		push	rax
		and	rax, 0FFFEFFFFh
		mov	cr0,rax
		pop	rax
		ret
DisableWP 	EndP

RestoreCR0 	Proc
		mov	cr0,rcx
		sti
		ret
RestoreCR0 	EndP

__swapgs PROC
		swapgs
		ret
__swapgs ENDP

HookTrap0E PROC


       JMP OldTrap0E
       
	
HookTrap0E ENDP


Trap0E_Ori PROC
		PUSH RBP
		SUB RSP, 158h
		LEA RBP, QWORD PTR [RSP+80h]
		JMP Trap0E_RET
Trap0E_Ori ENDP

End