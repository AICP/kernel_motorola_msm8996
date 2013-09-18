uint32_t nvd7_grgpc_data[] = {
/* 0x0000: gpc_mmio_list_head */
	0x0000006c,
/* 0x0004: gpc_mmio_list_tail */
/* 0x0004: tpc_mmio_list_head */
	0x0000006c,
/* 0x0008: tpc_mmio_list_tail */
/* 0x0008: unk_mmio_list_head */
	0x0000006c,
/* 0x000c: unk_mmio_list_tail */
	0x0000006c,
/* 0x0010: gpc_id */
	0x00000000,
/* 0x0014: tpc_count */
	0x00000000,
/* 0x0018: tpc_mask */
	0x00000000,
/* 0x001c: unk_count */
	0x00000000,
/* 0x0020: unk_mask */
	0x00000000,
/* 0x0024: cmd_queue */
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

uint32_t nvd7_grgpc_code[] = {
	0x03180ef5,
/* 0x0004: queue_put */
	0x9800d898,
	0x86f001d9,
	0x0489b808,
	0xf00c1bf4,
	0x21f502f7,
	0x00f802fe,
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
	0x010921f5,
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
	0xf094bd00,
	0x07f10099,
	0x03f00f00,
	0x0009d002,
	0x07f104bd,
	0x03f00600,
	0x000ad002,
/* 0x00e6: wait_donez_ne */
	0x87f104bd,
	0x83f00000,
	0x0088cf01,
	0xf4888aff,
	0x94bdf31b,
	0xf10099f0,
	0xf0170007,
	0x09d00203,
	0xf804bd00,
/* 0x0109: wait_doneo */
	0xf094bd00,
	0x07f10099,
	0x03f00f00,
	0x0009d002,
	0x87f104bd,
	0x84b60818,
	0x008ad006,
/* 0x0124: wait_doneo_e */
	0x040087f1,
	0xcf0684b6,
	0x8aff0088,
	0xf30bf488,
	0x99f094bd,
	0x0007f100,
	0x0203f017,
	0xbd0009d0,
/* 0x0147: mmctx_size */
	0xbd00f804,
/* 0x0149: nv_mmctx_size_loop */
	0x00e89894,
	0xb61a85b6,
	0x84b60180,
	0x0098bb02,
	0xb804e0b6,
	0x1bf404ef,
	0x029fb9eb,
/* 0x0166: mmctx_xfer */
	0x94bd00f8,
	0xf10199f0,
	0xf00f0007,
	0x09d00203,
	0xf104bd00,
	0xb6071087,
	0x94bd0684,
	0xf405bbfd,
	0x8bd0090b,
	0x0099f000,
/* 0x018c: mmctx_base_disabled */
	0xf405eefd,
	0x8ed00c0b,
	0xc08fd080,
/* 0x019b: mmctx_multi_disabled */
	0xb70199f0,
	0xc8010080,
	0xb4b600ab,
	0x0cb9f010,
	0xb601aec8,
	0xbefd11e4,
	0x008bd005,
/* 0x01b4: mmctx_exec_loop */
/* 0x01b4: mmctx_wait_free */
	0xf0008ecf,
	0x0bf41fe4,
	0x00ce98fa,
	0xd005e9fd,
	0xc0b6c08e,
	0x04cdb804,
	0xc8e81bf4,
	0x1bf402ab,
/* 0x01d5: mmctx_fini_wait */
	0x008bcf18,
	0xb01fb4f0,
	0x1bf410b4,
	0x02a7f0f7,
	0xf4c921f4,
/* 0x01ea: mmctx_stop */
	0xabc81b0e,
	0x10b4b600,
	0xf00cb9f0,
	0x8bd012b9,
/* 0x01f9: mmctx_stop_wait */
	0x008bcf00,
	0xf412bbc8,
/* 0x0202: mmctx_done */
	0x94bdfa1b,
	0xf10199f0,
	0xf0170007,
	0x09d00203,
	0xf804bd00,
/* 0x0215: strand_wait */
	0xf0a0f900,
	0x21f402a7,
	0xf8a0fcc9,
/* 0x0221: strand_pre */
	0xfc87f100,
	0x0283f04a,
	0xd00c97f0,
	0x21f50089,
	0x00f80215,
/* 0x0234: strand_post */
	0x4afc87f1,
	0xf00283f0,
	0x89d00d97,
	0x1521f500,
/* 0x0247: strand_set */
	0xf100f802,
	0xf04ffca7,
	0xaba202a3,
	0xc7f00500,
	0x00acd00f,
	0xd00bc7f0,
	0x21f500bc,
	0xaed00215,
	0x0ac7f000,
	0xf500bcd0,
	0xf8021521,
/* 0x0271: strand_ctx_init */
	0xf094bd00,
	0x07f10399,
	0x03f00f00,
	0x0009d002,
	0x21f504bd,
	0xe7f00221,
	0x4721f503,
	0xfca7f102,
	0x02a3f046,
	0x0400aba0,
	0xf040a0d0,
	0xbcd001c7,
	0x1521f500,
	0x010c9202,
	0xf000acd0,
	0xbcd002c7,
	0x1521f500,
	0x3421f502,
	0x8087f102,
	0x0684b608,
	0xb70089cf,
	0x95220080,
/* 0x02ca: ctx_init_strand_loop */
	0x8ed008fe,
	0x408ed000,
	0xb6808acf,
	0xa0b606a5,
	0x00eabb01,
	0xb60480b6,
	0x1bf40192,
	0x08e4b6e8,
	0xbdf2efbc,
	0x0399f094,
	0x170007f1,
	0xd00203f0,
	0x04bd0009,
/* 0x02fe: error */
	0xe0f900f8,
	0x9814e7f1,
	0xf440e3f0,
	0xe0b78d21,
	0xf7f0041c,
	0x8d21f401,
	0x00f8e0fc,
/* 0x0318: init */
	0x04fe04bd,
	0x0017f100,
	0x0227f012,
	0xf10012d0,
	0xfe047017,
	0x17f10010,
	0x10d00400,
	0x0427f0c0,
	0xf40012d0,
	0x17f11031,
	0x14b60608,
	0x0012cf06,
	0xf00137f0,
	0x32bb1f24,
	0x0132b604,
	0x80050280,
	0x10b70603,
	0x12cf0400,
	0x04028000,
	0x0c30e7f1,
	0xbd50e3f0,
	0xbd34bd24,
/* 0x0371: init_unk_loop */
	0x6821f444,
	0xf400f6b0,
	0xf7f00f0b,
	0x04f2bb01,
	0xb6054ffd,
/* 0x0386: init_unk_next */
	0x20b60130,
	0x04e0b601,
	0xf40126b0,
/* 0x0392: init_unk_done */
	0x0380e21b,
	0x08048007,
	0x010027f1,
	0xcf0223f0,
	0x34bd0022,
	0x070047f1,
	0x950644b6,
	0x45d00825,
	0x4045d000,
	0x98000e98,
	0x21f5010f,
	0x2fbb0147,
	0x003fbb00,
	0x98010e98,
	0x21f5020f,
	0x0e980147,
	0x00effd05,
	0xbb002ebb,
	0x0e98003e,
	0x030f9802,
	0x014721f5,
	0xfd070e98,
	0x2ebb00ef,
	0x003ebb00,
	0x130040b7,
	0xd00235b6,
	0x25b60043,
	0x0635b608,
	0xb60120b6,
	0x24b60130,
	0x0834b608,
	0xf5022fb9,
	0xbb027121,
	0x07f1003f,
	0x03f00100,
	0x0003d002,
	0x24bd04bd,
	0xf11f29f0,
	0xf0080007,
	0x02d00203,
/* 0x0433: main */
	0xf404bd00,
	0x28f40031,
	0x24d7f000,
	0xf43921f4,
	0xe4b0f401,
	0x1e18f404,
	0xf00181fe,
	0x20bd0627,
	0xb60412fd,
	0x1efd01e4,
	0x0018fe05,
	0x04f721f5,
/* 0x0463: main_not_ctx_xfer */
	0x94d30ef4,
	0xf5f010ef,
	0xfe21f501,
	0xc60ef402,
/* 0x0470: ih */
	0x88fe80f9,
	0xf980f901,
	0xf9a0f990,
	0xf9d0f9b0,
	0xbdf0f9e0,
	0x800acf04,
	0xf404abc4,
	0xb7f11d0b,
	0xd7f01900,
	0x40becf24,
	0xf400bfcf,
	0xb0b70421,
	0xe7f00400,
	0x00bed001,
/* 0x04a8: ih_no_fifo */
	0xfc400ad0,
	0xfce0fcf0,
	0xfcb0fcd0,
	0xfc90fca0,
	0x0088fe80,
	0x32f480fc,
/* 0x04c3: hub_barrier_done */
	0xf001f800,
	0x0e9801f7,
	0x04febb04,
	0x9418e7f1,
	0xf440e3f0,
	0x00f88d21,
/* 0x04d8: ctx_redswitch */
	0x0614e7f1,
	0xf006e4b6,
	0xefd020f7,
	0x08f7f000,
/* 0x04e8: ctx_redswitch_delay */
	0xf401f2b6,
	0xf7f1fd1b,
	0xefd00a20,
/* 0x04f7: ctx_xfer */
	0xf100f800,
	0xb60a0417,
	0x1fd00614,
	0x0711f400,
	0x04d821f5,
/* 0x0508: ctx_xfer_not_load */
	0x4afc17f1,
	0xf00213f0,
	0x12d00c27,
	0x1521f500,
	0xfc27f102,
	0x0223f047,
	0xf00020d0,
	0x20b6012c,
	0x0012d003,
	0xf001acf0,
	0xb7f002a5,
	0x50b3f000,
	0xb6040c98,
	0xbcbb0fc4,
	0x000c9800,
	0xf0010d98,
	0x21f500e7,
	0xacf00166,
	0x00b7f101,
	0x50b3f040,
	0xb6040c98,
	0xbcbb0fc4,
	0x010c9800,
	0x98020d98,
	0xe7f1060f,
	0x21f50800,
	0xacf00166,
	0x04a5f001,
	0x3000b7f1,
	0x9850b3f0,
	0xc4b6040c,
	0x00bcbb0f,
	0x98020c98,
	0x0f98030d,
	0x00e7f108,
	0x6621f502,
	0x1521f501,
	0x0601f402,
/* 0x05a3: ctx_xfer_post */
	0xf11412f4,
	0xf04afc17,
	0x27f00213,
	0x0012d00d,
	0x021521f5,
/* 0x05b4: ctx_xfer_done */
	0x04c321f5,
	0x000000f8,
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
