        ////////////////////////////////////////////////////
        //      TFT_eSPI Teensy 4.x driver functions      //
        ////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////
// Global variables
////////////////////////////////////////////////////////////////////////////////////////
#define DIRECT_WRITE_LOW(base, mask)    (*((base)+34) = (mask))
#define DIRECT_WRITE_HIGH(base, mask)   (*((base)+33) = (mask))

// SPI port selection dealt with in construcctor

////////////////////////////////////////////////////////////////////////////////////////
#if defined (TFT_SDA_READ) && !defined (TFT_PARALLEL_8_BIT)
////////////////////////////////////////////////////////////////////////////////////////

/***************************************************************************************
** Function name:           tft_Read_8
** Description:             Bit bashed SPI to read bidirectional SDA line
***************************************************************************************/
uint8_t TFT_eSPI::tft_Read_8(void)
{
  uint8_t  ret = 0;

  for (uint8_t i = 0; i < 8; i++) {  // read results
    ret <<= 1;
    SCLK_L;
    if (digitalRead(TFT_MOSI)) ret |= 1;
    SCLK_H;
  }

  return ret;
}

/***************************************************************************************
** Function name:           beginSDA
** Description:             Detach SPI from pin to permit software SPI
***************************************************************************************/
void TFT_eSPI::begin_SDA_Read(void)
{
  // Release configured SPI port for SDA read
  spi.end();
}

/***************************************************************************************
** Function name:           endSDA
** Description:             Attach SPI pins after software SPI
***************************************************************************************/
void TFT_eSPI::end_SDA_Read(void)
{
  // Configure SPI port ready for next TFT access
  spi.begin();
}

////////////////////////////////////////////////////////////////////////////////////////
#endif // #if defined (TFT_SDA_READ)
////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////
#if defined (TFT_PARALLEL_8_BIT) // Code for generic (i.e. any) processor
////////////////////////////////////////////////////////////////////////////////////////

/***************************************************************************************
** Function name:           pushBlock - for generic processor and parallel display
** Description:             Write a block of pixels of the same colour
***************************************************************************************/
void TFT_eSPI::pushBlock(uint16_t color, uint32_t len){

  while (len>1) {tft_Write_32D(color); len-=2;}
  if (len) {tft_Write_16(color);}
}

/***************************************************************************************
** Function name:           pushPixels - for gereric processor and parallel display
** Description:             Write a sequence of pixels
***************************************************************************************/
void TFT_eSPI::pushPixels(const void* data_in, uint32_t len){

  uint16_t *data = (uint16_t*)data_in;
  if(_swapBytes) {
    while (len>1) {tft_Write_16(*data); data++; tft_Write_16(*data); data++; len -=2;}
    if (len) {tft_Write_16(*data);}
    return;
  }

  while (len>1) {tft_Write_16S(*data); data++; tft_Write_16S(*data); data++; len -=2;}
  if (len) {tft_Write_16S(*data);}
}

/***************************************************************************************
** Function name:           GPIO direction control  - supports class functions
** Description:             Set parallel bus to INPUT or OUTPUT
***************************************************************************************/
void TFT_eSPI::busDir(uint32_t mask, uint8_t mode)
{
  // mask is unused for generic processor
  // Arduino native functions suited well to a generic driver
  pinMode(TFT_D0, mode);
  pinMode(TFT_D1, mode);
  pinMode(TFT_D2, mode);
  pinMode(TFT_D3, mode);
  pinMode(TFT_D4, mode);
  pinMode(TFT_D5, mode);
  pinMode(TFT_D6, mode);
  pinMode(TFT_D7, mode);
  return;
}

/***************************************************************************************
** Function name:           GPIO direction control  - supports class functions
** Description:             Faster GPIO pin input/output switch
***************************************************************************************/
void TFT_eSPI::gpioMode(uint8_t gpio, uint8_t mode)
{
  // No fast port based generic approach available
}

