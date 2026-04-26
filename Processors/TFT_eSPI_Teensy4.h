        ////////////////////////////////////////////////////
        //     TFT_eSPI Teensy 4.x driver functions       //
        ////////////////////////////////////////////////////

// This is a driver for Teensy 4.x boards, it supports SPI interface displays

#ifndef _TFT_eSPI_TEENSYH_
#define _TFT_eSPI_TEENSYH_

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
#define spi spi_dma.getSPI()

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
      #define tft_Write_8(C)   spi.transfer(C)
      #define tft_Write_16(C)  spi.transfer16(C)
      #define tft_Write_16S(C) spi.transfer16(((C)>>8) | ((C)<<8))
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
  #define tft_Read_8() spi.transfer(0)
#endif

#undef spi // done with sleazy hack

////////////////////////////////////////////////////////////////////////////////////////
// Teensy 4.x DMA-related stuff
////////////////////////////////////////////////////////////////////////////////////////
// Ngleton class to ensure clean access to one of the 
// three SPI busses, even if multiple displays are in use
class TFT_eSPI;
class TFT_eSPI_Teensy4_SPI_with_DMA
{
    const int LOOP_MINOR_PIXELS = 8; // number of pixels to transfer per minor loop

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
    bool DMAidle;

    TFT_eSPI* currentDMAtft;
    SPIClass* pSPI;
    DMAChannel* pDMA;
    IMXRT_LPSPI_t*  hardware; // actual peripheral
    const SPIClass::SPI_Hardware_t& SPIattr;  // attributes of that peripheral
    DMASetting chain; // settings to chain to for last few pixels
  public:
    TFT_eSPI_Teensy4_SPI_with_DMA(SPIClass& spi, uint32_t phw, const SPIClass::SPI_Hardware_t& attr); 
    void begin();

    SPIClass&   getSPI(void) { return *pSPI; }      
    DMAChannel& getDMA(void) { return *pDMA; }
    IMXRT_LPSPI_t& getHardware(void) { return *hardware; }

    void maybeUpdateTCR(uint32_t requested_tcr_state);
    void prepSPIforDMA(void);
    void SPIsendDirect(uint16_t* pdata, uint32_t len);
    bool SPItransmitComplete(void);
    void waitTransmitComplete(void) { while (!SPItransmitComplete()) {} }
    void fixupSPIafterDMA(void);
    bool prepDMAtransfer(uint16_t* image, int pixels, TFT_eSPI& tft);
    void startDMAtransfer(void);
    void finishDMAtransfer(void);
    bool cleanupNeeded(void) { return cleanupIsNeeded; }
    void DCcmd(void);
    void DCdata(void);
    
    void initDMA(void);
    void deInitDMA(void) { delete pDMA; pDMA = nullptr; }
    bool dmaBusy(void);
    void dmaWait(void);
};

class TFT_eSPI_Teensy4_SPD_Factory
{
    TFT_eSPI_Teensy4_SPD_Factory() {} // hide constructor

    // we implement 3 SPI buses for now
    static TFT_eSPI_Teensy4_SPI_with_DMA& getSPI(SPIClass& s)
    {
      static TFT_eSPI_Teensy4_SPI_with_DMA spi{s, IMXRT_LPSPI4_ADDRESS, 
                  SPIClass::spiclass_lpspi4_hardware};
      return spi;
    }

    static TFT_eSPI_Teensy4_SPI_with_DMA& getSPI1(SPIClass& s)
    {
      static TFT_eSPI_Teensy4_SPI_with_DMA spi{s, IMXRT_LPSPI3_ADDRESS, 
                  SPIClass::spiclass_lpspi3_hardware};
      return spi;
    }

    static TFT_eSPI_Teensy4_SPI_with_DMA& getSPI2(SPIClass& s)
    {
      static TFT_eSPI_Teensy4_SPI_with_DMA spi{s, IMXRT_LPSPI1_ADDRESS, 
                  SPIClass::spiclass_lpspi1_hardware};
      return spi;
    }

  public:
    static TFT_eSPI_Teensy4_SPI_with_DMA& getInstance(SPIClass& spi)
    {
      SPIClass* s = &spi;

      if (s == &SPI1)
          return getSPI1(*s);
      else if (s == &SPI2)
          return getSPI2(*s);

      // if (s == &SPI)
      return getSPI(*s); // must return something!     
    }

    // publicly disable copy and assignment
    TFT_eSPI_Teensy4_SPD_Factory(TFT_eSPI_Teensy4_SPD_Factory const&) = delete;
    void operator=(TFT_eSPI_Teensy4_SPD_Factory const&)               = delete;
  };

#endif // _TFT_eSPI_TEENSYH_ : Header end
