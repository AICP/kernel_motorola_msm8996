uint32_t nve0_grgpc_data[] = {
/* 0x0000: gpc_id */
	0x00000000,
/* 0x0004: gpc_mmio_list_head */
	0x00000000,
/* 0x0008: gpc_mmio_list_tail */
	0x00000000,
/* 0x000c: tpc_count */
	0x00000000,
/* 0x0010: tpc_mask */
	0x00000000,
/* 0x0014: tpc_mmio_list_head */
	0x00000000,
/* 0x0018: tpc_mmio_list_tail */
	0x00000000,
/* 0x001c: unk_count */
	0x00000001,
/* 0x0020: unk_mask */
	0x00000001,
/* 0x0024: unk_mmio_list_head */
	0x00000220,
/* 0x0028: unk_mmio_list_tail */
	0x00000238,
/* 0x002c: cmd_queue */
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
/* 0x0074: chipsets */
	0x000000e4,
	0x011400a8,
	0x01d00184,
	0x000000e7,
	0x011400a8,
	0x01d00184,
	0x000000e6,
	0x011400a8,
	0x01d00184,
	0x000000f0,
	0x01840114,
	0x022001d0,
	0x00000000,
/* 0x00a8: nve4_gpc_mmio_head */
	0x00000380,
	0x04000400,
	0x0800040c,
	0x20000450,
	0x00000600,
	0x00000684,
	0x10000700,
	0x00000800,
	0x08000808,
	0x00000828,
	0x00000830,
	0x000008d8,
	0x000008e0,
	0x140008e8,
	0x0000091c,
	0x08000924,
	0x00000b00,
	0x14000b08,
	0x00000bb8,
	0x00000c08,
	0x1c000c10,
	0x00000c40,
	0x00000c6c,
	0x00000c80,
	0x00000c8c,
	0x08001000,
	0x00001014,
/* 0x0114: nve4_gpc_mmio_tail */
/* 0x0114: nvf0_gpc_mmio_head */
	0x00000380,
	0x04000400,
	0x0800040c,
	0x20000450,
	0x00000600,
	0x00000684,
	0x10000700,
	0x00000800,
	0x08000808,
	0x00000828,
	0x00000830,
	0x000008d8,
	0x000008e0,
	0x140008e8,
	0x0000091c,
	0x08000924,
	0x00000b00,
	0x14000b08,
	0x00000bb8,
	0x00000c08,
	0x1c000c10,
	0x00000c40,
	0x00000c6c,
	0x00000c80,
	0x00000c8c,
	0x00000d24,
	0x08001000,
	0x00001014,
/* 0x0184: nvf0_gpc_mmio_tail */
/* 0x0184: nve4_tpc_mmio_head */
	0x00000048,
	0x00000064,
	0x00000088,
	0x14000200,
	0x0400021c,
	0x00000230,
	0x000002c4,
	0x08000400,
	0x08000420,
	0x000004e8,
	0x000004f4,
	0x0c000604,
	0x54000644,
	0x040006ac,
	0x000006c8,
	0x1c000730,
	0x00000758,
	0x00000770,
	0x04000778,
/* 0x01d0: nve4_tpc_mmio_tail */
/* 0x01d0: nvf0_tpc_mmio_head */
	0x00000048,
	0x00000064,
	0x00000088,
	0x14000200,
	0x0400021c,
	0x00000230,
	0x000002c4,
	0x08000400,
	0x08000420,
	0x000004e8,
	0x000004f4,
	0x0c000604,
	0x54000644,
	0x040006ac,
	0x000006b8,
	0x000006c8,
	0x1c000730,
	0x00000758,
	0x00000770,
	0x04000778,
/* 0x0220: nvf0_tpc_mmio_tail */
/* 0x0220: nve4_unk_mmio_head */
	0x00000024,
	0x040000c0,
	0x000000e4,
	0x14000100,
	0x000001d0,
	0x040001e0,
};