/***************************************************************************************
** Function name:           read byte  - supports class functions
** Description:             Read a byte - parallel bus only
***************************************************************************************/
uint8_t TFT_eSPI::readByte(void)
{
  uint8_t b = 0;

  busDir(0, INPUT);
  digitalWrite(TFT_RD, LOW);

  b |= digitalRead(TFT_D0) << 0;
  b |= digitalRead(TFT_D1) << 1;
  b |= digitalRead(TFT_D2) << 2;
  b |= digitalRead(TFT_D3) << 3;
  b |= digitalRead(TFT_D4) << 4;
  b |= digitalRead(TFT_D5) << 5;
  b |= digitalRead(TFT_D6) << 6;
  b |= digitalRead(TFT_D7) << 7;

  digitalWrite(TFT_RD, HIGH);
  busDir(0, OUTPUT); 

  return b;
}

////////////////////////////////////////////////////////////////////////////////////////
#elif defined (RPI_WRITE_STROBE)  // For RPi TFT with write strobe                      
////////////////////////////////////////////////////////////////////////////////////////

/***************************************************************************************
** Function name:           pushBlock - for ESP32 or STM32 RPi TFT
** Description:             Write a block of pixels of the same colour
***************************************************************************************/
void TFT_eSPI::pushBlock(uint16_t color, uint32_t len){

  if(len) { tft_Write_16(color); len--; }
  while(len--) {WR_L; WR_H;}
}

/***************************************************************************************
** Function name:           pushPixels - for ESP32 or STM32 RPi TFT
** Description:             Write a sequence of pixels
***************************************************************************************/
void TFT_eSPI::pushPixels(const void* data_in, uint32_t len)
{
  uint16_t *data = (uint16_t*)data_in;

  if (_swapBytes) while ( len-- ) {tft_Write_16S(*data); data++;}
  else while ( len-- ) {tft_Write_16(*data); data++;}
}

////////////////////////////////////////////////////////////////////////////////////////
#elif defined (SPI_18BIT_DRIVER) // SPI 18-bit colour                         
////////////////////////////////////////////////////////////////////////////////////////

/***************************************************************************************
** Function name:           pushBlock - for STM32 and 3 byte RGB display
** Description:             Write a block of pixels of the same colour
***************************************************************************************/
void TFT_eSPI::pushBlock(uint16_t color, uint32_t len)
{
  // Split out the colours
  uint8_t r = (color & 0xF800)>>8;
  uint8_t g = (color & 0x07E0)>>3;
  uint8_t b = (color & 0x001F)<<3;

  while ( len-- ) {tft_Write_8(r); tft_Write_8(g); tft_Write_8(b);}
}

/***************************************************************************************
** Function name:           pushPixels - for STM32 and 3 byte RGB display
** Description:             Write a sequence of pixels
***************************************************************************************/
void TFT_eSPI::pushPixels(const void* data_in, uint32_t len){

  uint16_t *data = (uint16_t*)data_in;
  if (_swapBytes) {
    while ( len-- ) {
      uint16_t color = *data >> 8 | *data << 8;
      tft_Write_8((color & 0xF800)>>8);
      tft_Write_8((color & 0x07E0)>>3);
      tft_Write_8((color & 0x001F)<<3);
      data++;
    }
  }
  else {
    while ( len-- ) {
      tft_Write_8((*data & 0xF800)>>8);
      tft_Write_8((*data & 0x07E0)>>3);
      tft_Write_8((*data & 0x001F)<<3);
      data++;
    }
  }
}

////////////////////////////////////////////////////////////////////////////////////////
#else //                   Standard SPI 16-bit colour TFT                               
////////////////////////////////////////////////////////////////////////////////////////

/***************************************************************************************
** Function name:           pushBlock - for STM32
** Description:             Write a block of pixels of the same colour
***************************************************************************************/
void TFT_eSPI::pushBlock(uint16_t color, uint32_t len){

  while ( len-- ) {tft_Write_16(color);}
}

