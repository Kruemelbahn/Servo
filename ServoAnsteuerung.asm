	list p=PIC12F675
;**************************************************************
;*  	Pinbelegung
;*	-------------------------------------------------------	
;*	GPIO: 	0 In  AN0, Endlage links / grün
;*		1 In  AN1, Endlage rechts / rot
;*		2 In  Taster grün
;*		3 In  Taster rot
;*		4 Out LED: + = grün, - = rot
;*		5 Out Impulsausgang für Servomotor
;*	
;**************************************************************
;
; Hauptdatei = ServoAnsteuerung.ASM
;
; M.Zimmermann 31.03.2011
;
; ServoAnsteuerung : einfache Ansteuerelektronik für Servomotoren
;
; Prozessor PIC 12F675
;
; Prozessor-Takt intern (~4MHz)
;
; zu diesem Project gehört auch die Datei:
;    ServoAnsteuerungCore.inc
;
; das HEX-File kann erstellt werden durch: build.bat
;
;**************************************************************
;
; Signalform für Servo:
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
; Pulsabstand 15..30ms möglich
;
; Testservo: Dymond D90eco
;            11.8 * 22.8 * 20.6 mm, Gewicht 9g
;            Stellkraft 15NCm
;            Stellzeit  0.09sec/45°
;            Anschluß: GND | +5V | Imp.
;                       sw | rt  | ws
;
; Programmablauf:
; - Interrupt alle 20ms über Timer:
; 	~ setze Ausgang auf High
;	~ warte 1ms (Grundbreite)
;	~ Wartezeit 0..1ms je nach Potistellung
;		Potistellung 0..5V = 0..1023 ./. 4 = 0..255
;	~ setze Ausgang auf Low
;
;**************************************************************
; History:
;	1 Abfrage beider Analogkanäle
;	2 Abfrage des benötigten Analogkanals
;	3 Speichern der letzten Stellung im EEPROM, Rampe
;	4 kleinerer Rampensummand, Impulszeit 0,5..2,5ms
;
;**************************************************************
; *  Copyright (c) 2018 Michael Zimmermann <http://www.kruemelsoft.privat.t-online.de>
; *  All rights reserved.
; *
; *  LICENSE
; *  -------
; *  This program is free software: you can redistribute it and/or modify
; *  it under the terms of the GNU General Public License as published by
; *  the Free Software Foundation, either version 3 of the License, or
; *  (at your option) any later version.
; *  
; *  This program is distributed in the hope that it will be useful,
; *  but WITHOUT ANY WARRANTY; without even the implied warranty of
; *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; *  GNU General Public License for more details.
; *  
; *  You should have received a copy of the GNU General Public License
; *  along with this program. If not, see <http://www.gnu.org/licenses/>.
; *
;**************************************************************
;
; sinnvolle Wertepaare:	INC_DEC	INC_20
;	Signal		5	1
;	Schranke	1	1
;

#define VERSION_NO	.4

#define LONG_PULS
#define INC_DEC		.1	; Rampensummand: je größer desto schneller
#define	INC_20		.1	; summand jede ...te 20ms-Schleife
				;		 je größer desto langsamer

#include ServoAnsteuerungCore.inc

; end of file
