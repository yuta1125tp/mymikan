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
    ret; RAXレジスタの値が戻り値になる。RAXの下32bitがEAX[ref](みかん本図3.2の71p)なので、RAXを返せばEAXを64bitで返すことに相当、上32bitの初期値は気にしなくて良い？

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
SetDSAll: ; 各セグメントレジスタ(ds, es, fs, gs)に引数の値(0)をコピー。ヌルディスクリプタを指すようにするのが目的（みかん本の193p）
    mov ds, di
    mov es, di
    mov fs, di
    mov gs, di
    ret

global SetCR3; void SetCR3(uint64_t value);
SetCR3: ; 与えられたPML4テーブルの物理アドレスをCR3レジスタに設定する。
    mov cr3, rdi
    ret

global GetCR3 ; uint64_t GetCR3();
GetCR3: ; 現在のcr3を返す
    mov rax, cr3
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

global SwitchContext ; void SwitchContext(void* next_ctx, void* current_ctx);
SwitchContext: ; RDI, RSIで受けたctxを交換する みかん本316から319p
    mov [rsi + 0x40], rax
    mov [rsi + 0x48], rbx
    mov [rsi + 0x50], rcx
    mov [rsi + 0x58], rdx
    mov [rsi + 0x60], rdi
    mov [rsi + 0x68], rsi

    lea rax, [rsp + 8]
    mov [rsi + 0x70], rax  ; RSP
    mov [rsi + 0x78], rbp

    mov [rsi + 0x80], r8
    mov [rsi + 0x88], r9
    mov [rsi + 0x90], r10
    mov [rsi + 0x98], r11
    mov [rsi + 0xa0], r12
    mov [rsi + 0xa8], r13
    mov [rsi + 0xb0], r14
    mov [rsi + 0xb8], r15

    mov rax, cr3
    mov [rsi + 0x00], rax  ; CR3
    mov rax, [rsp]
    mov [rsi + 0x08], rax  ; RIP
    pushfq
    pop qword [rsi + 0x10] ; RFLAGS

    mov ax, cs
    mov [rsi + 0x20], rax
    mov bx, ss
    mov [rsi + 0x28], rbx
    mov cx, fs
    mov [rsi + 0x30], rcx
    mov dx, gs
    mov [rsi + 0x38], rdx

    fxsave [rsi + 0xc0]

    ; iret 用のスタックフレーム
    push qword [rdi + 0x28] ; SS
    push qword [rdi + 0x70] ; RSP
    push qword [rdi + 0x10] ; RFLAGS
    push qword [rdi + 0x20] ; CS
    push qword [rdi + 0x08] ; RIP

    ; コンテキストの復帰
    fxrstor [rdi + 0xc0]

    mov rax, [rdi + 0x00]
    mov cr3, rax
    mov rax, [rdi + 0x30]
    mov fs, ax
    mov rax, [rdi + 0x38]
    mov gs, ax

    mov rax, [rdi + 0x40]
    mov rbx, [rdi + 0x48]
    mov rcx, [rdi + 0x50]
    mov rdx, [rdi + 0x58]
    mov rsi, [rdi + 0x68]
    mov rbp, [rdi + 0x78]
    mov r8,  [rdi + 0x80]
    mov r9,  [rdi + 0x88]
    mov r10, [rdi + 0x90]
    mov r11, [rdi + 0x98]
    mov r12, [rdi + 0xa0]
    mov r13, [rdi + 0xa8]
    mov r14, [rdi + 0xb0]
    mov r15, [rdi + 0xb8]

    mov rdi, [rdi + 0x60]

    o64 iret
; #@@range_end(switch_context)
