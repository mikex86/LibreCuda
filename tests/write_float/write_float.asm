	.headerflags	@"EF_CUDA_TEXMODE_UNIFIED EF_CUDA_64BIT_ADDRESS EF_CUDA_SM80 EF_CUDA_VIRTUAL_SM(EF_CUDA_SM80)"
	.elftype	@"ET_EXEC"


//--------------------- .debug_frame              --------------------------
	.section	.debug_frame,"",@progbits
.debug_frame:
        /*0000*/ 	.byte	0xff, 0xff, 0xff, 0xff, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff
        /*0010*/ 	.byte	0xff, 0xff, 0xff, 0xff, 0x03, 0x00, 0x04, 0x7c, 0xff, 0xff, 0xff, 0xff, 0x0f, 0x0c, 0x81, 0x80
        /*0020*/ 	.byte	0x80, 0x28, 0x00, 0x08, 0xff, 0x81, 0x80, 0x28, 0x08, 0x81, 0x80, 0x80, 0x28, 0x00, 0x00, 0x00
        /*0030*/ 	.byte	0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        /*0040*/ 	.byte	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        /*0048*/ 	.dword	write_float_sum
        /*0050*/ 	.byte	0x70, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 0x00, 0x00, 0x00, 0x04, 0x20, 0x00
        /*0060*/ 	.byte	0x00, 0x00, 0x0c, 0x81, 0x80, 0x80, 0x28, 0x00, 0x04, 0xfc, 0xff, 0xff, 0x3f, 0x00, 0x00, 0x00
        /*0070*/ 	.byte	0xff, 0xff, 0xff, 0xff, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff
        /*0080*/ 	.byte	0xff, 0xff, 0xff, 0xff, 0x03, 0x00, 0x04, 0x7c, 0xff, 0xff, 0xff, 0xff, 0x0f, 0x0c, 0x81, 0x80
        /*0090*/ 	.byte	0x80, 0x28, 0x00, 0x08, 0xff, 0x81, 0x80, 0x28, 0x08, 0x81, 0x80, 0x80, 0x28, 0x00, 0x00, 0x00
        /*00a0*/ 	.byte	0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        /*00b0*/ 	.byte	0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        /*00b8*/ 	.dword	write_float_value
        /*00c0*/ 	.byte	0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 0x00, 0x00, 0x00, 0x04, 0x18, 0x00
        /*00d0*/ 	.byte	0x00, 0x00, 0x0c, 0x81, 0x80, 0x80, 0x28, 0x00, 0x04, 0xfc, 0xff, 0xff, 0x3f, 0x00, 0x00, 0x00
        /*00e0*/ 	.byte	0xff, 0xff, 0xff, 0xff, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff
        /*00f0*/ 	.byte	0xff, 0xff, 0xff, 0xff, 0x03, 0x00, 0x04, 0x7c, 0xff, 0xff, 0xff, 0xff, 0x0f, 0x0c, 0x81, 0x80
        /*0100*/ 	.byte	0x80, 0x28, 0x00, 0x08, 0xff, 0x81, 0x80, 0x28, 0x08, 0x81, 0x80, 0x80, 0x28, 0x00, 0x00, 0x00
        /*0110*/ 	.byte	0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        /*0120*/ 	.byte	0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        /*0128*/ 	.dword	write_float_ptr
        /*0130*/ 	.byte	0x70, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 0x00, 0x00, 0x00, 0x04, 0x20, 0x00
        /*0140*/ 	.byte	0x00, 0x00, 0x0c, 0x81, 0x80, 0x80, 0x28, 0x00, 0x04, 0xfc, 0xff, 0xff, 0x3f, 0x00, 0x00, 0x00


//--------------------- .nv.info                  --------------------------
	.section	.nv.info,"",@"SHT_CUDA_INFO"
	.align	4


	//----- nvinfo : EIATTR_REGCOUNT
	.align		4
        /*0000*/ 	.byte	0x04, 0x2f
        /*0002*/ 	.short	(.L_2 - .L_1)
	.align		4
