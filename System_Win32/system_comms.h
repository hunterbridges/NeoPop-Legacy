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

	system_comms.h

//=========================================================================
//---------------------------------------------------------------------------

  History of changes:
  ===================

28 AUG 2002 - neopop_uk
=======================================
- Created this file to handle inter-NeoPop link-up communications.

//---------------------------------------------------------------------------
*/

#ifndef __SYSTEM_COMMS__
#define __SYSTEM_COMMS__
//=============================================================================

typedef enum {
	VECT_COMINIT = 0xFF2BBD,
	VECT_COMSENDSTART = 0xFF2C0C,
	VECT_COMRECIVESTART = 0xFF2C44,
	VECT_COMCREATEDATA = 0xFF2C86,
	VECT_COMGETDATA = 0xFF2CB4,
	VECT_COMONRTS = 0xFF2D27,
	VECT_COMOFFRTS = 0xFF2D33,
	VECT_COMSENDSTATUS = 0xFF2D3A,
	VECT_COMRECIVESTATUS = 0xFF2D4E,
	VECT_COMCREATEBUFDATA = 0xFF2D6C,
	VECT_COMGETBUFDATA = 0xFF2D85
} CommsBIOSCommand;

inline const char *system_comms_get_bios_str(CommsBIOSCommand category)
{
	switch (category)
	{
	case VECT_COMINIT: return "VECT_COMINIT";
	case VECT_COMSENDSTART: return "VECT_COMSENDSTART";
	case VECT_COMRECIVESTART: return "VECT_COMRECIVESTART";
	case VECT_COMCREATEDATA: return "VECT_COMCREATEDATA";
	case VECT_COMGETDATA: return "VECT_COMGETDATA";
	case VECT_COMONRTS: return "VECT_COMONRTS";
	case VECT_COMOFFRTS: return "VECT_COMOFFRTS";
	case VECT_COMSENDSTATUS: return "VECT_COMSENDSTATUS";
	case VECT_COMRECIVESTATUS: return "VECT_COMRECIVESTATUS";
	case VECT_COMCREATEBUFDATA: return "VECT_COMCREATEBUFDATA";
	case VECT_COMGETBUFDATA: return "VECT_COMGETBUFDATA";
	}

	return "VECT_UNKNOWN";
}

void system_comms_connect_dialog(void);
void system_comms_shutdown(void);

void system_comms_pause(BOOL remote_enable);

void system_comms_pause_check(void);

//=============================================================================
#endif
