        ////////////////////////////////////////////////////
        //     TFT_eSPI Teensy 4.x driver functions       //
        ////////////////////////////////////////////////////

// This is a driver for Teensy 4.x boards, it supports SPI interface displays

#ifndef _TFT_eSPI_TEENSYH_
#define _TFT_eSPI_TEENSYH_

#include <FlexIOSPI.h>

// We would
// #include <core_cm7.h>
// but it doesn't work, so
#define SCB_ICSR_VECTACTIVE_Pos             0U                                            /*!< SCB ICSR: VECTACTIVE Position */
#define SCB_ICSR_VECTACTIVE_Msk            (0x1FFUL /*<< SCB_ICSR_VECTACTIVE_Pos*/)       /*!< SCB ICSR: VECTACTIVE Mask */


// Processor ID reported by getSetup()
#if defined(ARDUINO_TEENSY40)
#define PROCESSOR_ID 0x40
#else
#define PROCESSOR_ID 0x41
extern uint8_t external_psram_size;
#endif // defined(ARDUINO_TEENSY40)


// Include processor specific header
// None
#define TFT_ESPI_MULTI_SPI // multiple SPI busses possible

// Processor specific code used by SPI bus transaction startWrite and endWrite functions
#define SET_BUS_WRITE_MODE // Not used
#define SET_BUS_READ_MODE  // Not used

// Code to check if DMA is busy, used by SPI bus transaction startWrite and endWrite functions
#define DMA_BUSY_CHECK dmaWait()
#define SPI_BUSY_CHECK spi_dma.waitTransmitComplete()

// To be safe, SUPPORT_TRANSACTIONS is assumed mandatory
#if !defined (SUPPORT_TRANSACTIONS)
  #define SUPPORT_TRANSACTIONS
#endif

// Initialise processor specific SPI functions, used by init()
#define INIT_TFT_DATA_BUS spi_dma.begin()

// If smooth fonts are enabled the filing system may need to be loaded
#ifdef SMOOTH_FONT
  // Call up the filing system for the anti-aliased fonts
  //#define FS_NO_GLOBALS
  //#include <FS.h>
#endif

////////////////////////////////////////////////////////////////////////////////////////
// Define the DC (TFT Data/Command or Register Select (RS))pin drive code
////////////////////////////////////////////////////////////////////////////////////////
#ifndef TFT_DC
  #define DC_C // No macro allocated so it generates no code
  #define DC_D // No macro allocated so it generates no code
#else
  #define DC_C spi_dma.DCcmd()  // digitalWrite(TFT_DC, LOW)
  #define DC_D spi_dma.DCdata() // digitalWrite(TFT_DC, HIGH)
#endif

////////////////////////////////////////////////////////////////////////////////////////
// Define the CS (TFT chip select) pin drive code
////////////////////////////////////////////////////////////////////////////////////////
#ifndef TFT_CS
  #define CS_L // No macro allocated so it generates no code
  #define CS_H // No macro allocated so it generates no code
#else
  #define CS_L (nullptr == CSfn)?digitalWrite(TFT_CS, LOW):CSfn(LOW)
  #define CS_H (nullptr == CSfn)?digitalWrite(TFT_CS, HIGH):CSfn(HIGH)
#endif

////////////////////////////////////////////////////////////////////////////////////////
// Make sure TFT_RD is defined if not used to avoid an error message
////////////////////////////////////////////////////////////////////////////////////////
#ifndef TFT_RD
  #define TFT_RD -1
#endif

////////////////////////////////////////////////////////////////////////////////////////
// Define the WR (TFT Write) pin drive code
////////////////////////////////////////////////////////////////////////////////////////
#ifdef TFT_WR
  #define WR_L digitalWrite(TFT_WR, LOW)
  #define WR_H digitalWrite(TFT_WR, HIGH)
#endif

////////////////////////////////////////////////////////////////////////////////////////
// Define the touch screen chip select pin drive code
////////////////////////////////////////////////////////////////////////////////////////
#if !defined TOUCH_CS || (TOUCH_CS < 0)
  #define T_CS_L // No macro allocated so it generates no code
  #define T_CS_H // No macro allocated so it generates no code
