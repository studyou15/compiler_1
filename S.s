	.option	nopic
	.data
a_global:
	.word	0
b_global:
	.word	0
d_global:
	.word	0
	.text
	.align	1
	.globl	MYinitfunc
	.type	MYinitfunc,	@function
MYinitfunc:
	addi	sp,sp,-520
	sw	ra,516(sp)
	sw	s0,512(sp)
	sw	s1,508(sp)
	sw	s2,504(sp)
	sw	s3,500(sp)
	addi	s0,sp,520
	fmv.w.x	ft0,zero
#0: def a_global, 0
	mv	s1,zero
	la	s2,a_global
	sw	s1,0(s2)
#1: def b_global, 0
	mv	s3,zero
	la	s1,b_global
	sw	s3,0(s1)
#2: def d_global, 0
	mv	s2,zero
	la	s3,d_global
	sw	s2,0(s3)
#3: return null
MYinitfunc_return:
	lw	ra,516(sp)
	lw	s0,512(sp)
	lw	s1,508(sp)
	lw	s2,504(sp)
	lw	s3,500(sp)
	addi	sp,sp,520
	jr	ra
	.size	MYinitfunc,	.-MYinitfunc
	.align	1
	.globl	set_a
	.type	set_a,	@function
set_a:
	addi	sp,sp,-520
	sw	ra,516(sp)
	sw	s0,512(sp)
	sw	s1,508(sp)
	sw	s2,504(sp)
	sw	s3,500(sp)
	addi	s0,sp,520
	fmv.w.x	ft0,zero
	mv	t0,a0
	sw	t0,-56(s0)
#4: mov a_global, val
	lw	s2,-56(s0)
	mv	s1,s2
	la	s3,a_global
	sw	s1,0(s3)
#5: return a_global
	la	s1,a_global
	lw	s1,0(s1)
	mv	a0,s1
	j	set_a_return
set_a_return:
	lw	ra,516(sp)
	lw	s0,512(sp)
	lw	s1,508(sp)
	lw	s2,504(sp)
	lw	s3,500(sp)
	addi	sp,sp,520
	jr	ra
	.size	set_a,	.-set_a
	.align	1
	.globl	set_b
	.type	set_b,	@function
set_b:
	addi	sp,sp,-520
	sw	ra,516(sp)
	sw	s0,512(sp)
	sw	s1,508(sp)
	sw	s2,504(sp)
	sw	s3,500(sp)
	addi	s0,sp,520
	fmv.w.x	ft0,zero
	mv	t0,a0
	sw	t0,-56(s0)
#6: mov b_global, val
	lw	s2,-56(s0)
	mv	s1,s2
	la	s3,b_global
	sw	s1,0(s3)
#7: return b_global
	la	s1,b_global
	lw	s1,0(s1)
	mv	a0,s1
	j	set_b_return
set_b_return:
	lw	ra,516(sp)
	lw	s0,512(sp)
	lw	s1,508(sp)
	lw	s2,504(sp)
	lw	s3,500(sp)
	addi	sp,sp,520
	jr	ra
	.size	set_b,	.-set_b
	.align	1
	.globl	set_d
	.type	set_d,	@function
set_d:
	addi	sp,sp,-520
	sw	ra,516(sp)
	sw	s0,512(sp)
	sw	s1,508(sp)
	sw	s2,504(sp)
	sw	s3,500(sp)
	addi	s0,sp,520
	fmv.w.x	ft0,zero
	mv	t0,a0
	sw	t0,-56(s0)
#8: mov d_global, val
	lw	s2,-56(s0)
	mv	s1,s2
	la	s3,d_global
	sw	s1,0(s3)
#9: return d_global
	la	s1,d_global
	lw	s1,0(s1)
	mv	a0,s1
	j	set_d_return
set_d_return:
	lw	ra,516(sp)
	lw	s0,512(sp)
	lw	s1,508(sp)
	lw	s2,504(sp)
	lw	s3,500(sp)
	addi	sp,sp,520
	jr	ra
	.size	set_d,	.-set_d
	.align	1
	.globl	main
	.type	main,	@function
