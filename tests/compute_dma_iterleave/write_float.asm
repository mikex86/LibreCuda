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
        /*0044*/ 	.dword	write_float
        /*004c*/ 	.byte	0x70, 0x16, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 0x00, 0x00, 0x00, 0x04, 0x08, 0x00
        /*005c*/ 	.byte	0x00, 0x00, 0x0c, 0x81, 0x80, 0x80, 0x28, 0x00, 0x04, 0x8c, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00
        /*006c*/ 	.byte	0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        /*007c*/ 	.byte	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x03, 0x00, 0x04, 0x7c, 0x80, 0x82, 0x80, 0x28
        /*008c*/ 	.byte	0x0c, 0x81, 0x80, 0x80, 0x28, 0x00, 0x08, 0xff, 0x81, 0x80, 0x28, 0x08, 0x81, 0x80, 0x80, 0x28
        /*009c*/ 	.byte	0x08, 0x80, 0x80, 0x80, 0x28, 0x16, 0x80, 0x82, 0x80, 0x28, 0x10, 0x03
        /*00a8*/ 	.dword	write_float
        /*00b0*/ 	.byte	0x92, 0x80, 0x80, 0x80, 0x28, 0x00, 0x22, 0x00, 0xff, 0xff, 0xff, 0xff, 0x2c, 0x00, 0x00, 0x00
        /*00c0*/ 	.byte	0x00, 0x00, 0x00, 0x00, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        /*00cc*/ 	.dword	(write_float + $__internal_0_$__cuda_sm20_div_rn_f64_full@srel)
        /*00d4*/ 	.byte	0x90, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x2c, 0x01, 0x00, 0x00, 0x09, 0x80, 0x80
        /*00e4*/ 	.byte	0x80, 0x28, 0x82, 0x80, 0x80, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00


//--------------------- .nv.info                  --------------------------
	.section	.nv.info,"",@"SHT_CUDA_INFO"
	.align	4


	//----- nvinfo : EIATTR_REGCOUNT
	.align		4
        /*0000*/ 	.byte	0x04, 0x2f
        /*0002*/ 	.short	(.L_1 - .L_0)
	.align		4
.L_0:
        /*0004*/ 	.word	index@(write_float)
        /*0008*/ 	.word	0x00000016


	//----- nvinfo : EIATTR_MIN_STACK_SIZE
	.align		4
.L_1:
        /*000c*/ 	.byte	0x04, 0x12
        /*000e*/ 	.short	(.L_3 - .L_2)
	.align		4
.L_2:
        /*0010*/ 	.word	index@($__internal_0_$__cuda_sm20_div_rn_f64_full)
        /*0014*/ 	.word	0x00000000


	//----- nvinfo : EIATTR_FRAME_SIZE
	.align		4
.L_3:
        /*0018*/ 	.byte	0x04, 0x11
        /*001a*/ 	.short	(.L_5 - .L_4)
	.align		4
.L_4:
        /*001c*/ 	.word	index@($__internal_0_$__cuda_sm20_div_rn_f64_full)
        /*0020*/ 	.word	0x00000000


	//----- nvinfo : EIATTR_MIN_STACK_SIZE
	.align		4
.L_5:
        /*0024*/ 	.byte	0x04, 0x12
        /*0026*/ 	.short	(.L_7 - .L_6)
	.align		4
.L_6:
        /*0028*/ 	.word	index@(write_float)
        /*002c*/ 	.word	0x00000000


	//----- nvinfo : EIATTR_FRAME_SIZE
	.align		4
.L_7:
        /*0030*/ 	.byte	0x04, 0x11
        /*0032*/ 	.short	(.L_9 - .L_8)
	.align		4
.L_8:
        /*0034*/ 	.word	index@(write_float)
        /*0038*/ 	.word	0x00000000


	//----- nvinfo : EIATTR_MIN_STACK_SIZE
	.align		4
.L_9:
        /*003c*/ 	.byte	0x04, 0x12
        /*003e*/ 	.short	(.L_11 - .L_10)
	.align		4
.L_10:
        /*0040*/ 	.word	index@(write_float)
        /*0044*/ 	.word	0x00000000
.L_11:


//--------------------- .nv.info.write_float      --------------------------
	.section	.nv.info.write_float,"",@"SHT_CUDA_INFO"
	.sectionflags	@""
	.align	4


	//----- nvinfo : EIATTR_CUDA_API_VERSION
	.align		4
        /*0000*/ 	.byte	0x04, 0x37
        /*0002*/ 	.short	(.L_13 - .L_12)
.L_12:
        /*0004*/ 	.word	0x0000007c


	//----- nvinfo : EIATTR_SW2861232_WAR
	.align		4
.L_13:
        /*0008*/ 	.byte	0x01, 0x35
	.zero		2


	//----- nvinfo : EIATTR_PARAM_CBANK
	.align		4
        /*000c*/ 	.byte	0x04, 0x0a
        /*000e*/ 	.short	(.L_15 - .L_14)
	.align		4
