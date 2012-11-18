;********************************************************
;														*
;	Filename:		f767duino.asm						*
;	Date:			20061023							*
;	File Version:	0.7									*
;		implement analogWrite for hardware pwm on		*
;		digital pins 9,10 and 11.						*
;	Date:			20061016							*
;	File Version:	0.6									*
;		Fixed a bug in receive interrupt handler and	*
;		buffer increment								*
;	Date:			20061012							*
;	File Version:	0.5									*
;		Using shadow registers for tris					*
;		registers to avoid bcf and bsf instructions		*
;	Date:			20061012							*
;	File Version:	0.4									*
;		Fixed another bug. Digital out still doesn't 	*
;		work. Next try using shadow registers for tris	*
;		registers to avoid bcf and bsf instructions		*
;	Date:			20061011							*
;	File Version:	0.3									*
;		fixed a couple of bugs. Now analog input works	*
;	Date:			20060927							*
;	File Version:	0.2									*
;		completed port to PIC asm						*
;	Date:			20060921							*
;	File Version:	0.1									*
;														*
;	Author:			Martin Peach						*
;														* 
;														*
;********************************************************
;											*
;	Files required:	P16F767.INC				*
;											*
;											*
;************************************************************************************************
;																								*
;	This is an implementation of the pd firmware on Arduino using a PIC16F767 instead of an ATmega8.*
;	See picduino.png for the schematic.															*
;																								*
;************************************************************************************************
;___Arduino_Pin_Name____=___PIC_Pin_Name____*
;   DigitalPin0 RXD		=	RC7				*
;   DigitalPin1 TXD		= 	RC6				*
;   DigitalPin2			= 	RC5				*
;   DigitalPin3			= 	RC4				*
;   DigitalPin4			= 	RC3				*
;   DigitalPin5			= 	RB0				*
;   DigitalPin6			= 	RB4				*
;   DigitalPin7			= 	RB7				*
;   DigitalPin8			= 	RB6				*
;   DigitalPin9	 PWM	= 	RB5				*
;   DigitalPin10 PWM	= 	RC2				*
;   DigitalPin11 PWM	= 	RC1				*
;   DigitalPin12		= 	RC0				*
;   DigitalPin13		= 	RB1				*
;	AnalogInput0		= 	RA0/AN0			*
;	AnalogInput1		= 	RA1/AN1			*
;	AnalogInput2		= 	RA2/AN2			*
;	AnalogInput3		= 	RA5/AN4			*
;	AnalogInput4		= 	RB2/AN8			*
;	AnalogInput5		= 	RB3/AN9			*
;	VRef				= 	RA3/AN3			*
;											*
; Timing parameters using PIC16F767:
; FOsc = 14745600Hz
; TOsc = 67.8168ns
; Tad = 32*TOsc = 2.17014us, Tcnv = 12*Tad = 26.0417us Taqu = 12*Tad = 26.0417us
; ADC needs 20us (~10Tad) to charge the sampling capacitor + 2Tad(~5us) after each conversion
; Total adc time = 26*Tad = 56.4236us 
; Maximum ADC rate (1 channel) = 17723.1 conversions per second
;
; Baud rate is 19200
; ADC conversion packet length 3 bytes = 30 bits
; Time to transmit 30 bits at 19200 baud: 0.0015625s = 640 results per second
;********************************************
; This is based on Pd_firmware_pde
; Copyright (C) 2006 Hans-Christoph Steiner
; This library is free software; you can redistribute it and/or
; modify it under the terms of the GNU Lesser General Public
; License as published by the Free Software Foundation; either
; version 2.1 of the License, or (at your option) any later version.
;
; This library is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
; Lesser General Public License for more details.
;
; You should have received a copy of the GNU Lesser General
; Public License along with this library; if not, write to the
; Free Software Foundation, Inc., 59 Temple Place, Suite 330,
; Boston, MA  02111-1307  USA
;
; -----------------------------
; Firmata, the Arduino firmware
; -----------------------------
; 
; Firmata turns the Arduino into a Plug-n-Play sensorbox, servo
; controller, and/or PWM motor/lamp controller.
;
; It was originally designed to work with the Pd object [arduino]
; which is included in Pd-extended.  This firmware is intended to
; work with any host computer software package.  It can easily be
; used with other programs like Max/MSP, Processing, or whatever can
; do serial communications.;
; @authors: Hans-Christoph Steiner <hans@at.or.at>
;   help with protocol redesign: Jamie Allen <jamie@heavyside.net>
;   key bugfixes: Georg Holzmann <grh@mur.at>
;                 Gerda Strobl <gerda.strobl@student.tugraz.at>
; @date: 2006-05-19
; @locations: STEIM, Amsterdam, Netherlands
;             IDMI/Polytechnic University, Brookyn, NY, USA
;             Electrolobby Ars Electronica, Linz, Austria
;
; 
; TODO: add pulseIn functionality
; TODO: add software PWM for servos, etc (servo.h or pulse.h)
; TODO: redesign protocol to accomodate boards with more I/Os
; TODO: 
; TODO: add "outputMode all 0/1" command
; TODO: add cycle markers to mark start of analog, digital, pulseIn, and PWM
;
; =========================================================================
	list		p=16f767			; list directive to define processor
	#include	<P16F767.inc>		; processor specific variable definitions
	
	__CONFIG    _CONFIG1,  _CP_OFF & _CCP2_RC1 & _DEBUG_OFF & _VBOR_2_0 & _BOREN_0 & _MCLR_ON & _PWRTE_ON & _WDT_OFF & _HS_OSC
	__CONFIG    _CONFIG2,  _BORSEN_0 & _IESO_OFF & _FCMEN_OFF

; === CONSTANTS ===========================================================
B19200					equ d'47'	; time constant for baud rate generator
B9600					equ d'95'	; time constant for baud rate generator
; firmware version numbers.  The protocol is still changing, so these version
; numbers are important
MAJOR_VERSION			equ	0
MINOR_VERSION			equ	1
; firmata protocol
; ===============
; data: 0-127
; control: 128-255
; computer->Arduino commands
; --------------------
; 128-129 UNUSED
SET_PIN_ZERO_TO_IN		equ	d'130'	; set digital pin 0 to INPUT
SET_PIN_ONE_TO_IN		equ	d'131'	; set digital pin 1 to INPUT
SET_PIN_TWO_TO_IN		equ	d'132'	; set digital pin 2 to INPUT
SET_PIN_THREE_TO_IN		equ	d'133'	; set digital pin 3 to INPUT
SET_PIN_FOUR_TO_IN		equ	d'134'	; set digital pin 4 to INPUT
SET_PIN_FIVE_TO_IN		equ	d'135'	; set digital pin 5 to INPUT
SET_PIN_SIX_TO_IN		equ	d'136'	; set digital pin 6 to INPUT
SET_PIN_SEVEN_TO_IN		equ	d'137'	; set digital pin 7 to INPUT
SET_PIN_EIGHT_TO_IN		equ	d'138'	; set digital pin 8 to INPUT
SET_PIN_NINE_TO_IN		equ	d'139'	; set digital pin 9 to INPUT
SET_PIN_TEN_TO_IN		equ	d'140'	; set digital pin 10 to INPUT
SET_PIN_ELEVEN_TO_IN	equ	d'141'	; set digital pin 11 to INPUT
SET_PIN_TWELVE_TO_IN	equ	d'142'	; set digital pin 12 to INPUT
SET_PIN_THIRTEEN_TO_IN	equ	d'143'	; set digital pin 13 to INPUT
; 144-149 UNUSED
DISABLE_DIGITAL_INPUTS	equ	d'150'	; disable reporting of digital inputs
ENABLE_DIGITAL_INPUTS	equ	d'151'	; enable reporting of digital inputs
; 152-159 UNUSED
ZERO_ANALOG_INS			equ	d'160'	; disable reporting on all analog ins
ONE_ANALOG_IN			equ	d'161'	; enable reporting for 1 analog in (0)
TWO_ANALOG_INS			equ	d'162'	; enable reporting for 2 analog ins (0,1)
THREE_ANALOG_INS		equ	d'163'	; enable reporting for 3 analog ins (0-2)
FOUR_ANALOG_INS			equ	d'164'	; enable reporting for 4 analog ins (0-3)
FIVE_ANALOG_INS			equ	d'165'	; enable reporting for 5 analog ins (0-4)
SIX_ANALOG_INS			equ	d'166'	; enable reporting for 6 analog ins (0-5)
; 167-199 UNUSED
SET_PIN_ZERO_TO_OUT		equ	d'200'	; set digital pin 0 to OUTPUT
SET_PIN_ONE_TO_OUT		equ	d'201'	; set digital pin 1 to OUTPUT
SET_PIN_TWO_TO_OUT		equ	d'202'	; set digital pin 2 to OUTPUT
SET_PIN_THREE_TO_OUT	equ	d'203'	; set digital pin 3 to OUTPUT
SET_PIN_FOUR_TO_OUT		equ	d'204'	; set digital pin 4 to OUTPUT
SET_PIN_FIVE_TO_OUT		equ	d'205'	; set digital pin 5 to OUTPUT
SET_PIN_SIX_TO_OUT		equ	d'206'	; set digital pin 6 to OUTPUT
SET_PIN_SEVEN_TO_OUT	equ	d'207'	; set digital pin 7 to OUTPUT
SET_PIN_EIGHT_TO_OUT	equ	d'208'	; set digital pin 8 to OUTPUT
SET_PIN_NINE_TO_OUT		equ	d'209'	; set digital pin 9 to OUTPUT
SET_PIN_TEN_TO_OUT		equ	d'210'	; set digital pin 10 to OUTPUT
SET_PIN_ELEVEN_TO_OUT	equ	d'211'	; set digital pin 11 to OUTPUT
SET_PIN_TWELVE_TO_OUT	equ	d'212'	; set digital pin 12 to OUTPUT
SET_PIN_THIRTEEN_TO_OUT	equ	d'213'	; set digital pin 13 to OUTPUT
; 214-228 UNUSED
OUTPUT_TO_DIGITAL_PINS	equ	d'229'	; next two bytes set digital output data 
; 230-239 UNUSED
REPORT_VERSION			equ	d'240'	; return the firmware version
; 240-249 UNUSED
DISABLE_PWM				equ	d'250'	; next byte sets pin # to disable
ENABLE_PWM				equ	d'251'	; next two bytes set pin # and duty cycle
DISABLE_SOFTWARE_PWM	equ	d'252'	; next byte sets pin # to disable
ENABLE_SOFTWARE_PWM		equ	d'253'	; next two bytes set pin # and duty cycle
SET_SOFTWARE_PWM_FREQ	equ	d'254'	; set master frequency for software PWMs
; 255 UNUSED
;*************** 
; two byte digital output data format
; ----------------------
; 0  get ready for digital input bytes (229)
; 1  digitalOut 7-13 bitmask
; 2  digitalOut 0-6 bitmask
; two byte PWM data format
; ----------------------
; 0  get ready for digital input bytes (ENABLE_SOFTWARE_PWM/ENABLE_PWM)
; 1  pin #
; 2  duty cycle expressed as 1 byte (255 = 100%)
; digital input message format
; ----------------------
; 0   digital input marker (255/11111111)
; 1   digital read from Arduino // 7-13 bitmask
; 2   digital read from Arduino // 0-6 bitmask
; analog input message format
; ----------------------
; 0   analog input marker
; 1   high byte from analog input pin 0
; 2   low byte from analog input pin 0
; 3   high byte from analog input pin 1
; 4   low byte from analog input pin 1
; 5   high byte from analog input pin 2
; 6   low byte from analog input pin 2
; 7   high byte from analog input pin 3
; 8   low byte from analog input pin 3
; 9   high byte from analog input pin 4
; 10  low byte from analog input pin 4
; 11  high byte from analog input pin 5
; 12  low byte from analog input pin 5
TOTAL_DIGITAL_PINS		equ	d'14'

INPUT					equ	d'0' ; these aren't defined in Pd_firmware.pde
OUTPUT					equ	d'1' ; these aren't defined in Pd_firmware.pde
; for comparing along with INPUT and OUTPUT
PWM 					equ	d'2'

; maximum number of post-command data bytes
MAX_DATA_BYTES 			equ	d'2'

; == VARIABLES ============================================================
; this flag says the next serial input will be data
;byte waitForData = 0;
waitForData				equ	0x20	; initialize to 0;
;byte executeMultiByteCommand = 0; // command to execute after getting multi-byte data
executeMultiByteCommand	equ	0x21	; initialize to 0
;byte storedInputData[MAX_DATA_BYTES] = {0,0}; // multi-byte data
storedInputData0		equ 0x22	; initialize to 0,0
storedInputData1		equ 0x23	; initialize to 0,0
; this flag says the first data byte for the digital outs is next
;boolean firstInputByte = false;
firstInputByte			equ 0x24	; initialize to 0
; this int serves as a bit-wise array to store pin status
; 0 = INPUT, 1 = OUTPUT
; int digitalPinStatus;
digitalPinStatusHi		equ	0x25	; initialize to 0,0
digitalPinStatusLo		equ	0x26	; initialize to 0,0
; this byte stores the status of whether PWM is on or not
; bit 9 = PWM0, bit 10 = PWM1, bit 11 = PWM2
; the rest of the bits are unused and should remain 0
;int pwmStatus;
pwmStatusHi				equ	0x27	; initialize to 0,0
pwmStatusLo				equ	0x28	; initialize to 0,0
; this byte stores the status of whether software PWM is on or not
; 00000010 00000000 means bit 10 is softWarePWM enabled
;int softPwmStatus;
softPwmStatusHi			equ	0x29	; initialize to 0,0
softPwmStatusLo			equ	0x2A	; initialize to 0,0
;boolean digitalInputsEnabled = true;
digitalInputsEnabled	equ	0x2B	; initialize to 1
;byte analogInputsEnabled = 6;
analogInputsEnabled		equ	0x2C	; initialize to 6;
;byte analogPin;
analogPin				equ	0x2D	;
;int analogData;
analogDataHi			equ	0x2E	;
analogDataLo			equ	0x2F	;
rxBuf					equ	0x40	;
rxBufEnd				equ	0x50	;
i						equ	0x51	; local variable in setup()
pin						equ	0x52	; pin number for pinMode()
mode					equ	0x53	; pin mode for pinMode()
maskHi					equ	0x54
maskLo					equ	0x55
rxWrPtr					equ	0x56
rxRdPtr					equ	0x57
inputData				equ	0x58
digitalPin				equ	0x59
transmitByte			equ	0x5A
digitalData				equ	0x5B
fsr_temp				equ	0x70	; variable used for context saving
w_temp					equ	0x71	; variable used for context saving
status_temp				equ	0x72	; variable used for context saving
pclath_temp				equ	0x73	; variable used for context saving
trisa_shadow			equ	0x74
trisb_shadow			equ	0x75
trisc_shadow			equ	0x76
portb_shadow			equ	0x77
portc_shadow			equ	0x78
; =========================================================================
	ORG			0x000			; processor reset vector
  	goto		setup			; go to beginning of program
; =========================================================================
	ORG			0x004			; interrupt vector location
	movwf		w_temp			; save off current W register contents
	movf		STATUS,w		; move status register into W register
	movwf		status_temp		; save off contents of STATUS register
	movf		PCLATH,w		; move pclath register into w register
	movwf		pclath_temp		; save off contents of PCLATH register

	btfss		PIR1,RCIF
	goto		intout
;receive interrupt
; 6. Flag bit RCIF will be set when reception is complete and an interrupt will be generated if enable bit RCIE is set.
; 7. Read the RCSTA register to get the ninth bit (if enabled) and determine if any error occurred during reception.
	bcf			STATUS,RP0		; bank 0
	movf		RCSTA,w
	andlw		b'00000110'		; Framing, Overrun Errors
	btfss		STATUS,Z
	goto		RxError
; 8. Read the 8-bit received data by reading the RCREG register.
; handle the character by saving it into rxBuf and incrementing rxWrPtr
	movf		FSR,w			; first save FSR
	movwf		fsr_temp
	movf		rxWrPtr,w		; get the pointer to the buffer
	movwf		FSR				; into FSR
	movf		RCREG,w			; read the character
	movwf		INDF			; store it in the buffer
	movf		fsr_temp,w		; restore FSR
	movwf		FSR
	incf		rxWrPtr,f		; point to next slot in buffer
	movlw		rxBufEnd		; check for end of buffer
	xorwf		rxWrPtr,w
	btfss		STATUS,Z
	goto		intout			; not yet at end
	movlw		rxBuf			; reset pointer to start of buffer
	movwf		rxWrPtr
intout
	movf		pclath_temp,w	; retrieve copy of PCLATH register
	movwf		PCLATH			; restore pre-isr PCLATH register contents
	movf		status_temp,w	; retrieve copy of STATUS register
	movwf		STATUS			; restore pre-isr STATUS register contents
	swapf		w_temp,f
	swapf		w_temp,w		; restore pre-isr W register contents
	retfie						; return from interrupt
; 9. If any error occurred, clear the error by clearing enable bit CREN.
RxError
	bcf			RCSTA,CREN
	movf		RCREG,w
	bsf			RCSTA,CREN
	goto		intout

; =========================================================================
; 
; -------------------------------------------------------------------------
;void setup()
;{
setup
		clrf	waitForData	; init all the variables
		clrf	executeMultiByteCommand
		clrf	storedInputData0
		clrf	storedInputData1
		clrf	firstInputByte
		clrf	digitalPinStatusHi
		clrf	digitalPinStatusLo
		clrf	pwmStatusHi
		clrf	pwmStatusLo
		clrf	softPwmStatusHi
		clrf	softPwmStatusLo
		movlw	1
		movwf	digitalInputsEnabled	; initialize to 1
		movlw	6
		movwf	analogInputsEnabled		; initialize to 6;
		movlw	rxBuf
		movwf	rxWrPtr
		movwf	rxRdPtr
		clrf	portb_shadow
		clrf	portc_shadow
		clrf	PORTB	; PORTB is cleared anyway at reset
		clrf	PORTC	; PORTC is cleared anyway at reset
		movlw	0xFF	; all inputs (they are anyway after reset)
		bsf		STATUS,RP0 ; bank 1
		movwf	TRISA
		movwf	trisa_shadow
		movwf	TRISB
		movwf	trisb_shadow
		movwf	TRISC
		movwf	trisc_shadow
		bcf		STATUS,RP0 ; bank 0
;  byte i;
;
;  beginSerial(19200);
		call 	beginSerial_19200
;  for(i=0; i<TOTAL_DIGITAL_PINS; ++i)
;  {
		clrf	i
;    setPinMode(i,INPUT);
slp_1
		movf	i,w
		movwf	pin
		movlw	INPUT
		movwf	mode
		call	setPinMode
		incf	i,f
		movlw	TOTAL_DIGITAL_PINS
		subwf	i,w
		btfss	STATUS,C
		goto	slp_1
; setup analog pins
;The following steps should be followed to do an A/D conversion:
;1. Configure the A/D module:
; Configure analog pins, voltage reference and digital I/O (ADCON1)
		movlw	b'10010101' ; right-justified 10-bit result. VRef+ = RA3, AN0-AN9 analog
		bsf		STATUS,RP0 ; bank 1
		movwf	ADCON1
		bcf		STATUS,RP0 ; bank 0
; Select A/D input channel (ADCON0)
		movlw	b'10000000'	; start with channel 0, 32 Tosc conversion, module off
		movwf	ADCON0
; Select A/D acquisition time (ADCON2)
		movlw	b'00101000' ; wait 12Tad
		bsf		STATUS,RP0 ; bank 1
		movwf	ADCON2	
		bcf		STATUS,RP0 ; bank 0
; Select A/D conversion clock (ADCON0)
; Turn on A/D module (ADCON0)
		bsf		ADCON0,ADON
;2. Configure A/D interrupt (if desired):
; Clear ADIF bit
; Set ADIE bit
; Set PEIE bit
; Set GIE bit
;3. Wait the required acquisition time (if required).
;4. Start conversion:
; Set GO/DONE bit (ADCON0 register)
;5. Wait for A/D conversion to complete, by either:
; Polling for the GO/DONE bit to be cleared
;OR
; Waiting for the A/D interrupt
;6. Read A/D Result registers (ADRESH:ADRESL); clear bit ADIF (if required).
;7. For next conversion, go to step 1 or step 2 as required. The A/D conversion time per bit is
; defined as TAD. A minimum wait of 2 TAD is required before the next acquisition starts.
;
; Setup Timer2 for hardware PWM (1.002kHz: timer prescale 1:16, PR2=254)
		movlw 	d'254' ; 903.53Hz
		bsf		STATUS,RP0 ; bank 1
		movwf	PR2
		bcf		STATUS,RP0 ; bank 0
		movlw	b'00000111' ; timer on, prescale 1:16
		movwf	T2CON
;
;  }
;}
;
; setup falls through into loop and stays there forever...
;
; -------------------------------------------------------------------------
;void loop()
;{
loop
;  checkForInput();  
		call 	checkForInput
;  
;  // read all digital pins, in enabled
;  if(digitalInputsEnabled)
		btfss	digitalInputsEnabled,0
		goto 	lp_1
;  {
;    printByte(ENABLE_DIGITAL_INPUTS);
		movlw	ENABLE_DIGITAL_INPUTS
		call	printByte
;    transmitDigitalInput(7);
		movlw	d'7'
		call	transmitDigitalInput
;    checkForInput();
		call 	checkForInput
;    transmitDigitalInput(0);
		movlw	d'0'
		call	transmitDigitalInput
;    checkForInput();
		call 	checkForInput
;  }
lp_1
;
;  /* get analog in, for the number enabled */
;  for(analogPin=0; analogPin<analogInputsEnabled; ++analogPin)
		clrf	analogPin
lp_2
		movf	analogInputsEnabled,w
		subwf	analogPin,w
		btfsc	STATUS,C
		goto	loop
;  {
;    analogData = analogRead(analogPin);
		call	analogRead ; analogRead sets analogDataHi and analogDataLo according to analogPin
;    // these two bytes get converted back into the whole number in Pd
;    // the higher bits should be zeroed so that the 8th bit doesn't get set
;    printByte(ONE_ANALOG_IN + analogPin); 
		movlw	ONE_ANALOG_IN
		addwf	analogPin,w
		call	printByte
;    printByte(analogData >> 7);  // bitshift the big stuff into the output byte
		rlf		analogDataLo,w ; bit7 into C
		rlf		analogDataHi,w ; C into bit0
		andlw	b'00000111' ; only 10 bits
		call	printByte
;    printByte(analogData % 128); // mod by 32 for the small byte
		movf	analogDataLo,w
		andlw	b'01111111'
		call	printByte
;    checkForInput();
		call 	checkForInput
		incf	analogPin,f
		goto	lp_2
;  }
;}
;
; ============subroutines======================================================
;
; -------------------------------------------------------------------------
; this function checks to see if there is data waiting on the serial port 
; then processes all of the stored data
;void checkForInput()
;{
checkForInput
		movf	rxWrPtr,w
		xorwf	rxRdPtr,w
		btfsc	STATUS,Z
		return
;  if(serialAvailable())
;  {  
;    while(serialAvailable())
;    {
		movf	rxRdPtr,w
		movwf	FSR
		movf	INDF,w
;      processInput( (byte)serialRead() );
		call	processInput
		incf	rxRdPtr,f
		movlw	rxBufEnd
		xorwf	rxRdPtr,w
		btfss	STATUS,Z
		goto	checkForInput	
		movlw	rxBuf ; wrap to start of buffer
		movwf	rxRdPtr
		goto	checkForInput
;    }
;  }
;}
; -------------------------------------------------------------------------
; printByte: on entry w contains the byte to send
printByte
		bsf		STATUS,RP0 ; bank 1
		btfss	TXSTA,TRMT
		goto	$-1
		bcf		STATUS,RP0 ; bank 0
		movwf	TXREG
		return
; -------------------------------------------------------------------------
beginSerial_19200 ; setup USART for 19200 asynchronous receive and transmit, 8bitsNoParity1Stop
; Bits TRISC<7:6> have to be set in order to configure pins RC6/TX/CK and RC7/RX/DT as the USART.
; (They are set at reset)
		bsf		STATUS,RP0 ; bank 1
; When setting up an Asynchronous Transmission, follow these steps:
; 1. Initialize the SPBRG register for the appropriate baud rate. If a high-speed baud rate is desired,
;   set bit BRGH (see Section 11.1 “AUSART Baud Rate Generator (BRG)”).
; * * * For 14.745600MHz crystal clock, SPBRG=47, BRGH=1 gives 19200 baud. * * *
		movlw	B19200 ; = d'47'
		movwf	SPBRG
		bsf		TXSTA,BRGH
; 2. Enable the asynchronous serial port by clearing bit SYNC and setting bit SPEN.
		bcf		STATUS,RP0 ; bank 0
		bsf		RCSTA,SPEN
; 3. If interrupts are desired, then set enable bit TXIE.
; 4. If 9-bit transmission is desired, then set transmit bit TX9.
; 5. Enable the transmission by setting bit TXEN which will also set bit TXIF.
		bsf		STATUS,RP0 ; bank 1
		bsf		TXSTA,TXEN	
; 6. If 9-bit transmission is selected, the ninth bit should be loaded in bit TX9D.
; 7. Load data to the TXREG register (starts transmission).
; 8. If using interrupts, ensure that GIE and PEIE (bits 7 and 6) of the INTCON register are set.
;When setting up an Asynchronous Reception, follow these steps:
; 1. Initialize the SPBRG register for the appropriate baud rate. If a high-speed baud rate is desired,
; set bit BRGH (see Section 11.1 “AUSART Baud Rate Generator (BRG)”).
; 2. Enable the asynchronous serial port by clearing bit SYNC and setting bit SPEN.
; 3. If interrupts are desired, then set enable bit RCIE.
		bsf		PIE1,RCIE	
; 4. If 9-bit reception is desired, then set bit RX9.
; 5. Enable the reception by setting bit CREN.
		bcf		STATUS,RP0 ; bank 0
		bsf		RCSTA,CREN
; 6. Flag bit RCIF will be set when reception is complete and an interrupt will be generated if enable bit RCIE is set.
; 7. Read the RCSTA register to get the ninth bit (if enabled) and determine if any error occurred during reception.
; 8. Read the 8-bit received data by reading the RCREG register.
; 9. If any error occurred, clear the error by clearing enable bit CREN.
; 10. If using interrupts, ensure that GIE and PEIE (bits 7 and 6) of the INTCON register are set.
		bsf		INTCON,PEIE
		bsf		INTCON,GIE
		return