main:
	addi	sp,sp,-520
	sw	ra,516(sp)
	sw	s0,512(sp)
	sw	s1,508(sp)
	sw	s2,504(sp)
	sw	s3,500(sp)
	addi	s0,sp,520
	fmv.w.x	ft0,zero
#10: call null, MYinitfunc()
	call	MYinitfunc
#11: mov a_global, 2
	li	s2,2
	mv	s1,s2
	la	s3,a_global
	sw	s1,0(s3)
#12: mov b_global, 3
	li	s2,3
	mv	s1,s2
	la	s3,b_global
	sw	s1,0(s3)
#13: def set_a_ret0, 0
	mv	s1,zero
	sw	s1,-56(s0)
#14: call set_a_ret0, set_a(0)
	li	s2,0
	mv	a0,s2
	call	set_a
	mv	s3,a0
	sw	s3,-56(s0)
#15: def LAnd1, 0
	mv	s1,zero
	sw	s1,-60(s0)
#16: if set_a_ret0 goto [pc, 2]
	lw	s2,-56(s0)
	bne	s2,zero,.MY16
#17: goto [pc, 4]
	j	.MY17
#18: def set_b_ret2, 0
.MY16:
	mv	s3,zero
	sw	s3,-64(s0)
#19: call set_b_ret2, set_b(1)
	li	s1,1
	mv	a0,s1
	call	set_b
	mv	s2,a0
	sw	s2,-64(s0)
#20: and LAnd1, set_a_ret0, set_b_ret2
	lw	s3,-56(s0)
	lw	s1,-64(s0)
	sltiu	s3,s3,1
	xori	s3,s3,1
	sltiu	s1,s1,1
	xori	s1,s1,1
	and s2, s3, s1
	sw	s2,-60(s0)
#21: if LAnd1 goto [pc, 2]
.MY17:
	lw	s3,-60(s0)
	bne	s3,zero,.MY21
#22: goto [pc, 2]
	j	.MY22
#23: goto [pc, 1]
.MY21:
#24: call putint_ret3, putint(a_global)
.MY22:
	la	s1,a_global
	lw	s1,0(s1)
	mv	a0,s1
	call	putint
#25: call putch_ret4, putch(32)
	li	s2,32
	mv	a0,s2
	call	putch
#26: call putint_ret5, putint(b_global)
	la	s3,b_global
	lw	s3,0(s3)
	mv	a0,s3
	call	putint
#27: call putch_ret6, putch(32)
	li	s1,32
	mv	a0,s1
	call	putch
#28: mov a_global, 2
	li	s3,2
	mv	s2,s3
	la	s1,a_global
	sw	s2,0(s1)
#29: mov b_global, 3
	li	s3,3
	mv	s2,s3
	la	s1,b_global
	sw	s2,0(s1)
#30: def set_a_ret7, 0
	mv	s2,zero
	sw	s2,-68(s0)
#31: call set_a_ret7, set_a(0)
	li	s3,0
	mv	a0,s3
	call	set_a
	mv	s1,a0
	sw	s1,-68(s0)
#32: def LAnd8, 0
	mv	s2,zero
	sw	s2,-72(s0)
#33: if set_a_ret7 goto [pc, 2]
	lw	s3,-68(s0)
	bne	s3,zero,.MY33
#34: goto [pc, 4]
	j	.MY34
#35: def set_b_ret9, 0
.MY33:
	mv	s1,zero
	sw	s1,-76(s0)
#36: call set_b_ret9, set_b(1)
	li	s2,1
	mv	a0,s2
	call	set_b
	mv	s3,a0
	sw	s3,-76(s0)
#37: and LAnd8, set_a_ret7, set_b_ret9
	lw	s1,-68(s0)
	lw	s2,-76(s0)
	sltiu	s1,s1,1
	xori	s1,s1,1
	sltiu	s2,s2,1
	xori	s2,s2,1
	and s3, s1, s2
	sw	s3,-72(s0)
