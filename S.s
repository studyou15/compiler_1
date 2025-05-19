	.option nopic
	.data
	.text
	.align	2
	.globl	_global
	.type	_global,	@function
_global:
	addi	sp,sp,-520
	sw	ra,516(sp)
	sw	s0,512(sp)
	addi	s0,sp,520
#0: return null
	j	_global_return
_global_return:
	lw	ra,516(sp)
	lw	s0,512(sp)
	addi	sp,sp,520
	jr	ra
	.size	_global,	.-_global
	.align	2
	.globl	main
	.type	main,	@function
main:
	addi	sp,sp,-520
	sw	ra,516(sp)
	sw	s0,512(sp)
	addi	s0,sp,520
#1: call null, _global()
	call	_global
#2: def a_main_S1, 0
	li	t2,0
	sw	t2,-56(sp)
#3: def b0_main_S1, 0
	li	t2,0
	sw	t2,-60(sp)
#4: def _c_main_S1, 0
	li	t2,0
	sw	t2,-64(sp)
#5: mov a_main_S1, 1
	lw	t0,1
	sw	t0,-56(sp)
#6: mov b0_main_S1, 2
	lw	t0,2
	sw	t0,-60(sp)
#7: mov _c_main_S1, 3
	lw	t0,3
	sw	t0,-64(sp)
#8: mov t3, b0_main_S1
	lw	t0,-60(sp)
	sw	t0,-68(sp)
#9: mov t4, _c_main_S1
	lw	t0,-64(sp)
	sw	t0,-72(sp)
#10: add t3, t4, t3
	lw	t0,-72(sp)
	lw	t1,-68(sp)
	add	t2,t0,t1
	sw	t2,-68(sp)
#11: return t3
	lw	a0,-68(sp)
	j	main_return
main_return:
	lw	ra,516(sp)
	lw	s0,512(sp)
	addi	sp,sp,520
	jr	ra
	.size	main,	.-main