.L_1:
        /*0004*/ 	.word	index@(write_float_ptr)
        /*0008*/ 	.word	0x00000008


	//----- nvinfo : EIATTR_MAX_STACK_SIZE
	.align		4
.L_2:
        /*000c*/ 	.byte	0x04, 0x23
        /*000e*/ 	.short	(.L_4 - .L_3)
	.align		4
.L_3:
        /*0010*/ 	.word	index@(write_float_ptr)
        /*0014*/ 	.word	0x00000000


	//----- nvinfo : EIATTR_MIN_STACK_SIZE
	.align		4
.L_4:
        /*0018*/ 	.byte	0x04, 0x12
        /*001a*/ 	.short	(.L_6 - .L_5)
	.align		4
.L_5:
        /*001c*/ 	.word	index@(write_float_ptr)
        /*0020*/ 	.word	0x00000000


	//----- nvinfo : EIATTR_FRAME_SIZE
	.align		4
.L_6:
        /*0024*/ 	.byte	0x04, 0x11
        /*0026*/ 	.short	(.L_8 - .L_7)
	.align		4
.L_7:
        /*0028*/ 	.word	index@(write_float_ptr)
        /*002c*/ 	.word	0x00000000


	//----- nvinfo : EIATTR_REGCOUNT
	.align		4
.L_8:
        /*0030*/ 	.byte	0x04, 0x2f
        /*0032*/ 	.short	(.L_10 - .L_9)
	.align		4
.L_9:
        /*0034*/ 	.word	index@(write_float_value)
        /*0038*/ 	.word	0x00000008


	//----- nvinfo : EIATTR_MAX_STACK_SIZE
	.align		4
.L_10:
        /*003c*/ 	.byte	0x04, 0x23
        /*003e*/ 	.short	(.L_12 - .L_11)
	.align		4
.L_11:
        /*0040*/ 	.word	index@(write_float_value)
        /*0044*/ 	.word	0x00000000


	//----- nvinfo : EIATTR_MIN_STACK_SIZE
	.align		4
.L_12:
        /*0048*/ 	.byte	0x04, 0x12
        /*004a*/ 	.short	(.L_14 - .L_13)
	.align		4
.L_13:
        /*004c*/ 	.word	index@(write_float_value)
        /*0050*/ 	.word	0x00000000


	//----- nvinfo : EIATTR_FRAME_SIZE
	.align		4
.L_14:
        /*0054*/ 	.byte	0x04, 0x11
        /*0056*/ 	.short	(.L_16 - .L_15)
	.align		4
.L_15:
        /*0058*/ 	.word	index@(write_float_value)
        /*005c*/ 	.word	0x00000000


	//----- nvinfo : EIATTR_REGCOUNT
	.align		4
.L_16:
        /*0060*/ 	.byte	0x04, 0x2f
        /*0062*/ 	.short	(.L_18 - .L_17)
	.align		4
.L_17:
        /*0064*/ 	.word	index@(write_float_sum)
        /*0068*/ 	.word	0x00000008


	//----- nvinfo : EIATTR_MAX_STACK_SIZE
	.align		4
.L_18:
        /*006c*/ 	.byte	0x04, 0x23
        /*006e*/ 	.short	(.L_20 - .L_19)
	.align		4
.L_19:
        /*0070*/ 	.word	index@(write_float_sum)
        /*0074*/ 	.word	0x00000000


	//----- nvinfo : EIATTR_MIN_STACK_SIZE
	.align		4
.L_20:
        /*0078*/ 	.byte	0x04, 0x12
        /*007a*/ 	.short	(.L_22 - .L_21)
	.align		4
.L_21:
        /*007c*/ 	.word	index@(write_float_sum)
        /*0080*/ 	.word	0x00000000


	//----- nvinfo : EIATTR_FRAME_SIZE
	.align		4
.L_22:
        /*0084*/ 	.byte	0x04, 0x11
        /*0086*/ 	.short	(.L_24 - .L_23)
	.align		4
.L_23:
        /*0088*/ 	.word	index@(write_float_sum)
        /*008c*/ 	.word	0x00000000
.L_24:


//--------------------- .nv.info.write_float_sum  --------------------------
	.section	.nv.info.write_float_sum,"",@"SHT_CUDA_INFO"
	.align	4


	//----- nvinfo : EIATTR_CUDA_API_VERSION
	.align		4
        /*0000*/ 	.byte	0x04, 0x37
        /*0002*/ 	.short	(.L_26 - .L_25)
.L_25:
        /*0004*/ 	.word	0x00000073


	//----- nvinfo : EIATTR_SW2861232_WAR
	.align		4
.L_26:
        /*0008*/ 	.byte	0x01, 0x35
	.zero		2


	//----- nvinfo : EIATTR_PARAM_CBANK
	.align		4
        /*000c*/ 	.byte	0x04, 0x0a
        /*000e*/ 	.short	(.L_28 - .L_27)
	.align		4
.L_27:
        /*0010*/ 	.word	index@(.nv.constant0.write_float_sum)
        /*0014*/ 	.short	0x0160
        /*0016*/ 	.short	0x0010


	//----- nvinfo : EIATTR_CBANK_PARAM_SIZE
	.align		4
.L_28:
        /*0018*/ 	.byte	0x03, 0x19
        /*001a*/ 	.short	0x0010


	//----- nvinfo : EIATTR_KPARAM_INFO
	.align		4
        /*001c*/ 	.byte	0x04, 0x17
        /*001e*/ 	.short	(.L_30 - .L_29)
.L_29:
        /*0020*/ 	.word	0x00000000
        /*0024*/ 	.short	0x0002
        /*0026*/ 	.short	0x000c
        /*0028*/ 	.byte	0x00, 0xf0, 0x11, 0x00


	//----- nvinfo : EIATTR_KPARAM_INFO
	.align		4
.L_30:
        /*002c*/ 	.byte	0x04, 0x17
        /*002e*/ 	.short	(.L_32 - .L_31)
.L_31:
        /*0030*/ 	.word	0x00000000
        /*0034*/ 	.short	0x0001
        /*0036*/ 	.short	0x0008
        /*0038*/ 	.byte	0x00, 0xf0, 0x09, 0x00


	//----- nvinfo : EIATTR_KPARAM_INFO
	.align		4
.L_32:
        /*003c*/ 	.byte	0x04, 0x17
        /*003e*/ 	.short	(.L_34 - .L_33)
.L_33:
        /*0040*/ 	.word	0x00000000
        /*0044*/ 	.short	0x0000
        /*0046*/ 	.short	0x0000
        /*0048*/ 	.byte	0x00, 0xf0, 0x21, 0x00


	//----- nvinfo : EIATTR_MAXREG_COUNT
	.align		4
.L_34:
        /*004c*/ 	.byte	0x03, 0x1b
        /*004e*/ 	.short	0x00ff


	//----- nvinfo : EIATTR_EXIT_INSTR_OFFSETS
	.align		4
        /*0050*/ 	.byte	0x04, 0x1c
        /*0052*/ 	.short	(.L_36 - .L_35)


	//   ....[0]....
.L_35:
        /*0054*/ 	.word	0x00000080
.L_36:


//--------------------- .nv.info.write_float_value --------------------------
	.section	.nv.info.write_float_value,"",@"SHT_CUDA_INFO"
	.align	4


	//----- nvinfo : EIATTR_CUDA_API_VERSION
	.align		4
        /*0000*/ 	.byte	0x04, 0x37
        /*0002*/ 	.short	(.L_38 - .L_37)
.L_37:
        /*0004*/ 	.word	0x00000073


	//----- nvinfo : EIATTR_SW2861232_WAR
	.align		4
.L_38:
        /*0008*/ 	.byte	0x01, 0x35
	.zero		2


	//----- nvinfo : EIATTR_PARAM_CBANK
	.align		4
        /*000c*/ 	.byte	0x04, 0x0a
        /*000e*/ 	.short	(.L_40 - .L_39)
	.align		4