uint32_t nve0_grgpc_code[] = {
	0x03060ef5,
/* 0x0004: queue_put */
	0x9800d898,
	0x86f001d9,
	0x0489b808,
	0xf00c1bf4,
	0x21f502f7,
	0x00f802ec,
/* 0x001c: queue_put_next */
	0xb60798c4,
	0x8dbb0384,
	0x0880b600,
	0x80008e80,
	0x90b6018f,
	0x0f94f001,
	0xf801d980,
/* 0x0039: queue_get */
	0x0131f400,
	0x9800d898,
	0x89b801d9,
	0x210bf404,
	0xb60789c4,
	0x9dbb0394,
	0x0890b600,
	0x98009e98,
	0x80b6019f,
	0x0f84f001,
	0xf400d880,
/* 0x0066: queue_get_done */
	0x00f80132,
/* 0x0068: nv_rd32 */
	0x0728b7f1,
	0xb906b4b6,
	0xc9f002ec,
	0x00bcd01f,
/* 0x0078: nv_rd32_wait */
	0xc800bccf,
	0x1bf41fcc,
	0x06a7f0fa,
	0x010321f5,
	0xf840bfcf,
/* 0x008d: nv_wr32 */
	0x28b7f100,
	0x06b4b607,
	0xb980bfd0,
	0xc9f002ec,
	0x1ec9f01f,
/* 0x00a3: nv_wr32_wait */
	0xcf00bcd0,
	0xccc800bc,
	0xfa1bf41f,
/* 0x00ae: watchdog_reset */
	0x87f100f8,
	0x84b60430,
	0x1ff9f006,
	0xf8008fd0,
/* 0x00bd: watchdog_clear */
	0x3087f100,
	0x0684b604,
	0xf80080d0,
/* 0x00c9: wait_donez */
	0x3c87f100,
	0x0684b608,
	0x99f094bd,
	0x0089d000,
	0x081887f1,
	0xd00684b6,
/* 0x00e2: wait_donez_ne */
	0x87f1008a,
	0x84b60400,
	0x0088cf06,
	0xf4888aff,
	0x87f1f31b,
	0x84b6085c,
	0xf094bd06,
	0x89d00099,
/* 0x0103: wait_doneo */
	0xf100f800,
	0xb6083c87,
	0x94bd0684,
	0xd00099f0,
	0x87f10089,
	0x84b60818,
	0x008ad006,
/* 0x011c: wait_doneo_e */
	0x040087f1,
	0xcf0684b6,
	0x8aff0088,
	0xf30bf488,
	0x085c87f1,
	0xbd0684b6,
	0x0099f094,
	0xf80089d0,
/* 0x013d: mmctx_size */
/* 0x013f: nv_mmctx_size_loop */
	0x9894bd00,
	0x85b600e8,
	0x0180b61a,
	0xbb0284b6,
	0xe0b60098,
	0x04efb804,
	0xb9eb1bf4,
	0x00f8029f,
/* 0x015c: mmctx_xfer */
	0x083c87f1,
	0xbd0684b6,
	0x0199f094,
	0xf10089d0,
	0xb6071087,
	0x94bd0684,
	0xf405bbfd,
	0x8bd0090b,
	0x0099f000,
/* 0x0180: mmctx_base_disabled */
	0xf405eefd,
	0x8ed00c0b,
	0xc08fd080,
/* 0x018f: mmctx_multi_disabled */
	0xb70199f0,
	0xc8010080,
	0xb4b600ab,
	0x0cb9f010,
	0xb601aec8,
	0xbefd11e4,
	0x008bd005,
/* 0x01a8: mmctx_exec_loop */
/* 0x01a8: mmctx_wait_free */
	0xf0008ecf,
	0x0bf41fe4,
	0x00ce98fa,
	0xd005e9fd,
	0xc0b6c08e,
	0x04cdb804,
	0xc8e81bf4,
	0x1bf402ab,
/* 0x01c9: mmctx_fini_wait */
	0x008bcf18,
	0xb01fb4f0,
	0x1bf410b4,
	0x02a7f0f7,
	0xf4c921f4,
/* 0x01de: mmctx_stop */
	0xabc81b0e,
	0x10b4b600,
	0xf00cb9f0,
	0x8bd012b9,
/* 0x01ed: mmctx_stop_wait */
	0x008bcf00,
	0xf412bbc8,
/* 0x01f6: mmctx_done */
	0x87f1fa1b,
	0x84b6085c,
	0xf094bd06,
	0x89d00199,
/* 0x0207: strand_wait */
	0xf900f800,
	0x02a7f0a0,
	0xfcc921f4,
/* 0x0213: strand_pre */
	0xf100f8a0,
	0xf04afc87,
	0x97f00283,
	0x0089d00c,
	0x020721f5,
/* 0x0226: strand_post */
	0x87f100f8,
	0x83f04afc,
	0x0d97f002,
	0xf50089d0,
	0xf8020721,
/* 0x0239: strand_set */
	0xfca7f100,
	0x02a3f04f,
	0x0500aba2,
	0xd00fc7f0,
	0xc7f000ac,
	0x00bcd00b,
	0x020721f5,
	0xf000aed0,
	0xbcd00ac7,
	0x0721f500,
/* 0x0263: strand_ctx_init */
	0xf100f802,
	0xb6083c87,
	0x94bd0684,
	0xd00399f0,
	0x21f50089,
	0xe7f00213,
	0x3921f503,
	0xfca7f102,
	0x02a3f046,
	0x0400aba0,
	0xf040a0d0,
	0xbcd001c7,
	0x0721f500,
	0x010c9202,
	0xf000acd0,
	0xbcd002c7,
	0x0721f500,
	0x2621f502,
	0x8087f102,
	0x0684b608,
	0xb70089cf,
	0x95220080,
/* 0x02ba: ctx_init_strand_loop */
	0x8ed008fe,
	0x408ed000,
	0xb6808acf,
	0xa0b606a5,
	0x00eabb01,
	0xb60480b6,
	0x1bf40192,
	0x08e4b6e8,
	0xf1f2efbc,
	0xb6085c87,
	0x94bd0684,
	0xd00399f0,
	0x00f80089,
/* 0x02ec: error */
	0xe7f1e0f9,
	0xe3f09814,
	0x8d21f440,
	0x041ce0b7,
	0xf401f7f0,
	0xe0fc8d21,
/* 0x0306: init */
	0x04bd00f8,
	0xf10004fe,
	0xf0120017,
	0x12d00227,
	0x5417f100,
	0x0010fe04,
	0x040017f1,
	0xf0c010d0,
	0x12d00427,
	0x1031f400,
	0x060817f1,
	0xcf0614b6,
	0x37f00012,
	0x1f24f001,
	0xb60432bb,
	0x02800132,
	0x04038003,
	0x040010b7,
	0x800012cf,
	0x27f10002,
	0x24b60800,
	0x0022cf06,
/* 0x035f: init_find_chipset */
	0xb66817f0,
	0x13980c10,
	0x0432b800,
	0xb00b0bf4,
	0x1bf40034,
/* 0x0373: init_context */
	0xf100f8f1,
	0xb6080027,
	0x22cf0624,
	0xf134bd40,
	0xb6070047,
	0x25950644,
	0x0045d008,
	0xbd4045d0,
	0x58f4bde4,
	0x1f58021e,
	0x020e4003,
	0xf5040f40,
	0xbb013d21,
	0x3fbb002f,
	0x041e5800,
	0x40051f58,
	0x0f400a0e,
	0x3d21f50c,
	0x030e9801,
	0xbb00effd,
	0x3ebb002e,
	0x090e9800,
	0xf50a0f98,
	0x98013d21,
	0xeffd070e,
	0x002ebb00,
	0xb7003ebb,
	0xb6130040,
	0x43d00235,
	0x0825b600,
	0xb60635b6,
	0x30b60120,
	0x0824b601,
	0xb90834b6,
	0x21f5022f,
	0x3fbb0263,
	0x0017f100,
	0x0614b608,
	0xb74013d0,
	0xbd080010,
	0x1f29f024,
/* 0x0417: main */
	0xf40012d0,
	0x28f40031,
	0x2cd7f000,
	0xf43921f4,
	0xe4b0f401,
	0x1e18f404,
	0xf00181fe,
	0x20bd0627,
	0xb60412fd,
	0x1efd01e4,
	0x0018fe05,
	0x04d921f5,
/* 0x0447: main_not_ctx_xfer */
	0x94d30ef4,
	0xf5f010ef,
	0xec21f501,
	0xc60ef402,
/* 0x0454: ih */
	0x88fe80f9,
	0xf980f901,
	0xf9a0f990,
	0xf9d0f9b0,
	0xcff0f9e0,
	0xabc4800a,
	0x1d0bf404,
	0x1900b7f1,
	0xcf2cd7f0,
	0xbfcf40be,
	0x0421f400,
	0x0400b0b7,
	0xd001e7f0,
/* 0x048a: ih_no_fifo */
	0x0ad000be,
	0xfcf0fc40,
	0xfcd0fce0,
	0xfca0fcb0,
	0xfe80fc90,
	0x80fc0088,
	0xf80032f4,
/* 0x04a5: hub_barrier_done */
	0x01f7f001,
	0xbb000e98,
	0xe7f104fe,
	0xe3f09418,
	0x8d21f440,
/* 0x04ba: ctx_redswitch */
	0xe7f100f8,
	0xe4b60614,
	0x20f7f006,
	0xf000efd0,
/* 0x04ca: ctx_redswitch_delay */
	0xf2b608f7,
	0xfd1bf401,
	0x0a20f7f1,
	0xf800efd0,
/* 0x04d9: ctx_xfer */
	0x0417f100,
	0x0614b60a,
	0xf4001fd0,
	0x21f50711,
/* 0x04ea: ctx_xfer_not_load */
	0x17f104ba,
	0x13f04afc,
	0x0c27f002,
	0xf50012d0,
	0xf1020721,
	0xf047fc27,
	0x20d00223,
	0x012cf000,
	0xd00320b6,
	0xacf00012,
	0x02a5f001,
	0xf000b7f0,
	0x0c9850b3,
	0x0fc4b600,
	0x9800bcbb,
	0x0d98010c,
	0x00e7f002,
	0x015c21f5,
	0xf101acf0,
	0xf04000b7,
	0x0c9850b3,
	0x0fc4b600,
	0x9800bcbb,
	0x0d98050c,
	0x040f9806,
	0x0800e7f1,
	0x015c21f5,
	0xf001acf0,
	0xb7f104a5,
	0xb3f03000,
	0x000c9850,
	0xbb0fc4b6,
	0x0c9800bc,
	0x0a0d9809,
	0xf1080f98,
	0xf50200e7,
	0xf5015c21,
	0xf4020721,
	0x12f40601,
/* 0x0585: ctx_xfer_post */
	0xfc17f114,
	0x0213f04a,
	0xd00d27f0,
	0x21f50012,
/* 0x0596: ctx_xfer_done */
	0x21f50207,
	0x00f804a5,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
};