#else
  #define T_CS_L digitalWrite(TOUCH_CS, LOW)
  #define T_CS_H digitalWrite(TOUCH_CS, HIGH)
#endif

////////////////////////////////////////////////////////////////////////////////////////
// Make sure TFT_MISO is defined if not used to avoid an error message
////////////////////////////////////////////////////////////////////////////////////////
#ifndef TFT_MISO
  #define TFT_MISO -1
#endif

////////////////////////////////////////////////////////////////////////////////////////
// Macros to write commands/pixel colour data to a SPI ILI948x TFT
////////////////////////////////////////////////////////////////////////////////////////
// How to access SPI:
//#define spi spi_dma.getSPI()

#if  defined (SPI_18BIT_DRIVER) // SPI 18-bit colour

  // Write 8 bits to TFT
  #define tft_Write_8(C)   spi.transfer(C)

  // Convert 16-bit colour to 18-bit and write in 3 bytes
  #define tft_Write_16(C)  spi.transfer(((C) & 0xF800)>>8); \
                           spi.transfer(((C) & 0x07E0)>>3); \
                           spi.transfer(((C) & 0x001F)<<3)

  // Convert swapped byte 16-bit colour to 18-bit and write in 3 bytes
  #define tft_Write_16S(C) spi.transfer((C) & 0xF8); \
                           spi.transfer(((C) & 0xE000)>>11 | ((C) & 0x07)<<5); \
                           spi.transfer(((C) & 0x1F00)>>5)
  // Write 32 bits to TFT
  #define tft_Write_32(C)  spi.transfer16((C)>>16); spi.transfer16((uint16_t)(C))

  // Write two address coordinates
  #define tft_Write_32C(C,D) spi.transfer16(C); spi.transfer16(D)

  // Write same value twice
  #define tft_Write_32D(C) spi.transfer16(C); spi.transfer16(C)

////////////////////////////////////////////////////////////////////////////////////////
// Macros to write commands/pixel colour data to other displays
////////////////////////////////////////////////////////////////////////////////////////
#else
  #if  defined (RPI_DISPLAY_TYPE) // RPi TFT type always needs 16-bit transfers
    #define tft_Write_8(C)   spi.transfer(C); spi.transfer(C)
    #define tft_Write_16(C)  spi.transfer((uint8_t)((C)>>8));spi.transfer((uint8_t)((C)>>0))
    #define tft_Write_16S(C) spi.transfer((uint8_t)((C)>>0));spi.transfer((uint8_t)((C)>>8))

    #define tft_Write_32(C) \
      tft_Write_16((uint16_t) ((C)>>16)); \
      tft_Write_16((uint16_t) ((C)>>0))

    #define tft_Write_32C(C,D) \
      spi.transfer(0); spi.transfer((C)>>8); \
      spi.transfer(0); spi.transfer((C)>>0); \
      spi.transfer(0); spi.transfer((D)>>8); \
      spi.transfer(0); spi.transfer((D)>>0)

    #define tft_Write_32D(C) \
      spi.transfer(0); spi.transfer((C)>>8); \
      spi.transfer(0); spi.transfer((C)>>0); \
      spi.transfer(0); spi.transfer((C)>>8); \
      spi.transfer(0); spi.transfer((C)>>0)

  #else
    #ifdef __AVR__ // AVR processors do not have 16-bit transfer
      #define tft_Write_8(C)   {SPDR=(C); while (!(SPSR&_BV(SPIF)));}
      #define tft_Write_16(C)  tft_Write_8((uint8_t)((C)>>8));tft_Write_8((uint8_t)((C)>>0))
      #define tft_Write_16S(C) tft_Write_8((uint8_t)((C)>>0));tft_Write_8((uint8_t)((C)>>8))
    #else
      #define tft_Write_8(C)   spi_dma.getSPI().transfer(C)
      #define tft_Write_16(C)  spi_dma.getSPI().transfer16(C)
      #define tft_Write_16S(C) spi_dma.getSPI().transfer16(((C)>>8) | ((C)<<8))
    #endif // AVR    

    #define tft_Write_32(C) \
    tft_Write_16((uint16_t) ((C)>>16)); \
    tft_Write_16((uint16_t) ((C)>>0))

    #define tft_Write_32C(C,D) \
    tft_Write_16((uint16_t) (C)); \
    tft_Write_16((uint16_t) (D))

    #define tft_Write_32D(C) \
    tft_Write_16((uint16_t) (C)); \
    tft_Write_16((uint16_t) (C))

    #define writecommand_no_end(C) \
        { /* Serial.printf("%d: C %02X: ", millis(), C); */ begin_tft_write(); DC_C; tft_Write_8(C); DC_D; }

    #define writedata_no_end(D) \
        { /* Serial.printf("%02X ", D); */ tft_Write_8(D); }

    #define writedata_last(D) \
        { /* Serial.printf("%02X\n", D); */ tft_Write_8(D); CS_L; end_tft_write(); }

  #endif // RPI_DISPLAY_TYPE
#endif

#ifndef tft_Write_16N
  #define tft_Write_16N tft_Write_16
#endif

////////////////////////////////////////////////////////////////////////////////////////
// Macros to read from display using SPI or software SPI
////////////////////////////////////////////////////////////////////////////////////////
#if defined (TFT_SDA_READ)
  // Use a bit banged function call for STM32 and bi-directional SDA pin
  #define TFT_eSPI_ENABLE_8_BIT_READ // Enable tft_Read_8(void);
  #define SCLK_L digitalWrite(TFT_SCLK, LOW)
  #define SCLK_H digitalWrite(TFT_SCLK, LOW)
#else
  // Use a SPI read transfer
  #define tft_Read_8() spi_dma.getSPI().transfer(0)
#endif

#undef spi // done with sleazy hack

////////////////////////////////////////////////////////////////////////////////////////
// Teensy 4.x DMA-related stuff
////////////////////////////////////////////////////////////////////////////////////////
//                                 
//     .d8888b.  8888888b. 8888888 
//    d88P  Y88b 888   Y88b  888   
//    Y88b.      888    888  888   
//     "Y888b.   888   d88P  888   
//        "Y88b. 8888888P"   888   
//          "888 888         888   
//    Y88b  d88P 888         888   
//     "Y8888P"  888       8888888 
//                                 
// Class to allow use of either LPSPI or FlexIOSPI
class TFT_eSPI;
class TFT_eSPI_Teensy4_SPIClass
{
  public:
    virtual ~TFT_eSPI_Teensy4_SPIClass() {}
    virtual void* getHWaddr(void) = 0;
    virtual void DMA_ISR(void) = 0;
    virtual void  begin(void) = 0;
    virtual void  beginTransaction(FlexIOSPISettings settings) = 0;
    virtual void  endTransaction(void) = 0;
    virtual uint8_t  transfer(uint8_t c) = 0;
    virtual uint16_t transfer16(uint16_t w) = 0;

    //virtual uint8_t pinIsChipSelect(uint8_t pin) = 0;
    //virtual uint8_t setCS(uint8_t pin) = 0; // this sets which pin is used for CS

    virtual void DCcmd(void) = 0;
    virtual void DCdata(void) = 0;
    virtual void initDMA(void) = 0;
    virtual void deInitDMA(void) = 0;
    virtual void prepSPIforDMA(void) = 0;
    virtual void fixupSPIafterDMA(void) = 0;
    virtual void SPIsendDirect(uint16_t* pdata, uint32_t len) = 0;
    virtual bool SPItransmitComplete(void) = 0;
    virtual bool prepDMAtransfer(uint16_t* image, int pixels, TFT_eSPI& tft) = 0;
    virtual void startDMAtransfer(void) = 0;
    virtual void finishDMAtransfer(void) = 0;
    virtual bool dmaBusy(void) = 0;
};

class TFT_eSPI_Teensy4_SPI_Helper
{    
    // SPI
    uint32_t _spi_fcr_save;
    uint32_t _spi_tcr_current;
    uint32_t _tcr_dc_assert;
    uint32_t _tcr_dc_not_assert;
    uint32_t _dcpinmask;
    volatile uint32_t *_dcport;