.L_39:
        /*0010*/ 	.word	index@(.nv.constant0.write_float_value)
        /*0014*/ 	.short	0x0160
        /*0016*/ 	.short	0x000c


	//----- nvinfo : EIATTR_CBANK_PARAM_SIZE
	.align		4
.L_40:
        /*0018*/ 	.byte	0x03, 0x19
        /*001a*/ 	.short	0x000c


	//----- nvinfo : EIATTR_KPARAM_INFO
	.align		4
        /*001c*/ 	.byte	0x04, 0x17
        /*001e*/ 	.short	(.L_42 - .L_41)
.L_41:
        /*0020*/ 	.word	0x00000000
        /*0024*/ 	.short	0x0001
        /*0026*/ 	.short	0x0008
        /*0028*/ 	.byte	0x00, 0xf0, 0x11, 0x00


	//----- nvinfo : EIATTR_KPARAM_INFO
	.align		4
.L_42:
        /*002c*/ 	.byte	0x04, 0x17
        /*002e*/ 	.short	(.L_44 - .L_43)
.L_43:
        /*0030*/ 	.word	0x00000000
        /*0034*/ 	.short	0x0000
        /*0036*/ 	.short	0x0000
        /*0038*/ 	.byte	0x00, 0xf0, 0x21, 0x00


	//----- nvinfo : EIATTR_MAXREG_COUNT
	.align		4
.L_44:
        /*003c*/ 	.byte	0x03, 0x1b
        /*003e*/ 	.short	0x00ff


	//----- nvinfo : EIATTR_EXIT_INSTR_OFFSETS
	.align		4
        /*0040*/ 	.byte	0x04, 0x1c
        /*0042*/ 	.short	(.L_46 - .L_45)


	//   ....[0]....
.L_45:
        /*0044*/ 	.word	0x00000060
.L_46:


//--------------------- .nv.info.write_float_ptr  --------------------------
	.section	.nv.info.write_float_ptr,"",@"SHT_CUDA_INFO"
	.align	4


	//----- nvinfo : EIATTR_CUDA_API_VERSION
	.align		4
        /*0000*/ 	.byte	0x04, 0x37
        /*0002*/ 	.short	(.L_48 - .L_47)
.L_47:
        /*0004*/ 	.word	0x00000073


	//----- nvinfo : EIATTR_SW2861232_WAR
	.align		4
.L_48:
        /*0008*/ 	.byte	0x01, 0x35
	.zero		2


	//----- nvinfo : EIATTR_PARAM_CBANK
	.align		4
        /*000c*/ 	.byte	0x04, 0x0a
        /*000e*/ 	.short	(.L_50 - .L_49)
	.align		4
.L_49:
        /*0010*/ 	.word	index@(.nv.constant0.write_float_ptr)
        /*0014*/ 	.short	0x0160
        /*0016*/ 	.short	0x0010


	//----- nvinfo : EIATTR_CBANK_PARAM_SIZE
	.align		4
.L_50:
        /*0018*/ 	.byte	0x03, 0x19
        /*001a*/ 	.short	0x0010


	//----- nvinfo : EIATTR_KPARAM_INFO
	.align		4
        /*001c*/ 	.byte	0x04, 0x17
        /*001e*/ 	.short	(.L_52 - .L_51)
.L_51:
        /*0020*/ 	.word	0x00000000
        /*0024*/ 	.short	0x0001
        /*0026*/ 	.short	0x0008
        /*0028*/ 	.byte	0x00, 0xf0, 0x21, 0x00


	//----- nvinfo : EIATTR_KPARAM_INFO
	.align		4
.L_52:
        /*002c*/ 	.byte	0x04, 0x17
        /*002e*/ 	.short	(.L_54 - .L_53)
.L_53:
        /*0030*/ 	.word	0x00000000
        /*0034*/ 	.short	0x0000
        /*0036*/ 	.short	0x0000
        /*0038*/ 	.byte	0x00, 0xf0, 0x21, 0x00


	//----- nvinfo : EIATTR_MAXREG_COUNT
	.align		4