/***************************************************************************************
** Function name:           pushPixels - for STM32
** Description:             Write a sequence of pixels
***************************************************************************************/
void TFT_eSPI::pushPixels(const void* data_in, uint32_t len){

  uint16_t *data = (uint16_t*)data_in;

  if (_swapBytes) while ( len-- ) {tft_Write_16(*data); data++;}
  else while ( len-- ) {tft_Write_16S(*data); data++;}
}

////////////////////////////////////////////////////////////////////////////////////////
#endif // End of display interface specific functions
////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////
//                                DMA FUNCTIONS                                         
////////////////////////////////////////////////////////////////////////////////////////
// DMA utilities
//
// The Teensy LPSPI peripherals have a 16-element FIFO, and a maximum major
// loop count of 32767 minor transfers. A minor transfer is atomic, so if it
// attempts to transfer more data than will fit in the FIFO then some data
// will be lost. A 480*320 screen is 153,600 pixels, so would require a minimum
// of 5 major loops at one pixel per minor transfer. Equally, we can set the 
// minor loop to transfer 8 pixels at a time, and the major loop can then 
// be 19,200. We need to ensure the FIFO has room for 8 pixels before the 
// transfer starts.
//
// If the transfer is 8n+k pixels, e.g. an area of 167*113 = 8*2358+7, then
// we need to transfer a final k pixels after the main 18864 transfer is done.
// We can do this by chaining a second TCD with a minor loop count of 1 and 
// a major loop count of k.
TFT_eSPI_Teensy4_SPI_with_DMA::TFT_eSPI_Teensy4_SPI_with_DMA(SPIClass& spi, uint32_t phw, const SPIClass::SPI_Hardware_t& attr) 
      : cleanupIsNeeded{false}, DMAidle{true},
        currentDMAtft{nullptr},
        pSPI(&spi), pDMA(nullptr), 
        hardware{(IMXRT_LPSPI_t*) phw},
        SPIattr{attr}
        {}


void TFT_eSPI_Teensy4_SPI_with_DMA::begin(void)        
{
  int _rs = TFT_DC;

	if (nullptr != pSPI && pSPI->pinIsChipSelect(_rs)) {
	 	uint8_t dc_cs_index = pSPI->setCS(_rs);
	 	_dcport = nullptr;
	 	_dcpinmask = 0;
	 	dc_cs_index--;	// convert to 0 based
		_tcr_dc_assert = LPSPI_TCR_PCS(dc_cs_index);
    _tcr_dc_not_assert = LPSPI_TCR_PCS(3);
		Serial.printf("dc_cs_index: %02X\n", dc_cs_index);
	} else {
		Serial.println("DC not dealt with in hardware");
		_dcport = portOutputRegister(_rs);
		_dcpinmask = digitalPinToBitMask(_rs);
		pinMode(_rs, OUTPUT);	
		DIRECT_WRITE_HIGH(_dcport, _dcpinmask);
		_tcr_dc_assert = LPSPI_TCR_PCS(0);
    _tcr_dc_not_assert = LPSPI_TCR_PCS(1);
	}

  // get pins in correct initial state
  pSPI->begin();
  pSPI->beginTransaction(SPISettings(SPI_FREQUENCY, MSBFIRST, TFT_SPI_MODE));
  pSPI->endTransaction();

  // record starting TCR settings
  _spi_tcr_current = hardware->TCR;
}


// Return `true` if Tx FIFO is empty and hardware is not busy
bool TFT_eSPI_Teensy4_SPI_with_DMA::SPItransmitComplete(void) 
{
  // see if hardware is not busy
  return  (0 == (hardware->FSR & 0x1f)) // FIFO empty
       && (0 == (hardware->SR  & LPSPI_SR_MBF)); // busy flag clear
}


#ifndef TCR_MASK
#define TCR_MASK  (LPSPI_TCR_PCS(3) | LPSPI_TCR_FRAMESZ(31) | LPSPI_TCR_CONT | LPSPI_TCR_RXMSK )
#endif  
// update TCR, but only if requested state is different
void TFT_eSPI_Teensy4_SPI_with_DMA::maybeUpdateTCR(uint32_t requested_tcr_state) 
{
  if ((_spi_tcr_current & TCR_MASK) != requested_tcr_state) 
  {
    if (echoTCR)
      Serial.print("...");
    bool dc_state_change = (_spi_tcr_current & LPSPI_TCR_PCS(3)) != (requested_tcr_state & LPSPI_TCR_PCS(3));
    _spi_tcr_current = (_spi_tcr_current & ~TCR_MASK) | requested_tcr_state;

    waitTransmitComplete();
    if (!dc_state_change || !_dcpinmask) 
    {
      while ((hardware->FSR & 0x1F) == 0x1F) // wait while no room in FIFO
        ;
      hardware->TCR = _spi_tcr_current;  // update the TCR
    } 
    else 
    {
      waitTransmitComplete();   // CS change: can only output when Transfer queue is empty.
      if (0 != (requested_tcr_state & LPSPI_TCR_PCS(3))) 
        DIRECT_WRITE_HIGH(_dcport, _dcpinmask);
      else 
        DIRECT_WRITE_LOW(_dcport, _dcpinmask);
      hardware->TCR = _spi_tcr_current & ~(LPSPI_TCR_PCS(3) | LPSPI_TCR_CONT); // go ahead and update TCR anyway?  
    }
//    Serial.printf("SPI TCR is %08X\n", hardware->TCR);
  }
  if (echoTCR)
    Serial.printf("TCR: %08X (%08X)\n",hardware->TCR, requested_tcr_state);
}

void TFT_eSPI_Teensy4_SPI_with_DMA::DCcmd(void)
{ 
  maybeUpdateTCR((_spi_tcr_current & ~LPSPI_TCR_PCS(3)) | _tcr_dc_assert); 
}

void TFT_eSPI_Teensy4_SPI_with_DMA::DCdata(void)
{ 
  maybeUpdateTCR((_spi_tcr_current & ~LPSPI_TCR_PCS(3)) | _tcr_dc_not_assert); 
}

/*
 * Initialise a DMA channel
 * Allocates the channel and attaches its interrupt
 */
void TFT_eSPI_Teensy4_SPI_with_DMA::initDMA(void) 
{ 
  if (nullptr == pDMA)
    pDMA = new DMAChannel;
  if (nullptr != pDMA)
  {
    pDMA->begin();
    if (pSPI == &SPI)
      pDMA->attachInterrupt(SPI_DMA_ISR);
    else if (pSPI == &SPI1)
      pDMA->attachInterrupt(SPI1_DMA_ISR);
    if (pSPI == &SPI2)
      pDMA->attachInterrupt(SPI2_DMA_ISR);
  }
}


void TFT_eSPI_Teensy4_SPI_with_DMA::prepSPIforDMA(void)
{
	_spi_fcr_save = hardware->FCR;      // remember the FCR
	hardware->FCR = LPSPI_FCR_TXWATER(LOOP_MINOR_PIXELS);  // set Tx watermark
	maybeUpdateTCR(_tcr_dc_not_assert | LPSPI_TCR_FRAMESZ(15) 
				|	LPSPI_TCR_RXMSK /*| LPSPI_TCR_CONT*/);
	hardware->DER = LPSPI_DER_TDDE;
	hardware->SR = 0x3f00; // clear out all of the other status...
  
  cleanupIsNeeded = true;
}


void TFT_eSPI_Teensy4_SPI_with_DMA::fixupSPIafterDMA(void)
{
  waitTransmitComplete();

  hardware->FCR = LPSPI_FCR_TXWATER(15); 
                   // _spi_fcr_save;	// restore the FSR status...
  hardware->DER = 0; // DMA no longer doing TX (or RX)

  hardware->CR =
      LPSPI_CR_MEN | LPSPI_CR_RRF | LPSPI_CR_RTF; // actually clear both...
  hardware->SR = 0x3f00; // clear out all of the other status...

  maybeUpdateTCR(_tcr_dc_assert |
                  LPSPI_TCR_FRAMESZ(7)); // output Command with 8 bits
  hardware->TDR = TFT_NOP; // transmit NOP command
  maybeUpdateTCR(_tcr_dc_not_assert |
                  LPSPI_TCR_FRAMESZ(7)); // output Command with 8 bits

  cleanupIsNeeded = false;
}


