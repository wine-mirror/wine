	add	$8,%esp
	popw	%es
	add	$2,%esp
	popw	%ds
	add	$2,%esp
	popl	%edi
	popl	%esi
	popl	%ebp
	add	$4,%esp
	popl	%ebx
	popl	%edx
	popl	%ecx
	popl	%eax
	add	$16,%esp
	popl	%gs:return_value
	add	$20,%esp
	pushl	%gs:return_value
	popfl
