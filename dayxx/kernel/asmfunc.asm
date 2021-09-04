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

global GetCS;uint16_t GetCS(void);
GetCS:
    xor eax, eax; also clears upper 32 bits of rax
    mov ax, cs
    ret

global LoadIDT; void LoadIDT(uint16_t limit, uint64_t offset);
LoadIDT:
    push rbp
    mov rbp, rsp
    sub rsp, 10; スタック上に10バイトのメモリ領域を確保し続く2行で値を書き込む。
    mov [rsp], di;第1引数limitはRDIレジスタに格納、それの下位16bitを示すDIレジスタを使っている。
    mov [rsp+2], rsi; offset
    lidt [rsp]
    mov rsp, rbp
    pop rbp
    ret

global LoadGDT ; void LoadGDT(uint16_t limit, uint64_t offset);
LoadGDT:
    push rbp
    mov rbp, rsp
    sub rsp, 10; スタックに10バイト(80bit)の空き容量を確保 
    mov [rsp], di; limit を書き込む (16bit)
    mov [rsp+2], rsi; offsetを書き込む (64bit)
    lgdt [rsp] ; 10バイトのメモリ領域を受け取りGDTRレジスタに登録する
    mov rsp, rbp
    pop rbp
    ret

global SetCSSS; void SetCSSS(uint16_t cs, uint16_t ss);
SetCSSS:; Code Segmentレジスタ(CS) Stack Segmentレジスタ(SS)に値を設定する
    push rbp
    mov rbp, rsp
    mov ss, si ; ssにsi(第2引数)を設定
    mov rax, .next
    push rdi ; スタックにCSへ代入したい値を積む
    push rax ; スタックにRIPへ代入したい値（戻り先のアドレス）を積む、積んだ値はretfでRIPとCSに設定される
    o64 retf ; CSにはmovで値を設定できないのでretfを代用。far call後のようなスタック状況を作ってretfを呼びCSに所望の値を代入する[ref](みかん本194p)
.next:
    mov rsp, rbp
    pop rbp
    ret

global SetDSAll; void SetDSAll(uint16_t value);
SetDSAll: ; 各セグメントレジスタ(ds, es, fs, gs)に引数の値(0)をコピー。ヌルディスクリプタを指すようにするのが目的（みかん本@193p）
    mov ds, di
    mov es, di
    mov fs, di
    mov gs, di
    ret

global SetCR3; void SetCR3(uint64_t value);
SetCR3: ; 与えられたPML4テーブルの物理アドレスをCR3レジスタに設定する。
    mov cr3, rdi
    ret

extern kernel_main_stack
extern KernelMainNewStack

global KernelMain
KernelMain:
    mov rsp, kernel_main_stack+1024*1024
    call KernelMainNewStack
.fin:
    hlt
    jmp .fin