.L_14:
        /*0010*/ 	.word	index@(.nv.constant0.write_float)
        /*0014*/ 	.short	0x0160
        /*0016*/ 	.short	0x0010


	//----- nvinfo : EIATTR_CBANK_PARAM_SIZE
	.align		4
.L_15:
        /*0018*/ 	.byte	0x03, 0x19
        /*001a*/ 	.short	0x0010


	//----- nvinfo : EIATTR_KPARAM_INFO
	.align		4
        /*001c*/ 	.byte	0x04, 0x17
        /*001e*/ 	.short	(.L_17 - .L_16)
.L_16:
        /*0020*/ 	.word	0x00000000
        /*0024*/ 	.short	0x0001
        /*0026*/ 	.short	0x0008
        /*0028*/ 	.byte	0x00, 0xf0, 0x21, 0x00


	//----- nvinfo : EIATTR_KPARAM_INFO
	.align		4
.L_17:
        /*002c*/ 	.byte	0x04, 0x17
        /*002e*/ 	.short	(.L_19 - .L_18)
.L_18:
        /*0030*/ 	.word	0x00000000
        /*0034*/ 	.short	0x0000
        /*0036*/ 	.short	0x0000
        /*0038*/ 	.byte	0x00, 0xf0, 0x21, 0x00


	//----- nvinfo : EIATTR_MAXREG_COUNT
	.align		4
.L_19:
        /*003c*/ 	.byte	0x03, 0x1b
        /*003e*/ 	.short	0x00ff


	//----- nvinfo : EIATTR_EXIT_INSTR_OFFSETS
	.align		4
        /*0040*/ 	.byte	0x04, 0x1c
        /*0042*/ 	.short	(.L_21 - .L_20)


	//   ....[0]....
.L_20:
        /*0044*/ 	.word	0x00001660


	//----- nvinfo : EIATTR_CRS_STACK_SIZE
	.align		4
.L_21:
        /*0048*/ 	.byte	0x04, 0x1e
        /*004a*/ 	.short	(.L_23 - .L_22)
.L_22:
        /*004c*/ 	.word	0x00000000
.L_23:


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


//--------------------- .nv.constant0.write_float --------------------------
	.section	.nv.constant0.write_float,"a",@progbits
	.sectionflags	@""
	.align	4
.nv.constant0.write_float:
	.zero		368


//--------------------- .text.write_float         --------------------------
	.section	.text.write_float,"ax",@progbits
	.sectioninfo	@"SHI_REGISTERS=22"
	.align	128
        .global         write_float
        .type           write_float,@function
        .size           write_float,(.L_x_7 - write_float)
        .other          write_float,@"STO_CUDA_ENTRY STV_DEFAULT"
write_float:
.text.write_float:
        /*0000*/                   IMAD.MOV.U32 R1, RZ, RZ, c[0x0][0x28] ;
        /*0010*/                   IMAD.MOV.U32 R0, RZ, RZ, RZ ;
        /*0020*/                   CS2R R4, SRZ ;
