These are the ABI conventions for 64 bit Windows and Linux. Where there is variance among compilers, the rule for GNU is written and this noted. It concerns only integral (inc. enum), pointer, structure and array types.

http://www.agner.org/optimize/calling_conventions.pdf

Todo:
- Type sizes
- Alignment (in and out of structures, stack)
- Red zone/shadow space
- Structure param passing/returning
- Mangling
- Exceptions
- Static constructors

Callee save
- w: rbx, rbp, r12-15, rsi, rdi
- l: rbx, rbp, r12-15
	
Params (in order)
- w: rcx, rdx
- l: rdi, rsi, rdx, rcx, r8, r9

Return
- w: rax
- l: rax, rdx (!!WHAT DOES THIS MEAN!!)

Caller stack cleanup

Right to left param order