void dumpDMA_TCD(DMABaseClass *dmabc)
{
	Serial.printf("%x %x:", (uint32_t)dmabc, (uint32_t)dmabc->TCD);

	Serial.printf("SA:%x SO:%d AT:%x NB:%x SL:%d DA:%x DO: %d CI:%x DL:%x CS:%x BI:%x\n", (uint32_t)dmabc->TCD->SADDR,
		dmabc->TCD->SOFF, dmabc->TCD->ATTR, dmabc->TCD->NBYTES, dmabc->TCD->SLAST, (uint32_t)dmabc->TCD->DADDR, 
		dmabc->TCD->DOFF, dmabc->TCD->CITER, dmabc->TCD->DLASTSGA, dmabc->TCD->CSR, dmabc->TCD->BITER);

  Serial.flush();    
}


/*
 * Generic DMA completion ISR method.
 * Applies to any of the N SPI+DMA channels available; only
 * one TFT display can be using a given channel at a time.
 */
void TFT_eSPI_Teensy4_SPI_with_DMA::DMA_ISR(void)
{ 
  pDMA->clearInterrupt();
  if (nullptr != currentDMAtft) // defensive, but ought to be valid!
  {
    TFT_eSPI* tft = currentDMAtft; // keep a copy because...
    tft->dmaWait();                // ...this zaps currentDMAtft!
    tft->callCompletionISR();
  }
  asm volatile ("dsb":::"memory"); // ensure interrupt is cleared
}


// Separate ISRs, one per channel.
// Call the generic ISR method for the channel in use.
void TFT_eSPI_Teensy4_SPI_with_DMA::SPI_DMA_ISR(void) 
{
  TFT_eSPI_Teensy4_SPD_Factory::getInstance(SPI).DMA_ISR();
}


void TFT_eSPI_Teensy4_SPI_with_DMA::SPI1_DMA_ISR(void) 
{
  TFT_eSPI_Teensy4_SPD_Factory::getInstance(SPI2).DMA_ISR();
}


void TFT_eSPI_Teensy4_SPI_with_DMA::SPI2_DMA_ISR(void) 
{
  TFT_eSPI_Teensy4_SPD_Factory::getInstance(SPI2).DMA_ISR();
}


/*
 * Prepare the DMA channel for the transfer
 * Sets up source address, SPI transmit register, trigger and interrupts.
 * Also flushes the source cache to RAM
 */
void TFT_eSPI_Teensy4_SPI_with_DMA::prepDMAtransfer(uint16_t* image, int pixels, TFT_eSPI& tft)
{
  int mainTransfer = pixels / LOOP_MINOR_PIXELS;
  
  pDMA->sourceBuffer(image, mainTransfer * sizeof *image); // pretend its fewer writes...
  pDMA->destination(hardware->TDR);  // this is our destination: SPI transmit register (may stuff NBYTES)
  pDMA->TCD->NBYTES = sizeof *image * LOOP_MINOR_PIXELS;   // ...then bump up minor loop size
  pDMA->TCD->ATTR_DST = 1; // and say the destination size is 16 bits
  pDMA->triggerAtHardwareEvent(SPIattr.tx_dma_channel);    // pick the correct event to trigger DMA
  pDMA->TCD->CSR &= ~(DMA_TCD_CSR_INTMAJOR | DMA_TCD_CSR_INTHALF); // no interrupts for now
  pDMA->disableOnCompletion(); // disable DMA when done

  arm_dcache_flush(image, pixels * sizeof *image); // this can take some time!

  // Fix up the remaining pixels
  mainTransfer *= LOOP_MINOR_PIXELS; // pixels transferred by first TCD
  pixels = pixels - mainTransfer; // how remaining pixels we have
  if (pixels > 0) // if some left, will be 1 - LOOP_MINOR_PIXELS-1
  {
    chain = *pDMA; // copy main transfer settings
    chain.sourceBuffer(image + mainTransfer, pixels * sizeof *image); // last few pixels
    pDMA->replaceSettingsOnCompletion(chain);
    pDMA->TCD->CSR &= ~DMA_TCD_CSR_DREQ;  // don't disable on completion of first transfer
    chain.interruptAtCompletion();        // interrupt after odd size mop-up transfer
  }
  else
    pDMA->interruptAtCompletion(); // just main transfer - interrupt after that

  currentDMAtft = &tft; // stash pointer to the display that's currently using the DMA+SPI
}


/*
 * Start the DMA channel transferring - all setup has previously been done
 */
void TFT_eSPI_Teensy4_SPI_with_DMA::startDMAtransfer(void)
{
  DMAidle = false;
  pDMA->begin();
  pDMA->enable(); // start the transfer
}

/*
 * Set DMA channel back to inactive state after transfer is complete
 */
void TFT_eSPI_Teensy4_SPI_with_DMA::finishDMAtransfer(void)
{
  pDMA->clearComplete();
  pDMA->disable(); // just in case
  DMAidle = true;
  currentDMAtft = nullptr;
}


/*
 * Find out if DMA transfer is still in progress.
 * We also say it's busy if the SPI FIFO still has pending data.
 */
bool TFT_eSPI_Teensy4_SPI_with_DMA::dmaBusy(void) 
{ 
  return (!DMAidle && !pDMA->complete())
      || !SPItransmitComplete(); 
}


//==================================================================================
// Minimal function set to support DMA:
bool TFT_eSPI::initDMA(bool ctrl_cs) 
{ 
  spi_dma.initDMA(); 
  DMA_Enabled=true; 
  return true; 
}

void TFT_eSPI::deInitDMA(void) { DMA_Enabled=false; /* spi_dma.getDMA().release(); */ }

// return true until DMA has completed and SPI FIFO is empty,
// i.e. we're completely done with the hardware
bool TFT_eSPI::dmaBusy(void) 
{ 
  return spi_dma.dmaBusy(); 
}

// This waits for a completed transfer, then tidies up. If called
// after dmaBusy() starts returning false, won't take long
void TFT_eSPI::dmaWait(void) 
{ 
  bool inISR = (SCB_ICSR & SCB_ICSR_VECTACTIVE_Msk) != 0;
  while (dmaBusy())
    if (!inISR)
      yield();
  if (spi_dma.cleanupNeeded())
  {
    spi_dma.finishDMAtransfer();
    spi_dma.fixupSPIafterDMA();
    //end_tft_write();
  }
}

void TFT_eSPI::pushPixelsDMA(uint16_t* image, // pointer to image data
                             uint32_t len)    // length of image data in pixels
{
  dmaWait(); // should take no time as long as previous DMA is finished

  spi_dma.prepDMAtransfer(image, len, *this);  // get DMA channel ready

  //begin_tft_write();          // this modifies the SPI settings!
  spi_dma.prepSPIforDMA();    // get SPI hardware ready
  spi_dma.startDMAtransfer(); 
}

void TFT_eSPI::pushImageDMA(int32_t x, int32_t y, int32_t w, int32_t h, 
                            uint16_t* image, uint16_t* buffer)
{
  int pixels=w*h;

  // just straight image for now - existing code
  // deals with clipping and swapped bytes, but
  // appears to be buggy...
  if (nullptr == buffer)
    buffer = image;
  else
    memcpy(buffer, image, pixels * sizeof *buffer);
  spi_dma.echoTCR = true;
  setAddrWindow(x,y,w,h);
  spi_dma.echoTCR = false;
  Serial.println();
  pushPixelsDMA((uint16_t*) buffer,pixels);
}