.L_x_1:
        /*0030*/                   DADD R4, R4, 1 ;
        /*0040*/                   IADD3 R0, R0, 0x140, RZ ;
        /*0050*/                   ISETP.NE.AND P0, PT, R0, 0x5f5e100, PT ;
        /*0060*/                   DADD R4, R4, 1 ;
        /*0070*/                   DADD R4, R4, 1 ;
        /*0080*/                   DADD R4, R4, 1 ;
        /*0090*/                   DADD R4, R4, 1 ;
        /*00a0*/                   DADD R4, R4, 1 ;
        /*00b0*/                   DADD R4, R4, 1 ;
        /*00c0*/                   DADD R4, R4, 1 ;
        /*00d0*/                   DADD R4, R4, 1 ;
        /*00e0*/                   DADD R4, R4, 1 ;
        /*00f0*/                   DADD R4, R4, 1 ;
        /*0100*/                   DADD R4, R4, 1 ;
        /*0110*/                   DADD R4, R4, 1 ;
        /*0120*/                   DADD R4, R4, 1 ;
        /*0130*/                   DADD R4, R4, 1 ;
        /*0140*/                   DADD R4, R4, 1 ;
        /*0150*/                   DADD R4, R4, 1 ;
        /*0160*/                   DADD R4, R4, 1 ;
        /*0170*/                   DADD R4, R4, 1 ;
        /*0180*/                   DADD R4, R4, 1 ;
        /*0190*/                   DADD R4, R4, 1 ;
        /*01a0*/                   DADD R4, R4, 1 ;
        /*01b0*/                   DADD R4, R4, 1 ;
        /*01c0*/                   DADD R4, R4, 1 ;
        /*01d0*/                   DADD R4, R4, 1 ;
        /*01e0*/                   DADD R4, R4, 1 ;
        /*01f0*/                   DADD R4, R4, 1 ;
        /*0200*/                   DADD R4, R4, 1 ;
        /*0210*/                   DADD R4, R4, 1 ;
        /*0220*/                   DADD R4, R4, 1 ;
        /*0230*/                   DADD R4, R4, 1 ;
        /*0240*/                   DADD R4, R4, 1 ;
        /*0250*/                   DADD R4, R4, 1 ;
        /*0260*/                   DADD R4, R4, 1 ;
        /*0270*/                   DADD R4, R4, 1 ;
        /*0280*/                   DADD R4, R4, 1 ;
        /*0290*/                   DADD R4, R4, 1 ;
        /*02a0*/                   DADD R4, R4, 1 ;
        /*02b0*/                   DADD R4, R4, 1 ;
        /*02c0*/                   DADD R4, R4, 1 ;
        /*02d0*/                   DADD R4, R4, 1 ;
        /*02e0*/                   DADD R4, R4, 1 ;
        /*02f0*/                   DADD R4, R4, 1 ;
        /*0300*/                   DADD R4, R4, 1 ;
        /*0310*/                   DADD R4, R4, 1 ;
        /*0320*/                   DADD R4, R4, 1 ;
        /*0330*/                   DADD R4, R4, 1 ;
        /*0340*/                   DADD R4, R4, 1 ;
        /*0350*/                   DADD R4, R4, 1 ;
        /*0360*/                   DADD R4, R4, 1 ;
        /*0370*/                   DADD R4, R4, 1 ;
        /*0380*/                   DADD R4, R4, 1 ;
        /*0390*/                   DADD R4, R4, 1 ;
        /*03a0*/                   DADD R4, R4, 1 ;
        /*03b0*/                   DADD R4, R4, 1 ;
        /*03c0*/                   DADD R4, R4, 1 ;
        /*03d0*/                   DADD R4, R4, 1 ;
        /*03e0*/                   DADD R4, R4, 1 ;
        /*03f0*/                   DADD R4, R4, 1 ;
        /*0400*/                   DADD R4, R4, 1 ;
        /*0410*/                   DADD R4, R4, 1 ;
        /*0420*/                   DADD R4, R4, 1 ;
        /*0430*/                   DADD R4, R4, 1 ;
        /*0440*/                   DADD R4, R4, 1 ;
        /*0450*/                   DADD R4, R4, 1 ;
        /*0460*/                   DADD R4, R4, 1 ;
        /*0470*/                   DADD R4, R4, 1 ;
        /*0480*/                   DADD R4, R4, 1 ;
        /*0490*/                   DADD R4, R4, 1 ;
        /*04a0*/                   DADD R4, R4, 1 ;
        /*04b0*/                   DADD R4, R4, 1 ;
        /*04c0*/                   DADD R4, R4, 1 ;
        /*04d0*/                   DADD R4, R4, 1 ;
        /*04e0*/                   DADD R4, R4, 1 ;
        /*04f0*/                   DADD R4, R4, 1 ;
        /*0500*/                   DADD R4, R4, 1 ;
        /*0510*/                   DADD R4, R4, 1 ;
        /*0520*/                   DADD R4, R4, 1 ;
        /*0530*/                   DADD R4, R4, 1 ;
        /*0540*/                   DADD R4, R4, 1 ;
        /*0550*/                   DADD R4, R4, 1 ;
        /*0560*/                   DADD R4, R4, 1 ;
        /*0570*/                   DADD R4, R4, 1 ;
        /*0580*/                   DADD R4, R4, 1 ;
        /*0590*/                   DADD R4, R4, 1 ;
        /*05a0*/                   DADD R4, R4, 1 ;
        /*05b0*/                   DADD R4, R4, 1 ;
        /*05c0*/                   DADD R4, R4, 1 ;
        /*05d0*/                   DADD R4, R4, 1 ;
        /*05e0*/                   DADD R4, R4, 1 ;
        /*05f0*/                   DADD R4, R4, 1 ;
        /*0600*/                   DADD R4, R4, 1 ;
        /*0610*/                   DADD R4, R4, 1 ;
        /*0620*/                   DADD R4, R4, 1 ;
        /*0630*/                   DADD R4, R4, 1 ;
        /*0640*/                   DADD R4, R4, 1 ;
        /*0650*/                   DADD R4, R4, 1 ;
        /*0660*/                   DADD R4, R4, 1 ;
        /*0670*/                   DADD R4, R4, 1 ;
        /*0680*/                   DADD R4, R4, 1 ;
        /*0690*/                   DADD R4, R4, 1 ;
        /*06a0*/                   DADD R4, R4, 1 ;
        /*06b0*/                   DADD R4, R4, 1 ;
        /*06c0*/                   DADD R4, R4, 1 ;
        /*06d0*/                   DADD R4, R4, 1 ;
        /*06e0*/                   DADD R4, R4, 1 ;
        /*06f0*/                   DADD R4, R4, 1 ;
        /*0700*/                   DADD R4, R4, 1 ;
        /*0710*/                   DADD R4, R4, 1 ;
        /*0720*/                   DADD R4, R4, 1 ;
        /*0730*/                   DADD R4, R4, 1 ;
        /*0740*/                   DADD R4, R4, 1 ;
        /*0750*/                   DADD R4, R4, 1 ;
        /*0760*/                   DADD R4, R4, 1 ;
        /*0770*/                   DADD R4, R4, 1 ;
        /*0780*/                   DADD R4, R4, 1 ;
        /*0790*/                   DADD R4, R4, 1 ;
        /*07a0*/                   DADD R4, R4, 1 ;
        /*07b0*/                   DADD R4, R4, 1 ;
        /*07c0*/                   DADD R4, R4, 1 ;
        /*07d0*/                   DADD R4, R4, 1 ;
        /*07e0*/                   DADD R4, R4, 1 ;
        /*07f0*/                   DADD R4, R4, 1 ;
        /*0800*/                   DADD R4, R4, 1 ;
        /*0810*/                   DADD R4, R4, 1 ;
        /*0820*/                   DADD R4, R4, 1 ;
        /*0830*/                   DADD R4, R4, 1 ;
        /*0840*/                   DADD R4, R4, 1 ;
        /*0850*/                   DADD R4, R4, 1 ;
        /*0860*/                   DADD R4, R4, 1 ;
        /*0870*/                   DADD R4, R4, 1 ;
        /*0880*/                   DADD R4, R4, 1 ;
        /*0890*/                   DADD R4, R4, 1 ;
        /*08a0*/                   DADD R4, R4, 1 ;
        /*08b0*/                   DADD R4, R4, 1 ;
        /*08c0*/                   DADD R4, R4, 1 ;
        /*08d0*/                   DADD R4, R4, 1 ;
        /*08e0*/                   DADD R4, R4, 1 ;
        /*08f0*/                   DADD R4, R4, 1 ;
        /*0900*/                   DADD R4, R4, 1 ;
        /*0910*/                   DADD R4, R4, 1 ;
        /*0920*/                   DADD R4, R4, 1 ;
        /*0930*/                   DADD R4, R4, 1 ;
        /*0940*/                   DADD R4, R4, 1 ;
        /*0950*/                   DADD R4, R4, 1 ;
        /*0960*/                   DADD R4, R4, 1 ;
        /*0970*/                   DADD R4, R4, 1 ;
        /*0980*/                   DADD R4, R4, 1 ;
        /*0990*/                   DADD R4, R4, 1 ;
        /*09a0*/                   DADD R4, R4, 1 ;
        /*09b0*/                   DADD R4, R4, 1 ;
        /*09c0*/                   DADD R4, R4, 1 ;
        /*09d0*/                   DADD R4, R4, 1 ;
        /*09e0*/                   DADD R4, R4, 1 ;
        /*09f0*/                   DADD R4, R4, 1 ;
        /*0a00*/                   DADD R4, R4, 1 ;
        /*0a10*/                   DADD R4, R4, 1 ;
        /*0a20*/                   DADD R4, R4, 1 ;
        /*0a30*/                   DADD R4, R4, 1 ;
        /*0a40*/                   DADD R4, R4, 1 ;
        /*0a50*/                   DADD R4, R4, 1 ;
        /*0a60*/                   DADD R4, R4, 1 ;
        /*0a70*/                   DADD R4, R4, 1 ;
        /*0a80*/                   DADD R4, R4, 1 ;
        /*0a90*/                   DADD R4, R4, 1 ;
        /*0aa0*/                   DADD R4, R4, 1 ;
        /*0ab0*/                   DADD R4, R4, 1 ;
        /*0ac0*/                   DADD R4, R4, 1 ;
        /*0ad0*/                   DADD R4, R4, 1 ;
        /*0ae0*/                   DADD R4, R4, 1 ;
        /*0af0*/                   DADD R4, R4, 1 ;
        /*0b00*/                   DADD R4, R4, 1 ;
        /*0b10*/                   DADD R4, R4, 1 ;
        /*0b20*/                   DADD R4, R4, 1 ;
        /*0b30*/                   DADD R4, R4, 1 ;
        /*0b40*/                   DADD R4, R4, 1 ;
        /*0b50*/                   DADD R4, R4, 1 ;
        /*0b60*/                   DADD R4, R4, 1 ;
        /*0b70*/                   DADD R4, R4, 1 ;
        /*0b80*/                   DADD R4, R4, 1 ;
        /*0b90*/                   DADD R4, R4, 1 ;
        /*0ba0*/                   DADD R4, R4, 1 ;
        /*0bb0*/                   DADD R4, R4, 1 ;
        /*0bc0*/                   DADD R4, R4, 1 ;
        /*0bd0*/                   DADD R4, R4, 1 ;
        /*0be0*/                   DADD R4, R4, 1 ;
        /*0bf0*/                   DADD R4, R4, 1 ;
        /*0c00*/                   DADD R4, R4, 1 ;
        /*0c10*/                   DADD R4, R4, 1 ;
        /*0c20*/                   DADD R4, R4, 1 ;
        /*0c30*/                   DADD R4, R4, 1 ;
        /*0c40*/                   DADD R4, R4, 1 ;
        /*0c50*/                   DADD R4, R4, 1 ;
        /*0c60*/                   DADD R4, R4, 1 ;
        /*0c70*/                   DADD R4, R4, 1 ;
        /*0c80*/                   DADD R4, R4, 1 ;
        /*0c90*/                   DADD R4, R4, 1 ;
        /*0ca0*/                   DADD R4, R4, 1 ;
        /*0cb0*/                   DADD R4, R4, 1 ;
        /*0cc0*/                   DADD R4, R4, 1 ;
        /*0cd0*/                   DADD R4, R4, 1 ;
        /*0ce0*/                   DADD R4, R4, 1 ;
        /*0cf0*/                   DADD R4, R4, 1 ;
        /*0d00*/                   DADD R4, R4, 1 ;
        /*0d10*/                   DADD R4, R4, 1 ;
        /*0d20*/                   DADD R4, R4, 1 ;
        /*0d30*/                   DADD R4, R4, 1 ;
        /*0d40*/                   DADD R4, R4, 1 ;
        /*0d50*/                   DADD R4, R4, 1 ;
        /*0d60*/                   DADD R4, R4, 1 ;
        /*0d70*/                   DADD R4, R4, 1 ;
        /*0d80*/                   DADD R4, R4, 1 ;
        /*0d90*/                   DADD R4, R4, 1 ;
        /*0da0*/                   DADD R4, R4, 1 ;
        /*0db0*/                   DADD R4, R4, 1 ;
        /*0dc0*/                   DADD R4, R4, 1 ;
        /*0dd0*/                   DADD R4, R4, 1 ;
        /*0de0*/                   DADD R4, R4, 1 ;
        /*0df0*/                   DADD R4, R4, 1 ;
        /*0e00*/                   DADD R4, R4, 1 ;
        /*0e10*/                   DADD R4, R4, 1 ;
        /*0e20*/                   DADD R4, R4, 1 ;
        /*0e30*/                   DADD R4, R4, 1 ;
        /*0e40*/                   DADD R4, R4, 1 ;
        /*0e50*/                   DADD R4, R4, 1 ;
        /*0e60*/                   DADD R4, R4, 1 ;
        /*0e70*/                   DADD R4, R4, 1 ;
        /*0e80*/                   DADD R4, R4, 1 ;
        /*0e90*/                   DADD R4, R4, 1 ;
        /*0ea0*/                   DADD R4, R4, 1 ;
        /*0eb0*/                   DADD R4, R4, 1 ;
        /*0ec0*/                   DADD R4, R4, 1 ;
        /*0ed0*/                   DADD R4, R4, 1 ;
        /*0ee0*/                   DADD R4, R4, 1 ;
        /*0ef0*/                   DADD R4, R4, 1 ;
        /*0f00*/                   DADD R4, R4, 1 ;
        /*0f10*/                   DADD R4, R4, 1 ;
        /*0f20*/                   DADD R4, R4, 1 ;
        /*0f30*/                   DADD R4, R4, 1 ;
        /*0f40*/                   DADD R4, R4, 1 ;
        /*0f50*/                   DADD R4, R4, 1 ;
        /*0f60*/                   DADD R4, R4, 1 ;
        /*0f70*/                   DADD R4, R4, 1 ;
        /*0f80*/                   DADD R4, R4, 1 ;
        /*0f90*/                   DADD R4, R4, 1 ;
        /*0fa0*/                   DADD R4, R4, 1 ;
        /*0fb0*/                   DADD R4, R4, 1 ;
        /*0fc0*/                   DADD R4, R4, 1 ;
        /*0fd0*/                   DADD R4, R4, 1 ;
        /*0fe0*/                   DADD R4, R4, 1 ;
        /*0ff0*/                   DADD R4, R4, 1 ;
        /*1000*/                   DADD R4, R4, 1 ;
        /*1010*/                   DADD R4, R4, 1 ;
        /*1020*/                   DADD R4, R4, 1 ;
        /*1030*/                   DADD R4, R4, 1 ;
        /*1040*/                   DADD R2, R4, 1 ;
        /*1050*/                   DADD R2, R2, 1 ;
        /*1060*/                   DADD R2, R2, 1 ;
        /*1070*/                   DADD R2, R2, 1 ;
        /*1080*/                   DADD R2, R2, 1 ;
        /*1090*/                   DADD R2, R2, 1 ;
        /*10a0*/                   DADD R2, R2, 1 ;
        /*10b0*/                   DADD R2, R2, 1 ;
        /*10c0*/                   DADD R2, R2, 1 ;
        /*10d0*/                   DADD R2, R2, 1 ;
        /*10e0*/                   DADD R2, R2, 1 ;
        /*10f0*/                   DADD R2, R2, 1 ;
        /*1100*/                   DADD R2, R2, 1 ;
        /*1110*/                   DADD R2, R2, 1 ;
        /*1120*/                   DADD R2, R2, 1 ;
        /*1130*/                   DADD R2, R2, 1 ;
        /*1140*/                   DADD R2, R2, 1 ;
        /*1150*/                   DADD R2, R2, 1 ;
        /*1160*/                   DADD R2, R2, 1 ;
        /*1170*/                   DADD R2, R2, 1 ;
        /*1180*/                   DADD R2, R2, 1 ;
        /*1190*/                   DADD R2, R2, 1 ;
        /*11a0*/                   DADD R2, R2, 1 ;
        /*11b0*/                   DADD R2, R2, 1 ;
        /*11c0*/                   DADD R2, R2, 1 ;
        /*11d0*/                   DADD R2, R2, 1 ;
        /*11e0*/                   DADD R2, R2, 1 ;
        /*11f0*/                   DADD R2, R2, 1 ;
        /*1200*/                   DADD R2, R2, 1 ;
        /*1210*/                   DADD R2, R2, 1 ;
        /*1220*/                   DADD R2, R2, 1 ;
        /*1230*/                   DADD R2, R2, 1 ;
        /*1240*/                   DADD R2, R2, 1 ;
        /*1250*/                   DADD R2, R2, 1 ;
        /*1260*/                   DADD R2, R2, 1 ;
        /*1270*/                   DADD R2, R2, 1 ;
        /*1280*/                   DADD R2, R2, 1 ;
        /*1290*/                   DADD R2, R2, 1 ;
        /*12a0*/                   DADD R2, R2, 1 ;
        /*12b0*/                   DADD R2, R2, 1 ;
        /*12c0*/                   DADD R2, R2, 1 ;
        /*12d0*/                   DADD R2, R2, 1 ;
        /*12e0*/                   DADD R2, R2, 1 ;
        /*12f0*/                   DADD R2, R2, 1 ;
        /*1300*/                   DADD R2, R2, 1 ;
        /*1310*/                   DADD R2, R2, 1 ;
        /*1320*/                   DADD R2, R2, 1 ;
        /*1330*/                   DADD R2, R2, 1 ;
        /*1340*/                   DADD R2, R2, 1 ;
        /*1350*/                   DADD R2, R2, 1 ;
        /*1360*/                   DADD R2, R2, 1 ;
        /*1370*/                   DADD R2, R2, 1 ;
        /*1380*/                   DADD R2, R2, 1 ;
        /*1390*/                   DADD R2, R2, 1 ;
        /*13a0*/                   DADD R2, R2, 1 ;
        /*13b0*/                   DADD R2, R2, 1 ;
        /*13c0*/                   DADD R2, R2, 1 ;
        /*13d0*/                   DADD R2, R2, 1 ;
        /*13e0*/                   DADD R2, R2, 1 ;
        /*13f0*/                   DADD R2, R2, 1 ;
        /*1400*/                   DADD R2, R2, 1 ;
        /*1410*/                   DADD R2, R2, 1 ;
        /*1420*/                   DADD R2, R2, 1 ;
        /*1430*/                   DADD R2, R2, 1 ;
        /*1440*/                   DADD R4, R2, 1 ;
        /*1450*/              @!P0 CALL.REL.NOINC `(.L_x_0) ;
        /*1460*/                   BRA `(.L_x_1) ;
.L_x_0:
        /*1470*/                   NOP ;
        /*1480*/                   NOP ;
        /*1490*/                   MUFU.RCP64H R3, 100000000 ;
        /*14a0*/                   IMAD.MOV.U32 R8, RZ, RZ, 0x0 ;
        /*14b0*/                   FSETP.GEU.AND P1, PT, |R5|, 6.5827683646048100446e-37, PT ;
        /*14c0*/                   IMAD.MOV.U32 R9, RZ, RZ, 0x4197d784 ;
        /*14d0*/                   ULDC.64 UR4, c[0x0][0x118] ;
        /*14e0*/                   IMAD.MOV.U32 R2, RZ, RZ, 0x1 ;
        /*14f0*/                   DFMA R6, R2, -R8, 1 ;
        /*1500*/                   DFMA R6, R6, R6, R6 ;
        /*1510*/                   DFMA R6, R2, R6, R2 ;
        /*1520*/                   DFMA R2, R6, -R8, 1 ;
        /*1530*/                   DFMA R2, R6, R2, R6 ;
        /*1540*/                   DMUL R6, R4, R2 ;
        /*1550*/                   DFMA R8, R6, -100000000, R4 ;
        /*1560*/                   DFMA R2, R2, R8, R6 ;
        /*1570*/                   FFMA R0, RZ, 18.98023223876953125, R3 ;
        /*1580*/                   FSETP.GT.AND P0, PT, |R0|, 1.469367938527859385e-39, PT ;
        /*1590*/               @P0 BRA P1, `(.L_x_2) ;
        /*15a0*/                   MOV R0, 0x15c0 ;
        /*15b0*/                   CALL.REL.NOINC `($__internal_0_$__cuda_sm20_div_rn_f64_full) ;
        /*15c0*/                   IMAD.MOV.U32 R2, RZ, RZ, R8 ;
        /*15d0*/                   IMAD.MOV.U32 R3, RZ, RZ, R9 ;
.L_x_2:
        /*15e0*/                   IMAD.MOV.U32 R5, RZ, RZ, c[0x0][0x16c] ;
        /*15f0*/                   IMAD.MOV.U32 R4, RZ, RZ, c[0x0][0x168] ;
        /*1600*/                   LDG.E R5, [R4.64] ;
        /*1610*/                   F2F.F32.F64 R2, R2 ;
        /*1620*/                   IMAD.MOV.U32 R6, RZ, RZ, c[0x0][0x160] ;
        /*1630*/                   IMAD.MOV.U32 R7, RZ, RZ, c[0x0][0x164] ;
        /*1640*/                   FADD R9, R2, R5 ;
        /*1650*/                   STG.E [R6.64], R9 ;
        /*1660*/                   EXIT ;
        .weak           $__internal_0_$__cuda_sm20_div_rn_f64_full
        .type           $__internal_0_$__cuda_sm20_div_rn_f64_full,@function
        .size           $__internal_0_$__cuda_sm20_div_rn_f64_full,(.L_x_7 - $__internal_0_$__cuda_sm20_div_rn_f64_full)
$__internal_0_$__cuda_sm20_div_rn_f64_full:
        /*1670*/                   IMAD.MOV.U32 R3, RZ, RZ, 0x3ff7d784 ;
        /*1680*/                   FSETP.GEU.AND P1, PT, |R5|.reuse, 1.469367938527859385e-39, PT ;
        /*1690*/                   IMAD.MOV.U32 R2, RZ, RZ, 0x0 ;
        /*16a0*/                   LOP3.LUT R8, R5, 0x7ff00000, RZ, 0xc0, !PT ;
        /*16b0*/                   MUFU.RCP64H R7, R3 ;
        /*16c0*/                   IMAD.MOV.U32 R6, RZ, RZ, 0x1 ;
        /*16d0*/                   IMAD.MOV.U32 R9, RZ, RZ, 0x1ca00000 ;
        /*16e0*/                   ISETP.GE.U32.AND P0, PT, R8, 0x41900000, PT ;
        /*16f0*/                   IMAD.MOV.U32 R16, RZ, RZ, R8 ;
        /*1700*/                   IMAD.MOV.U32 R17, RZ, RZ, 0x41900000 ;
        /*1710*/                   SEL R9, R9, 0x63400000, !P0 ;
        /*1720*/              @!P1 LOP3.LUT R14, R9, 0x80000000, R5, 0xf8, !PT ;
        /*1730*/                   IADD3 R19, R17, -0x1, RZ ;
        /*1740*/                   DFMA R10, R6, -R2, 1 ;
        /*1750*/                   DFMA R10, R10, R10, R10 ;
        /*1760*/                   DFMA R12, R6, R10, R6 ;
        /*1770*/                   LOP3.LUT R7, R9, 0x800fffff, R5, 0xf8, !PT ;
        /*1780*/                   IMAD.MOV.U32 R6, RZ, RZ, R4 ;
        /*1790*/              @!P1 LOP3.LUT R11, R14, 0x100000, RZ, 0xfc, !PT ;
        /*17a0*/              @!P1 IMAD.MOV.U32 R10, RZ, RZ, RZ ;
        /*17b0*/                   DFMA R14, R12, -R2, 1 ;
        /*17c0*/              @!P1 DFMA R6, R6, 2, -R10 ;
        /*17d0*/                   DFMA R14, R12, R14, R12 ;
        /*17e0*/              @!P1 LOP3.LUT R16, R7, 0x7ff00000, RZ, 0xc0, !PT ;
        /*17f0*/                   DMUL R10, R14, R6 ;
        /*1800*/                   IADD3 R18, R16, -0x1, RZ ;
        /*1810*/                   DFMA R12, R10, -R2, R6 ;
        /*1820*/                   ISETP.GT.U32.AND P0, PT, R18, 0x7feffffe, PT ;
        /*1830*/                   ISETP.GT.U32.OR P0, PT, R19, 0x7feffffe, P0 ;
        /*1840*/                   DFMA R10, R14, R12, R10 ;
        /*1850*/               @P0 BRA `(.L_x_3) ;
        /*1860*/                   IADD3 R8, R8, -0x41900000, RZ ;
        /*1870*/                   IMAD.MOV.U32 R4, RZ, RZ, RZ ;
        /*1880*/                   IMNMX R8, R8, -0x46a00000, !PT ;
        /*1890*/                   IMNMX R8, R8, 0x46a00000, PT ;
        /*18a0*/                   IMAD.IADD R12, R8, 0x1, -R9 ;
        /*18b0*/                   IADD3 R5, R12, 0x7fe00000, RZ ;
        /*18c0*/                   DMUL R8, R10, R4 ;
        /*18d0*/                   FSETP.GTU.AND P0, PT, |R9|, 1.469367938527859385e-39, PT ;
        /*18e0*/               @P0 BRA `(.L_x_4) ;
        /*18f0*/                   DFMA R2, R10, -R2, R6 ;
        /*1900*/                   IMAD.MOV.U32 R4, RZ, RZ, RZ ;
        /*1910*/                   FSETP.NEU.AND P0, PT, R3.reuse, RZ, PT ;
        /*1920*/                   LOP3.LUT R2, R3, 0x4197d784, RZ, 0x3c, !PT ;
        /*1930*/                   LOP3.LUT R7, R2, 0x80000000, RZ, 0xc0, !PT ;
        /*1940*/                   LOP3.LUT R5, R7, R5, RZ, 0xfc, !PT ;
        /*1950*/              @!P0 BRA `(.L_x_4) ;
        /*1960*/                   IMAD.MOV R3, RZ, RZ, -R12 ;
        /*1970*/                   DMUL.RP R4, R10, R4 ;
        /*1980*/                   IMAD.MOV.U32 R2, RZ, RZ, RZ ;
        /*1990*/                   DFMA R2, R8, -R2, R10 ;
        /*19a0*/                   LOP3.LUT R7, R5, R7, RZ, 0x3c, !PT ;
        /*19b0*/                   IADD3 R2, -R12, -0x43300000, RZ ;
        /*19c0*/                   FSETP.NEU.AND P0, PT, |R3|, R2, PT ;
        /*19d0*/                   FSEL R8, R4, R8, !P0 ;
        /*19e0*/                   FSEL R9, R7, R9, !P0 ;
        /*19f0*/                   BRA `(.L_x_4) ;
.L_x_3:
        /*1a00*/                   DSETP.NAN.AND P0, PT, R4, R4, PT ;
        /*1a10*/               @P0 BRA `(.L_x_5) ;
        /*1a20*/                   ISETP.NE.AND P0, PT, R16, R17, PT ;
        /*1a30*/                   IMAD.MOV.U32 R8, RZ, RZ, 0x0 ;
        /*1a40*/                   IMAD.MOV.U32 R9, RZ, RZ, -0x80000 ;
        /*1a50*/              @!P0 BRA `(.L_x_4) ;
        /*1a60*/                   ISETP.NE.AND P0, PT, R16, 0x7ff00000, PT ;
        /*1a70*/                   LOP3.LUT R4, R5, 0x4197d784, RZ, 0x3c, !PT ;
        /*1a80*/                   ISETP.EQ.OR P0, PT, R17, RZ, !P0 ;
        /*1a90*/                   LOP3.LUT R9, R4, 0x80000000, RZ, 0xc0, !PT ;
        /*1aa0*/               @P0 LOP3.LUT R2, R9, 0x7ff00000, RZ, 0xfc, !PT ;
        /*1ab0*/              @!P0 IMAD.MOV.U32 R8, RZ, RZ, RZ ;
        /*1ac0*/               @P0 IMAD.MOV.U32 R8, RZ, RZ, RZ ;
        /*1ad0*/               @P0 IMAD.MOV.U32 R9, RZ, RZ, R2 ;
        /*1ae0*/                   BRA `(.L_x_4) ;
.L_x_5:
        /*1af0*/                   LOP3.LUT R9, R5, 0x80000, RZ, 0xfc, !PT ;
        /*1b00*/                   IMAD.MOV.U32 R8, RZ, RZ, R4 ;
.L_x_4:
        /*1b10*/                   IMAD.MOV.U32 R2, RZ, RZ, R0 ;
        /*1b20*/                   IMAD.MOV.U32 R3, RZ, RZ, 0x0 ;
        /*1b30*/                   RET.REL.NODEC R2 `(write_float) ;
.L_x_6:
        /*1b40*/                   BRA `(.L_x_6);
        /*1b50*/                   NOP;
        /*1b60*/                   NOP;
        /*1b70*/                   NOP;
        /*1b80*/                   NOP;
        /*1b90*/                   NOP;
        /*1ba0*/                   NOP;
        /*1bb0*/                   NOP;
        /*1bc0*/                   NOP;
        /*1bd0*/                   NOP;
        /*1be0*/                   NOP;
        /*1bf0*/                   NOP;
.L_x_7:
