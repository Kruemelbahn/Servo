;**************************************************************
;*  	Pinbelegung
;*	-------------------------------------------------------	
;*	GPIO: 	0 In  AN0, Endlage links / gr�n
;*		1 In  AN1, Endlage rechts / rot
;*		2 In  Taster gr�n
;*		3 In  Taster rot
;*		4 Out LED: + = gr�n, - = rot
;*		5 Out Impulsausgang f�r Servomotor
;*	
;**************************************************************
;
; M.Zimmermann 31.03.2011
;
;**************************************************************
;
; Signalform f�r Servo:
;
;  +--+          +--+
;  |  |          |  |
;  |  |          |  |
; -+  +----------+  +-
;  |<-- ~20ms -->|
;  |  |
;   ^
;   |
;   +-- Breite = 1..2ms
;                1 ms  = links
;                1,5ms = mitte
;                2 ms  = rechts
;
; Programmablauf:
; - Interrupt alle 20ms �ber Timer:
; 	~ setze Ausgang auf High
;	~ warte 1ms (Grundbreite)
;	~ Wartezeit 0..1ms je nach Potistellung
;		Potistellung 0..5V = 0..1023 ./. 4 = 0..255
;	~ setze Ausgang auf Low
;
;**************************************************************

; Includedatei f�r den 12F675 einbinden

	#include <P12F675.INC>

	ERRORLEVEL      -302   	;SUPPRESS BANK SELECTION MESSAGES

; Configuration festlegen: 
; Power up Timer, kein Watchdog, int-Oscillator, kein Brown out 
	__CONFIG	_MCLRE_OFF & _PWRTE_OFF & _WDT_OFF & _INTRC_OSC_NOCLKOUT & _BODEN_OFF


;#define OSC_CALIB

#define LOOP_CNT	.5	; 20ms/4,096ms
#define LOOP_LED	.25	; 500ms/20ms

;**************************************************************

#define END_li		GPIO,0	; Input
#define END_re		GPIO,1	; Input
#define TA_li		GPIO,2	; Input
#define	TA_re		GPIO,3	; Input
#define LED		GPIO,4	; Output 0 = rechts, 1 = links
#define SERVO		GPIO,5	; Output

;**************************************************************
; Variablen
;**************************************************************
; EEPROM
#define eeprom_start	2100h

		org	eeprom_start
sw_kennung:	de	"MZ", .10
version:	de	VERSION_NO

incdec:		de	INC_DEC		; Geschwindigkeitskennungen
inc20:		de	INC_20		;  um die Software
#ifdef LONG_PULS			;  identifizieren zu k�nnen
		de	.1
#else
		de	.0
#endif		
modus:		de	.0		; Einstellung verschiedener
					; Verhaltensmodi

eflags:		de	0

;**************************************************************
	CBLOCK		H'20'
W_save		: 1	; ISR-Zwischenspeicher
Status_save	: 1	; ISR-Zwischenspeicher
counter		: 1	; ISR-Z�hler
counter_l	: 1	; ISR-Z�hler f�r LED-Blinken

loops		: 1	; interner Z�hler f�r wait
loop_adc	: 1	; interner Z�hler f�r wait adc

flags		: 1	; Zustand der Tasten
cflags		: 1	; Zustand der Tasten (Kopie f�r EEPROM)
ziel_pos	: 1	; Position Poti links
ist_pos		: 1	; Position Poti rechts

inc__dec	: 1	; Rampensummand
inc__20		: 1	; Rampensummand jede ...te 20ms
inc__20fix	: 1	; Rampensummand jede ...te 20ms
ba_modus	:	; Betriebsmodus
	ENDC

#define	Move_L		flags,0
#define	Move_R		flags,1
#define	LED_flg 	flags,2

;##############################################################

	ORG	0x0000
	; Oszillatorkalibrierung:
	CALL	0x3FF
	banksel	OSCCAL
	MOVWF	OSCCAL
	GOTO	Init

;**************************************************************
        org     0x04 		; InterruptServiceVector 
        movwf   W_save         	; save W 
        swapf   STATUS,W 
        bcf     STATUS, RP0	; Bank 0 
        movwf   Status_save 

;--------------------------------------------------------------
        decfsz  counter, F 	; 20ms vorbei? 
        goto    end_isr		; nein, noch nicht 
				; ja, neue Impulsfolge ausgeben

	btfsc	Move_L
	goto	servo_ok
	btfsc	Move_R
	goto	servo_ok
				; keine Richtung gesetzt
				; LED ein/aus
        decfsz  counter_l, F 	; 0,5s vorbei? 
	goto	load_cnt

	movlw	LOOP_LED
	movwf	counter_l

	btfsc	LED_flg
	goto	led_inv
	bsf	LED
	bsf	LED_flg
	goto	load_cnt
	
led_inv:bcf	LED
	bcf	LED_flg
	goto	load_cnt
		
servo_ok:			; Servo ansteuern
	bsf	SERVO		; Impuls ein
	call	WAIT_minimum	; Grundtaktbreite 1ms
	movfw	ziel_pos

				; Position �ber Rampe anfahren
	decfsz  inc__20, F
	  goto	servoistpos

	movfw	inc__20fix
	movwf	inc__20
	movfw	ziel_pos

	subwf	ist_pos,0
	skpnz
	  goto	reached
				; Position noch nicht erreicht
	movfw	inc__dec
	skpnc
	  goto	decrement
				; C=1 -> Wert erh�hen
	addwf	ist_pos,0
	skpnc			; mehr als 255 geht nicht
	  goto	reached
	movwf	ist_pos
	subwf	ziel_pos,0
	skpnz
	  goto	reached
	skpc
	  goto	reached
	movfw	ist_pos
	goto	servotime

decrement:
	subwf	ist_pos,0
	skpnz
	  goto	reached		; weniger als 0 geht nicht
	skpc
	  goto	reached		; weniger als 0 geht nicht
	movwf	ist_pos
	subwf	ziel_pos,0
	skpnz
	  goto	reached		; weniger als 0 geht nicht
	skpnc
	  goto	reached		; weniger als 0 geht nicht
servoistpos:
	movfw	ist_pos
	goto	servotime

reached:
	movfw	ziel_pos
	movwf	ist_pos		; ziel_pos -> ist_pos

servotime:
	call	Wait		; Restzeit hinzu
	bcf	SERVO

load_cnt:
	movlw	LOOP_CNT
	movwf	counter		; Counter laden (5/244Hz=0,02s)
;--------------------------------------------------------------
end_isr:
	bcf     INTCON, T0IF 

        ; End ISR, restore context and return to the main program 
        swapf   Status_save, w 
        movwf   STATUS 
        swapf   W_save,f	; restore W without corrupting STATUS 
        swapf   W_save,w 
        retfie 

;**************************************************************
Init:
	; Initialisierung des Timer0 
        ; TIMER0 mu� eingestellt sein!  
        ; 16:1 bei 4 MHz -> 62,55 kHz 
        ; �berlauf nach 4,096 ms 
        ; 244 int in 1 Sekunde
	; Vorteiler 16:1 am Timer0, interner Takt/4
	; GPPU setzen, kein PullUp's
	movlw	B'11010011'
	movwf	OPTION_REG

	; Analogeing�nge vorbereiten:
	movlw	B'00010011'	; ANS0, ANS1, Fosc /8
	movwf	ANSEL

	; Portinitialisierung:
	MOVLW	B'00001111'	; input = 1, output = 0
	MOVWF	TRISIO

	banksel	GPIO
	CLRF	GPIO

	movlw	B'00000001'
	movwf	ADCON0

	movlw	LOOP_CNT
	movwf	counter		; Counter laden (5/244Hz=0,02s)
	movlw	LOOP_LED
	movwf	counter_l

	MOVLW	B'00000111'	; CM0:CM2 setzen (Comparator off)
	MOVWF	CMCON

        movlw   incdec-eeprom_start
       	call 	read_eeprom
	iorlw	.0		; setzt ZeroFlag
	skpnz
	  addlw	.1		; W darf nicht 0 sein!
	movwf	inc__dec

        movlw   inc20-eeprom_start
       	call 	read_eeprom
	iorlw	.0		; setzt ZeroFlag
	skpnz
	  addlw	.1		; W darf nicht 0 sein!
	movwf	inc__20
	movwf	inc__20fix

        movlw   modus-eeprom_start
       	call 	read_eeprom
	movwf	ba_modus

        movlw   eflags-eeprom_start
       	call 	read_eeprom
	movwf	flags
	btfsc	Move_L
	bsf	LED

	movlw	.0
	movwf	ziel_pos	; ziel_pos == 0
	call	do_adc		; in W steht ziel_pos
	movwf	ist_pos		; ist_pos = ziel_pos

        ; Interrupt freigeben nach der ersten ADC-Wandlung
	movlw	B'10100000'  	; INT-Flag l�schen 
        movwf	INTCON		; Timer0-Int ein, INT aktiviert 

;**************************************************************
; Hauptschleife

main:	; Tasten pr�fen:
	btfsc	TA_li
	goto	test_r
	bsf	Move_L		; Links!
	bcf	Move_R
	bsf	LED
	call	writeflags

test_r:	btfsc	TA_re
	goto	rd_adc
	bsf	Move_R		; Rechts!
	bcf	Move_L
	bcf	LED
	call	writeflags

rd_adc:	call	do_adc
	goto	main

;**************************************************************
; Analogwert �ber ADC-Wandler einlesen
; Ergebnis (1..255) steht in 'ziel_pos'
do_adc:
	btfsc	Move_L
	goto	readL
	btfss	Move_R
	return
				
	bsf	ADCON0,2	; Rechts...
	goto	strt_adc

readL:	bcf	ADCON0,2	; Links...

strt_adc:
	bsf	ADCON0,1	; Wandlung starten
adc_loop:
	btfsc	ADCON0,1
	goto	adc_loop	; warten bis fertig

        movlw   .5		; Zeitkonstante f�r 20�s 
	movwf   loop_adc 
Wai2:   nop  
        decfsz  loop_adc, F 	; Zeit vorbei? 
        goto    Wai2		; nein, noch nicht 

	movfw	ADRESH
	iorlw	.0		; setzt ZeroFlag
	skpnz
	  addlw	.1		; W darf nicht 0 sein!
				; sonst wird die Impulsbreite
				; falsch!
	movwf	ziel_pos
	return

;**************************************************************
; Warteschleife 1 ms f�r einen 4MHz-PIC-Takt (1Takt = 1�s) 
; 6 * NOP = 9 Takte * 110 + 4 =  994 Takte(�s)
; 2 * NOP = 5 Takte * 199 + 4 =  999 Takte(�s)
;                     255     = 1254 Takte(�s)
; 1 * NOP = 4 Takte * 249 + 4 = 1000 Takte(�s)
WAIT_minimum:
#ifdef LONG_PULS
        movlw   .125		; Zeitkonstante f�r 0.5ms 
#else
        movlw   .249		; Zeitkonstante f�r 1ms 
#endif
Wait:	movwf   loops 

Wait_l:	nop  
#ifdef LONG_PULS
        nop       		; jedes nop verl�ngert die Zeit
        nop        		; um ca. 0,255 ms, mit insgesamt 
        nop        		; 5 nop's wird also der Analogwert 1..255
        nop  			; auf eine Zeit von 0..2,044ms umgesetzt
#endif
        decfsz  loops, F 	; Zeit vorbei? 
        goto    Wait_l		; nein, noch nicht 
	return

;**************************************************************
writeflags:
	movfw	flags
	subwf	cflags,0
	skpnz
	  return

	; Status�nderung: im EEPROM merken
	; f�r's n�chste Power-On!
	movfw	flags
	movwf	cflags
	call	write_eeprom_data

        movlw   eflags-eeprom_start
       	call 	write_eeprom
	return

;**************************************************************
; liest ein Zeichen aus dem EEPROM-Bereich
; in W wird die EEPROM-Adresse (0..) �bergeben,
; das gelesene Zeichen ist wieder in W
read_eeprom:
	
		banksel	EEADR
		movwf   EEADR
		bsf     EECON1,RD
		movf    EEDATA,w
		banksel	GPIO
        
		return
 
;**************************************************************
; schreibt ein Zeichen in den EEPROM-Bereich
; in W wird die EEPROM-Adresse (0..) �bergeben.
;
; der zu schreibende Wert muss zuvor in EEDATA geschrieben worden sein
; -> call write_eeprom_data
;
write_eeprom_data:
		banksel	EEDATA
		movwf	EEDATA		; Wert f�r EEPROM
        	banksel	GPIO
		return
write_eeprom:
		bcf	PIR1,EEIF
		banksel EEADR
		movwf	EEADR		; Adresse 
		bsf 	EECON1, WREN	; Enable write
		bcf 	INTCON, GIE 	; Disable INTs.

		; DON'T change the following 5 lines!
		movlw 	0x55 		;
		movwf 	EECON2 		; Write 55h
		movlw 	0xAA 		;
		movwf 	EECON2 		; Write AAh
		bsf 	EECON1,WR	; Set WR bit, begin write

        	banksel	GPIO
write_loop:	btfss	PIR1,EEIF
		goto	write_loop	; wait until write is finished
		banksel	EECON1

		bcf 	EECON1, WREN	; Disable write
        	banksel	GPIO
		bsf 	INTCON, GIE 	; Enable INTs.
	        return

;**************************************************************
; Kalibrierwert f�r Oszillator
#ifdef	OSC_CALIB
	ORG	0x03FF
	RETLW	0x80		; center frequency
#endif

	end
; end of file
