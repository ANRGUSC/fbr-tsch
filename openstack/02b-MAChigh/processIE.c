#include "opendefs.h"
#include "processIE.h"
#include "sixtop.h"
#include "idmanager.h"
#include "openserial.h"
#include "IEEE802154.h"
#include "openqueue.h"
#include "neighbors.h"
#include "IEEE802154E.h"
#include "schedule.h"
#include "scheduler.h"
#include "packetfunctions.h"
#include "light.h"

//=========================== variables =======================================

//=========================== public ==========================================

port_INLINE void processIE_prependMLMEIE(
      OpenQueueEntry_t* pkt, 
      uint8_t           len
   ){
   payload_IE_ht payload_IE_desc;
   
   // reserve space
   packetfunctions_reserveHeaderSize(
      pkt, 
      sizeof(payload_IE_ht)
   );
   
   // prepare header
   payload_IE_desc.length_groupid_type = len;
   payload_IE_desc.length_groupid_type |= 
      (IEEE802154E_PAYLOAD_DESC_GROUP_ID_MLME  | IEEE802154E_PAYLOAD_DESC_TYPE_MLME); 
   
   // copy header
   pkt->payload[0] = payload_IE_desc.length_groupid_type        & 0xFF;
   pkt->payload[1] = (payload_IE_desc.length_groupid_type >> 8) & 0xFF;
}

//===== prepend IEs

port_INLINE uint8_t processIE_prependSyncIE(OpenQueueEntry_t* pkt){
   mlme_IE_ht mlme_subHeader;
   uint8_t    len;
  
   len = 0;
  
   /* Inserting Counter and State into Beacon */
   packetfunctions_reserveHeaderSize(pkt,3);
   
   *((uint16_t*)&pkt->payload[0]) = light_counter();
   *((uint8_t*)&pkt->payload[2]) = light_state();
       
   len += 3;
   
   //=== sync IE
   
   // reserve space
   packetfunctions_reserveHeaderSize(
      pkt,
      sizeof(sync_IE_ht)
   );
   
   // Keep a pointer to where the ASN will be
   // Note: the actual value of the current ASN and JP will be written by the
   //    IEEE802.15.4e when transmitting
   pkt->l2_ASNpayload               = pkt->payload; 
   
   len += sizeof(sync_IE_ht);
   
   //=== MLME IE
  
//   // reserve space
//   packetfunctions_reserveHeaderSize(
//      pkt,
//      sizeof(mlme_IE_ht)
//   );
//   
//   // prepare header
//   mlme_subHeader.length_subID_type = sizeof(sync_IE_ht);
//   mlme_subHeader.length_subID_type |= 
//      (IEEE802154E_MLME_SYNC_IE_SUBID << IEEE802154E_MLME_SYNC_IE_SUBID_SHIFT)|
//      IEEE802154E_DESC_TYPE_SHORT;
//   
//   // copy header
//   pkt->payload[0]= mlme_subHeader.length_subID_type        & 0xFF;
//   pkt->payload[1]= (mlme_subHeader.length_subID_type >> 8) & 0xFF;
//   
//   len += 2;
   
   return len;
}

port_INLINE uint8_t processIE_prependScheduleIE(
      OpenQueueEntry_t* pkt,
      uint8_t           type,
      uint8_t           frameID,
      uint8_t           flag,
      cellInfo_ht*      cellList
   ){
   uint8_t    i;
   uint8_t    len;
   uint8_t    temp8b;
   uint8_t    numOfCells;
   mlme_IE_ht mlme_subHeader;
  
   len        = 0;
   numOfCells = 0;
   
   //===== cell list
   
   for(i=0;i<SCHEDULEIEMAXNUMCELLS;i++) {
      if(cellList[i].linkoptions != CELLTYPE_OFF){
         // cellobjects:
         // - [2B] slotOffset
         // - [2B] channelOffset
         // - [1B] link_type
         packetfunctions_reserveHeaderSize(pkt,5); 
         pkt->payload[0] = (uint8_t)(cellList[i].tsNum  & 0x00FF);
         pkt->payload[1] = (uint8_t)((cellList[i].tsNum & 0xFF00)>>8);
         pkt->payload[2] = (uint8_t)(cellList[i].choffset  & 0x00FF);
         pkt->payload[3] = (uint8_t)((cellList[i].choffset & 0xFF00)>>8);
         pkt->payload[4] = cellList[i].linkoptions;
         len += 5;
         numOfCells++;
      }
   }
   
   // record the position of cellObjects
   pkt->l2_scheduleIE_numOfCells  = numOfCells;
   pkt->l2_scheduleIE_cellObjects = pkt->payload;
   
   //===== number of cells
   
   // reserve space
   packetfunctions_reserveHeaderSize(pkt,sizeof(uint8_t));
   
   // prepare header
   temp8b  = numOfCells;
   temp8b |= flag << 7;
   
   // copy header
   *((uint8_t*)(pkt->payload)) = temp8b;
   
   len += 1;
   
   //===== slotframeID
   
   // reserve space
   packetfunctions_reserveHeaderSize(pkt,sizeof(uint8_t));
   
   // prepare header
   temp8b = frameID;
   
   // copy header
   *((uint8_t*)(pkt->payload)) = temp8b;
   
   len += 1;
  
   // record the frameID 
   pkt->l2_scheduleIE_frameID = frameID;
   
   //===== length
   
   // reserve space
   packetfunctions_reserveHeaderSize(pkt,sizeof(uint8_t));
   
   // prepare header
   temp8b = len;
   
   // copy header
   *((uint8_t*)(pkt->payload)) = temp8b;
   
   // length
   len += 1;
  
   //===== type
   
   // reserve space
   packetfunctions_reserveHeaderSize(pkt,sizeof(uint8_t));
   
   // prepare header
   temp8b = type;
   
   // copy header
   *((uint8_t*)(pkt->payload)) = temp8b;
   
   len += 1;
   
   //===== MLME IE header
   
   // reserve space
   packetfunctions_reserveHeaderSize(pkt, sizeof(mlme_IE_ht));
   
   // prepare header
   mlme_subHeader.length_subID_type  = len;
   mlme_subHeader.length_subID_type |= 
      (MLME_IE_SUBID_SCHEDULE << MLME_IE_SUBID_SHIFT) | 
      IEEE802154E_DESC_TYPE_SHORT;
   
   // copy header
   pkt->payload[0] = mlme_subHeader.length_subID_type       & 0xFF;
   pkt->payload[1] = (mlme_subHeader.length_subID_type >> 8)& 0xFF;
   
   len+=2;
  
   return len;
}

//===== retrieve IEs

port_INLINE void processIE_retrieveSlotframeLinkIE(
      OpenQueueEntry_t* pkt,
      uint8_t*          ptr
   ){
   uint8_t              numSlotFrames;
   uint8_t              i;
   uint8_t              j;
   uint8_t              localptr;
   slotframeLink_IE_ht  sfInfo; 
   cellInfo_ht          linkInfo;
   open_addr_t          temp_neighbor;
   frameLength_t        oldFrameLength;
   
   localptr = *ptr; 
  
   // number of slot frames 1B
   numSlotFrames = *((uint8_t*)(pkt->payload)+localptr);
   localptr++;
   
   schedule_setFrameNumber(numSlotFrames);

   // for each slotframe
   i=0;
   while(i < numSlotFrames){
      
      // [1B] slotftramehandle 1B
      sfInfo.slotframehandle =*((uint8_t*)(pkt->payload)+localptr);
      localptr++;
      
      schedule_setFrameHandle(sfInfo.slotframehandle);
      
      // [2B] slotframe size
      sfInfo.slotframesize   = *((uint8_t*)(pkt->payload)+localptr);
      localptr++;
      sfInfo.slotframesize  |= (*((uint8_t*)(pkt->payload)+localptr))<<8;
      localptr++;;
      
      oldFrameLength = schedule_getFrameLength();
      schedule_setFrameLength(sfInfo.slotframesize);
      
      // [1B] number of links
      sfInfo.numlinks        = *((uint8_t*)(pkt->payload)+localptr);
      localptr++;
      
      if (oldFrameLength == 0) {
         
         for (j=0;j<sfInfo.numlinks;j++){
            
            // [2B] TimeSlot
            linkInfo.tsNum = *((uint8_t*)(pkt->payload)+localptr);
            localptr++;
            linkInfo.tsNum  |= (*((uint8_t*)(pkt->payload)+localptr))<<8;
            localptr++;
            
            // [2B] Ch.Offset
            linkInfo.choffset = *((uint8_t*)(pkt->payload)+localptr);
            localptr++;
            linkInfo.choffset  |= (*((uint8_t*)(pkt->payload)+localptr))<<8;
            localptr++;
            
            // [1B] LinkOption bitmap
            linkInfo.linkoptions = *((uint8_t*)(pkt->payload)+localptr);
            localptr++;
            
            // shared TXRX anycast slot(s)
            memset(&temp_neighbor,0,sizeof(temp_neighbor));
            temp_neighbor.type             = ADDR_ANYCAST;
            schedule_addActiveSlot(
               linkInfo.tsNum,                     // slot offset
               CELLTYPE_TXRX,                      // type of slot
               TRUE,                               // shared?
               linkInfo.choffset,                  // channel offset
               &temp_neighbor                      // neighbor
            );
         }
      }
      i++;
      break; //TODO: this break is put since a single slotframe is managed
   }
   
   *ptr=localptr;
} 

