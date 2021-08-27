; asmfunc.asm
;
; System V AMD64 Calling Convention
; Registers: 引数はRDI, RSI, RDX, RCX, R8, R9の襦袢に割り当てられる。

bits 64
section .text

global IoOut32; void IoOut32(uint16_t addr, uint32_t data);
IoOut32:
    mov dx, di; dx = addr
    mov eax, esi; eax = data
    out dx, eax
    ret

global IoIn32 ; uint 32_t IoIn32(uint16_t addr);
IoIn32:
    mov dx, di; dx=addr
    in eax, dx; DXに設定されたIOポートアドレスから32bit整数を入力してEAXに設定する。
    ret; RAXレジスタの値が戻り値になる。RAXの下32bitがEAX[ref](みかん本図3.2@71p)なので、RAXを返せばEAXを64bitで返すことに相当、上32bitの初期値は気にしなくて良い？