    SPIClass& spi;
    IMXRT_LPSPI_t* hardware{0}; // pointer to actual peripheral registers
    const SPIClass::SPI_Hardware_t& SPIattr; // holds DMA trigger source setting

    // DMA
    const int LOOP_MINOR_PIXELS = 8; // number of pixels to transfer per minor loop
    DMAChannel* pDMA;
    TFT_eSPI* currentDMAtft;
    DMASetting chain;
    bool DMAidle;

    void maybeUpdateTCR(uint32_t requested_tcr_state);
  public:
    TFT_eSPI_Teensy4_SPI_Helper(SPIClass& _spi, uint32_t _phw, const SPIClass::SPI_Hardware_t& _attr)
         : spi{_spi}, hardware{(IMXRT_LPSPI_t*) _phw}, SPIattr{_attr},
           pDMA{nullptr}, currentDMAtft{nullptr}, DMAidle{true}
           {}

    //uint8_t pinIsChipSelect(uint8_t pin) { return spi.pinIsChipSelect(pin); };
    //uint8_t setCS(uint8_t pin) { return spi.setCS(pin); };
    void beginTransaction(FlexIOSPISettings s)
    {
      SPISettings ss{s._clock,s._bitOrder,s._dataMode};
      spi.beginTransaction(ss);
    }

    void begin(void);
    bool SPItransmitComplete(void);
    void waitTransmitComplete(void) { while (!SPItransmitComplete()) ; }
    void DCcmd(void);
    void DCdata(void);
    void initDMA(void);
    void deInitDMA(void) { delete pDMA; pDMA = nullptr; }
    void prepSPIforDMA(void);
    void sendDirect16(uint16_t* pdata, uint32_t len);
    void fixupSPIafterDMA(void);
    void DMA_ISR(void);
    bool prepDMAtransfer(uint16_t* image, int pixels, TFT_eSPI& tft);
    void startDMAtransfer(void);
    void finishDMAtransfer(void);
    bool dmaBusy(void);
};

class TFT_eSPI_Teensy4_FlexIOSPI_Helper
{
    FlexIOSPI& spi;
  public:
    TFT_eSPI_Teensy4_FlexIOSPI_Helper(FlexIOSPI& _spi) : spi{_spi} {}

    //uint8_t pinIsChipSelect(uint8_t pin) { return 0; };
    //uint8_t setCS(uint8_t pin) { return 0; };
    void beginTransaction(FlexIOSPISettings s)
    {
      spi.beginTransaction(s);
    }

    void begin(void){}
    bool SPItransmitComplete(void){ return true; }
    void waitTransmitComplete(void) { while (!SPItransmitComplete()) ; }
    void DCcmd(void){}
    void DCdata(void){}
    void initDMA(void){}
    void deInitDMA(void){}
    void prepSPIforDMA(void){}
    void sendDirect16(uint16_t* pdata, uint32_t len){}
    void fixupSPIafterDMA(void){}
    void DMA_ISR(void){}
    bool prepDMAtransfer(uint16_t* image, int pixels, TFT_eSPI& tft){ return true; }
    void startDMAtransfer(void){}
    void finishDMAtransfer(void){}
    bool dmaBusy(void){ return false; }
};

/*
 * There MUST be only ONE of these constructed for each LPSPI or 
 * FlexIOSPI hardware instance: this is controlled by the
 * TFT_eSPI_Teensy4_SPD_Factory class, which will provide a 
 * static reference on demand. It is opaque to the user of this
 * class as to which SPI bus is in use.
 */
template <class SPIhw, class SPIhelper>
class TTFT_eSPI_Teensy4_SPIClass : public TFT_eSPI_Teensy4_SPIClass
{
  public:	
    TTFT_eSPI_Teensy4_SPIClass(SPIhw& _hw, SPIhelper& _hlp) 
      : hw{_hw}, helper(_hlp)
        {}
    SPIhw& hw;
    SPIhelper& helper;
    virtual void DMA_ISR(void) { helper.DMA_ISR(); }
    virtual void* getHWaddr(void) { return (void*) &hw; };

    virtual void begin(void) { hw.begin(); helper.begin(); }
    virtual void beginTransaction(FlexIOSPISettings s) { helper.beginTransaction(s); }
    virtual void endTransaction(void)   { hw.endTransaction(); }
    virtual uint8_t transfer(uint8_t b) { return hw.transfer(b); }
    virtual uint16_t transfer16(uint16_t b) { return hw.transfer16(b); }
    //virtual void transfer(void *buf, size_t count) { hw.transfer(buf, count); }
    //virtual void transfer(const void * buf, void * retbuf, uint32_t count) { hw.transfer(buf, retbuf, count);}
    
    //virtual uint8_t pinIsChipSelect(uint8_t pin) { return helper.pinIsChipSelect(pin); };
    //virtual uint8_t setCS(uint8_t pin) { return helper.setCS(pin); };
    virtual void DCcmd(void) { helper.DCcmd(); };
    virtual void DCdata(void) { helper.DCdata(); };
    virtual void initDMA(void) { helper.initDMA(); }
    virtual void deInitDMA(void) { helper.deInitDMA(); }
    virtual void prepSPIforDMA(void) { helper.prepSPIforDMA(); };
    virtual void fixupSPIafterDMA(void) { helper.fixupSPIafterDMA(); };

    virtual void SPIsendDirect(uint16_t* pdata, uint32_t len) { helper.sendDirect16(pdata, len); }
    virtual bool SPItransmitComplete(void) { return helper.SPItransmitComplete();}
    void waitTransmitComplete(void) { while (!SPItransmitComplete()) {} }
    virtual bool prepDMAtransfer(uint16_t* image, int pixels, TFT_eSPI& tft) { return helper.prepDMAtransfer(image, pixels, tft); }
    virtual void startDMAtransfer(void) { helper.startDMAtransfer(); }
    virtual void finishDMAtransfer(void) { helper.finishDMAtransfer();}
    virtual bool dmaBusy(void) { return helper.dmaBusy(); }
};

/*
// template specialization for SPIClass: allow beginTransaction 
// to use the less-opaque FlexIOSPISettings
template<>
inline uint8_t TTFT_eSPI_Teensy4_SPIClass<FlexIOSPI>::pinIsChipSelect(uint8_t pin)
{
  return 0;
}

template<>
inline uint8_t TTFT_eSPI_Teensy4_SPIClass<FlexIOSPI>::setCS(uint8_t pin)
{
  return 0;
}

template<>
inline void TTFT_eSPI_Teensy4_SPIClass<SPIClass>::beginTransaction(FlexIOSPISettings s)
{
	SPISettings ss{s._clock,s._bitOrder,s._dataMode};
	hw.beginTransaction(ss);
}
*/
#define SPISettings FlexIOSPISettings // hack all subsequent occurrences!

////////////////////////////////////////////////////////////////////////////////////////
//                                                        
//     8888b.   .d8888b .d8888b .d88b.  .d8888b  .d8888b  
//        "88b d88P"   d88P"   d8P  Y8b 88K      88K      
//    .d888888 888     888     88888888 "Y8888b. "Y8888b. 
//    888  888 Y88b.   Y88b.   Y8b.          X88      X88 
//    "Y888888  "Y8888P "Y8888P "Y8888   88888P'  88888P' 
//                                                        
////////////////////////////////////////////////////////////////////////////////////////
/*
 * There is one instance of this class per TFT display in use. It may be
 * that they reference different or identical instances of the 
 * TFT_eSPI_Teensy4_SPIClass, depending on whether there are multiple
 * displays on one bus, or on different busses, or a mix thereof.
 */
class TFT_eSPI_Teensy4_SPI_with_DMA
{
    static void SPI_DMA_ISR(void);
    static void SPI1_DMA_ISR(void);
    static void SPI2_DMA_ISR(void);
    void DMA_ISR(void);

    uint32_t _spi_fcr_save;
    uint32_t _spi_tcr_current;
    uint32_t _tcr_dc_assert;
    uint32_t _tcr_dc_not_assert;
    uint32_t _dcpinmask;
    volatile uint32_t *_dcport;
    bool cleanupIsNeeded; // cleanup not done after transfer

    TFT_eSPI* currentDMAtft;
    TFT_eSPI_Teensy4_SPIClass* pSPI; // pointer to one of our Ngleton instances
    //DMAChannel* pDMA;
    //IMXRT_LPSPI_t*  hardware; // actual peripheral
    //const SPIClass::SPI_Hardware_t& SPIattr;  // attributes of that peripheral
    //DMASetting chain; // settings to chain to for last few pixels
    
  public:
    TFT_eSPI_Teensy4_SPI_with_DMA(TFT_eSPI_Teensy4_SPIClass& spi);
    ~TFT_eSPI_Teensy4_SPI_with_DMA() { /* Serial.println("destroy TFT_eSPI_Teensy4_SPI_with_DMA"); */ }
    void begin(void);

    TFT_eSPI_Teensy4_SPIClass&   getSPI(void) { return *pSPI; }  
    static void attachInterrupt(SPIClass* pSPI, DMAChannel* pDMA);
    //DMAChannel& getDMA(void) { return *pDMA; }
    // IMXRT_LPSPI_t& getHardware(void) { return *hardware; }

    // pass through to SPI-like object
    void prepSPIforDMA(void)       { pSPI->prepSPIforDMA(); cleanupIsNeeded = true; }
    void SPIsendDirect(uint16_t* pdata, uint32_t len) { pSPI->SPIsendDirect(pdata, len); }
    bool SPItransmitComplete(void) { return pSPI->SPItransmitComplete();}
    void fixupSPIafterDMA(void)    { pSPI->fixupSPIafterDMA(); cleanupIsNeeded = false; }
    bool prepDMAtransfer(uint16_t* image, int pixels, TFT_eSPI& tft) { return pSPI->prepDMAtransfer(image, pixels, tft); }
    void startDMAtransfer(void)    { pSPI->startDMAtransfer(); }
    void finishDMAtransfer(void)   { pSPI->finishDMAtransfer();}
    void DCcmd(void)               { pSPI->DCcmd(); }
    void DCdata(void)              { pSPI->DCdata(); };
    
    // internal use
    void waitTransmitComplete(void) { while (!SPItransmitComplete()) {} }
    bool cleanupNeeded(void) { return cleanupIsNeeded; }

    // API functions
    void initDMA(void) { pSPI->initDMA(); }
    void deInitDMA(void) { pSPI->deInitDMA(); }
    bool dmaBusy(void) { return pSPI->dmaBusy(); }
    void dmaWait(void);
};

////////////////////////////////////////////////////////////////////////////////////////
//                                 
//    888b    888          888          888                     
//    8888b   888          888          888                     
//    88888b  888          888          888                     
//    888Y88b 888  .d88b.  888  .d88b.  888888 .d88b.  88888b.  
//    888 Y88b888 d88P"88b 888 d8P  Y8b 888   d88""88b 888 "88b 
//    888  Y88888 888  888 888 88888888 888   888  888 888  888 
//    888   Y8888 Y88b 888 888 Y8b.     Y88b. Y88..88P 888  888 
//    888    Y888  "Y88888 888  "Y8888   "Y888 "Y88P"  888  888 
//                     888                                      
//                Y8b d88P                                      
//                 "Y88P"                                       
//
// Ngleton class to ensure clean access to one of the 
// three SPI busses, even if multiple displays are in use
class TFT_eSPI_Teensy4_SPD_Factory
{
    TFT_eSPI_Teensy4_SPD_Factory() {} // hide constructor

    // we implement 3 SPI buses for now
    static TFT_eSPI_Teensy4_SPIClass& getSPI(SPIClass& s)
    {
      static TFT_eSPI_Teensy4_SPI_Helper helper
                {s, IMXRT_LPSPI4_ADDRESS, SPIClass::spiclass_lpspi4_hardware};
      static TTFT_eSPI_Teensy4_SPIClass<SPIClass, TFT_eSPI_Teensy4_SPI_Helper>
              spi{s, helper}; 
      return spi;
    }

    static TFT_eSPI_Teensy4_SPIClass& getSPI1(SPIClass& s)
    {
      static TFT_eSPI_Teensy4_SPI_Helper helper
                {s, IMXRT_LPSPI3_ADDRESS, SPIClass::spiclass_lpspi3_hardware};
      static TTFT_eSPI_Teensy4_SPIClass<SPIClass, TFT_eSPI_Teensy4_SPI_Helper>
                spi{s, helper}; 
      return spi;
    }

    static TFT_eSPI_Teensy4_SPIClass& getSPI2(SPIClass& s)
    {
      static TFT_eSPI_Teensy4_SPI_Helper helper
                {s, IMXRT_LPSPI1_ADDRESS, SPIClass::spiclass_lpspi1_hardware};
      static TTFT_eSPI_Teensy4_SPIClass<SPIClass, TFT_eSPI_Teensy4_SPI_Helper>
                spi{s, helper}; 
      return spi;
    }

    static TFT_eSPI_Teensy4_SPIClass& getInstanceFromSPIClass(void* s)
    {
      if (s == &SPI1)
          return getSPI1(*((SPIClass*) s));
      else if (s == &SPI2)
          return getSPI2(*((SPIClass*) s));

      // if (s == &SPI)
      return getSPI(*((SPIClass*) s)); // must return something!     
    }

 // we may also be implementing one or more FlexIOSPI busses:
 #if defined(TFT_FLEXIOSPI_COUNT)
    static constexpr uint8_t FlexCSpins[TFT_FLEXIOSPI_COUNT]{TFT_FLEXIOSPI_CS_LIST};
    static constexpr uint8_t FlexDCpins[TFT_FLEXIOSPI_COUNT]{TFT_FLEXIOSPI_DC_LIST};
    static FlexIOSPI* FlexBusses[TFT_FLEXIOSPI_COUNT];
    static TFT_eSPI_Teensy4_FlexIOSPI_Helper* FlexHelpers[TFT_FLEXIOSPI_COUNT];
    static TTFT_eSPI_Teensy4_SPIClass<FlexIOSPI, TFT_eSPI_Teensy4_FlexIOSPI_Helper>* 
      FlexInstances[TFT_FLEXIOSPI_COUNT];

    static TFT_eSPI_Teensy4_SPIClass& getInstanceFromFlexIOSPI(void* fs)
    {
      FlexIOSPI* s = (FlexIOSPI*) fs;

      TTFT_eSPI_Teensy4_SPIClass<FlexIOSPI, TFT_eSPI_Teensy4_FlexIOSPI_Helper>* result = nullptr;

      for (int i=0;i<TFT_FLEXIOSPI_COUNT && nullptr == result;i++)
      {
        if (s == FlexBusses[i]) // seen it before
          result = FlexInstances[i];

        if (nullptr == FlexBusses[i]) // never seen this before, create a new one 
        {
          FlexBusses[i] = s;
          FlexHelpers[i] = new TFT_eSPI_Teensy4_FlexIOSPI_Helper{*s};
          FlexInstances[i] = new TTFT_eSPI_Teensy4_SPIClass<FlexIOSPI, TFT_eSPI_Teensy4_FlexIOSPI_Helper>
                                {*s, *FlexHelpers[i]};
          result = FlexInstances[i];                                
        }
      }
      return *result;
    }

#endif // defined(TFT_FLEXIOSPI_COUNT)    


  public:
    /*
    static TFT_eSPI_Teensy4_SPI_with_DMA& getInstance(TFT_eSPI_Teensy4_SPIClass& spi)
    {
      return getInstance(spi.getHWaddr());
    }
    */
    static TFT_eSPI_Teensy4_SPIClass& getInstance(SPIClass& spi)
    {
      return getInstanceFromSPIClass(&spi);
    }

#if defined(TFT_FLEXIOSPI_COUNT)
     static TFT_eSPI_Teensy4_SPIClass& getInstance(FlexIOSPI& spi)
    {
      return getInstanceFromFlexIOSPI(&spi);
    }
#endif // defined(TFT_FLEXIOSPI_COUNT)
 

    // publicly disable copy and assignment
    TFT_eSPI_Teensy4_SPD_Factory(TFT_eSPI_Teensy4_SPD_Factory const&) = delete;
    void operator=(TFT_eSPI_Teensy4_SPD_Factory const&)               = delete;
  };

#endif // _TFT_eSPI_TEENSYH_ : Header end
