extern scanf
extern printf
extern pow
extern sqrt


section .data


BUFFER: times 100 dq 0
PRINT: db "%d", 10, 0
SCAN:  db "%d"


section .text


global main


main:


	mov rsi, BUFFER
	xor r14, r14
	xor r15, r15


	mov rax, 0
	mov [rsi + 0 * 8], rax


	mov rax, 0
	push rax


	mov rax, 1
	push rax


	pop rbx
	pop rax
	sub rax, rbx
	push rax

	pop rax
	mov [rsi + 0 * 8], rax


	sub rsp, 24
	mov qword [rsp + 8], 0
	lea rsi, [rsp + 8]
	mov rdi, SCAN
	mov rax, 0
	call scanf
	mov rsi, BUFFER
	mov rax, qword [rsp + 8] 
	add rsp, 24
	push rax


	inc r15


	pop rax
	mov [rsi + (r14 + 1 + 0) * 8], rax


	sub rsp, 24
	mov qword [rsp + 8], 0
	lea rsi, [rsp + 8]
	mov rdi, SCAN
	mov rax, 0
	call scanf
	mov rsi, BUFFER
	mov rax, qword [rsp + 8] 
	add rsp, 24
	push rax


	inc r15


	pop rax
	mov [rsi + (r14 + 1 + 1) * 8], rax


	sub rsp, 24
	mov qword [rsp + 8], 0
	lea rsi, [rsp + 8]
	mov rdi, SCAN
	mov rax, 0
	call scanf
	mov rsi, BUFFER
	mov rax, qword [rsp + 8] 
	add rsp, 24
	push rax


	inc r15


	pop rax
	mov [rsi + (r14 + 1 + 2) * 8], rax


	mov rax, 0
	push rax


	inc r15


	pop rax
	mov [rsi + (r14 + 1 + 3) * 8], rax


	mov rax, 0
	push rax


	inc r15


	pop rax
	mov [rsi + (r14 + 1 + 4) * 8], rax


	mov rax, 0
	push rax


	inc r15


	pop rax
	mov [rsi + (r14 + 1 + 5) * 8], rax


	mov rax, [rsi + (r14 + 1 + 0) * 8]
	push rax


	mov rax, 0
	push rax


	pop rbx
	pop rax
	cmp rax, rbx
	je .next0


	jmp .next1


.next0:


	push r14
	push r15


	mov rax, [rsi + (r14 + 1 + 2) * 8]
	push rax


	mov rax, [rsi + (r14 + 1 + 1) * 8]
	push rax


	add r14, r15
	call f
	pop r15
	pop r14
	push rax


	pop rax
	mov [rsi + (r14 + 1 + 4) * 8], rax


	mov rax, [rsi + (r14 + 1 + 4) * 8]
	push rax


	mov rax, 0
	push rax


	mov rax, 1
	push rax


	pop rbx
	pop rax
	sub rax, rbx
	push rax


	pop rbx
	pop rax
	cmp rax, rbx
	je .next2


	jmp .next3


.next2:


	mov rdi, PRINT


	mov rax, 0
	push rax


	mov rax, 1
	push rax


	pop rbx
	pop rax
	sub rax, rbx
	push rax


	pop rax
	mov rsi, rax
	mov rax, 0
	call printf
	mov rsi, BUFFER


	ret


.next3:


	mov rax, [rsi + (r14 + 1 + 4) * 8]
	push rax


	mov rax, 0
	push rax


	pop rbx
	pop rax
	cmp rax, rbx
	je .next4


	jmp .next5


.next4:


	mov rdi, PRINT


	mov rax, 0
	push rax


	pop rax
	mov rsi, rax
	mov rax, 0
	call printf
	mov rsi, BUFFER


	ret


.next5:


	mov rdi, PRINT


	mov rax, 1
	push rax


	pop rax
	mov rsi, rax
	mov rax, 0
	call printf
	mov rsi, BUFFER


	mov rdi, PRINT


	mov rax, [rsi + (r14 + 1 + 4) * 8]
	push rax


	pop rax
	mov rsi, rax
	mov rax, 0
	call printf
	mov rsi, BUFFER


	ret


.next1:


	mov rax, [rsi + (r14 + 1 + 1) * 8]
	push rax


	mov rax, 2
	push rax


	pop rbx
	pop rax
	cvtsi2sd xmm0, eax
	cvtsi2sd xmm1, ebx
	call pow
	mov rsi, BUFFER
	cvttsd2si eax, xmm0
	push rax


	mov rax, 4
	push rax


	mov rax, [rsi + (r14 + 1 + 0) * 8]
	push rax


	pop rbx
	pop rax
	mul ebx
	push rax


	mov rax, [rsi + (r14 + 1 + 2) * 8]
	push rax


	pop rbx
	pop rax
	mul ebx
	push rax


	pop rbx
	pop rax
	sub rax, rbx
	push rax


	pop rax
	mov [rsi + (r14 + 1 + 3) * 8], rax


	mov rax, [rsi + (r14 + 1 + 3) * 8]
	push rax


	mov rax, 0
	push rax


	pop rbx
	pop rax
	cmp rax, rbx
	jl .next6


	jmp .next7


.next6:


	mov rdi, PRINT


	mov rax, 0
	push rax


	pop rax
	mov rsi, rax
	mov rax, 0
	call printf
	mov rsi, BUFFER


	ret


.next7:


	mov rax, [rsi + (r14 + 1 + 3) * 8]
	push rax


	mov rax, 0
	push rax


	pop rbx
	pop rax
	cmp rax, rbx
	je .next8


	jmp .next9


.next8:


	mov rax, 0
	push rax


	mov rax, [rsi + (r14 + 1 + 1) * 8]
	push rax


	pop rbx
	pop rax
	sub rax, rbx
	push rax


	mov rax, 2
	push rax


	mov rax, [rsi + (r14 + 1 + 0) * 8]
	push rax


	pop rbx
	pop rax
	mul ebx
	push rax


	pop rbx
	pop rax
	cdq
	idiv ebx
	push rax


	pop rax
	mov [rsi + (r14 + 1 + 4) * 8], rax


	mov rdi, PRINT


	mov rax, 1
	push rax


	pop rax
	mov rsi, rax
	mov rax, 0
	call printf
	mov rsi, BUFFER


	mov rdi, PRINT


	mov rax, [rsi + (r14 + 1 + 4) * 8]
	push rax


	pop rax
	mov rsi, rax
	mov rax, 0
	call printf
	mov rsi, BUFFER


	ret


.next9:


	mov rax, [rsi + (r14 + 1 + 3) * 8]
	push rax


	mov rax, 0
	push rax


	pop rbx
	pop rax
	cmp rax, rbx
	jg .next10


	jmp .next11


.next10:


	mov rax, [rsi + (r14 + 1 + 3) * 8]
	push rax


	pop rax
	cvtsi2sd xmm0, eax
	call sqrt
	cvttsd2si eax, xmm0
	push rax


	pop rax
	mov [rsi + (r14 + 1 + 3) * 8], rax


	mov rax, 0
	push rax


	mov rax, [rsi + (r14 + 1 + 1) * 8]
	push rax


	pop rbx
	pop rax
	sub rax, rbx
	push rax


	mov rax, [rsi + (r14 + 1 + 3) * 8]
	push rax


	pop rbx
	pop rax
	add rax, rbx
	push rax


	mov rax, 2
	push rax


	mov rax, [rsi + (r14 + 1 + 0) * 8]
	push rax


	pop rbx
	pop rax
	mul ebx
	push rax


	pop rbx
	pop rax
	cdq
	idiv ebx
	push rax


	pop rax
	mov [rsi + (r14 + 1 + 4) * 8], rax


	mov rax, 0
	push rax


	mov rax, [rsi + (r14 + 1 + 1) * 8]
	push rax


	pop rbx
	pop rax
	sub rax, rbx
	push rax


	mov rax, [rsi + (r14 + 1 + 3) * 8]
	push rax


	pop rbx
	pop rax
	sub rax, rbx
	push rax


	mov rax, 2
	push rax


	mov rax, [rsi + (r14 + 1 + 0) * 8]
	push rax


	pop rbx
	pop rax
	mul ebx
	push rax


	pop rbx
	pop rax
	cdq
	idiv ebx
	push rax


	pop rax
	mov [rsi + (r14 + 1 + 5) * 8], rax


	mov rdi, PRINT


	mov rax, 2
	push rax


	pop rax
	mov rsi, rax
	mov rax, 0
	call printf
	mov rsi, BUFFER


	mov rdi, PRINT


	mov rax, [rsi + (r14 + 1 + 4) * 8]
	push rax


	pop rax
	mov rsi, rax
	mov rax, 0
	call printf
	mov rsi, BUFFER


	mov rdi, PRINT


	mov rax, [rsi + (r14 + 1 + 5) * 8]
	push rax


	pop rax
	mov rsi, rax
	mov rax, 0
	call printf
	mov rsi, BUFFER


	ret


.next11:


	ret


f:


	pop rcx
	xor r15, r15


	pop rax
	pop rbx


	inc r15


	mov [rsi + (r14 + 1 + 0) * 8], rbx


	inc r15


	mov [rsi + (r14 + 1 + 1) * 8], rax


	push rcx


	mov rax, [rsi + (r14 + 1 + 1) * 8]
	push rax


	mov rax, 0
	push rax


	pop rbx
	pop rax
	cmp rax, rbx
	je .next12


	mov rax, [rsi + 0 * 8]

	push rax


	mov rax, [rsi + (r14 + 1 + 0) * 8]
	push rax


	pop rbx
	pop rax
	mul ebx
	push rax


	mov rax, [rsi + (r14 + 1 + 1) * 8]
	push rax


	pop rbx
	pop rax
	cdq
	idiv ebx
	push rax


	pop rax
	ret


	jmp .next13


.next12:


	mov rax, [rsi + (r14 + 1 + 0) * 8]
	push rax


	mov rax, 0
	push rax


	pop rbx
	pop rax
	cmp rax, rbx
	je .next14


	mov rax, 0
	push rax


	pop rax
	ret


	jmp .next15


.next14:


	mov rax, 0
	push rax


	mov rax, 1
	push rax


	pop rbx
	pop rax
	sub rax, rbx
	push rax


	pop rax
	ret


.next15:


.next13:


times 2055 nop

