/*
             LUFA Library
     Copyright (C) Dean Camera, 2015.

  dean [at] fourwalledcubicle [dot] com
           www.lufa-lib.org
*/

/*
  Copyright 2015  Dean Camera (dean [at] fourwalledcubicle [dot] com)

  Permission to use, copy, modify, distribute, and sell this
  software and its documentation for any purpose is hereby granted
  without fee, provided that the above copyright notice appear in
  all copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaims all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

/** \file
 *
 *  Main source file for the HAP project. This file contains the main tasks and
 *  is responsible for the initial application hardware configuration.
 */

#include "hap.h"
#include <LUFA/Drivers/Peripheral/Serial.h>
#include <LUFA/Drivers/Misc/RingBuffer.h>

static RingBuffer_t g_ring_buffer;
static uint8_t g_ring_buffer_data[sizeof(USB_Report_Data_t) * 8];

int main(void)
{
	SetupHardware();
	GlobalInterruptEnable();
	
	RingBuffer_InitBuffer(&g_ring_buffer, g_ring_buffer_data, sizeof(g_ring_buffer_data));
	
	for (;;)
	{
		HID_Task();
		USB_USBTask();
	}
}

void SetupHardware(void)
{
	/* Disable watchdog if enabled by bootloader/fuses */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	/* Disable clock division */
	clock_prescale_set(clock_div_1);

	/* Hardware Initialization */
	Serial_Init(115200, true);
	UCSR1B |= 1 << RXCIE1; // Enable serial receive interrupt
	USB_Init();
}

/** Event handler for the USB_Connect event. This indicates that the device is enumerating via the status LEDs and
 *  starts the library USB task to begin the enumeration and USB management process.
 */
void EVENT_USB_Device_Connect(void)
{
	/* USB enumerating */
}

/** Event handler for the USB_Disconnect event. This indicates that the device is no longer connected to a host via
 *  the status LEDs and stops the USB management and joystick reporting tasks.
 */
void EVENT_USB_Device_Disconnect(void)
{
	/* USB not ready */
}

/** Event handler for the USB_ConfigurationChanged event. This is fired when the host set the current configuration
 *  of the USB device after enumeration - the device endpoints are configured and the joystick reporting task started.
 */
void EVENT_USB_Device_ConfigurationChanged(void)
{
	Endpoint_ConfigureEndpoint(HAP_EPADDR, EP_TYPE_INTERRUPT, HAP_EPSIZE, 1);
}

void EVENT_USB_Device_ControlRequest(void)
{
	/* Handle HID Class specific requests */
	switch (USB_ControlRequest.bRequest)
	{
		case HID_REQ_GetReport:
			if (USB_ControlRequest.bmRequestType == (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE))
			{
				USB_Report_Data_t report_data;
				GetNextReport(&report_data);

				Endpoint_ClearSETUP();
				Endpoint_Write_Control_Stream_LE(&report_data, sizeof(report_data));
				Endpoint_ClearOUT();
			}
			break;
	}
}

bool GetNextReport(USB_Report_Data_t* const report_data)
{
	static uint8_t prev_stick = 0;
	static uint8_t prev_button = 0;
	bool changed = false;

	memset(report_data, 0, sizeof(USB_Report_Data_t));
	
	uint16_t read_size = RingBuffer_GetCount(&g_ring_buffer);
	if (read_size >= sizeof(USB_Report_Data_t)) {
		report_data->stick = RingBuffer_Remove(&g_ring_buffer);
		report_data->button = RingBuffer_Remove(&g_ring_buffer);
	} else { // (read_size < sizeof(USB_Report_Data_t))
		report_data->stick = prev_stick;
		report_data->button = prev_button;
	}

	changed = (uint8_t)(prev_stick ^ report_data->stick) | (uint8_t)(prev_button ^ report_data->button);

	prev_stick = report_data->stick;
	prev_button = report_data->button;

	return changed;
}

void HID_Task(void)
{
	if (USB_DeviceState != DEVICE_STATE_Configured)
	  return;

	Endpoint_SelectEndpoint(HAP_EPADDR);

	if (Endpoint_IsINReady())
	{
		USB_Report_Data_t report_data;
		GetNextReport(&report_data);
		Endpoint_Write_Stream_LE(&report_data, sizeof(report_data), NULL);
		Endpoint_ClearIN();
		memset(&report_data, 0, sizeof(report_data));
	}
}

/* 
 *  Interrupt Service Routines
 */
ISR(USART1_RX_vect, ISR_BLOCK)
{
    uint8_t receivedByte = UDR1;

    if (USB_DeviceState == DEVICE_STATE_Configured)
		RingBuffer_Insert(&g_ring_buffer, receivedByte);
}
