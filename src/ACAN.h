//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
// A simple Arduino Teensy 3.1/3.2/3.5/3.6 CAN driver
// by Pierre Molinaro
// https://github.com/pierremolinaro/acan
//
// This driver is written from FlexCan Library by teachop
// dual CAN support for MK66FX1M0 and updates for MK64FX512 by Pawelsky
// Interrupt driven Rx/Tx with buffers, object oriented callbacks by Collin Kidder
// RTR related code by H4nky84
//
//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

#pragma once

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

#include <ACANSettings.h>
#include <CANMessage.h>

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

typedef enum {kActive, kPassive, kBusOff} tControllerState ;

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

class ACANPrimaryFilter {
  public: uint32_t mFilterMask ;
  public: uint32_t mAcceptanceFilter ;
  public: ACANCallBackRoutine mCallBackRoutine ;

  public: inline ACANPrimaryFilter (const ACANCallBackRoutine inCallBackRoutine) :  // Accept any frame
  mFilterMask (0),
  mAcceptanceFilter (0),
  mCallBackRoutine (inCallBackRoutine) {
  }

  public: ACANPrimaryFilter (const tFrameKind inKind,
                             const tFrameFormat inFormat, // Accept any identifier
                             const ACANCallBackRoutine inCallBackRoutine = NULL) ;

  public: ACANPrimaryFilter (const tFrameKind inKind,
                             const tFrameFormat inFormat,
                             const uint32_t inIdentifier,
                             const ACANCallBackRoutine inCallBackRoutine = NULL) ;

  public: ACANPrimaryFilter (const tFrameKind inKind,
                             const tFrameFormat inFormat,
                             const uint32_t inMask,
                             const uint32_t inAcceptance,
                             const ACANCallBackRoutine inCallBackRoutine = NULL) ;
} ;

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

class ACANSecondaryFilter {
  public: uint32_t mSingleAcceptanceFilter ;
  public: ACANCallBackRoutine mCallBackRoutine ;

  public: ACANSecondaryFilter (const tFrameKind inKind,
                                const tFrameFormat inFormat,
                                const uint32_t inIdentifier,
                                const ACANCallBackRoutine inCallBackRoutine = NULL) ;
} ;

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

class ACAN {
//--- Constructor
  private: ACAN (const uint32_t inFlexcanBaseAddress) ;

//--- begin; returns a result code :
//  0 : Ok
//  other: every bit denotes an error
  public: static const uint32_t kTooMuchPrimaryFilters     = 1 << 12 ;
  public: static const uint32_t kNotConformPrimaryFilter   = 1 << 13 ;
  public: static const uint32_t kTooMuchSecondaryFilters   = 1 << 14 ;
  public: static const uint32_t kNotConformSecondaryFilter = 1 << 15 ;
  public: static const uint32_t kNoAlternateTxPinForCan1   = 1 << 16 ;
  public: static const uint32_t kNoAlternateRxPinForCan1   = 1 << 17 ;
  public: static const uint32_t kCANBitConfiguration       = 1 << 18 ;

  public: uint32_t begin (const ACANSettings & inSettings,
                          const ACANPrimaryFilter inPrimaryFilters [] = NULL ,
                          const uint32_t inPrimaryFilterCount = 0,
                          const ACANSecondaryFilter inSecondaryFilters [] = NULL,
                          const uint32_t inSecondaryFilterCount = 0) ;

//--- end: stop CAN controller
  public: void end (void) ;

//--- Transmitting messages
  public: bool tryToSend (const CANMessage & inMessage) ;
  public: inline uint32_t transmitBufferSize (void) const { return mTransmitBufferSize ; }
  public: inline uint32_t transmitBufferCount (void) const { return mTransmitBufferCount ; }
  public: inline uint32_t transmitBufferPeakCount (void) const { return mTransmitBufferPeakCount ; }

//--- Receiving messages
  public: inline bool available (void) const { return mReceiveBufferCount > 0 ; }
  public: bool receive (CANMessage & outMessage) ;
  public: typedef void (*tFilterMatchCallBack) (const uint32_t inFilterIndex) ;
  public: bool dispatchReceivedMessage (const tFilterMatchCallBack inFilterMatchCallBack = NULL) ;
  public: inline uint32_t receiveBufferSize (void) const { return mReceiveBufferSize ; }
  public: inline uint32_t receiveBufferCount (void) const { return mReceiveBufferCount ; }
  public: inline uint32_t receiveBufferPeakCount (void) const { return mReceiveBufferPeakCount ; }
  public: inline uint8_t flexcanRxFIFOFlags (void) const { return mFlexcanRxFIFOFlags ; }

//--- FlexCAN controller state
  public: tControllerState controllerState (void) const ;
  public: uint32_t receiveErrorCounter (void) const ;
  public: uint32_t transmitErrorCounter (void) const ;

//--- Call back function array
  private: ACANCallBackRoutine * mCallBackFunctionArray = NULL ;
  private: uint32_t mCallBackFunctionArraySize = 0 ;

//--- Base address
  private: const uint32_t mFlexcanBaseAddress ; // Initialized in constructor

//--- Driver receive buffer
  private: CANMessage * volatile mReceiveBuffer = NULL ;
  private: uint32_t mReceiveBufferSize = 0 ;
  private: uint32_t mReceiveBufferReadIndex = 0 ; // Only used in user mode --> no volatile
  private: volatile uint32_t mReceiveBufferCount = 0 ; // Used in isr and user mode --> volatile
  private: volatile uint32_t mReceiveBufferPeakCount = 0 ; // == mReceiveBufferSize + 1 if overflow did occur
  private : uint8_t mFlexcanRxFIFOFlags = 0 ;
  private : void readRxRegisters (const uint32_t inFlexcanBaseAddress, CANMessage & outMessage) ;

//--- Primary filters
  private : uint8_t mActualPrimaryFilterCount = 0 ;
  private : uint8_t mMaxPrimaryFilterCount = 0 ;

//--- Driver transmit buffer
  private: CANMessage * volatile mTransmitBuffer = NULL ;
  private: uint32_t mTransmitBufferSize = 0 ;
  private: uint32_t mTransmitBufferReadIndex = 0 ; // Only used in isr --> no volatile
  private: volatile uint32_t mTransmitBufferCount = 0 ; // Used in isr and user mode --> volatile
  private: volatile uint32_t mTransmitBufferPeakCount = 0 ; // == mTransmitBufferSize + 1 if tentative overflow did occur
  private: void writeTxRegisters (const CANMessage & inMessage, const uint32_t inMBIndex) ;

//--- Message interrupt service routine
  private: void message_isr (void) ;
  friend void can0_message_isr (void) ;
  #ifdef __MK66FX1M0__
    friend void can1_message_isr (void) ;
  #endif

//--- Driver instances
  public: static ACAN can0 ;
  #ifdef __MK66FX1M0__
    public: static ACAN can1 ;
  #endif

//--- No copy
  private : ACAN (const ACAN &) ;
  private : ACAN & operator = (const ACAN &) ;
} ;

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