#38: if LAnd8 goto [pc, 2]
.MY34:
	lw	s1,-72(s0)
	bne	s1,zero,.MY38
#39: goto [pc, 2]
	j	.MY39
#40: goto [pc, 1]
.MY38:
#41: call putint_ret10, putint(a_global)
.MY39:
	la	s2,a_global
	lw	s2,0(s2)
	mv	a0,s2
	call	putint
#42: call putch_ret11, putch(32)
	li	s3,32
	mv	a0,s3
	call	putch
#43: call putint_ret12, putint(b_global)
	la	s1,b_global
	lw	s1,0(s1)
	mv	a0,s1
	call	putint
#44: call putch_ret13, putch(10)
	li	s2,10
	mv	a0,s2
	call	putch
#45: mov d_global, 2
	li	s1,2
	mv	s3,s1
	la	s2,d_global
	sw	s3,0(s2)
#46: def set_d_ret14, 0
	mv	s3,zero
	sw	s3,-80(s0)
#47: call set_d_ret14, set_d(3)
	li	s1,3
	mv	a0,s1
	call	set_d
	mv	s2,a0
	sw	s2,-80(s0)
#48: if set_d_ret14 goto [pc, 2]
	lw	s3,-80(s0)
	bne	s3,zero,.MY48
#49: goto [pc, 2]
	j	.MY49
#50: goto [pc, 1]
.MY48:
#51: call putint_ret15, putint(d_global)
.MY49:
	la	s1,d_global
	lw	s1,0(s1)
	mv	a0,s1
	call	putint
#52: call putch_ret16, putch(32)
	li	s2,32
	mv	a0,s2
	call	putch
#53: if 1 goto [pc, 2]
	li	s3,1
	bne	s3,zero,.MY53
#54: goto [pc, 2]
	j	.MY54
#55: goto [pc, 1]
.MY53:
#56: call putint_ret17, putint(d_global)
.MY54:
	la	s1,d_global
	lw	s1,0(s1)
	mv	a0,s1
	call	putint
#57: call putch_ret18, putch(10)
	li	s2,10
	mv	a0,s2
	call	putch
#58: if 1 goto [pc, 2]
	li	s3,1
	bne	s3,zero,.MY58
#59: goto [pc, 3]
	j	.MY59
#60: call putch_ret19, putch(65)
.MY58:
	li	s1,65
	mv	a0,s1
	call	putch
#61: goto [pc, 1]
#62: if 0 goto [pc, 2]
.MY59:
	li	s2,0
	bne	s2,zero,.MY62
#63: goto [pc, 3]
	j	.MY63
#64: call putch_ret20, putch(66)
.MY62:
	li	s3,66
	mv	a0,s3
	call	putch
#65: goto [pc, 1]
#66: if 0 goto [pc, 2]
.MY63:
	li	s1,0
	bne	s1,zero,.MY66
#67: goto [pc, 3]
	j	.MY67
#68: call putch_ret21, putch(67)
.MY66:
	li	s2,67
	mv	a0,s2
	call	putch
#69: goto [pc, 1]
#70: if 1 goto [pc, 2]
.MY67:
	li	s3,1
	bne	s3,zero,.MY70
#71: goto [pc, 3]
	j	.MY71
#72: call putch_ret22, putch(68)
.MY70:
	li	s1,68
	mv	a0,s1
	call	putch
#73: goto [pc, 1]
#74: if 0 goto [pc, 2]
.MY71:
	li	s2,0
	bne	s2,zero,.MY74
#75: goto [pc, 3]
	j	.MY75
#76: call putch_ret23, putch(69)
.MY74:
	li	s3,69
	mv	a0,s3
	call	putch
#77: goto [pc, 1]
#78: def ZERO, 0
.MY75:
	mv	s1,zero
	sw	s1,-84(s0)
#79: sub Unary_24, ZERO, 1
	lw	s3,-84(s0)
	li	s1,1
	sub s2, s3, s1
	sw	s2,-88(s0)
#80: def Eq25, -1
	li	s3,-1
	mv	s2,s3
	sw	s2,-92(s0)
#81: eq Eq25, Eq25, Unary_24
	lw	s2,-92(s0)
	lw	s3,-88(s0)
	xor s1, s2, s3
	sltiu	s1,s1,1
	sw	s1,-92(s0)
#82: if Eq25 goto [pc, 2]
	lw	s1,-92(s0)
	bne	s1,zero,.MY82
#83: goto [pc, 3]
	j	.MY83
#84: call putch_ret26, putch(70)
.MY82:
	li	s2,70
	mv	a0,s2
	call	putch
#85: goto [pc, 1]
#86: call putch_ret27, putch(10)
.MY83:
	li	s3,10
	mv	a0,s3
	call	putch
#87: def i0_main, 0
	mv	s1,zero
	sw	s1,-96(s0)
#88: def i1_main, 1
	li	s3,1
	mv	s2,s3
	sw	s2,-100(s0)
#89: def i2_main, 2
	li	s2,2
	mv	s1,s2
	sw	s1,-104(s0)
#90: def i3_main, 3
	li	s1,3
	mv	s3,s1
	sw	s3,-108(s0)
#91: def i4_main, 4
	li	s3,4
	mv	s2,s3
	sw	s2,-112(s0)
#92: def LAnd28, 0
.MY99:
	mv	s1,zero
	sw	s1,-116(s0)
#93: if i0_main goto [pc, 2]
	lw	s2,-96(s0)
	bne	s2,zero,.MY93
#94: goto [pc, 2]
	j	.MY94
#95: and LAnd28, i0_main, i1_main
.MY93:
	lw	s3,-96(s0)
	lw	s1,-100(s0)
	sltiu	s3,s3,1
	xori	s3,s3,1
	sltiu	s1,s1,1
	xori	s1,s1,1
	and s2, s3, s1
	sw	s2,-116(s0)
#96: if LAnd28 goto [pc, 2]
.MY94:
	lw	s3,-116(s0)
	bne	s3,zero,.MY96
#97: goto [pc, 3]
	j	.MY97
#98: call putch_ret29, putch(32)
.MY96:
	li	s1,32
	mv	a0,s1
	call	putch
#99: goto [pc, -7]
	j	.MY99
#100: def LOr30, 1
.MY97:
	li	s3,1
	mv	s2,s3
	sw	s2,-120(s0)
#101: if i0_main goto [pc, 2]
	lw	s1,-96(s0)
	bne	s1,zero,.MY101
#102: or LOr30, i0_main, i1_main
	lw	s3,-96(s0)
	lw	s1,-100(s0)
	or s2, s3, s1
	sw	s2,-120(s0)
#103: if LOr30 goto [pc, 2]
.MY101:
	lw	s2,-120(s0)
	bne	s2,zero,.MY103
#104: goto [pc, 3]
	j	.MY104
#105: call putch_ret31, putch(67)
.MY103:
	li	s3,67
	mv	a0,s3
	call	putch
#106: goto [pc, 1]
#107: def Rel_32, i0_main
.MY104:
	lw	s2,-96(s0)
	mv	s1,s2
	sw	s1,-124(s0)
#108: geq Rel_32, Rel_32, i1_main
	lw	s1,-124(s0)
	lw	s2,-100(s0)
	slt s3, s1, s2
	xori	s3,s3,1
	sw	s3,-124(s0)
#109: def LOr33, 1
	li	s1,1
	mv	s3,s1
	sw	s3,-128(s0)
#110: if Rel_32 goto [pc, 4]
	lw	s2,-124(s0)
	bne	s2,zero,.MY110
#111: def Rel_34, i1_main
	lw	s1,-100(s0)
	mv	s3,s1
	sw	s3,-132(s0)
#112: leq Rel_34, Rel_34, i0_main
	lw	s3,-132(s0)
	lw	s1,-96(s0)
	slt s2, s1, s3
	xori	s2,s2,1
	sw	s2,-132(s0)
#113: or LOr33, Rel_32, Rel_34
	lw	s3,-124(s0)
	lw	s1,-132(s0)
	or s2, s3, s1
	sw	s2,-128(s0)
#114: if LOr33 goto [pc, 2]
.MY110:
	lw	s2,-128(s0)
	bne	s2,zero,.MY114
#115: goto [pc, 3]
	j	.MY115
#116: call putch_ret35, putch(72)
.MY114:
	li	s3,72
	mv	a0,s3
	call	putch
#117: goto [pc, 1]
#118: def Rel_36, i2_main
.MY115:
	lw	s2,-104(s0)
	mv	s1,s2
	sw	s1,-136(s0)
#119: geq Rel_36, Rel_36, i1_main
	lw	s1,-136(s0)
	lw	s2,-100(s0)
	slt s3, s1, s2
	xori	s3,s3,1
	sw	s3,-136(s0)
#120: def LAnd37, 0
	mv	s3,zero
	sw	s3,-140(s0)
#121: if Rel_36 goto [pc, 2]
	lw	s1,-136(s0)
	bne	s1,zero,.MY121
#122: goto [pc, 5]
	j	.MY122
#123: def Eq38, 0
.MY121:
	mv	s2,zero
	sw	s2,-144(s0)
#124: mov Eq38, i4_main
	lw	s1,-112(s0)
	mv	s3,s1
	sw	s3,-144(s0)
#125: neq Eq38, Eq38, i3_main
	lw	s3,-144(s0)
	lw	s1,-108(s0)
	xor s2, s3, s1
	sltiu	s2,s2,1
	xori	s2,s2,1
	sw	s2,-144(s0)
#126: and LAnd37, Rel_36, Eq38
	lw	s2,-136(s0)
	lw	s3,-144(s0)
	sltiu	s2,s2,1
	xori	s2,s2,1
	sltiu	s3,s3,1
	xori	s3,s3,1
	and s1, s2, s3
	sw	s1,-140(s0)
#127: if LAnd37 goto [pc, 2]
.MY122:
	lw	s2,-140(s0)
	bne	s2,zero,.MY127
#128: goto [pc, 3]
	j	.MY128
#129: call putch_ret39, putch(73)
.MY127:
	li	s3,73
	mv	a0,s3
	call	putch
#130: goto [pc, 1]
#131: not Unary_40, i1_main
.MY128:
	lw	s2,-100(s0)
	seqz s1, s2
	sw	s1,-148(s0)
#132: def Eq41, 0
	mv	s3,zero
	sw	s3,-152(s0)
#133: mov Eq41, i0_main
	lw	s2,-96(s0)
	mv	s1,s2
	sw	s1,-152(s0)
#134: eq Eq41, Eq41, Unary_40
	lw	s1,-152(s0)
	lw	s2,-148(s0)
	xor s3, s1, s2
	sltiu	s3,s3,1
	sw	s3,-152(s0)
#135: def LAnd42, 0
	mv	s3,zero
	sw	s3,-156(s0)
#136: if Eq41 goto [pc, 2]
	lw	s1,-152(s0)
	bne	s1,zero,.MY136
#137: goto [pc, 4]
	j	.MY137
#138: def Rel_43, i3_main
.MY136:
	lw	s3,-108(s0)
	mv	s2,s3
	sw	s2,-160(s0)
#139: lss Rel_43, Rel_43, i3_main
	lw	s2,-160(s0)
	lw	s3,-108(s0)
	slt s1, s2, s3
	sw	s1,-160(s0)
#140: and LAnd42, Eq41, Rel_43
	lw	s1,-152(s0)
	lw	s2,-160(s0)
	sltiu	s1,s1,1
	xori	s1,s1,1
	sltiu	s2,s2,1
	xori	s2,s2,1
	and s3, s1, s2
	sw	s3,-156(s0)
#141: def LOr44, 1
.MY137:
	li	s2,1
	mv	s1,s2
	sw	s1,-164(s0)
#142: if LAnd42 goto [pc, 4]
	lw	s3,-156(s0)
	bne	s3,zero,.MY142
#143: def Rel_45, i4_main
	lw	s2,-112(s0)
	mv	s1,s2
	sw	s1,-168(s0)
#144: geq Rel_45, Rel_45, i4_main
	lw	s1,-168(s0)
	lw	s2,-112(s0)
	slt s3, s1, s2
	xori	s3,s3,1
	sw	s3,-168(s0)
#145: or LOr44, LAnd42, Rel_45
	lw	s1,-156(s0)
	lw	s2,-168(s0)
	or s3, s1, s2
	sw	s3,-164(s0)
#146: if LOr44 goto [pc, 2]
.MY142:
	lw	s3,-164(s0)
	bne	s3,zero,.MY146
#147: goto [pc, 3]
	j	.MY147
#148: call putch_ret46, putch(74)
.MY146:
	li	s1,74
	mv	a0,s1
	call	putch
#149: goto [pc, 1]
#150: not Unary_47, i1_main
.MY147:
	lw	s3,-100(s0)
	seqz s2, s3
	sw	s2,-172(s0)
#151: def Eq48, 0
	mv	s1,zero
	sw	s1,-176(s0)
#152: mov Eq48, i0_main
	lw	s3,-96(s0)
	mv	s2,s3
	sw	s2,-176(s0)
#153: eq Eq48, Eq48, Unary_47
	lw	s2,-176(s0)
	lw	s3,-172(s0)
	xor s1, s2, s3
	sltiu	s1,s1,1
	sw	s1,-176(s0)
#154: def LOr49, 1
	li	s2,1
	mv	s1,s2
	sw	s1,-180(s0)
#155: if Eq48 goto [pc, 10]
	lw	s3,-176(s0)
	bne	s3,zero,.MY155
#156: def Rel_50, i3_main
	lw	s2,-108(s0)
	mv	s1,s2
	sw	s1,-184(s0)
#157: lss Rel_50, Rel_50, i3_main
	lw	s1,-184(s0)
	lw	s2,-108(s0)
	slt s3, s1, s2
	sw	s3,-184(s0)
#158: def LAnd51, 0
	mv	s3,zero
	sw	s3,-188(s0)
#159: if Rel_50 goto [pc, 2]
	lw	s1,-184(s0)
	bne	s1,zero,.MY159
#160: goto [pc, 4]
	j	.MY160
#161: def Rel_52, i4_main
.MY159:
	lw	s3,-112(s0)
	mv	s2,s3
	sw	s2,-192(s0)
#162: geq Rel_52, Rel_52, i4_main
	lw	s2,-192(s0)
	lw	s3,-112(s0)
	slt s1, s2, s3
	xori	s1,s1,1
	sw	s1,-192(s0)
#163: and LAnd51, Rel_50, Rel_52
	lw	s1,-184(s0)
	lw	s2,-192(s0)
	sltiu	s1,s1,1
	xori	s1,s1,1
	sltiu	s2,s2,1
	xori	s2,s2,1
	and s3, s1, s2
	sw	s3,-188(s0)
#164: or LOr49, Eq48, LAnd51
.MY160:
	lw	s2,-176(s0)
	lw	s3,-188(s0)
	or s1, s2, s3
	sw	s1,-180(s0)
#165: if LOr49 goto [pc, 2]
.MY155:
	lw	s1,-180(s0)
	bne	s1,zero,.MY165
#166: goto [pc, 3]
	j	.MY166
#167: call putch_ret53, putch(75)
.MY165:
	li	s2,75
	mv	a0,s2
	call	putch
#168: goto [pc, 1]
#169: call putch_ret54, putch(10)
.MY166:
	li	s3,10
	mv	a0,s3
	call	putch
#170: return 0
	li	s1,0
	mv	a0,s1
	j	main_return
main_return:
	lw	ra,516(sp)
	lw	s0,512(sp)
	lw	s1,508(sp)
	lw	s2,504(sp)
	lw	s3,500(sp)
	addi	sp,sp,520
	jr	ra
	.size	main,	.-main