port_INLINE void processIE_retrieveOpcodeIE(
      OpenQueueEntry_t* pkt,
      uint8_t*          ptr,
      opcode_IE_ht*     opcodeInfo
   ){
   uint8_t localptr;
   
   localptr=*ptr; 
   
   opcodeInfo->opcode = *((uint8_t*)(pkt->payload)+localptr);
   localptr++;
  
   *ptr=localptr; 
} 

port_INLINE void processIE_retrieveBandwidthIE(
      OpenQueueEntry_t* pkt,
      uint8_t *         ptr,
      bandwidth_IE_ht*  bandwidthInfo
   ){
   uint8_t localptr;
   
   localptr=*ptr; 
   
   // [1B] slotframeID
   bandwidthInfo->slotframeID = *((uint8_t*)(pkt->payload)+localptr);
   localptr++;
   
   // [1B] number of cells
   bandwidthInfo->numOfLinks = *((uint8_t*)(pkt->payload)+localptr);
   localptr++;
   
   *ptr=localptr; 
}

port_INLINE void processIE_retrieveScheduleIE(
      OpenQueueEntry_t* pkt,
      uint8_t*          ptr,
      schedule_IE_ht*   scheduleInfo
   ){
   uint8_t i;
   uint8_t temp8b;
   uint8_t localptr;
  
   localptr=*ptr;
   
   // [1B] type
   scheduleInfo->type             = *((uint8_t*)(pkt->payload)+localptr);
   localptr++;
   
   // [1B] length
   scheduleInfo->length           = *((uint8_t*)(pkt->payload)+localptr);
   localptr++;
   
   // [1B] frameID
   scheduleInfo->frameID          = *((uint8_t*)(pkt->payload)+localptr);
   localptr++;
   
   // [1B] number of cell and flag
   temp8b = *((uint8_t*)(pkt->payload)+localptr);
   scheduleInfo->numberOfcells    = temp8b & 0x7F;
   scheduleInfo->flag             = (temp8b >> 7) ? TRUE : FALSE;
   localptr++;
  
   if(scheduleInfo->numberOfcells > SCHEDULEIEMAXNUMCELLS) {
      //log error
      return;
   }
   
   for (i=0;i<scheduleInfo->numberOfcells;i++){
      
      // [2B] TimeSlot
      scheduleInfo->cellList[i].tsNum = 
         *((uint8_t*)(pkt->payload)+localptr);
      localptr++;
      
      scheduleInfo->cellList[i].tsNum  |= 
         (*((uint8_t*)(pkt->payload)+localptr))<<8;
      localptr++;
      
      // [2B] Ch.Offset
      scheduleInfo->cellList[i].choffset = 
         *((uint8_t*)(pkt->payload)+localptr);
      localptr++;
      
      scheduleInfo->cellList[i].choffset  |= 
         (*((uint8_t*)(pkt->payload)+localptr))<<8;
      localptr++;
      
      // [1B] LinkOption bitmap
      scheduleInfo->cellList[i].linkoptions = 
         *((uint8_t*)(pkt->payload)+localptr);
      localptr++;
   }
   *ptr=localptr; 
}
