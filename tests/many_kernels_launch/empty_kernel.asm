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
        /*0044*/ 	.dword	emtpy_kernel
        /*004c*/ 	.byte	0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 0x00, 0x00, 0x00, 0x04, 0x04, 0x00
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
        /*0004*/ 	.word	index@(emtpy_kernel)
        /*0008*/ 	.word	0x00000004


	//----- nvinfo : EIATTR_MIN_STACK_SIZE
	.align		4
.L_1:
        /*000c*/ 	.byte	0x04, 0x12
        /*000e*/ 	.short	(.L_3 - .L_2)
	.align		4
.L_2:
        /*0010*/ 	.word	index@(emtpy_kernel)
        /*0014*/ 	.word	0x00000000


	//----- nvinfo : EIATTR_FRAME_SIZE
	.align		4
.L_3:
        /*0018*/ 	.byte	0x04, 0x11
        /*001a*/ 	.short	(.L_5 - .L_4)
	.align		4
.L_4:
        /*001c*/ 	.word	index@(emtpy_kernel)
        /*0020*/ 	.word	0x00000000


	//----- nvinfo : EIATTR_MIN_STACK_SIZE
	.align		4
.L_5:
        /*0024*/ 	.byte	0x04, 0x12
        /*0026*/ 	.short	(.L_7 - .L_6)
	.align		4
.L_6:
        /*0028*/ 	.word	index@(emtpy_kernel)
        /*002c*/ 	.word	0x00000000
.L_7:


//--------------------- .nv.info.emtpy_kernel     --------------------------
	.section	.nv.info.emtpy_kernel,"",@"SHT_CUDA_INFO"
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


	//----- nvinfo : EIATTR_MAXREG_COUNT
	.align		4
        /*000c*/ 	.byte	0x03, 0x1b
        /*000e*/ 	.short	0x00ff


	//----- nvinfo : EIATTR_EXIT_INSTR_OFFSETS
	.align		4
        /*0010*/ 	.byte	0x04, 0x1c
        /*0012*/ 	.short	(.L_11 - .L_10)


	//   ....[0]....
.L_10:
        /*0014*/ 	.word	0x00000010
.L_11:


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


//--------------------- .nv.constant0.emtpy_kernel --------------------------
	.section	.nv.constant0.emtpy_kernel,"a",@progbits
	.sectionflags	@""
	.align	4
.nv.constant0.emtpy_kernel:
	.zero		352


//--------------------- .text.emtpy_kernel        --------------------------
	.section	.text.emtpy_kernel,"ax",@progbits
	.sectioninfo	@"SHI_REGISTERS=4"
	.align	128
        .global         emtpy_kernel
        .type           emtpy_kernel,@function
        .size           emtpy_kernel,(.L_x_1 - emtpy_kernel)
        .other          emtpy_kernel,@"STO_CUDA_ENTRY STV_DEFAULT"
emtpy_kernel:
.text.emtpy_kernel:
        /*0000*/                   MOV R1, c[0x0][0x28] ;
        /*0010*/                   EXIT ;
.L_x_0:
        /*0020*/                   BRA `(.L_x_0);
        /*0030*/                   NOP;
        /*0040*/                   NOP;
        /*0050*/                   NOP;
        /*0060*/                   NOP;
        /*0070*/                   NOP;
        /*0080*/                   NOP;
        /*0090*/                   NOP;
        /*00a0*/                   NOP;
        /*00b0*/                   NOP;
        /*00c0*/                   NOP;
        /*00d0*/                   NOP;
        /*00e0*/                   NOP;
        /*00f0*/                   NOP;
.L_x_1:
