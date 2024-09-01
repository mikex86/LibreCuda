	.headerflags	@"EF_CUDA_TEXMODE_UNIFIED EF_CUDA_64BIT_ADDRESS EF_CUDA_SM80 EF_CUDA_VIRTUAL_SM(EF_CUDA_SM80)"
	.elftype	@"ET_EXEC"


//--------------------- .debug_frame              --------------------------
	.section	.debug_frame,"",@progbits
.debug_frame:
        /*0000*/ 	.byte	0xff, 0xff, 0xff, 0xff, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff
        /*0010*/ 	.byte	0xff, 0xff, 0xff, 0xff, 0x03, 0x00, 0x04, 0x7c, 0xff, 0xff, 0xff, 0xff, 0x0f, 0x0c, 0x81, 0x80
        /*0020*/ 	.byte	0x80, 0x28, 0x00, 0x08, 0xff, 0x81, 0x80, 0x28, 0x08, 0x81, 0x80, 0x80, 0x28, 0x00, 0x00, 0x00
        /*0030*/ 	.byte	0xff, 0xff, 0xff, 0xff, 0x34, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        /*0040*/ 	.byte	0x00, 0x00, 0x00, 0x00
        /*0044*/ 	.dword	read_from_struct
        /*004c*/ 	.byte	0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 0x00, 0x00, 0x00, 0x04, 0x18, 0x00
        /*005c*/ 	.byte	0x00, 0x00, 0x0c, 0x81, 0x80, 0x80, 0x28, 0x00, 0x04, 0xfc, 0xff, 0xff, 0x3f, 0x00, 0x00, 0x00
        /*006c*/ 	.byte	0x00, 0x00, 0x00, 0x00


//--------------------- .nv.info                  --------------------------
	.section	.nv.info,"",@"SHT_CUDA_INFO"
	.align	4


	//----- nvinfo : EIATTR_REGCOUNT
	.align		4
        /*0000*/ 	.byte	0x04, 0x2f
        /*0002*/ 	.short	(.L_1 - .L_0)
	.align		4
.L_0:
        /*0004*/ 	.word	index@(read_from_struct)
        /*0008*/ 	.word	0x00000008


	//----- nvinfo : EIATTR_MIN_STACK_SIZE
	.align		4
.L_1:
        /*000c*/ 	.byte	0x04, 0x12
        /*000e*/ 	.short	(.L_3 - .L_2)
	.align		4
.L_2:
        /*0010*/ 	.word	index@(read_from_struct)
        /*0014*/ 	.word	0x00000000


	//----- nvinfo : EIATTR_FRAME_SIZE
	.align		4
.L_3:
        /*0018*/ 	.byte	0x04, 0x11
        /*001a*/ 	.short	(.L_5 - .L_4)
	.align		4
.L_4:
        /*001c*/ 	.word	index@(read_from_struct)
        /*0020*/ 	.word	0x00000000


	//----- nvinfo : EIATTR_MIN_STACK_SIZE
	.align		4
.L_5:
        /*0024*/ 	.byte	0x04, 0x12
        /*0026*/ 	.short	(.L_7 - .L_6)
	.align		4
.L_6:
        /*0028*/ 	.word	index@(read_from_struct)
        /*002c*/ 	.word	0x00000000
.L_7:


//--------------------- .nv.info.read_from_struct --------------------------
	.section	.nv.info.read_from_struct,"",@"SHT_CUDA_INFO"
	.sectionflags	@""
	.align	4


	//----- nvinfo : EIATTR_CUDA_API_VERSION
	.align		4
        /*0000*/ 	.byte	0x04, 0x37
        /*0002*/ 	.short	(.L_9 - .L_8)
.L_8:
        /*0004*/ 	.word	0x0000007c


	//----- nvinfo : EIATTR_SW2861232_WAR
	.align		4
.L_9:
        /*0008*/ 	.byte	0x01, 0x35
	.zero		2


	//----- nvinfo : EIATTR_PARAM_CBANK
	.align		4
        /*000c*/ 	.byte	0x04, 0x0a
        /*000e*/ 	.short	(.L_11 - .L_10)
	.align		4
.L_10:
        /*0010*/ 	.word	index@(.nv.constant0.read_from_struct)
        /*0014*/ 	.short	0x0160
        /*0016*/ 	.short	0x0040


	//----- nvinfo : EIATTR_CBANK_PARAM_SIZE
	.align		4
