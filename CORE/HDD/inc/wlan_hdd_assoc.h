/*
 * Copyright (c) 2013-2016 The Linux Foundation. All rights reserved.
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

#if !defined(WLAN_HDD_ASSOC_H__)
#define WLAN_HDD_ASSOC_H__
#include <wlan_hdd_mib.h>
#include <sme_Api.h>

#define HDD_MAX_NUM_IBSS_STA          ( 32 )
#ifdef FEATURE_WLAN_TDLS
#define HDD_MAX_NUM_TDLS_STA          ( 8 )
#define HDD_MAX_NUM_TDLS_STA_P_UAPSD_OFFCHAN  ( 1 )
#define TDLS_STA_INDEX_VALID(staId) \
                          (((staId) >= 1) && ((staId) < 0xFF))
#endif
#define TKIP_COUNTER_MEASURE_STARTED 1
#define TKIP_COUNTER_MEASURE_STOPED  0
/* Timeout (in ms) for Link to Up before Registering Station */
#define ASSOC_LINKUP_TIMEOUT 60

/* In pronto case, IBSS owns the first peer for bss peer.
   In Rome case, IBSS uses the 2nd peer as bss peer */
#define IBSS_BROADCAST_STAID 1

/* Timeout in ms for peer info request completion */
#define IBSS_PEER_INFO_REQ_TIMOEUT 1000

/**
 * enum eConnectionState - connection state values at HDD
 * @eConnectionState_NotConnected: Not associated in Infra or participating in
 *   an IBSS / Ad-hoc network
 * @eConnectionState_Connecting: While connection in progress
 * @eConnectionState_Associated: Associated in an Infrastructure network
 * @eConnectionState_IbssDisconnected: Participating in an IBSS network though
 *   disconnected (no partner STA in IBSS)
 * @eConnectionState_IbssConnected: Participating in an IBSS network with
 *   partner stations also present
 * @eConnectionState_Disconnecting: Disconnecting in an Infrastructure network.
 * @eConnectionState_NdiDisconnected: NDI in disconnected state - no peers
 * @eConnectionState_NdiConnected: NDI in connected state - at least one peer
 */
typedef enum {
   eConnectionState_NotConnected,
   eConnectionState_Connecting,
   eConnectionState_Associated,
   eConnectionState_IbssDisconnected,
   eConnectionState_IbssConnected,
   eConnectionState_Disconnecting,
   eConnectionState_NdiDisconnected,
   eConnectionState_NdiConnected,
}eConnectionState;

/**This structure stores the connection information */
typedef struct connection_info_s
{
   /** connection state of the NIC.*/
   eConnectionState connState;

   /** BSS type of the current connection.   Comes from the MIB at the
       time the connect request is issued in combination with the BssDescription
      from the associated entity.*/

   eMib_dot11DesiredBssType connDot11DesiredBssType;
   /** BSSID */
   tCsrBssid bssId;

   /** SSID Info*/
   tCsrSSIDInfo SSID;

   /** Station ID */
   v_U8_t staId[ HDD_MAX_NUM_IBSS_STA ];
   /** Peer Mac Address of the IBSS Stations */
   v_MACADDR_t peerMacAddress[ HDD_MAX_NUM_IBSS_STA ];
   /** Auth Type */
   eCsrAuthType   authType;

   /** Unicast Encryption Type */
   eCsrEncryptionType ucEncryptionType;

   /** Multicast Encryption Type */
   eCsrEncryptionType mcEncryptionType;

   /** Keys */
   tCsrKeys Keys;

   /** Operation Channel  */
   v_U8_t operationChannel;

    /** Remembers authenticated state */
   v_U8_t uIsAuthenticated;

   /** Dot11Mode */
   tANI_U32 dot11Mode;

   v_U8_t proxyARPService;

   /** NSS and RateFlags used for this connection */
   uint8_t   nss;
   uint32_t  rate_flags;

   /* ptk installed state */
   bool ptk_installed;

   /* gtk installed state */
   bool gtk_installed;
}connection_info_t;

/*Forward declaration of Adapter*/
typedef struct hdd_adapter_s hdd_adapter_t;
typedef struct hdd_context_s hdd_context_t;
typedef struct hdd_station_ctx hdd_station_ctx_t;
typedef struct hdd_ap_ctx_s  hdd_ap_ctx_t;
typedef struct hdd_mon_ctx_s  hdd_mon_ctx_t;

typedef enum
{
   ePeerConnected = 1,
   ePeerDisconnected
}ePeerStatus;

extern v_BOOL_t hdd_connIsConnected( hdd_station_ctx_t *pHddStaCtx );
extern bool hdd_is_connecting(hdd_station_ctx_t *hdd_sta_ctx);
eCsrBand hdd_connGetConnectedBand( hdd_station_ctx_t *pHddStaCtx );
extern eHalStatus hdd_smeRoamCallback( void *pContext, tCsrRoamInfo *pRoamInfo, v_U32_t roamId,
                                eRoamCmdStatus roamStatus, eCsrRoamResult roamResult );

v_BOOL_t hdd_connGetConnectedBssType( hdd_station_ctx_t *pHddCtx,
        eMib_dot11DesiredBssType *pConnectedBssType );

int hdd_SetGENIEToCsr( hdd_adapter_t *pAdapter, eCsrAuthType *RSNAuthType );

int hdd_set_csr_auth_type( hdd_adapter_t *pAdapter, eCsrAuthType RSNAuthType );
VOS_STATUS hdd_roamRegisterTDLSSTA(hdd_adapter_t *pAdapter,
                                   const tANI_U8 *peerMac, tANI_U16 staId,
                                   tANI_U8 ucastSig, uint8_t qos);
void hdd_PerformRoamSetKeyComplete(hdd_adapter_t *pAdapter);

VOS_STATUS hdd_roamDeregisterTDLSSTA(hdd_adapter_t *adapter, uint8_t staId);

void hdd_SendPeerStatusIndToOemApp(v_MACADDR_t *peerMac,
                                   tANI_U8 peerStatus,
                                   tANI_U8 peerTimingMeasCap,
                                   tANI_U8 sessionId,
                                   tSirSmeChanInfo *chan_info);

#if defined(FEATURE_WLAN_ESE) && defined(FEATURE_WLAN_ESE_UPLOAD)
void hdd_indicateEseBcnReportNoResults(const hdd_adapter_t *pAdapter,
                                       const tANI_U16 measurementToken,
                                       const tANI_BOOLEAN flag,
                                       const tANI_U8 numBss);
#endif /* FEATURE_WLAN_ESE && FEATURE_WLAN_ESE_UPLOAD */

VOS_STATUS hdd_roamRegisterSTA(hdd_adapter_t *adapter, tCsrRoamInfo *roam_info,
			       uint8_t sta_id, v_MACADDR_t *peer_mac_addr,
			       tSirBssDescription *bss_desc);

bool hdd_save_peer(hdd_station_ctx_t *sta_ctx, uint8_t sta_id,
		   v_MACADDR_t *peer_mac_addr);
#endif