; -------------------------------------------------------------------------
;void transmitDigitalInput(byte startPin)
;{
transmitDigitalInput ; on entry w contains startPin
;  byte i;
;  byte digitalPin;
;//  byte digitalPinBit;
;  byte transmitByte = 0;
		clrf	transmitByte
;  byte digitalData;
;
		movwf	digitalPin
		movwf	i
		clrf	maskLo
		clrf	maskHi
		bsf		STATUS,C
tdilp_1 ; set up the mask with bit startPin set
		rlf		maskLo,f
		rlf		maskHi,f
		movf	i,f
		btfsc	STATUS,Z
		goto	tdi_1 ; mask is at startPin
		decf	i,f
		goto	tdilp_1		
;  for(i=0;i<7;++i)
;  {
tdi_2
;    digitalPin = i+startPin;
		incf	i,f ; ++i
		movlw	d'7'
		xorwf	i,w ; i == 7?
		btfsc	STATUS,Z
		goto	tdi_3 ; yes
		incf	digitalPin,f ; no
		bcf		STATUS,C
		rlf		maskLo,f
		rlf		maskHi,f
;/*    digitalPinBit = OUTPUT << digitalPin;
;// only read the pin if its set to input
;if(digitalPinStatus & digitalPinBit) {
;digitalData = 0; // pin set to OUTPUT, don't read
;}
;else if( (digitalPin >= 9) && (pwmStatus & (1 << digitalPin)) ) {
;digitalData = 0; // pin set to PWM, don't read
;}*/
;    if( !(digitalPinStatus & (1 << digitalPin)) )
tdi_1
		movf	digitalPinStatusLo,w 
		andwf	maskLo,w ; maskLo is lowbyte of 1<<digitalPin
		btfss	STATUS,Z
		goto	tdi_2
		movf	digitalPinStatusHi,w
		andwf	maskHi,w
		btfss	STATUS,Z
		goto	tdi_2
;    {
;
;      digitalData = (byte) digitalRead(digitalPin);
		call	digitalRead ; sets carry to match pin w input data
;      transmitByte = transmitByte + ((1 << i) * digitalData);
;		rlf		transmitByte,f ; carry bit shifts into transmitByte
		rrf		transmitByte,f ; carry bit shifts into transmitByte
		goto	tdi_2	
;    }
;  }
;  printByte(transmitByte);
tdi_3
		bcf		STATUS,C ; shift 0 into high bit
		rrf		transmitByte,f
		movf	transmitByte,w
		call	printByte
;}
		return
;
; -------------------------------------------------------------------------
digitalRead0
		btfsc	PORTC,7
		bsf		STATUS,C
		return		
; -------------------------------------------------------------------------
digitalRead1
		btfsc	PORTC,6
		bsf		STATUS,C
		return		
; -------------------------------------------------------------------------
digitalRead2
		btfsc	PORTC,5
		bsf		STATUS,C
		return		
; -------------------------------------------------------------------------
digitalRead3
		btfsc	PORTC,4
		bsf		STATUS,C
		return		
; -------------------------------------------------------------------------
digitalRead4
		btfsc	PORTC,3
		bsf		STATUS,C
		return		
; -------------------------------------------------------------------------
digitalRead5
		btfsc	PORTB,0
		bsf		STATUS,C
		return		
; -------------------------------------------------------------------------
digitalRead6
		btfsc	PORTB,4
		bsf		STATUS,C
		return		
; -------------------------------------------------------------------------
digitalRead7
		btfsc	PORTB,7
		bsf		STATUS,C
		return		
; -------------------------------------------------------------------------
digitalRead8
		btfsc	PORTB,6
		bsf		STATUS,C
		return		
; -------------------------------------------------------------------------
digitalRead9
		btfsc	PORTB,5
		bsf		STATUS,C
		return		
; -------------------------------------------------------------------------
digitalRead10
		btfsc	PORTC,2
		bsf		STATUS,C
		return		
; -------------------------------------------------------------------------
digitalRead11
		btfsc	PORTC,1
		bsf		STATUS,C
		return		
; -------------------------------------------------------------------------
digitalRead12
		btfsc	PORTC,0
		bsf		STATUS,C
		return		
; -------------------------------------------------------------------------
digitalRead13
		btfsc	PORTB,1
		bsf		STATUS,C
		return		
;
; -------------------------------------------------------------------------
analogRead0 ; AN0
		movlw	b'10000101' ; Fosc/32 channel0 Go On
		goto	analogWait
; -------------------------------------------------------------------------
analogRead1 ; AN1
		movlw	b'10001101' ; Fosc/32 channel1 Go On
		goto	analogWait
; -------------------------------------------------------------------------
analogRead2 ; AN2
		movlw	b'10010101' ; Fosc/32 channel2 Go On
		goto	analogWait
; -------------------------------------------------------------------------
analogRead3 ; AN4
		movlw	b'10100101' ; Fosc/32 channel4 Go On
		goto	analogWait
; -------------------------------------------------------------------------
analogRead4 ; AN8
		movlw	b'10000111' ; Fosc/32 channel8 Go On
		goto	analogWait
; -------------------------------------------------------------------------
analogRead5 ; AN9
		movlw	b'10001111' ; Fosc/32 channel9 Go On
		goto	analogWait
; -------------------------------------------------------------------------
analogWait
		movwf	ADCON0
;3. Wait the required acquisition time (if required).
; we set 12Tad in ADCON2
;4. Start conversion:
; Set GO/DONE bit (ADCON0 register)
;5. Wait for A/D conversion to complete, by either:
; Polling for the GO/DONE bit to be cleared
		btfsc	ADCON0,GO_DONE
		goto	$-1
;OR
; Waiting for the A/D interrupt
;6. Read A/D Result registers (ADRESH:ADRESL); clear bit ADIF (if required).
		movf	ADRESH,w
		movwf	analogDataHi
		bsf		STATUS,RP0 ; bank 1
		movf	ADRESL,w
		bcf		STATUS,RP0 ; bank 0
		movwf	analogDataLo
;7. For next conversion, go to step 1 or step 2 as required. The A/D conversion time per bit is
; defined as TAD. A minimum wait of 2 TAD is required before the next acquisition starts.
		return
; -------------------------------------------------------------------------
; this function sets the pin mode to the correct state and sets the relevant
; bits in the two bit-arrays that track Digital I/O and PWM status
;void setPinMode(int pin, int mode)
;{
;  if(mode == INPUT)
;  {
setPinMode ; on entry pin contains the digital pin number, mode contains the mode
		movlw	INPUT
		xorwf	mode,w
		btfss	STATUS,Z
		goto	setPinToOutOrPwm
		goto	setPinToIn
;    digitalPinStatus = digitalPinStatus &~ (1 << pin);
;    pwmStatus = pwmStatus &~ (1 << pin);
;    pinMode(pin,INPUT);
; -------------------------------------------------------------------------
setPinZeroToIn ; pin0 is RX!!!
		bcf		digitalPinStatusLo,0
		bcf		pwmStatusLo,0
		bsf		trisc_shadow,7
write_trisc
		movf	trisc_shadow,w
		bsf		STATUS,RP0 ; bank 1
		movwf	TRISC	; was bsf TRISC,7
		bcf		STATUS,RP0 ; bank 0
		return
; -------------------------------------------------------------------------
setPinOneToIn ; pin1 is TX!!!!
		bcf		digitalPinStatusLo,1
		bcf		pwmStatusLo,1
		bsf		trisc_shadow,6
		goto	write_trisc
; -------------------------------------------------------------------------
setPinTwoToIn	
		bcf		digitalPinStatusLo,2
		bcf		pwmStatusLo,2
		bsf		trisc_shadow,5
		goto	write_trisc
; -------------------------------------------------------------------------
setPinThreeToIn	
		bcf		digitalPinStatusLo,3
		bcf		pwmStatusLo,3
		bsf		trisc_shadow,4
		goto	write_trisc
; -------------------------------------------------------------------------
setPinFourToIn	
		bcf		digitalPinStatusLo,4
		bcf		pwmStatusLo,4
		bsf		trisc_shadow,3
		goto	write_trisc
; -------------------------------------------------------------------------
setPinFiveToIn	
		bcf		digitalPinStatusLo,5
		bcf		pwmStatusLo,5
		bsf		trisb_shadow,0
write_trisb
		movf	trisb_shadow,w
		bsf		STATUS,RP0 ; bank 1
		movwf	TRISB	; was bsf TRISB,0
		bcf		STATUS,RP0 ; bank 0
		return
; -------------------------------------------------------------------------
setPinSixToIn	
		bcf		digitalPinStatusLo,6
		bcf		pwmStatusLo,6
		bsf		trisb_shadow,4
		goto	write_trisb
; -------------------------------------------------------------------------
setPinSevenToIn	
		bcf		digitalPinStatusLo,7
		bcf		pwmStatusLo,7
		bsf		trisb_shadow,7
		goto	write_trisb
; -------------------------------------------------------------------------
setPinEightToIn	
		bcf		digitalPinStatusHi,0
		bcf		pwmStatusHi,0
		bsf		trisb_shadow,6
		goto	write_trisb
; -------------------------------------------------------------------------
setPinNineToIn	
		bcf		digitalPinStatusHi,1
		bcf		pwmStatusHi,1
		bsf		trisb_shadow,5
		goto	write_trisb
; -------------------------------------------------------------------------
setPinTenToIn	
		bcf		digitalPinStatusHi,2
		bcf		pwmStatusHi,2
		bsf		trisc_shadow,2
		goto	write_trisc
; -------------------------------------------------------------------------
setPinElevenToIn	
		bcf		digitalPinStatusHi,3
		bcf		pwmStatusHi,3
		bsf		trisc_shadow,1
		goto	write_trisc
; -------------------------------------------------------------------------
setPinTwelveToIn	
		bcf		digitalPinStatusHi,4
		bcf		pwmStatusHi,4
		bsf		trisc_shadow,0
		goto	write_trisc
; -------------------------------------------------------------------------
setPinThirteenToIn	
		bcf		digitalPinStatusHi,5
		bcf		pwmStatusHi,5
		bsf		trisb_shadow,1
		goto	write_trisb
;  }
;  else if(mode == OUTPUT)
;  {
; -------------------------------------------------------------------------
setPinToOutOrPwm
		movlw	OUTPUT
		xorwf	mode,w
		btfsc	STATUS,Z
		goto	setPinToPwmOrNo
		goto	setPinToOut
;    digitalPinStatus = digitalPinStatus | (1 << pin);
;    pwmStatus = pwmStatus &~ (1 << pin);
;    pinMode(pin,OUTPUT);
; -------------------------------------------------------------------------
setPinZeroToOut ; pin0 is RX!!! Don't allow this to happen! TRISC.7 must be 1
;		bsf		digitalPinStatusLo,0
;		bcf		pwmStatusLo,0
;		bcf		trisc_shadow,7
;		goto	write_trisc
		return
; -------------------------------------------------------------------------
setPinOneToOut ; pin1 is TX!!!! Don't allow this to happen! TRISC.6 must be 1
;		bsf		digitalPinStatusLo,1
;		bcf		pwmStatusLo,1
;		bcf		trisc_shadow,6
;		goto	write_trisc
		return
; -------------------------------------------------------------------------
setPinTwoToOut	
		bsf		digitalPinStatusLo,2
		bcf		pwmStatusLo,2
		bcf		trisc_shadow,5
		goto	write_trisc
; -------------------------------------------------------------------------
setPinThreeToOut	
		bsf		digitalPinStatusLo,3
		bcf		pwmStatusLo,3
		bcf		trisc_shadow,4
		goto	write_trisc
; -------------------------------------------------------------------------
setPinFourToOut	
		bsf		digitalPinStatusLo,4
		bcf		pwmStatusLo,4
		bcf		trisc_shadow,3
		goto	write_trisc
; -------------------------------------------------------------------------
setPinFiveToOut	
		bsf		digitalPinStatusLo,5
		bcf		pwmStatusLo,5
		bcf		trisb_shadow,0
		goto	write_trisb
; -------------------------------------------------------------------------
setPinSixToOut	
		bsf		digitalPinStatusLo,6
		bcf		pwmStatusLo,6
		bcf		trisb_shadow,4
		goto	write_trisb
; -------------------------------------------------------------------------
setPinSevenToOut	
		bsf		digitalPinStatusLo,7
		bcf		pwmStatusLo,7
		bcf		trisb_shadow,7
		goto	write_trisb
; -------------------------------------------------------------------------
setPinEightToOut	
		bsf		digitalPinStatusHi,0
		bcf		pwmStatusHi,0
		bcf		trisb_shadow,6
		goto	write_trisb
; -------------------------------------------------------------------------
setPinNineToOut	
		bsf		digitalPinStatusHi,1
		bcf		pwmStatusHi,1
		bcf		trisb_shadow,5
		goto	write_trisb
; -------------------------------------------------------------------------
setPinTenToOut	
		bsf		digitalPinStatusHi,2
		bcf		pwmStatusHi,2
		bcf		trisc_shadow,2
		goto	write_trisc
; -------------------------------------------------------------------------
setPinElevenToOut	
		bsf		digitalPinStatusHi,3
		bcf		pwmStatusHi,3
		bcf		trisc_shadow,1
		goto	write_trisc
; -------------------------------------------------------------------------
setPinTwelveToOut	
		bsf		digitalPinStatusHi,4
		bcf		pwmStatusHi,4
		bcf		trisc_shadow,0
		goto	write_trisc
; -------------------------------------------------------------------------
setPinThirteenToOut	
		bsf		digitalPinStatusHi,5
		bcf		pwmStatusHi,5
		bcf		trisb_shadow,1
		goto	write_trisb
;  }
;  else if( (mode == PWM) && (pin >= 9) && (pin <= 11) )
;  {
; -------------------------------------------------------------------------
setPinToPwmOrNo
		movlw	PWM
		xorwf	mode,w
		btfsc	STATUS,Z
		return	; no more choices
		goto	setPinToPwm
;    digitalPinStatus = digitalPinStatus | (1 << pin);
;    pwmStatus = pwmStatus | (1 << pin);
;    pinMode(pin,OUTPUT);
; -------------------------------------------------------------------------
setPinZeroToPWM ; pin0 is RX!!! Don't allow this to happen! TRISC.7 must be 1
;		bsf		digitalPinStatusLo,0
;		bsf		pwmStatusLo,0
;		bcf		trisc_shadow,7
;		goto	write_trisc
;		bsf		STATUS,RP0 ; bank 1
;		bcf		TRISC,7
;		bcf		STATUS,RP0 ; bank 0
		return
; -------------------------------------------------------------------------
setPinOneToPWM ; pin1 is TX!!!! Don't allow this to happen! TRISC.6 must be 1
;		bsf		digitalPinStatusLo,1
;		bsf		pwmStatusLo,1
;		bcf		trisc_shadow,6
;		goto	write_trisc
;		bsf		STATUS,RP0 ; bank 1
;		bcf		TRISC,6
;		bcf		STATUS,RP0 ; bank 0
		return
; -------------------------------------------------------------------------
setPinTwoToPWM	
		bsf		digitalPinStatusLo,2
		bsf		pwmStatusLo,2
		bcf		trisc_shadow,5
		goto	write_trisc
; -------------------------------------------------------------------------
setPinThreeToPWM	
		bsf		digitalPinStatusLo,3
		bsf		pwmStatusLo,3
		bcf		trisc_shadow,4
		goto	write_trisc
; -------------------------------------------------------------------------
setPinFourToPWM	
		bsf		digitalPinStatusLo,4
		bsf		pwmStatusLo,4
		bcf		trisc_shadow,3
		goto	write_trisc
; -------------------------------------------------------------------------
setPinFiveToPWM	
		bsf		digitalPinStatusLo,5
		bsf		pwmStatusLo,5
		bcf		trisb_shadow,0
		goto	write_trisb
; -------------------------------------------------------------------------
setPinSixToPWM	
		bsf		digitalPinStatusLo,6
		bsf		pwmStatusLo,6
		bcf		trisb_shadow,4
		goto	write_trisb
; -------------------------------------------------------------------------
setPinSevenToPWM	
		bsf		digitalPinStatusLo,7
		bsf		pwmStatusLo,7
		bcf		trisb_shadow,7
		goto	write_trisb
; -------------------------------------------------------------------------
setPinEightToPWM	
		bsf		digitalPinStatusHi,0
		bsf		pwmStatusHi,0
		bcf		trisb_shadow,6
		goto	write_trisb
; -------------------------------------------------------------------------
setPinNineToPWM	
		bsf		digitalPinStatusHi,1
		bsf		pwmStatusHi,1
		bcf		trisb_shadow,5
		goto	write_trisb
; -------------------------------------------------------------------------
setPinTenToPWM	
		bsf		digitalPinStatusHi,2
		bsf		pwmStatusHi,2
		bcf		trisc_shadow,2
		goto	write_trisc
; -------------------------------------------------------------------------
setPinElevenToPWM	
		bsf		digitalPinStatusHi,3
		bsf		pwmStatusHi,3
		bcf		trisc_shadow,1
		goto	write_trisc
; -------------------------------------------------------------------------
setPinTwelveToPWM	
		bsf		digitalPinStatusHi,4
		bsf		pwmStatusHi,4
		bcf		trisc_shadow,0
		goto	write_trisc
; -------------------------------------------------------------------------
setPinThirteenToPWM	
		bsf		digitalPinStatusHi,5
		bsf		pwmStatusHi,5
		bcf		trisb_shadow,1
		goto	write_trisb
;  }
;}
;		return
; -------------------------------------------------------------------------
;
;void setSoftPwm (int pin, byte pulsePeriod) {
;  byte i;
;  /*    for(i=0; i<7; ++i) {
;      mask = 1 << i;
;      if(digitalPinStatus & mask) {
;      digitalWrite(i, inputData & mask);
;      } 
;      }
;  */    
;  //read timer type thing
;
;  //loop through each pin, turn them on if selected
;  //softwarePWMStatus
;  //check timer type thing against pulsePeriods for each pin 
;  //throw pin low if expired
;}
;
; -------------------------------------------------------------------------
;void setSoftPwmFreq(byte freq) {
;}
;
;
; -------------------------------------------------------------------------
;void disSoftPwm(int pin) {
;  //throw pin low
;    
;}
;
write_portc
		movf	portc_shadow,w
		movwf	PORTC
		return
write_portb
		movf	portb_shadow,w
		movwf	PORTB
		return
; -------------------------------------------------------------------------
digitalWrite0 ; write carry bit to digital pin 0 (Pin0 is RX!!!)
		bsf		portc_shadow,7
		btfss	STATUS,C
		bcf		portc_shadow,7
		goto	write_portc
; -------------------------------------------------------------------------
digitalWrite1 ; write carry bit to digital pin 1 (Pin1 is tX!!!)
		bsf		portc_shadow,6
		btfss	STATUS,C
		bcf		portc_shadow,6
		goto	write_portc
; -------------------------------------------------------------------------
digitalWrite2 ; write carry bit to digital pin 2
		bsf		portc_shadow,5
		btfss	STATUS,C
		bcf		portc_shadow,5
		goto	write_portc
; -------------------------------------------------------------------------
digitalWrite3 ; write carry bit to digital pin 3
		bsf		portc_shadow,4
		btfss	STATUS,C
		bcf		portc_shadow,4
		goto	write_portc
; -------------------------------------------------------------------------
digitalWrite4 ; write carry bit to digital pin 4
		bsf		portc_shadow,3
		btfss	STATUS,C
		bcf		portc_shadow,3
		goto	write_portc
; -------------------------------------------------------------------------
digitalWrite5 ; write carry bit to digital pin 5
		bsf		portb_shadow,0
		btfss	STATUS,C
		bcf		portb_shadow,0
		goto	write_portb
; -------------------------------------------------------------------------
digitalWrite6 ; write carry bit to digital pin 6
		bsf		portb_shadow,4
		btfss	STATUS,C
		bcf		portb_shadow,4
		goto	write_portb
; -------------------------------------------------------------------------
digitalWrite7 ; write carry bit to digital pin 7
		bsf		portb_shadow,7
		btfss	STATUS,C
		bcf		portb_shadow,7
		goto	write_portb
; -------------------------------------------------------------------------
digitalWrite8 ; write carry bit to digital pin 8
		bsf		portb_shadow,6
		btfss	STATUS,C
		bcf		portb_shadow,6
		goto	write_portb
; -------------------------------------------------------------------------
digitalWrite9 ; write carry bit to digital pin 9
		bsf		portb_shadow,5
		btfss	STATUS,C
		bcf		portb_shadow,5
		goto	write_portb
; -------------------------------------------------------------------------
digitalWrite10 ; write carry bit to digital pin 10
		bsf		portc_shadow,2
		btfss	STATUS,C
		bcf		portc_shadow,2
		goto	write_portc
; -------------------------------------------------------------------------
digitalWrite11 ; write carry bit to digital pin 11
		bsf		portc_shadow,1
		btfss	STATUS,C
		bcf		portc_shadow,1
		goto	write_portc
; -------------------------------------------------------------------------
digitalWrite12 ; write carry bit to digital pin 12
		bsf		portc_shadow,0
		btfss	STATUS,C
		bcf		portc_shadow,0
		goto	write_portc
; -------------------------------------------------------------------------
digitalWrite13 ; write carry bit to digital pin 13
		bsf		portb_shadow,1
		btfss	STATUS,C
		bcf		portb_shadow,1
		goto	write_portb
;
; 
; -------------------------------------------------------------------------
disableDigitalInputs
		clrf	digitalInputsEnabled
		return
; -------------------------------------------------------------------------
enableDigitalInputs
		bsf		digitalInputsEnabled,0
		return
; -------------------------------------------------------------------------
setZeroAnalogIns
		clrw
		goto	setAnalogIns
; -------------------------------------------------------------------------
setOneAnalogIn
		movlw	d'1'
		goto	setAnalogIns
; -------------------------------------------------------------------------
setTwoAnalogIns
		movlw	d'2'
		goto	setAnalogIns
; -------------------------------------------------------------------------
setThreeAnalogIns
		movlw	d'3'
		goto	setAnalogIns
; -------------------------------------------------------------------------
setFourAnalogIns
		movlw	d'4'
		goto	setAnalogIns
; -------------------------------------------------------------------------
setFiveAnalogIns
		movlw	d'5'
		goto	setAnalogIns
; -------------------------------------------------------------------------
setSixAnalogIns
		movlw	d'6'
; -------------------------------------------------------------------------
setAnalogIns
;      analogInputsEnabled = inputData - ZERO_ANALOG_INS;
;      break;
		movwf	analogInputsEnabled
		return
; -------------------------------------------------------------------------
;    case ENABLE_PWM:
enablePwm
;    case ENABLE_SOFTWARE_PWM:
enableSoftPwm
;      waitForData = 2;  // 2 bytes needed (pin#, dutyCycle) 
		movlw	d'2'
		movwf	waitForData
;      executeMultiByteCommand = inputData;
		movf	inputData,w
		movwf	executeMultiByteCommand
;      break;
		return
; -------------------------------------------------------------------------
;    case DISABLE_PWM:
disablePwm
;    case SET_SOFTWARE_PWM_FREQ:
setSoftPwmFreq
;    case DISABLE_SOFTWARE_PWM:
disableSoftPwm
;      waitForData = 1;  // 1 byte needed (pin#)
		movlw	d'1'
		movwf	waitForData
;      executeMultiByteCommand = inputData;
		movf	inputData,w
		movwf	executeMultiByteCommand
;      break;      
		return
; -------------------------------------------------------------------------
;    case OUTPUT_TO_DIGITAL_PINS:   // bytes to send to digital outputs
setOutputToDigitalPins
;      firstInputByte = true;
		bsf		firstInputByte,0
;      break;
		return
; -------------------------------------------------------------------------
;    case REPORT_VERSION:
reportVersion
;      printByte(REPORT_VERSION);
		movlw	REPORT_VERSION
		call	printByte
;      printByte(MAJOR_VERSION);
		movlw	MAJOR_VERSION
		call	printByte
;      printByte(MINOR_VERSION);
		movlw	MINOR_VERSION
		call	printByte
;      break;
;    }
;  }
		return
;}
;
; -------------------------------------------------------------------------
; processInput() is called whenever a byte is available on the
; Arduino's serial port.  This is where the commands are handled.
; 
;void processInput(byte inputData)
;{
processInput ; on entry w contains a byte of input data from rxBuf
;  int i;
;  int mask;
		movwf	inputData ; save the new byte
;  
;  // a few commands have byte(s) of data following the command
;  if( waitForData > 0) 
;  {  
		movf	waitForData,f
		btfsc	STATUS,Z
		goto	pi_1
;    waitForData--;
		decf	waitForData,f
;    storedInputData[waitForData] = inputData;
		movlw	storedInputData0
		addwf	waitForData,w
		movwf	FSR
		movf	inputData,w
		movwf	INDF
;
;    if(executeMultiByteCommand && (waitForData==0))
		movf	executeMultiByteCommand,f
		btfsc	STATUS,Z
		return	; executeMultiByteCommand == 0
		movf	waitForData,f
		btfss	STATUS,Z
		return	; waitForData != 0
;    {
;      //we got everything
		goto	doMultiByteCommand
; -------------------------------------------------------------------------
mb_enablePwm
;      case ENABLE_PWM: ; 251
;        setPinMode(storedInputData[1],PWM);
		movlw	PWM
		movwf	mode
		movf	storedInputData1,w
		movwf	pin
		sublw	d'9'	; 9-PIN pins 9,10,11 only
		btfsc	STATUS,Z
		goto	setPWM9
		addlw	d'1'
		btfsc	STATUS,Z
		goto	setPWM10
		addlw	d'1'
		btfss	STATUS,Z
		goto	pi_6	; not a hardware PWM pin
setPWM11
		call	setPinMode
		movf	storedInputData0,w
		movwf	CCPR2L
		movlw	b'00001100' ; PWM mode, PWM LSBs=00
		movwf	CCP2CON
		goto	pi_6
setPWM10
		call	setPinMode
		movf	storedInputData0,w
		movwf	CCPR1L
		movlw	b'00001100' ; PWM mode, PWM LSBs=00
		movwf	CCP1CON
		goto	pi_6
setPWM9
		call	setPinMode
		movf	storedInputData0,w
		bsf		STATUS,RP0 ; bank 1
		movwf	CCPR3L
		movlw	b'00001100' ; PWM mode, PWM LSBs=00
		movwf	CCP3CON
		bcf		STATUS,RP0 ; bank 0
		goto	pi_6
				
;        analogWrite(storedInputData[1], storedInputData[0]);
; analogWrite from wiring.c (http://svn.berlios.de/svnroot/repos/arduino/trunk/targets/arduino/wiring.c):
;// Right now, PWM output only works on the pins with
;// hardware support.  These are defined in the appropriate
;// pins_*.c file.  For the rest of the pins, we default
;// to digital output.
;void analogWrite(int pin, int val)
;{
;	// We need to make sure the PWM output is enabled for those pins
;	// that support it, as we turn it off when digitally reading or
;	// writing with them.  Also, make sure the pin is in output mode
;	// for consistenty with Wiring, which doesn't require a pinMode
;	// call for the analog output pins.
;	pinMode(pin, OUTPUT);
;	
;	if (analogOutPinToTimer(pin) == TIMER1A) {
;		// connect pwm to pin on timer 1, channel A
;		sbi(TCCR1A, COM1A1);
;		// set pwm duty
;		OCR1A = val;
;	} else if (analogOutPinToTimer(pin) == TIMER1B) {
;		// connect pwm to pin on timer 1, channel B
;		sbi(TCCR1A, COM1B1);
;		// set pwm duty
;		OCR1B = val;
;#if defined(__AVR_ATmega168__)
;	} else if (analogOutPinToTimer(pin) == TIMER0A) {
;		// connect pwm to pin on timer 0, channel A
;		sbi(TCCR0A, COM0A1);
;		// set pwm duty
;		OCR0A = val;	
;	} else if (analogOutPinToTimer(pin) == TIMER0B) {
;		// connect pwm to pin on timer 0, channel B
;		sbi(TCCR0A, COM0B1);
;		// set pwm duty
;		OCR0B = val;
;	} else if (analogOutPinToTimer(pin) == TIMER2A) {
;		// connect pwm to pin on timer 2, channel A
;		sbi(TCCR2A, COM2A1);
;		// set pwm duty
;		OCR2A = val;	
;	} else if (analogOutPinToTimer(pin) == TIMER2B) {
;		// connect pwm to pin on timer 2, channel B
;		sbi(TCCR2A, COM2B1);
;		// set pwm duty
;		OCR2B = val;
;#else
;	} else if (analogOutPinToTimer(pin) == TIMER2) {
;		// connect pwm to pin on timer 2, channel B
;		sbi(TCCR2, COM21);
;		// set pwm duty
;		OCR2 = val;
;#endif
;	} else if (val < 128)
;		digitalWrite(pin, LOW);
;	else
;		digitalWrite(pin, HIGH);
;}
; end of analogWrite() from wiring.c
;        break;
		goto	pi_6
; -------------------------------------------------------------------------
mb_disablePwm
;      case DISABLE_PWM: ; 250
;        setPinMode(storedInputData[0],INPUT);
		movlw	INPUT
		movwf	mode
		movf	storedInputData0,w
		movwf	pin
		sublw	d'9'	; 9 - PIN, pins 9,10,11 only
		btfsc	STATUS,Z
		goto	disPWM9
		addlw	d'1'
		btfsc	STATUS,Z
		goto	disPWM10
		addlw	d'1'
		btfss	STATUS,Z
		goto	pi_6	; not a hardware PWM pin
disPWM11
		call 	setPinMode
		clrf	CCP2CON
		goto	pi_6	; not a hardware PWM pin
disPWM10
		call 	setPinMode
		clrf	CCP1CON
		goto	pi_6	; not a hardware PWM pin
disPWM9
		call 	setPinMode
		bsf		STATUS,RP0 ; bank 1
		clrf	CCP3CON
		bcf		STATUS,RP0 ; bank 0
		goto	pi_6	; not a hardware PWM pin
;        break;
; -------------------------------------------------------------------------
mb_enableSoftPwm
;      case ENABLE_SOFTWARE_PWM: ; 253
;        setPinMode(storedInputData[1],PWM);
		movf	storedInputData1,w
		movwf	pin
		movlw	PWM
		movwf	mode
		call	setPinMode
;        setSoftPwm(storedInputData[1], storedInputData[0]);     
;        break; 
		goto	pi_6
; -------------------------------------------------------------------------
mb_disableSoftPwm
;      case DISABLE_SOFTWARE_PWM: ; 252
;        disSoftPwm(storedInputData[0]);
;        break;
		goto	pi_6
; -------------------------------------------------------------------------
mb_setSoftPwmFreq
;      case SET_SOFTWARE_PWM_FREQ: ; 254
;        setSoftPwmFreq(storedInputData[0]);
;        break;
		goto	pi_6
;      }
; -------------------------------------------------------------------------
pi_6
;      executeMultiByteCommand = 0;
		clrf	executeMultiByteCommand
;    }
		return
;  }
; -------------------------------------------------------------------------
;  else if(inputData < 128)
;  {
pi_1
		btfsc	inputData,7
		goto	doCommand
;    if(firstInputByte)
		movf	pwmStatusLo,w
		xorlw	0xFF
		andwf	digitalPinStatusLo,w
		movwf	maskLo
		movf	pwmStatusHi,w
		xorlw	0xFF
		andwf	digitalPinStatusHi,w
		movwf	maskHi
		btfss	firstInputByte,0
		goto	pi_8
;    {
;      // output data for pins 7-13
;      for(i=7; i<TOTAL_DIGITAL_PINS; ++i)
;      {
;        mask = 1 << i;
;        if( (digitalPinStatus & mask) && !(pwmStatus & mask) )
;        {
;          // inputData is a byte and mask is an int, so align the high part of mask
		btfss	maskLo,7
		goto	pi_12
		bcf		STATUS,C ; the data bit is zero
		btfsc	inputData,0
		bsf		STATUS,C ; unless it's one
		call	digitalWrite7
pi_12
		btfss	maskHi,0
		goto	pi_11
		bcf		STATUS,C ; the data bit is zero
		btfsc	inputData,1
		bsf		STATUS,C ; unless it's one
		call	digitalWrite8
pi_11
		btfss	maskHi,1
		goto	pi_10
		bcf		STATUS,C ; the data bit is zero
		btfsc	inputData,2
		bsf		STATUS,C ; unless it's one
		call	digitalWrite9
pi_10
		btfss	maskHi,2
		goto	pi_09
		bcf		STATUS,C ; the data bit is zero
		btfsc	inputData,3
		bsf		STATUS,C ; unless it's one
		call	digitalWrite10
pi_09
		btfss	maskHi,3
		goto	pi_08
		bcf		STATUS,C ; the data bit is zero
		btfsc	inputData,4
		bsf		STATUS,C ; unless it's one
		call	digitalWrite11
pi_08
		btfss	maskHi,4
		goto	pi_07
		bcf		STATUS,C ; the data bit is zero
		btfsc	inputData,5
		bsf		STATUS,C ; unless it's one
		call	digitalWrite12
pi_07
		btfss	maskHi,5
		goto	pi_06
		bcf		STATUS,C ; the data bit is zero
		btfsc	inputData,6
		bsf		STATUS,C ; unless it's one
		call	digitalWrite13
;          digitalWrite(i, inputData & (mask >> 7));
;        }        
;      }
pi_06
;      firstInputByte = false;
		clrf	firstInputByte
		return
;    }
;    else
pi_8
;    { //
;      for(i=0; i<7; ++i)
;      {
;        mask = 1 << i;
;        if( (digitalPinStatus & mask) && !(pwmStatus & mask) )
;        {
;          digitalWrite(i, inputData & mask);
		btfss	maskLo,0
		goto	pi_13
		bcf		STATUS,C ; the data bit is zero
		btfsc	inputData,0
		bsf		STATUS,C ; unless it's one
		call	digitalWrite0
pi_13
		btfss	maskLo,1
		goto	pi_14
		bcf		STATUS,C ; the data bit is zero
		btfsc	inputData,1
		bsf		STATUS,C ; unless it's one
		call	digitalWrite1
pi_14
		btfss	maskLo,2
		goto	pi_15
		bcf		STATUS,C ; the data bit is zero
		btfsc	inputData,2
		bsf		STATUS,C ; unless it's one
		call	digitalWrite2
pi_15
		btfss	maskLo,3
		goto	pi_16
		bcf		STATUS,C ; the data bit is zero
		btfsc	inputData,3
		bsf		STATUS,C ; unless it's one
		call	digitalWrite3
pi_16
		btfss	maskLo,4
		goto	pi_17
		bcf		STATUS,C ; the data bit is zero
		btfsc	inputData,4
		bsf		STATUS,C ; unless it's one
		call	digitalWrite4
pi_17
		btfss	maskLo,5
		goto	pi_18
		bcf		STATUS,C ; the data bit is zero
		btfsc	inputData,5
		bsf		STATUS,C ; unless it's one
		call	digitalWrite5
pi_18
		btfss	maskLo,6
		goto	pi_19
		bcf		STATUS,C ; the data bit is zero
		btfsc	inputData,6
		bsf		STATUS,C ; unless it's one
		call	digitalWrite6
;        } 
;      }
;    }
pi_19
		return
;  }

		ORG		0x400	; table lookup works better when it doesn't cross a 256-byte page boundary
; -------------------------------------------------------------------------
;  else
;  {
; inputData > 127
;    switch (inputData)
;    {
doCommand
		movlw	HIGH commandTab
		movwf	PCLATH
		movf	inputData,w
		andlw	0x7F ; skip the first 127 values
		addwf	PCL,f	; jump to command
commandTab
; 128-129 UNUSED
		return	; 128
		return	; 129
		goto	setPinZeroToIn		;SET_PIN_ZERO_TO_IN equ d'130' ; set digital pin 0 to INPUT
		goto	setPinOneToIn		;SET_PIN_ONE_TO_IN equ d'131' ; set digital pin 1 to INPUT
		goto	setPinTwoToIn		;SET_PIN_TWO_TO_IN equ d'132' ; set digital pin 2 to INPUT
		goto	setPinThreeToIn		;SET_PIN_THREE_TO_IN equ d'133' ; set digital pin 3 to INPUT
		goto	setPinFourToIn		;SET_PIN_FOUR_TO_IN equ d'134' ; set digital pin 4 to INPUT
		goto	setPinFiveToIn		;SET_PIN_FIVE_TO_IN equ d'135' ; set digital pin 5 to INPUT
		goto	setPinSixToIn		;SET_PIN_SIX_TO_IN equ d'136' ; set digital pin 6 to INPUT
		goto	setPinSevenToIn		;SET_PIN_SEVEN_TO_IN equ d'137' ; set digital pin 7 to INPUT
		goto	setPinEightToIn		;SET_PIN_EIGHT_TO_IN equ d'138' ; set digital pin 8 to INPUT
		goto	setPinNineToIn		;SET_PIN_NINE_TO_IN equ	d'139' ; set digital pin 9 to INPUT
		goto	setPinTenToIn		;SET_PIN_TEN_TO_IN equ d'140' ; set digital pin 10 to INPUT
		goto	setPinElevenToIn	;SET_PIN_ELEVEN_TO_IN equ d'141' ; set digital pin 11 to INPUT
		goto	setPinTwelveToIn	;SET_PIN_TWELVE_TO_IN equ d'142' ; set digital pin 12 to INPUT
		goto	setPinThirteenToIn	;SET_PIN_THIRTEEN_TO_IN equ d'143' ; set digital pin 13 to INPUT
; 144-149 UNUSED
		return	; 144
		return	; 145
		return	; 146
		return	; 147
		return	; 148
		return	; 149
		goto	disableDigitalInputs	;DISABLE_DIGITAL_INPUTS equ d'150' ; disable reporting of digital inputs
		goto	enableDigitalInputs		;ENABLE_DIGITAL_INPUTS equ d'151' ; enable reporting of digital inputs
; 152-159 UNUSED
		return	; 152
		return	; 153
		return	; 154
		return	; 155
		return	; 156
		return	; 157
		return	; 158
		return	; 159
		goto	setZeroAnalogIns	;ZERO_ANALOG_INS equ d'160' ; disable reporting on all analog ins
		goto	setOneAnalogIn		;ONE_ANALOG_IN equ d'161' ; enable reporting for 1 analog in (0)
		goto	setTwoAnalogIns		;TWO_ANALOG_INS equ d'162' ; enable reporting for 2 analog ins (0,1)
		goto	setThreeAnalogIns	;THREE_ANALOG_INS equ d'163' ; enable reporting for 3 analog ins (0-2)
		goto	setFourAnalogIns	;FOUR_ANALOG_INS equ d'164' ; enable reporting for 4 analog ins (0-3)
		goto	setFiveAnalogIns	;FIVE_ANALOG_INS equ d'165' ; enable reporting for 5 analog ins (0-4)
		goto	setSixAnalogIns		;SIX_ANALOG_INS equ d'166' ; enable reporting for 6 analog ins (0-5)
; 167-199 UNUSED
		return	; 167
		return	; 168
		return	; 169
		return	; 170
		return	; 171
		return	; 172
		return	; 173
		return	; 174
		return	; 175
		return	; 176
		return	; 177
		return	; 178
		return	; 179
		return	; 180
		return	; 181
		return	; 182
		return	; 183
		return	; 184
		return	; 185
		return	; 186
		return	; 187
		return	; 188
		return	; 189
		return	; 190
		return	; 191
		return	; 192
		return	; 193
		return	; 194
		return	; 195
		return	; 196
		return	; 197
		return	; 198
		return	; 199
		goto	setPinZeroToOut		;SET_PIN_ZERO_TO_OUT equ d'200' ; set digital pin 0 to OUTPUT
		goto	setPinOneToOut		;SET_PIN_ONE_TO_OUT equ d'201' ; set digital pin 1 to OUTPUT
		goto	setPinTwoToOut		;SET_PIN_TWO_TO_OUT equ d'202' ; set digital pin 2 to OUTPUT
		goto	setPinThreeToOut	;SET_PIN_THREE_TO_OUT equ d'203' ; set digital pin 3 to OUTPUT
		goto	setPinFourToOut		;SET_PIN_FOUR_TO_OUT equ d'204' ; set digital pin 4 to OUTPUT
		goto	setPinFiveToOut		;SET_PIN_FIVE_TO_OUT equ d'205' ; set digital pin 5 to OUTPUT
		goto	setPinSixToOut		;SET_PIN_SIX_TO_OUT equ d'206' ; set digital pin 6 to OUTPUT
		goto	setPinSevenToOut	;SET_PIN_SEVEN_TO_OUT equ d'207' ; set digital pin 7 to OUTPUT
		goto	setPinEightToOut	;SET_PIN_EIGHT_TO_OUT equ d'208' ; set digital pin 8 to OUTPUT
		goto	setPinNineToOut		;SET_PIN_NINE_TO_OUT equ d'209' ; set digital pin 9 to OUTPUT
		goto	setPinTenToOut		;SET_PIN_TEN_TO_OUT equ d'210' ; set digital pin 10 to OUTPUT
		goto	setPinElevenToOut	;SET_PIN_ELEVEN_TO_OUT equ d'211' ; set digital pin 11 to OUTPUT
		goto	setPinTwelveToOut	;SET_PIN_TWELVE_TO_OUT equ d'212' ; set digital pin 12 to OUTPUT
		goto	setPinThirteenToOut	;SET_PIN_THIRTEEN_TO_OUT equ d'213'	; set digital pin 13 to OUTPUT
; 214-228 UNUSED
		return	; 214
		return	; 215
		return	; 216
		return	; 217
		return	; 218
		return	; 219
		return	; 220
		return	; 221
		return	; 222
		return	; 223
		return	; 224
		return	; 225
		return	; 226
		return	; 227
		return	; 228
		goto	setOutputToDigitalPins	;OUTPUT_TO_DIGITAL_PINS equ d'229' ; next two bytes set digital output data 
; 230-239 UNUSED
		return; 230
		return; 231
		return; 232
		return; 233
		return; 234
		return; 235
		return; 236
		return; 237
		return; 238
		return; 239
		goto	reportVersion	;REPORT_VERSION equ d'240' ; return the firmware version
; 240-249 UNUSED
		return	; 241
		return	; 242
		return	; 243
		return	; 244
		return	; 245
		return	; 246
		return	; 247
		return	; 248
		return	; 249
		goto	disablePwm	;DISABLE_PWM equ d'250' ; next byte sets pin # to disable
		goto	enablePwm	;ENABLE_PWM equ d'251' ; next two bytes set pin # and duty cycle
		goto	disableSoftPwm	;DISABLE_SOFTWARE_PWM equ d'252' ; next byte sets pin # to disable
		goto	enableSoftPwm	;ENABLE_SOFTWARE_PWM equ d'253' ; next two bytes set pin # and duty cycle
		goto	setSoftPwmFreq	;SET_SOFTWARE_PWM_FREQ equ d'254' ; set master frequency for software PWMs
; 255 UNUSED
		return	; 255
; -------------------------------------------------------------------------
digitalRead ; on entry digitalPin is the pin to read, returns value in carry
		movlw	HIGH digRdTab
		movwf	PCLATH
		movf	digitalPin,w	
		bcf		STATUS,C
		addwf	PCL,f
digRdTab
		goto	digitalRead0
		goto	digitalRead1
		goto	digitalRead2
		goto	digitalRead3
		goto	digitalRead4
		goto	digitalRead5
		goto	digitalRead6
		goto	digitalRead7
		goto	digitalRead8
		goto	digitalRead9
		goto	digitalRead10
		goto	digitalRead11
		goto	digitalRead12
		goto	digitalRead13
; -------------------------------------------------------------------------
setPinToIn
;    digitalPinStatus = digitalPinStatus &~ (1 << pin);
;    pwmStatus = pwmStatus &~ (1 << pin);
;    pinMode(pin,INPUT);
		movlw	HIGH pinintab
		movwf	PCLATH
		movf	pin,w
		addwf	PCL,f
pinintab
		goto	setPinZeroToIn	
		goto	setPinOneToIn	
		goto	setPinTwoToIn	
		goto	setPinThreeToIn	
		goto	setPinFourToIn	
		goto	setPinFiveToIn	
		goto	setPinSixToIn	
		goto	setPinSevenToIn	
		goto	setPinEightToIn	
		goto	setPinNineToIn	
		goto	setPinTenToIn	
		goto	setPinElevenToIn	
		goto	setPinTwelveToIn	
		goto	setPinThirteenToIn	
; -------------------------------------------------------------------------
setPinToOut
		movlw	HIGH pinoutTab
		movwf	PCLATH
		movf	pin,w
		addwf	PCL,f
pinoutTab
		goto	setPinZeroToOut
		goto	setPinOneToOut	
		goto	setPinTwoToOut	
		goto	setPinThreeToOut	
		goto	setPinFourToOut	
		goto	setPinFiveToOut	
		goto	setPinSixToOut	
		goto	setPinSevenToOut	
		goto	setPinEightToOut	
		goto	setPinNineToOut	
		goto	setPinTenToOut	
		goto	setPinElevenToOut	
		goto	setPinTwelveToOut	
		goto	setPinThirteenToOut	
; -------------------------------------------------------------------------
setPinToPwm
		movlw	HIGH pinpwmTab
		movwf	PCLATH
		movf	pin,w
		addwf	PCL,f
pinpwmTab
		goto	setPinZeroToPWM
		goto	setPinOneToPWM	
		goto	setPinTwoToPWM	
		goto	setPinThreeToPWM	
		goto	setPinFourToPWM	
		goto	setPinFiveToPWM	
		goto	setPinSixToPWM	
		goto	setPinSevenToPWM	
		goto	setPinEightToPWM	
		goto	setPinNineToPWM	
		goto	setPinTenToPWM	
		goto	setPinElevenToPWM	
		goto	setPinTwelveToPWM	
		goto	setPinThirteenToPWM	
; -------------------------------------------------------------------------
analogRead ; analogRead sets analogDataHi and analogDataLo according to analogPin
		movlw	HIGH analogRdTab
		movwf	PCLATH
		movf	analogPin,w
		addwf	PCL,f
analogRdTab	
		goto	analogRead0
		goto	analogRead1
		goto	analogRead2
		goto	analogRead3
		goto	analogRead4
		goto	analogRead5
; -------------------------------------------------------------------------
doMultiByteCommand
;      switch(executeMultiByteCommand)
;      {
		movlw	HIGH mbctab
		movwf	PCLATH
		movlw	DISABLE_PWM ; 250
		subwf	executeMultiByteCommand,w ; remove offset of 250
		addwf	PCL,f
mbctab
		goto	mb_disablePwm ; 250
		goto	mb_enablePwm ; 251
		goto	mb_disableSoftPwm ; 252
		goto	mb_enableSoftPwm ; 253
		goto	mb_setSoftPwmFreq ; 254
; -------------------------------------------------------------------------
;
	END                       ; directive 'end of program'