.L_54:
        /*003c*/ 	.byte	0x03, 0x1b
        /*003e*/ 	.short	0x00ff


	//----- nvinfo : EIATTR_EXIT_INSTR_OFFSETS
	.align		4
        /*0040*/ 	.byte	0x04, 0x1c
        /*0042*/ 	.short	(.L_56 - .L_55)


	//   ....[0]....
.L_55:
        /*0044*/ 	.word	0x00000080
.L_56:


//--------------------- .nv.rel.action            --------------------------
	.section	.nv.rel.action,"",@"SHT_CUDA_RELOCINFO"
	.align	8
	.sectionentsize	8
        /*0000*/ 	.byte	0x4b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x02, 0x08, 0x10, 0x0a, 0x2f, 0x22
        /*0010*/ 	.byte	0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x08, 0x00, 0x00, 0x00, 0x00
        /*0020*/ 	.byte	0x00, 0x00, 0x10, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x08, 0x00, 0x00, 0x00, 0x00
        /*0030*/ 	.byte	0x00, 0x00, 0x20, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x28, 0x08, 0x00, 0x00, 0x00, 0x00
        /*0040*/ 	.byte	0x00, 0x00, 0x30, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x08, 0x00, 0x00, 0x00, 0x00
        /*0050*/ 	.byte	0x01, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x08, 0x08, 0x00, 0x00, 0x00, 0x00
        /*0060*/ 	.byte	0x01, 0x00, 0x10, 0x08, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x18, 0x08, 0x00, 0x00, 0x00, 0x00
        /*0070*/ 	.byte	0x01, 0x00, 0x20, 0x08, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x28, 0x08, 0x00, 0x00, 0x00, 0x00
        /*0080*/ 	.byte	0x01, 0x00, 0x30, 0x08, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x38, 0x08, 0x00, 0x00, 0x00, 0x00
        /*0090*/ 	.byte	0x02, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x08, 0x08, 0x00, 0x00, 0x00, 0x00
        /*00a0*/ 	.byte	0x02, 0x00, 0x10, 0x08, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x18, 0x08, 0x00, 0x00, 0x00, 0x00
        /*00b0*/ 	.byte	0x02, 0x00, 0x20, 0x08, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x28, 0x08, 0x00, 0x00, 0x00, 0x00
        /*00c0*/ 	.byte	0x02, 0x00, 0x30, 0x08, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x38, 0x08, 0x00, 0x00, 0x00, 0x00
        /*00d0*/ 	.byte	0x00, 0x00, 0x00, 0x14, 0x2c, 0x00, 0x00, 0x00


//--------------------- .nv.constant0.write_float_sum --------------------------
	.section	.nv.constant0.write_float_sum,"a",@progbits
	.align	4
.nv.constant0.write_float_sum:
	.zero		368


//--------------------- .nv.constant0.write_float_value --------------------------
	.section	.nv.constant0.write_float_value,"a",@progbits
	.align	4
.nv.constant0.write_float_value:
	.zero		364


//--------------------- .nv.constant0.write_float_ptr --------------------------
	.section	.nv.constant0.write_float_ptr,"a",@progbits
	.align	4
.nv.constant0.write_float_ptr:
	.zero		368


//--------------------- .text.write_float_sum     --------------------------
	.section	.text.write_float_sum,"ax",@progbits
	.sectioninfo	@"SHI_REGISTERS=8"
	.align	128
        .global         write_float_sum
        .type           write_float_sum,@function
        .size           write_float_sum,(.L_x_3 - write_float_sum)
        .other          write_float_sum,@"STO_CUDA_ENTRY STV_DEFAULT"
write_float_sum:
.text.write_float_sum:
        /*0000*/                   MOV R1, c[0x0][0x28] ;
        /*0010*/                   ULDC.U16 UR4, c[0x0][0x168] ;
        /*0020*/                   MOV R2, c[0x0][0x160] ;
        /*0030*/                   I2F.S16 R0, UR4 ;
        /*0040*/                   MOV R3, c[0x0][0x164] ;
        /*0050*/                   ULDC.64 UR4, c[0x0][0x118] ;
        /*0060*/                   FADD R5, R0, c[0x0][0x16c] ;
        /*0070*/                   STG.E [R2.64], R5 ;
        /*0080*/                   EXIT ;
