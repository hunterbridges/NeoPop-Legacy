//---------------------------------------------------------------------------
// NEOPOP : Emulator as in Dreamland
//
// Copyright (c) 2001-2002 by neopop_uk
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//	This program is free software; you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation; either version 2 of the License, or
//	(at your option) any later version. See also the license.txt file for
//	additional informations.
//---------------------------------------------------------------------------

/*
//---------------------------------------------------------------------------
//=========================================================================

	biosHLE.c

//=========================================================================
//---------------------------------------------------------------------------

  History of changes:
  ===================

20 JUL 2002 - neopop_uk
=======================================
- Cleaned and tidied up for the source release

21 JUL 2002 - neopop_uk
=======================================
- Omitted the failure message for VECT_FLASHWRITE until games like
	"Bust-a-Move" can use it properly.

22 JUL 2002 - neopop_uk
=======================================
- Re-added the failure message because the games that had problems are
	now fixed due to the modification of the "SWI" instruction.
- Fixed "Fatal Fury" by removing the failure message on VECT_COMCREATEDATA

23 JUL 2002 - neopop_uk
=======================================
- Added HLE support for VECT_SHUTDOWN.

26 JUL 2002 - neopop_uk
=======================================
- Added call to the new flash_write function.

28 JUL 2002 - neopop_uk
=======================================
- Added call decoding for debug mode.
- Added support for the new 'filter_comms' option.
- Added a user friendly message when system has shutdown.

01 AUG 2002 - neopop_uk
=======================================
- Added a crude, but effective way of making the shutdown
message appear only once.

15 AUG 2002 - neopop_uk
=======================================
- Added a warning if the clock gear is set. I'm not sure if any
	games use it...

28 AUG 2002 - neopop_uk
=======================================
- Implemented most of the comms. bios calls.

30 AUG 2002 - neopop_uk
=======================================
- Added comms. write interrupt/dma calls.  

//---------------------------------------------------------------------------
*/

#include "neopop.h"
#include "bios.h"
#include "TLCS900h_registers.h"
#include "TLCS900h_interpret.h"
#include "mem.h"
#include "flash.h"
#include "dma.h"
#include "interrupt.h"

//=============================================================================

/* This is the custom HLE instruction. I considered it was the fastest and
most streamlined way of intercepting a bios call. The operation performed
is dependant on the current program counter. */

void iBIOSHLE(void)
{
	//Only works within the bios
	if ((pc & 0xFF0000) != 0xFF0000)
		return;

	pc --;	//Compensate for processing this instruction.

	cycles = 8;		//TODO: Correct cycle counts (or approx?)

	switch (pc)
	{	
		//VECT_SHUTDOWN
	case 0xFF27A2:
#ifdef NEOPOP_DEBUG
		if (filter_bios)
		{
			_u32 a = pop32();
			if (a != 0xBAADC0DE)
				system_debug_message("VECT_SHUTDOWN: called before %06X", a);
			push32(a);
		}

		system_debug_stop();
#endif
		{
			//Cheap bit of code to stop the message appearing repeatedly.
			_u32 a = pop32();
			if (a != 0xBAADC0DE)
				system_message(system_get_string(IDS_POWER));
			push32(0xBAADC0DE); //Sure is!
		}

		return;	//Don't pop a return address, stay here

		//VECT_CLOCKGEARSET
	case 0xFF1030:
#ifdef NEOPOP_DEBUG
		if (filter_bios)
		{
			_u32 a = pop32();
			system_debug_message("VECT_CLOCKGEARSET: called before %06X", a);
			push32(a);
		}
#endif
		//TODO

	//	if (rCodeB(0x35) > 0)
	//		system_message("Unsupported clock gear %d set\nPlease inform the author", rCodeB(0x35));

		break;

		//VECT_RTCGET
	case 0xFF1440:
#ifdef NEOPOP_DEBUG
		if (filter_bios)
		{
			_u32 a = pop32();
			system_debug_message("VECT_RTCGET: called before %06X", a);
			push32(a);
		}
#endif
		
		update_rtc();	//Update hardware area

		if (rCodeL(0x3C) < 0xC000)
		{
			//Copy data from hardware area
			ram[rCodeL(0x3C) + 0] = ram[0x91];
			ram[rCodeL(0x3C) + 1] = ram[0x92];
			ram[rCodeL(0x3C) + 2] = ram[0x93];
			ram[rCodeL(0x3C) + 3] = ram[0x94];
			ram[rCodeL(0x3C) + 4] = ram[0x95];
			ram[rCodeL(0x3C) + 5] = ram[0x96];
			ram[rCodeL(0x3C) + 6] = ram[0x97];
		}

		break; 
	
		//?
	case 0xFF12B4:
		break;
	
		//VECT_INTLVSET
	case 0xFF1222:
#ifdef NEOPOP_DEBUG
		if (filter_bios)
		{
			_u32 a = pop32();
			system_debug_message("VECT_INTLVSET: called before %06X", a);
			push32(a);
		}
#endif

		{

	_u8 level = rCodeB(0x35); //RB3
	_u8 interrupt = rCodeB(0x34);	//RC3

	//   0 - Interrupt from RTC alarm
	//   1 - Interrupt from the Z80 CPU
	//   2 - Interrupt from the 8 bit timer 0
	//   3 - Interrupt from the 8 bit timer 1
	//   4 - Interrupt from the 8 bit timer 2
	//   5 - Interrupt from the 8 bit timer 3
	//   6 - End of transfer interrupt from DMA channel 0
	//   7 - End of transfer interrupt from DMA channel 1
	//   8 - End of transfer interrupt from DMA channel 2
	//   9 - End of transfer interrupt from DMA channel 3

	switch(interrupt)
	{
	case 0x00:	ram[0x70] = (ram[0x70] & 0xf0) |  (level & 0x07);		break;
	case 0x01:	ram[0x71] = (ram[0x71] & 0x0f) | ((level & 0x07)<<4);	break;
	case 0x02:	ram[0x73] = (ram[0x73] & 0xf0) |  (level & 0x07);		break;
	case 0x03:	ram[0x73] = (ram[0x73] & 0x0f) | ((level & 0x07)<<4);	break;
	case 0x04:	ram[0x74] = (ram[0x74] & 0xf0) |  (level & 0x07);		break;
	case 0x05:	ram[0x74] = (ram[0x74] & 0x0f) | ((level & 0x07)<<4);	break;
	case 0x06:	ram[0x79] = (ram[0x79] & 0xf0) |  (level & 0x07);		break;
	case 0x07:	ram[0x79] = (ram[0x79] & 0x0f) | ((level & 0x07)<<4);	break;
	case 0x08:	ram[0x7a] = (ram[0x7a] & 0xf0) |  (level & 0x07);		break;
	case 0x09:	ram[0x7a] = (ram[0x7a] & 0x0f) | ((level & 0x07)<<4);	break;
	}
		}
		break;	
	
		//VECT_SYSFONTSET
	case 0xFF8D8A:
#ifdef NEOPOP_DEBUG
		if (filter_bios)
		{
			_u32 a = pop32();
			system_debug_message("VECT_SYSFONTSET: called before %06X", a);
			push32(a);
		}
#endif
		{
			_u8 a,b,c, j;
			_u16 i, dst = 0xA000;

			b = rCodeB(0x30) >> 4;
			a = rCodeB(0x30) & 3;

			for (i = 0; i < 0x800; i++)
			{
				c = bios[0x8DCF + i];

				for (j = 0; j < 8; j++, c<<=1)
				{
					*(_u16*)(ram + dst) <<= 2;

					if (c & 0x80)	ram[dst] |= a;
					else			ram[dst] |= b;
				}

				dst += 2;
			}
		}
		
		break;
	
		//VECT_FLASHWRITE
	case 0xFF6FD8:
#ifdef NEOPOP_DEBUG
		if (filter_bios)
		{
			_u32 a = pop32();
			system_debug_message("VECT_FLASHWRITE: called before %06X", a);
			push32(a);
		}
#endif
		{
			_u32 i, bank = 0x200000;

			//Select HI rom?
			if (rCodeB(0x30) == 1)
				bank = 0x800000;

#ifdef NEOPOP_DEBUG
			if (filter_bios)
				system_debug_message("VECT_FLASHWRITE: Copy %06X -> %06X,  %d bytes",
					rCodeL(0x3C), rCodeL(0x38) + bank, rCodeW(0x34) * 256);
#endif
			
			memory_flash_error = FALSE;
			memory_unlock_flash_write = TRUE;
			//Copy as 32 bit values for speed
			for (i = 0; i < rCodeW(0x34) * 64ul; i++)
				storeL(rCodeL(0x38) + bank + (i * 4), loadL(rCodeL(0x3C) + (i * 4)));
			memory_unlock_flash_write = FALSE;

			if (memory_flash_error)
			{
#ifdef NEOPOP_DEBUG
				if (filter_bios)
					system_debug_message("VECT_FLASHWRITE: Error");
#endif
				rCodeB(0x30) = 0xFF;	//RA3 = SYS_FAILURE
			}
			else
			{
				_u32 address = rCodeL(0x38);
				if (rCodeB(0x30) == 1)
					address += 0x800000;
				else
					address += 0x200000;

				//Save this data to an external file
				flash_write(address, rCodeW(0x34) * 256);

				rCodeB(0x30) = 0;		//RA3 = SYS_SUCCESS
			}
		}

		break;
	
		//VECT_FLASHALLERS
	case 0xFF7042:
#ifdef NEOPOP_DEBUG
		if (filter_bios)
		{
			_u32 a = pop32();
			system_debug_message("VECT_FLASHALLERS: called before %06X", a);
			push32(a);
		}
#endif
		//TODO
		rCodeB(0x30) = 0;	//RA3 = SYS_SUCCESS
		break;
	
		//VECT_FLASHERS
	case 0xFF7082:
#ifdef NEOPOP_DEBUG
		if (filter_bios)
		{
			_u32 a = pop32();
			system_debug_message("VECT_FLASHERS: called before %06X", a);
			push32(a);
		}
#endif
#ifdef NEOPOP_DEBUG
		if (filter_bios)
			system_debug_message("VECT_FLASHERS: bank %d, block %d (?)", rCodeB(0x30), rCodeB(0x35));
#endif
		//TODO
		rCodeB(0x30) = 0;	//RA3 = SYS_SUCCESS
		break;
	
		//VECT_ALARMSET
	case 0xFF149B:
#ifdef NEOPOP_DEBUG
		if (filter_bios)
		{
			_u32 a = pop32();
			system_debug_message("VECT_ALARMSET: called before %06X", a);
			push32(a);
		}
#endif
		//TODO
		rCodeB(0x30) = 0;	//RA3 = SYS_SUCCESS
		break;
	
		//?
	case 0xFF1033: break;
	
		//VECT_ALARMDOWNSET
	case 0xFF1487:
#ifdef NEOPOP_DEBUG
		if (filter_bios)
		{
			_u32 a = pop32();
			system_debug_message("VECT_ALARMDOWNSET: called before %06X", a);
			push32(a);
		}
#endif
		//TODO
		rCodeB(0x30) = 0;	//RA3 = SYS_SUCCESS
		break;

		//?
	case 0xFF731F: break;
	
		//VECT_FLASHPROTECT
	case 0xFF70CA:
#ifdef NEOPOP_DEBUG
		if (filter_bios)
		{
			_u32 a = pop32();
			system_debug_message("VECT_FLASHPROTECT: called before %06X", a);
			push32(a);
		}
#endif
		//TODO
		rCodeB(0x30) = 0;	//RA3 = SYS_SUCCESS
		break;
	
		//VECT_GEMODESET
	case 0xFF17C4:
#ifdef NEOPOP_DEBUG
		if (filter_bios)
		{
			_u32 a = pop32();
			system_debug_message("VECT_GEMODESET: called before %06X", a);
			push32(a);
		}
#endif
		//TODO
		break;
	
		//?
	case 0xFF1032: break;
	
		// VECT_COMINIT
	case 0xFF2BBD:
		// DEFINITION: Defines  ports  necessary  for  serial  communication.
		//             Please  use  this  call  when  serial communication is needed.
		// INPUT VARIABLES : None
		// OUTPUT VARIABLES : None
		// RETURN VALUES : None
		// CONSTANTS : None
#ifdef NEOPOP_DEBUG
		if (filter_comms)
		{
			_u32 a = pop32();
			system_debug_message("VECT_COMINIT: called before %06X", a);
			push32(a);
		}
#endif
		// Nothing to do.
		rCodeB(0x30) = 0;	//RA3 = COM_BUF_OK
		system_comms_log(pc, 0, NULL);
		break;
	
		//VECT_COMSENDSTART
	case 0xFF2C0C:
		// DEFINITION: This is the BIOS to commence transmission of data created in the internal buffer.
		//             Please use this call once after the data is created in the buffer.
		// INPUT VARIABLES : None
		// OUTPUT VARIABLES : None
		// RETURN VALUES : None
		// CONSTANTS : None
#ifdef NEOPOP_DEBUG
		if (filter_comms)
		{
			_u32 a = pop32();
			system_debug_message("VECT_COMSENDSTART: called before %06X", a);
			push32(a);
		}
#endif
		// Nothing to do.
		system_comms_log(pc, 0, NULL);
		break;
	
		//VECT_COMRECIVESTART
	case 0xFF2C44:
		// DEFINITION: This is the BIOS to obtain permission to commence reception.
		//             Please use this call once to allow reception.
		// INPUT VARIABLES: None
		// OUTPUT VARIABLES: None
		// RETURN VALUES: None
		// CONTSTANTS: None
#ifdef NEOPOP_DEBUG
		if (filter_comms)
		{
			_u32 a = pop32();
			system_debug_message("VECT_COMRECIVESTART: called before %06X", a);
			push32(a);
		}
#endif
		// Nothing to do.
		system_comms_log(pc, 0, NULL);
		break;
	
		//VECT_COMCREATEDATA
	case 0xFF2C86:
		// DEFINITION: Creates transmission data in the internal (system) buffer.
		//             The buffer is a 64 byte loopbuffer.
		//             Please use this BIOS before VECT_COMSENDSTART is called.
		// INPUT VARIABLES:
		//             register rb3 : 1 byte transmission data
		// OUTPUT VARIABLES: None
		// RETURN VALUES:
		//             register ra3 : Transmission buffer status flag
		// CONSTANTS:
		//             COM_BUF_OVER : Transmission buffer over
		//             COM_BUF_OK : Transmission buffer normal
#ifdef NEOPOP_DEBUG
		if (filter_comms)
		{
			_u32 a = pop32();
			system_debug_message("VECT_COMCREATEDATA: called before %06X", a);
			push32(a);
		}
#endif
		{
			//Write the byte
			_u8 data = rCodeB(0x35);
			system_comms_log(pc, 1, &data);
			system_comms_write(data);
		}


		//Restore $PC after BIOS-HLE instruction
		pc = pop32();
		
		interrupt(11); //Comms. Write interrupt
		if (ram[0x007C] == 0x18)		DMA_update(0);
		else { if (ram[0x007D] == 0x18)	DMA_update(1);
		else { if (ram[0x007E] == 0x18)	DMA_update(2);
		else { if (ram[0x007F] == 0x18)	DMA_update(3);	}}}

		//Always COM_BUF_OK because the write call always succeeds.
		rCodeB(0x30) = 0x0;			//RA3 = COM_BUF_OK
		return;
	
		//VECT_COMGETDATA
	case 0xFF2CB4:
		// DEFINITION: 1 byte of data received in the internal (system) buffer is obtained.
		// INPUT VARIABLES: None
		// OUTPUT VARIABLES:
		//             register rb3 : 1 byte data received
		// RETURN VALUES:
		//             register ra3 : Reception buffer status flag
		// CONSTANTS:
		//             COM_BUF_EMPTY : Reception buffer empty
		//             COM_BUF_OK : Reception buffer normal
#ifdef NEOPOP_DEBUG
		if (filter_comms)
		{
			_u32 a = pop32();
			system_debug_message("VECT_COMGETDATA: called before %06X", a);
			push32(a);
		}
#endif
		{
			_u8 data;


			if (system_comms_read(&data))
			{
				system_comms_log(pc, 1, &data);
				rCodeB(0x30) = 0;	//COM_BUF_OK
				rCodeB(0x35) = data;

				pc = pop32();
			
				if (statusIFF() <= (ram[0x77] & 7))
				{
					//Comms. Read interrupt
					ram[0x50] = data;
					interrupt(12); 
					if (ram[0x007C] == 0x19)		DMA_update(0);
					else { if (ram[0x007D] == 0x19)	DMA_update(1);
					else { if (ram[0x007E] == 0x19)	DMA_update(2);
					else { if (ram[0x007F] == 0x19)	DMA_update(3);	}}}
				}

				return;
			}
			else
			{
				system_comms_log(pc, 0, NULL);
				rCodeB(0x30) = 1;	//COM_BUF_EMPTY
			}
		}

		break;
	
		//VECT_COMONRTS
	case 0xFF2D27:
	    // DEFINITION: RTS signal is set to low to allow transmission from others units.
		// INPUT VARIABLES: None
		// OUTPUT VARIABLES: None
		// RETURN VALUES: None
		// CONSTANTS:None
#ifdef NEOPOP_DEBUG
		if (filter_comms)
		{
			_u32 a = pop32();
			system_debug_message("VECT_COMONRTS: called before %06X", a);
			push32(a);
		}
#endif
		ram[0xB2] = 0;
		system_comms_log(pc, 0, NULL);
		break;
	
		//VECT_COMOFFRTS
	case 0xFF2D33: 	
	    // DEFINITION: RTS signal is set to high to prohibit transmission from other units.
		// INPUT VARIABLES: None
		// OUTPUT VARIABLES: None
		// RETURN VALUES: None
		// CONSTANTS:None
#ifdef NEOPOP_DEBUG
		if (filter_comms)
		{
			_u32 a = pop32();
			system_debug_message("VECT_COMOFFRTS: called before %06X", a);
			push32(a);
		}
#endif
		ram[0xB2] = 1;
		system_comms_log(pc, 0, NULL);
		break;	
	
		//VECT_COMSENDSTATUS
	case 0xFF2D3A:
		// DEFINITION: Buffer  over  flag  and  data  count  in  the  internal  buffer  is  obtained.
		//             Reserved ERROR status bits do not have set values.
		// INPUT VARIABLES: None
		// OUTPUT VARIABLES:None
		// RETURN VALUES:
		//             register rwa3 : ERROR status bits & transmission buffer count
		// CONSTANTS:
		//             COM_BUFOVERERROR : Buffer over flag
		//
		// (16-bit mask)
		//			   bits 0 - 7 : transmission buffer count
		//             bit 8 : transmission buffer over flag
		//             bits 9 - 15 : reserved
#ifdef NEOPOP_DEBUG
		if (filter_comms)
		{
			_u32 a = pop32();
			system_debug_message("VECT_COMSENDSTATUS: called before %06X", a);
			push32(a);
		}
#endif
		// Nothing to do.
		rCodeW(0x30) = 0;	//Send Buffer Count, never any pending data!
		system_comms_log(pc, 0, NULL);
		break;
	
		//VECT_COMRECIVESTATUS
	case 0xFF2D4E:
		// DEFINITION: Transmission errors which have occurred up till now and the data count in the buffer
		//             is obtained.  After this BIOS is called, communication error flag is cleared.
		//             Reserved area ERROR status bits are 0.
		// INPUT VARIABLES: None
		// OUTPUT VARIABLES: None
		// RETURN VALUES:
		//             register rwa3 : ERROR status bits & reception buffer count
		// CONSTANTS:
		//             COM_BUFOVERERROR : Buffer over flag
		//             COM_FLAMEERROR : Framing error flag
		//             COM_PARITYERROR : Parity error flag
	    //             COM_OVERRUNERROR : Over run error flag
		//
		// (16-bit mask)
		//			   bits 0 - 7 : reception buffer count
		//             bit 8 : reception buffer over flag
		//             bit 9 : framing error flag
		//             bit 10 : parity error flag
		//             bit 11 : overrun error flag
		//             bits 12 - 15 : reserved
#ifdef NEOPOP_DEBUG
		if (filter_comms)
		{
			_u32 a = pop32();
			system_debug_message("VECT_COMRECIVESTATUS: called before %06X", a);
			push32(a);
		}
#endif

		// Receive Buffer Count
		rCodeW(0x30) = system_comms_read(NULL);
		system_comms_log(pc, 0, NULL);
		break;
	
		// VECT_COMCREATEBUFDATA
	case 0xFF2D6C:
		// DEFINITION: Address and size of the transmission data stored in user buffer is defined and stored in
		//             the system buffer. This is a "block transfer" version of VECT_COMCREATEDATA.
		//             It is quicker than using a loop with VECT_COMCREATEDATA.
		// INPUT VARIABLES:
		//             register xhl3 : Transmission data buffer address
		//             register rb3 : Transmission data buffer size
		// OUTPUT VARIABLES:
		//             register xhl3 : When the buffer works normally, points to the next data address.
		//                           : If data has been left from previous transmission, points to the data.
		//             register rb3 : Number of data left
		// RETURN VALUES: None
	    // CONSTANTS:
	    //             COM_BUF_OK : Buffer transfer normal (no transfer left over)
#ifdef NEOPOP_DEBUG
		if (filter_comms)
		{
			_u32 a = pop32();
			system_debug_message("VECT_COMCREATEBUFDATA: called before %06X", a);
			push32(a);
		}
#endif
		pc = pop32();

		system_comms_log(pc, 0, NULL);
		while(rCodeB(0x35) > 0)
		{
			_u8 data;
			data = loadB(rCodeL(0x3C));

			system_comms_log(0, 1, &data);

			//Write data from (XHL3++)
			system_comms_write(data);
			rCodeL(0x3C)++; //Next data

			rCodeB(0x35)--;	//RB3 = Count Left
		}

		interrupt(11); //Comms. Write interrupt
		if (ram[0x007C] == 0x18)		DMA_update(0);
		else { if (ram[0x007D] == 0x18)	DMA_update(1);
		else { if (ram[0x007E] == 0x18)	DMA_update(2);
		else { if (ram[0x007F] == 0x18)	DMA_update(3);	}}}

		return;
	
		//VECT_COMGETBUFDATA
	case 0xFF2D85:
		// DEFINITION: Address and size of the reception data wanted in the user buffer is defined and
		//             obtained from the system buffer. This is a "block transfer" version of
		//             VECT_COMGETDATA. It is quicker than using a loop with VECT_COMGETDATA.
		// INPUT VARIABLES:
		//             register xhl3 : Reception data buffer address
		//             register rb3 : Reception data buffer size
		// OUTPUT VARIABLES:
		//             register xhl3 : Points to the address where the next data will be stored.
		//             register rb3 : Number of data left
		// RETURN VALUES:None
		// CONSTANTS:
		//             COM_BUF_OK : Buffer obtained normally (no reception left over)
#ifdef NEOPOP_DEBUG
		if (filter_comms)
		{
			_u32 a = pop32();
			system_debug_message("VECT_COMGETBUFDATA: called before %06X", a);
			push32(a);
		}
#endif
		{
			BOOL readanything = FALSE;

			pc = pop32();

			system_comms_log(pc, 0, NULL);
			while(rCodeB(0x35) > 0)
			{
				_u8 data;

				if (system_comms_read(&data))
				{
					system_comms_log(0, 1, &data);
					//Read data into (XHL3++)
					storeB(rCodeL(0x3C), data);
					rCodeL(0x3C)++; //Next data
					rCodeB(0x35)--;	//RB3 = Count Left

					if (statusIFF() < (ram[0x77] & 7))
					{
						//Comms. Read interrupt
						ram[0x50] = data;
						interrupt(12);
						if (ram[0x007C] == 0x19)		DMA_update(0);
						else { if (ram[0x007D] == 0x19)	DMA_update(1);
						else { if (ram[0x007E] == 0x19)	DMA_update(2);
						else { if (ram[0x007F] == 0x19)	DMA_update(3);	}}}
						return;
					}
				}
				else
					break;
			}

		}

		return;
	}

	//RET
	pc = pop32();
}

//=============================================================================