.L_11:
        /*0018*/ 	.byte	0x03, 0x19
        /*001a*/ 	.short	0x0040


	//----- nvinfo : EIATTR_KPARAM_INFO
	.align		4
        /*001c*/ 	.byte	0x04, 0x17
        /*001e*/ 	.short	(.L_13 - .L_12)
.L_12:
        /*0020*/ 	.word	0x00000000
        /*0024*/ 	.short	0x0001
        /*0026*/ 	.short	0x0038
        /*0028*/ 	.byte	0x00, 0xf0, 0x21, 0x00


	//----- nvinfo : EIATTR_KPARAM_INFO
	.align		4
.L_13:
        /*002c*/ 	.byte	0x04, 0x17
        /*002e*/ 	.short	(.L_15 - .L_14)
.L_14:
        /*0030*/ 	.word	0x00000000
        /*0034*/ 	.short	0x0000
        /*0036*/ 	.short	0x0000
        /*0038*/ 	.byte	0x00, 0xf0, 0xe1, 0x00


	//----- nvinfo : EIATTR_MAXREG_COUNT
	.align		4
.L_15:
        /*003c*/ 	.byte	0x03, 0x1b
        /*003e*/ 	.short	0x00ff


	//----- nvinfo : EIATTR_EXIT_INSTR_OFFSETS
	.align		4
        /*0040*/ 	.byte	0x04, 0x1c
        /*0042*/ 	.short	(.L_17 - .L_16)


	//   ....[0]....
.L_16:
        /*0044*/ 	.word	0x00000060
.L_17:


//--------------------- .nv.callgraph             --------------------------
	.section	.nv.callgraph,"",@"SHT_CUDA_CALLGRAPH"
	.align	4
	.sectionentsize	8
	.align		4
        /*0000*/ 	.word	0x00000000
	.align		4
        /*0004*/ 	.word	0xffffffff
	.align		4
        /*0008*/ 	.word	0x00000000
	.align		4
        /*000c*/ 	.word	0xfffffffe
	.align		4
        /*0010*/ 	.word	0x00000000
	.align		4
        /*0014*/ 	.word	0xfffffffd
	.align		4
        /*0018*/ 	.word	0x00000000
	.align		4
        /*001c*/ 	.word	0xfffffffc


//--------------------- .nv.rel.action            --------------------------
	.section	.nv.rel.action,"",@"SHT_CUDA_RELOCINFO"
	.align	8
	.sectionentsize	8
        /*0000*/ 	.byte	0x73, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x25, 0x00, 0x05, 0x36


//--------------------- .nv.constant0.read_from_struct --------------------------
	.section	.nv.constant0.read_from_struct,"a",@progbits
	.sectionflags	@""
	.align	4
.nv.constant0.read_from_struct:
	.zero		416


//--------------------- .text.read_from_struct    --------------------------
	.section	.text.read_from_struct,"ax",@progbits
	.sectioninfo	@"SHI_REGISTERS=8"
	.align	128
        .global         read_from_struct
        .type           read_from_struct,@function
        .size           read_from_struct,(.L_x_1 - read_from_struct)
        .other          read_from_struct,@"STO_CUDA_ENTRY STV_DEFAULT"
read_from_struct:
.text.read_from_struct:
        /*0000*/                   IMAD.MOV.U32 R1, RZ, RZ, c[0x0][0x28] ;
        /*0010*/                   IMAD.MOV.U32 R5, RZ, RZ, c[0x0][0x16c] ;
        /*0020*/                   MOV R2, c[0x0][0x198] ;
        /*0030*/                   ULDC.64 UR4, c[0x0][0x118] ;
        /*0040*/                   MOV R3, c[0x0][0x19c] ;
        /*0050*/                   STG.E [R2.64], R5 ;
        /*0060*/                   EXIT ;
.L_x_0:
        /*0070*/                   BRA `(.L_x_0);
        /*0080*/                   NOP;
        /*0090*/                   NOP;
        /*00a0*/                   NOP;
        /*00b0*/                   NOP;
        /*00c0*/                   NOP;
        /*00d0*/                   NOP;
        /*00e0*/                   NOP;
        /*00f0*/                   NOP;
.L_x_1:
