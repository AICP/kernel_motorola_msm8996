/*
 * Copyright (c) 2012-2013 The Linux Foundation. All rights reserved.
 *
 * Previously licensed under the ISC license by Qualcomm Atheros, Inc.
 *
 *
 * Permission to use, copy, modify, and/or distribute this software for
 * any purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all
 * copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * This file was originally distributed by Qualcomm Atheros, Inc.
 * under proprietary terms before Copyright ownership was assigned
 * to the Linux Foundation.
 */

#ifndef QWLAN_VERSION_H
#define QWLAN_VERSION_H
/*===========================================================================

FILE:
   qwlan_version.h

BRIEF DESCRIPTION:
   WLAN Host Version file.
   Build number automaticly updated by build scripts.

===========================================================================*/

#define QWLAN_VERSION_MAJOR            1
#define QWLAN_VERSION_MINOR            0
#define QWLAN_VERSION_PATCH            0
#define QWLAN_VERSION_EXTRA            ""
#define QWLAN_VERSION_BUILD            34

#define QWLAN_VERSIONSTR               "1.0.0.34"

#ifdef QCA_WIFI_2_0

#define AR6320_REV1_VERSION             0x5000000
#define AR6320_REV1_1_VERSION           0x5000001
#define AR6320_REV1_3_VERSION           0x5000003
#define AR6320_REV2_1_VERSION           0x5010000

struct qwlan_hw {
    unsigned long id;
    const char *name;
};

#endif

#endif /* QWLAN_VERSION_H */