.L_x_0:
        /*0090*/                   BRA `(.L_x_0);
        /*00a0*/                   NOP;
        /*00b0*/                   NOP;
        /*00c0*/                   NOP;
        /*00d0*/                   NOP;
        /*00e0*/                   NOP;
        /*00f0*/                   NOP;
        /*0100*/                   NOP;
        /*0110*/                   NOP;
        /*0120*/                   NOP;
        /*0130*/                   NOP;
        /*0140*/                   NOP;
        /*0150*/                   NOP;
        /*0160*/                   NOP;
        /*0170*/                   NOP;
.L_x_3:


//--------------------- .text.write_float_value   --------------------------
	.section	.text.write_float_value,"ax",@progbits
	.sectioninfo	@"SHI_REGISTERS=8"
	.align	128
        .global         write_float_value
        .type           write_float_value,@function
        .size           write_float_value,(.L_x_4 - write_float_value)
        .other          write_float_value,@"STO_CUDA_ENTRY STV_DEFAULT"
write_float_value:
.text.write_float_value:
        /*0000*/                   IMAD.MOV.U32 R1, RZ, RZ, c[0x0][0x28] ;
        /*0010*/                   IMAD.MOV.U32 R5, RZ, RZ, c[0x0][0x168] ;
        /*0020*/                   MOV R2, c[0x0][0x160] ;
        /*0030*/                   ULDC.64 UR4, c[0x0][0x118] ;
        /*0040*/                   MOV R3, c[0x0][0x164] ;
        /*0050*/                   STG.E [R2.64], R5 ;
        /*0060*/                   EXIT ;
.L_x_1:
        /*0070*/                   BRA `(.L_x_1);
        /*0080*/                   NOP;
        /*0090*/                   NOP;
        /*00a0*/                   NOP;
        /*00b0*/                   NOP;
        /*00c0*/                   NOP;
        /*00d0*/                   NOP;
        /*00e0*/                   NOP;
        /*00f0*/                   NOP;
.L_x_4:


//--------------------- .text.write_float_ptr     --------------------------
	.section	.text.write_float_ptr,"ax",@progbits
	.sectioninfo	@"SHI_REGISTERS=8"
	.align	128
        .global         write_float_ptr
        .type           write_float_ptr,@function
        .size           write_float_ptr,(.L_x_5 - write_float_ptr)
        .other          write_float_ptr,@"STO_CUDA_ENTRY STV_DEFAULT"
write_float_ptr:
.text.write_float_ptr:
        /*0000*/                   MOV R1, c[0x0][0x28] ;
        /*0010*/                   IMAD.MOV.U32 R3, RZ, RZ, c[0x0][0x16c] ;
        /*0020*/                   MOV R2, c[0x0][0x168] ;
        /*0030*/                   ULDC.64 UR4, c[0x0][0x118] ;
        /*0040*/                   LDG.E R3, [R2.64] ;
        /*0050*/                   MOV R4, c[0x0][0x160] ;
        /*0060*/                   IMAD.MOV.U32 R5, RZ, RZ, c[0x0][0x164] ;
        /*0070*/                   STG.E [R4.64], R3 ;
        /*0080*/                   EXIT ;
.L_x_2:
        /*0090*/                   BRA `(.L_x_2);
        /*00a0*/                   NOP;
        /*00b0*/                   NOP;
        /*00c0*/                   NOP;
        /*00d0*/                   NOP;
        /*00e0*/                   NOP;
        /*00f0*/                   NOP;
        /*0100*/                   NOP;
        /*0110*/                   NOP;
        /*0120*/                   NOP;
        /*0130*/                   NOP;
        /*0140*/                   NOP;
        /*0150*/                   NOP;
        /*0160*/                   NOP;
        /*0170*/                   NOP;
.L_x_5:
