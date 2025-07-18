/*
 * Title: ClearCoreCommandProtocol
 *
 * Objective:
 *    This example demonstrates control of various functionality of the Teknic 
 *     ClearCoreI/O and motion controller, including controlling ClearPath-SD 
 *     motors instep and direction mode.
 *
 * Description:
 *    This example processes strings of characters formatted according to the
 *     specifications below and commands the corresponding action on a ClearCore device. 
 *    This example is designed to be highly configurable to meet the requirements
 *     of a variety of applications. The protocol accepts input and sends output
 *     via USB connection by default but can be configured to accept commands from
 *     other streams - such as ClearCore's COM ports, ethernet port, or XBee
 *     connection - and sources - such as manual user input, input from a text
 *     file, or control from another device sending text commands to ClearCore.
 *    
 * Setup:
 * 0. Consult the accompanying ClearCore Command Protocol User Guide for more
 *     information on using this example project.
 *		https://teknic.com/files/downloads/ClearCoreCommandProtocol_UserGuide.pdf
 * 1. Connect ClearCore via USB to a terminal that can send and receive ASCII
 *     (standard text encoding) messages, or modify the code and connect to a
 *     different source of messages. Other ClearCore examples demonstrate
 *     communication via these alternate connection options. 
 * 2. ClearPath-SD motors can be connected to connector M-0, M-1, M-2, and/or M-3.
 * 3. The connected ClearPath-SD motor(s) must be configured through the MSP software
 *    for Step and Direction mode (In MSP select Mode>>Step and Direction).
 * 4. The ClearPath-SD motor(s) must be set to use the HLFB mode "ASG-Position
 *    w/Measured Torque" with a PWM carrier frequency of 482 Hz through the MSP
 *    software (select Advanced>>High Level Feedback [Mode]... then choose
 *    "ASG-Position w/Measured Torque" from the dropdown, make sure that 482 Hz
 *    is selected in the "PWM Carrier Frequency" dropdown, and hit the OK
 *    button).
 * 5. Set the Input Format in MSP for "Step + Direction".  
 * 6. Set the Input Resolution in MSP the same as your motor's Positioning
 *    Resolution spec if you'd like the pulses sent by ClearCore to command a
 *    move of the same number of Encoder Counts, a 1:1 ratio.
 * 7. Input and output devices can be wired to connectors IO0-IO5, DI6-DI8, and/or
 *     AI9-AI12. A table summarizing the acceptable connector modes for each
 *     ClearCore connector can be found in the ClearCore manual and in the User
 *     Guide for this example. This example explicitly sets the operational mode
 *     for each connector, but these modes can be reconfigured according to the
 *     table in the manual. Be sure to consult the ClearCore manual and
 *     supplemental wiring and connection diagrams to view the operating modes
 *     for each pin and corresponding connection setups.
 *    
 * Example Command Sequences
 *	The following provides an example command sequence and corresponding behavior:
 *		e1			//enable motor1
 *		e2			//enable motor2
 *		m1 1000		// if ABSOLUTE_MOVE==1, move motor1 to absolute position 1000 steps 
 *					// if ABSOLUTE_MOVE==0, move motor1 1000 steps in the positive direction 
 *		v2 -200		//move motor2 at -200 steps/s in the negative direction
 *		q2s			//query the status of motor2
 *		l3v	100		//limit motor3's velocity (of positional moves only) to 100 steps/s
 *		z1			//zero Clear's position reference for motor1 (no motion is commanded to the motor)  
 *		i6			//read the current state of connector6 (DI6)
 *		o5 1		//output a value of 1 (digital high) to connector5 (IO5)
 *		h 			//display the help message
 *		
 *	The following example command sequence highlights special cases and notable behavior:
 *		v0 1000		//since motor0 has not yet been enabled, no motion will be commanded
 *		e0			//enable motor0
 *		v0 1000		//move motor0 at 1000 steps/s
 *		d0			//disable motor0. since motor0 was actively moving, motor0 will fault
 *		v0 1000		//since motor0 is in fault (disabled during motion), no motion will be commanded
 *		q1s			//query the status of motor0. since the motor is in alert, the alert status will also print
 *		f 0			//disable verbose feedback 
 *		q1s			//with verbose feedback disabled, only the numerical status (and alert, in this case) 
 *					//	register(s) will print, not the full status (and alert) message
 *		c0			//clear alerts on motor0
 *		v0 1000		//move motor0 at 1000 steps/s
 *		
 *		m1 2000		//assuming ABSOLUTE_MOVE is left as default, move motor1 to absolute position 2000 steps
 *		m1 2000		//since motor1 is already at absolute position 2000 steps, no motion will be commanded
 *		z1			//define motor1's current position as the zero position 
 *		m1 2000		//move motor1 to absolute position 2000, which is 2000 steps  
 *					//	away from its freshly-zeroed current position
 *    
 * Additional Resources:
 * ** ClearCore Command Protocol User Guide:
 *     https://teknic.com/files/downloads/ClearCoreCommandProtocol_UserGuide.pdf
 * ** ClearCore Documentation:
 * 		https://teknic-inc.github.io/ClearCore-library/
 * ** ClearCore Manual:
 * 		 https://www.teknic.com/files/downloads/clearcore_user_manual.pdf
 * ** ClearCore System Diagram and Connection Diagrams:
 * 		https://www.teknic.com/files/downloads/clearcore_user_manual.pdf
 * ** ClearPath Manual (DC Power):
 * 		https://www.teknic.com/files/downloads/clearpath_user_manual.pdf
 * ** ClearPath Manual (AC Power):
 * 		https://www.teknic.com/files/downloads/ac_clearpath-mc-sd_manual.pdf
 * 
 * Copyright (c) 2020 Teknic Inc. This work is free to use, copy and distribute under the terms of
 * the standard MIT permissive software license which can be found at https://opensource.org/licenses/MIT
 */

#include "ClearCore.h"
#include <stdint.h>
#include <stdlib.h> //for atoi (ASCII to Integer) parsing utility

// specify which serial interface to use as input/output
#define ioPort ConnectorUsb
// select the baud rate to match the target device.
#define ioPortBaudRate  115200
// specify whether the target serial interface uses CTS/RTS flow control
// 	set to true if your target device uses CTS/RTS flow control
// this is only necessary if using COM ports or XBee module (not necessary for USB)
#define ioFlowControl false

// structure to hold information on the feedback messages
struct FeedbackMessage{
	uint32_t number;
	char *message; 
};

// feedback message numbers
#define FB_COMMAND_OK									( 0)	
#define FB_ERR_BUFFER_OVERRUN                   		( 1)
#define FB_ERR_INPUT_INVALID_NONLETTER          		( 2)
#define FB_ERR_MOTOR_NUM_INVALID						( 3)	
#define FB_ERR_CONNECTOR_NUM_INVALID					( 4)	
#define FB_ERR_CONNECTOR_MODE_INCOMPATIBLE				( 5)
#define FB_ENABLED_WAITING_ON_HLFB						( 6)
#define FB_ENABLE_FAILURE								( 7)
#define FB_ERR_IO_OUTPUT								( 8)
#define FB_ERR_MOVE_NOT_ENABLED							( 9)	
#define FB_ERR_MOVE_IN_ALERT                    		(10)
#define FB_ERR_INVALID_QUERY_REQUEST            		(11)
#define FB_ERR_LIMIT_OUT_OF_BOUNDS						(12)				
#define FB_ERR_INVALID_LIMIT_REQUEST            		(13)
#define FB_ERR_INVALID_FEEDBACK_OPTION          		(14)
#define FB_ERR_UNRECOGNIZED_COMMAND             		(15)
#define FB_HELP                                 		(16)

// verbose feedback messages						
char *msg_command_ok = 
	"Command received";
char *msg_err_buffer_overrun = 
	"Error: input buffer overrun.";
char *msg_err_input_invalid_nonletter = 
	"Error: invalid input. Commands begin with a single letter character.";
char *msg_err_motor_num_invalid = 
	"Error: a required motor was not specified or specified incorrectly. Acceptable motor numbers are 0, 1, 2, and 3.";
char *msg_err_connector_num_invalid = 
	"Error: a required connector was not specified or specified incorrectly. Acceptable connector numbers are 0 through 12, inclusive.";
char *msg_err_io_output = 
	"Error: an I/O output parameter is invalid. Ensure the output value is appropriate for the type of output pin.";
char *msg_err_connector_mode_incompatible = 
	"Error: a specified connector is of an inappropriate mode. Verify the I/O connector is configured as necessary."; 
char *msg_enabled_waiting_on_hlfb = 
	"Motor enabled; waiting on HLFB to assert before accepting other commands.";
char *msg_enable_failure = 
	"Motor failed to enable due to motor fault, loss of power, or loss/absence of connection. Motor disabled.";
char *msg_err_move_not_enabled = 
	"Error: motion commanded while motor not enabled. Command e# to enable motor number #.";
char *msg_err_move_in_alert = 
	"Error: motion commanded while motor in fault. Command c# to clear alerts on motor number #.";
char *msg_err_invalid_query_request = 
	"Error: invalid query request. Command h for more information.";
char *msg_err_limit_out_of_bounds = 
	"Error: commanded limit falls outside the acceptable bounds for this limit.";
char *msg_err_invalid_limit_request = 
	"Error: invalid limit request. Command h for more information.";
char *msg_err_invalid_feedback_option = 
	"Error: invalid feedback request. Command h for more information.";
char *msg_err_unrecognized_command = 
	"Error: unrecognized command. Command h for more information.";
char *msg_help = 
      "ClearCore Command Protocol\n"
      "Acceptable commands, where # specifies a motor number* (0, 1, 2, or 3): \n"
      "    e#              | enable specified motor\n"
      "    d#              | disable specified motor\n"
      "    m# distance     | if(ABSOLUTE_MOVE==1) move to the specified position\n"
      "                      if(ABSOLUTE_MOVE==0) move the specified number of steps\n"
      "    v# velocity     | move at the specified velocity (steps/s)\n"
      "    q#<p/v/s>       | query specified motor's position/velocity/status\n"
      "    l#<v/a> limit   | set specified motor's velocity/acceleration limit\n"
      "    c#              | clear alerts\n"
      "    z#              | set the zero position for motor # to the current commanded position\n"
	  "    i#              | read input on pin #\n"
	  "                        Digital pins return 1 or 0; analog pins return [0,4095] corresponding to [0,10]V\n"
	  "                        (*note that # for this command can be 0 through 5)\n"
	  "    o# outputVal    | write output on pin #\n"
	  "                        Digital pins allow 1 or 0; analog pins allow [409,2047] corresponding to [4,20]mA\n"
	  "                        (*note that # for this command can be 0 through 12)\n"
      "    f fdbkType      | specify the type of feedback printed:\n"
      "                        0  : send message number only\n"
      "                        1  : send verbose message\n"
      "    h               | print this help message\n";

// array of feedback messages, accessed by SendFeedback() when feedback is sent 
FeedbackMessage FeedbackMessages[17] = {
	{FB_COMMAND_OK					, 	msg_command_ok						},
	{FB_ERR_BUFFER_OVERRUN          ,  	msg_err_buffer_overrun				},
	{FB_ERR_INPUT_INVALID_NONLETTER	,   msg_err_input_invalid_nonletter		},
	{FB_ERR_MOTOR_NUM_INVALID		, 	msg_err_motor_num_invalid			},
	{FB_ERR_CONNECTOR_NUM_INVALID	, 	msg_err_connector_num_invalid		},
	{FB_ERR_CONNECTOR_MODE_INCOMPATIBLE, 	
										msg_err_connector_mode_incompatible	},
	{FB_ENABLED_WAITING_ON_HLFB		, 	msg_enabled_waiting_on_hlfb			},
	{FB_ENABLE_FAILURE				, 	msg_enable_failure					},
	{FB_ERR_IO_OUTPUT				, 	msg_err_io_output					},
	{FB_ERR_MOVE_NOT_ENABLED		, 	msg_err_move_not_enabled			},
	{FB_ERR_MOVE_IN_ALERT           ,  	msg_err_move_in_alert				},
	{FB_ERR_INVALID_QUERY_REQUEST   ,  	msg_err_invalid_query_request		},
	{FB_ERR_LIMIT_OUT_OF_BOUNDS		,  	msg_err_limit_out_of_bounds			},
	{FB_ERR_INVALID_LIMIT_REQUEST   ,  	msg_err_invalid_limit_request		},
	{FB_ERR_INVALID_FEEDBACK_OPTION ,  	msg_err_invalid_feedback_option		},
	{FB_ERR_UNRECOGNIZED_COMMAND    ,	msg_err_unrecognized_command		},
	{FB_HELP                        ,  	msg_help							}
};

// global variable to select between printing only feedback number or verbose feedback message
bool verboseFeedback = true;

// macro to select between commanding absolute positional moves or relative 
//	positional moves. See User Guide for more information.											
#define ABSOLUTE_MOVE (1)

// container for the char stream to be read-in. 
//	allows for IN_BUFFER_LEN characters to be stored followed by a NULL terminator 
//todo thoughts on this?
#define IN_BUFFER_LEN 32
char input[IN_BUFFER_LEN+1];

// motor connectors
MotorDriver *const motors[MOTOR_CON_CNT] = {
	&ConnectorM0, &ConnectorM1, &ConnectorM2, &ConnectorM3
};

// I/O connectors
Connector *const connectors[13] = {
	&ConnectorIO0, &ConnectorIO1, &ConnectorIO2, &ConnectorIO3, &ConnectorIO4, &ConnectorIO5, 
	&ConnectorDI6, &ConnectorDI7, &ConnectorDI8,
	&ConnectorA9, &ConnectorA10, &ConnectorA11, &ConnectorA12
};

// acceleration and velocity limit bounds 
// (note that velocity limits take effect only on positional moves)
#define DEFAULT_ACCEL_LIMIT (100000) // pulses per sec^2
#define MAX_ACCEL_LIMIT	(1000000000) 
#define MIN_ACCEL_LIMIT	(1)
#define DEFAULT_VEL_LIMIT (10000) // pulses per sec
#define MAX_VEL_LIMIT	(500000)
#define MIN_VEL_LIMIT	(1)

// helper functions to print feedback and status information
// the implementations of these functions are at the bottom of this example
void SendFeedback(int32_t messageNumber);
void SendVerboseStatus(int32_t motorNumber);


int main() {

	// configure serial communication to USB port and wait for the port to open
	ioPort.Mode(Connector::USB_CDC);	
	ioPort.Speed(ioPortBaudRate);
	// ioPort.FlowControl(ioFlowControl); //only necessary if using COM ports or XBee module
	ioPort.PortOpen();
	while (!ioPort) {
		continue;
	}
	
	// configure input clocking rate
	// this normal rate is ideal for ClearPath step and direction applications
	MotorMgr.MotorInputClocking(MotorManager::CLOCK_RATE_NORMAL);

	// set all motor connectors into step and direction mode
	MotorMgr.MotorModeSet(MotorManager::MOTOR_ALL, Connector::CPM_MODE_STEP_AND_DIR);
	
	// local storage for velocity and acceleration limits
	// (note that velocity limits only take effect on positional moves)
	uint32_t accelerationLimits[MOTOR_CON_CNT] = { 
		DEFAULT_ACCEL_LIMIT, DEFAULT_ACCEL_LIMIT, DEFAULT_ACCEL_LIMIT, DEFAULT_ACCEL_LIMIT
	};
	uint32_t velocityLimits[MOTOR_CON_CNT] = { 
		DEFAULT_VEL_LIMIT, DEFAULT_VEL_LIMIT, DEFAULT_VEL_LIMIT, DEFAULT_VEL_LIMIT
	};

	// generic iterator
	uint32_t i;
	
	// configure all motor connectors for bipolar PWM HLFB mode at 482Hz, and set 
	//	velocity and acceleration limits
	for (i=0; i<MOTOR_CON_CNT; i++){
		motors[i]->HlfbMode(MotorDriver::HLFB_MODE_HAS_BIPOLAR_PWM);
		motors[i]->HlfbCarrier(MotorDriver::HLFB_CARRIER_482_HZ);
		motors[i]->VelMax(velocityLimits[i]);
		motors[i]->AccelMax(accelerationLimits[i]);
	}

	// configure I/O pins
	//	these defaults can be modified according to application needs
	//	for more information and to view a list of configurable modes for each pin, 
	//	see the ClearCore manual or the User Guide for this example
	ConnectorIO0.Mode(Connector::OUTPUT_ANALOG );
	ConnectorIO1.Mode(Connector::OUTPUT_DIGITAL);
	ConnectorIO2.Mode(Connector::OUTPUT_DIGITAL);
	ConnectorIO3.Mode(Connector::OUTPUT_DIGITAL);
	ConnectorIO4.Mode(Connector::OUTPUT_DIGITAL);
	ConnectorIO5.Mode(Connector::OUTPUT_DIGITAL);
	ConnectorDI6.Mode(Connector::INPUT_DIGITAL );
	ConnectorDI7.Mode(Connector::INPUT_DIGITAL );
	ConnectorDI8.Mode(Connector::INPUT_DIGITAL );
	ConnectorA9 .Mode(Connector::INPUT_ANALOG  );
	ConnectorA10.Mode(Connector::INPUT_ANALOG  );
	ConnectorA11.Mode(Connector::INPUT_ANALOG  );
	ConnectorA12.Mode(Connector::INPUT_ANALOG  );
	
	// program control variables
	bool inputValid;
	bool motorNumValid;
	bool connectorNumValid;
	bool motorEnabledBeforeClearingAlerts;
	
	// parsing variables
	int32_t motorNum_in; 
	int32_t connectorNum_in; 
	int32_t moveDistance_in;
	int32_t velocity_in;
	int32_t limit_in; 
	int32_t queriedValue;
	int16_t inputConnectorValue;
	int16_t outputConnectorValue_in;

	ioPort.SendLine("Setup successful");
	ioPort.SendLine("Send 'h' to receive a list of valid commands"); 

	// main loop to read and process input
	while (true) {

		// reset the input buffer by populating each index with a NULL character
		for (i = 0; i<IN_BUFFER_LEN+1; i++){
			input[i] = (char) NULL;
		}
		
		// read and store the input character by character
		// input buffer has a default maximum size, defined by IN_BUFFER_LEN, of 32. 
		// if more characters are provided by the user, program will reject input.
		inputValid = true;
		i = 0;
		while(i<IN_BUFFER_LEN && ioPort.CharPeek() != -1){
			input[i] = (char) ioPort.CharGet();
			i++;
			Delay_ms(1);
		}
		
		//-------------------------
		// This line divides the command reading and storing section of the code from 
		//	the command parsing section of the code. To use this protocol to accept commands
		//	from another source than the default USB input, ensure that the command is stored
		//	in a character array called "input" and is ready to be parsed after this line.
		//-------------------------

		// echo nonempty input when in verbose feedback mode
		if(verboseFeedback && i!=0){
			ioPort.SendLine(input);
		}
		
		if (i==0){
			// user did not input any characters. input invalid, but no need to report 
			inputValid = false;
		} else if (ioPort.CharPeek()!=-1){ 
			// buffer overflow (there is no space left in the buffer, but there are still characters to read from ioPort)
			// report error, reject input, and flush input stream (so the leftover characters aren't read as part of the next command)
			SendFeedback(FB_ERR_BUFFER_OVERRUN);
			inputValid = false;
			while(ioPort.CharPeek()!=-1){
				ioPort.FlushInput();
				Delay_ms(10);
			}
		}
		
		// verify first character of command is a letter
		if (inputValid){
			if (('A' <= input[0] && input[0] <= 'Z') || ('a' <= input[0] && input[0] <= 'z')){
				// if letter is uppercase, convert to lowercase
				if ('A' <= input[0] && input[0] <= 'Z'){
					// to convert an ASCII character from upper to lower case, add 32 to the character
					input[0] += 32; 
				}
			} else {
				// if first letter of command is not letter, reject input
				SendFeedback(FB_ERR_INPUT_INVALID_NONLETTER);
				inputValid = false;
				ioPort.FlushInput();
			}
		}
		
		if (inputValid){

			// store and validate motor number input from command
			motorNum_in = atoi(&input[1]); 
			motorNumValid = (0<=motorNum_in && motorNum_in<=3 && input[1]!=NULL);
			
			// store and validate connector number input from command
			connectorNum_in = atoi(&input[1]); 
			connectorNumValid = (0<=connectorNum_in && connectorNum_in<=12 && input[1]!=NULL);
			
			// status register for accessing motor status information
			volatile const MotorDriver::StatusRegMotor &statusReg = motors[motorNum_in]->StatusReg();
			volatile const MotorDriver::AlertRegMotor  &alertReg  = motors[motorNum_in]->AlertReg();


			// process the command based on the command letter (first character of input)
			switch(input[0]){
							
				// enable 
				case 'e':
					// verify motor number valid
					if (!motorNumValid){
						SendFeedback(FB_ERR_MOTOR_NUM_INVALID);
					} else {
						// enable the motor
						motors[motorNum_in]->EnableRequest(true);
						SendFeedback(FB_ENABLED_WAITING_ON_HLFB);
						// wait until motor is ready before accepting other commands 
						//	(allows any automatic homing move to complete if configured)
						//	(loop will exit on fault during homing, or if motor is disconnected or loses power)
						while (statusReg.bit.HlfbState != MotorDriver::HLFB_ASSERTED &&
								!statusReg.bit.MotorInFault){
							continue;
						}
						if(statusReg.bit.MotorInFault){
							// if there is a fault while trying to enable, disable and report
							motors[motorNum_in]->EnableRequest(false);
							SendFeedback(FB_ENABLE_FAILURE);
						} else {
							SendFeedback(FB_COMMAND_OK);
						}
					}
					break; 

				// disable
				case 'd':
					// verify motor number valid
					if (!motorNumValid){
						SendFeedback(FB_ERR_MOTOR_NUM_INVALID);
					} else {
						// disable the motor
						motors[motorNum_in]->EnableRequest(false);
						SendFeedback(FB_COMMAND_OK);
					}
					break;
					
				// position move 
				case 'm': 
					// verify motor number valid, motor enabled, and no faults present
					if (!motorNumValid){
						SendFeedback(FB_ERR_MOTOR_NUM_INVALID);
					} else if ( statusReg.bit.ReadyState==MotorDriver::MOTOR_DISABLED || 
									statusReg.bit.ReadyState==MotorDriver::MOTOR_ENABLING){
						SendFeedback(FB_ERR_MOVE_NOT_ENABLED);
					} else if (motors[motorNum_in]->StatusReg().bit.AlertsPresent) {
						SendFeedback(FB_ERR_MOVE_IN_ALERT);
					} else {
						// command the move
						// #define ABSOLUTE_MOVE (0) commands absolute moves.
						// #define ABSOLUTE_MOVE (1) commands relative moves.
						// absolute moves are configured by default.
						// See User Guide for more information.
						moveDistance_in = atoi(&input[2]);		
						if (ABSOLUTE_MOVE){
							motors[motorNum_in]->Move(moveDistance_in, MotorDriver::MOVE_TARGET_ABSOLUTE);
						} else {
							motors[motorNum_in]->Move(moveDistance_in);
						}				
						SendFeedback(FB_COMMAND_OK);
					}
					break;

				// velocity move 
				case 'v': 
					// verify motor number valid, motor enabled, and no faults present
					if (!motorNumValid){
						SendFeedback(FB_ERR_MOTOR_NUM_INVALID);
					} else if ( statusReg.bit.ReadyState==MotorDriver::MOTOR_DISABLED ||
									statusReg.bit.ReadyState==MotorDriver::MOTOR_ENABLING){
						SendFeedback(FB_ERR_MOVE_NOT_ENABLED);
					} else if (motors[motorNum_in]->StatusReg().bit.AlertsPresent) {
						SendFeedback(FB_ERR_MOVE_IN_ALERT);
					} else {
						// command the move
						velocity_in = atoi(&input[2]);
						motors[motorNum_in]->MoveVelocity(velocity_in);
						SendFeedback(FB_COMMAND_OK);
					}
					break;
				
				// query position, velocity, or status
				case 'q':
					// verify motor number valid
					if (!motorNumValid){
						SendFeedback(FB_ERR_MOTOR_NUM_INVALID);
						break;
					}
					switch(input[2]){
						// query commanded position 
						case 'p':
						case 'P':
							// send commanded position. 
							// this can differ from the position counter in MSP if the ClearCore 
							//	position reference has not been synced with ClearPath's position
							queriedValue = (int32_t) motors[motorNum_in]->PositionRefCommanded();
							if (verboseFeedback){
								ioPort.Send("Motor ");
								ioPort.Send(motorNum_in);
								ioPort.Send(" is in position (steps) ");
							}
							ioPort.SendLine(queriedValue);
							break;
							
						// query velocity
						case 'v':
						case 'V':
							// send motor velocity
							queriedValue = motors[motorNum_in]->VelocityRefCommanded();
							if (verboseFeedback){
								ioPort.Send("Motor ");
								ioPort.Send(motorNum_in);
								ioPort.Send(" is at velocity (steps/s) ");
							}
							ioPort.SendLine(queriedValue);
							break;
							
						// query status 
						case 's':
						case 'S':
							// send motor status
							if (verboseFeedback){
								SendVerboseStatus(motorNum_in);
							} else {
								ioPort.SendLine(statusReg.reg, 2);	// prints the status register in binary
								if (statusReg.bit.AlertsPresent){
									ioPort.SendLine(alertReg.reg, 2); // prints the alert register in binary
								}
							}
							break;
						
						// invalid query request
						default:
							SendFeedback(FB_ERR_INVALID_QUERY_REQUEST);
					}
					break;
				
				// set acceleration or velocity limit
				case 'l': 
					// verify motor number valid
					if ( !motorNumValid){
						SendFeedback(FB_ERR_MOTOR_NUM_INVALID);
						break;
					}
					// store limit input from command
					limit_in = atoi(&input[3]);
					
					switch(input[2]){
						// velocity limit
						case 'v':
						case 'V':
							//verify limit is valid, store, then propagate the change to the motor
							if (MIN_VEL_LIMIT<=limit_in && limit_in<=MAX_VEL_LIMIT){
								velocityLimits[motorNum_in] = limit_in;
								motors[motorNum_in]->VelMax(velocityLimits[motorNum_in]);
								SendFeedback(FB_COMMAND_OK);
							} else {
								SendFeedback(FB_ERR_LIMIT_OUT_OF_BOUNDS);
							}
							break;
							
						// acceleration limit
						case 'a':
						case 'A':
							//verify limit is valid, store, then propagate the change to the motor
							if (MIN_ACCEL_LIMIT<=limit_in && limit_in<=MAX_ACCEL_LIMIT){
								accelerationLimits[motorNum_in] = limit_in;
								motors[motorNum_in]->AccelMax(accelerationLimits[motorNum_in]);
								SendFeedback(FB_COMMAND_OK);
							} else {
								SendFeedback(FB_ERR_LIMIT_OUT_OF_BOUNDS);
							}
							break;
							
						// invalid limit request
						default:
							SendFeedback(FB_ERR_INVALID_LIMIT_REQUEST);
							break;
					}
					break;

				// clear alerts
				case 'c':
					// verify motor number valid
					if (!motorNumValid){
						SendFeedback(FB_ERR_MOTOR_NUM_INVALID);
						break;
					}
					
					// capture the current state of enable 
					//	this value will be restored after alerts are cleared
					motorEnabledBeforeClearingAlerts = motors[motorNum_in]->EnableRequest();
					
					// to clear all ClearCore alerts (which can include motor faults): 
					//	- cycle enable if faults present (clears faults, if any)
					//	- clear alert register (clears alerts)
					// this command clears both ClearCore motor alerts and motor faults
					if (statusReg.bit.MotorInFault){ 
						motors[motorNum_in]->EnableRequest(false);
						Delay_ms(10);
						if(motorEnabledBeforeClearingAlerts){
							motors[motorNum_in]->EnableRequest(true);
						}
					}
					motors[motorNum_in]->ClearAlerts();
					SendFeedback(FB_COMMAND_OK);
					break;
					
				// set the zero position for motor # to the current commanded position
				case 'z':
					// verify motor number valid
					if (!motorNumValid){
						SendFeedback(FB_ERR_MOTOR_NUM_INVALID);
						break;
					}
					// zero position
					motors[motorNum_in]->PositionRefSet(0);
					SendFeedback(FB_COMMAND_OK);
					break;

				// read input from ClearCore connector 
				case 'i':
					// verify connector number valid
					if (!connectorNumValid){
						SendFeedback(FB_ERR_CONNECTOR_NUM_INVALID);
						break;
					}
					// verify connector mode valid
					if (connectors[connectorNum_in]->Mode() != Connector::INPUT_DIGITAL && 
						connectors[connectorNum_in]->Mode() != Connector::INPUT_ANALOG){
						SendFeedback(FB_ERR_CONNECTOR_MODE_INCOMPATIBLE); 
						break;
					}
					inputConnectorValue = connectors[connectorNum_in]->State();
					if(verboseFeedback){
						ioPort.Send("Connector ");
						ioPort.Send(connectorNum_in);
						ioPort.Send(" value: ");
					}
					ioPort.SendLine(inputConnectorValue);
					break;
				
				
				// write digital output to ClearCore digital output connector 
				case 'o':
					// verify connector number valid
					if (!connectorNumValid){
						SendFeedback(FB_ERR_CONNECTOR_NUM_INVALID); 
						break;
					}
					// verify connector type valid
					if (connectors[connectorNum_in]->Mode() != Connector::OUTPUT_DIGITAL && 
						connectors[connectorNum_in]->Mode() != Connector::OUTPUT_ANALOG){
						SendFeedback(FB_ERR_CONNECTOR_MODE_INCOMPATIBLE); 
						break;
					}
					// store and write value
					outputConnectorValue_in = atoi(&input[3]);
					if(!connectors[connectorNum_in]->State(outputConnectorValue_in)){
						SendFeedback(FB_ERR_IO_OUTPUT);
					} else {
						SendFeedback(FB_COMMAND_OK);
					}
					break;
				
				
				// change feedback type 
				case 'f':
					switch(atoi(&input[1])){
						
						// feedback number only
						case 0:
							verboseFeedback = false;
							SendFeedback(FB_COMMAND_OK);
						break;
						
						// verbose feedback messages
						case 1:
							verboseFeedback = true;
							SendFeedback(FB_COMMAND_OK);
							break;
						
						// invalid feedback request
						default:
							SendFeedback(FB_ERR_INVALID_FEEDBACK_OPTION);
						break;
					}
					break;

				// help
				case 'h':
					SendFeedback(FB_HELP);
					break;

				// invalid command letter
				default:
					SendFeedback(FB_ERR_UNRECOGNIZED_COMMAND);
					break;

			} // switch(command letter)
		} // if (inputValid)
	} // while(true)
} // main()


/*------------------------------------------------------------------------------
 * SendFeedback
 *
 *    Helper function to send feedback for specified feedback message.
 *   
 * Parameters:
 *    int messageNumber	- The feedback identification number corresponding the 
 *			feedback struct array index
 *
 * Returns: None (feedback message is sent directly to output)
*/
 void SendFeedback(int32_t messageNumber){
	// send either the verbose message or only the message number, based on verboseFeedback bool
	if (verboseFeedback){
		ioPort.SendLine(FeedbackMessages[messageNumber].message);
	} else {
		ioPort.SendLine(FeedbackMessages[messageNumber].number);
	}
 } // SendFeedback()
//------------------------------------------------------------------------------



/*------------------------------------------------------------------------------
 * SendVerboseStatus
 *
 *    Outputs verbose status information. 
 *    Functionality adapted from MotorStatusRegister example project
 *
 * Parameters:
 *    int motorNumber  - The motor whose status should be printed
 *
 * Returns: None (status message is sent directly to output)
 */
void SendVerboseStatus(int32_t motorNumber) {
	// status register for accessing motor status information
	volatile const MotorDriver::StatusRegMotor &statusReg = motors[motorNumber]->StatusReg();
	volatile const MotorDriver::AlertRegMotor &alertReg = motors[motorNumber]->AlertReg();
	
	ioPort.Send("Motor status register for motor M");
	ioPort.Send(motorNumber);
	ioPort.Send(": ");
	ioPort.SendLine(statusReg.reg, 2); // prints the status register in binary
	
	ioPort.Send("AtTargetPosition:	");
	ioPort.SendLine(statusReg.bit.AtTargetPosition);

	ioPort.Send("StepsActive:     	");
	ioPort.SendLine(statusReg.bit.StepsActive);

	ioPort.Send("AtTargetVelocity:	");
	ioPort.SendLine(statusReg.bit.AtTargetVelocity);
		
	ioPort.Send("MoveDirection:   	");
	ioPort.SendLine(statusReg.bit.MoveDirection);

	ioPort.Send("MotorInFault:    	");
	ioPort.SendLine(statusReg.bit.MotorInFault);

	ioPort.Send("Enabled:         	");
	ioPort.SendLine(statusReg.bit.Enabled);

	ioPort.Send("PositionalMove:  	");
	ioPort.SendLine(statusReg.bit.PositionalMove);

	ioPort.Send("HLFB State:		");
	switch (statusReg.bit.HlfbState) {
		case 0:
			ioPort.SendLine("HLFB_DEASSERTED");
			break;
		case 1:
			ioPort.SendLine("HLFB_ASSERTED");
			break;
		case 2:
			ioPort.SendLine("HLFB_HAS_MEASUREMENT");
			break;
		case 3:
			ioPort.SendLine("HLFB_UNKNOWN");
			break;
		default:
			// something has gone wrong if this is printed
			ioPort.SendLine("???");
	}

	ioPort.Send("AlertsPresent:   	");
	ioPort.SendLine(statusReg.bit.AlertsPresent);

	ioPort.Send("Ready state:		");
	switch (statusReg.bit.ReadyState) {
		case MotorDriver::MOTOR_DISABLED:
			ioPort.SendLine("Disabled");
			break;
		case MotorDriver::MOTOR_ENABLING:
			ioPort.SendLine("Enabling");
			break;
		case MotorDriver::MOTOR_FAULTED:
			ioPort.SendLine("Faulted");
			break;
		case MotorDriver::MOTOR_READY:
			ioPort.SendLine("Ready");
			break;
		case MotorDriver::MOTOR_MOVING:
			ioPort.SendLine("Moving");
			break;
		default:
			// something has gone wrong if this is printed
			ioPort.SendLine("???");
	}
	
	ioPort.Send("Triggering:      	");
	ioPort.SendLine(statusReg.bit.Triggering);

	ioPort.Send("InPositiveLimit: 	");
	ioPort.SendLine(statusReg.bit.InPositiveLimit);

	ioPort.Send("InNegativeLimit: 	");
	ioPort.SendLine(statusReg.bit.InNegativeLimit);

	ioPort.Send("InEStopSensor:   	");
	ioPort.SendLine(statusReg.bit.InEStopSensor);	

	ioPort.SendLine("--------------------------------");
	
		
	if (statusReg.bit.AlertsPresent){
		ioPort.Send("Alert register:	");
		ioPort.SendLine(alertReg.reg, 2); // prints the alert register in binary

		ioPort.Send("MotionCanceledInAlert:         ");
		ioPort.SendLine(alertReg.bit.MotionCanceledInAlert);

		ioPort.Send("MotionCanceledPositiveLimit:   ");
		ioPort.SendLine(alertReg.bit.MotionCanceledPositiveLimit);

		ioPort.Send("MotionCanceledNegativeLimit:   ");
		ioPort.SendLine(alertReg.bit.MotionCanceledNegativeLimit);

		ioPort.Send("MotionCanceledSensorEStop:     ");
		ioPort.SendLine(alertReg.bit.MotionCanceledSensorEStop);

		ioPort.Send("MotionCanceledMotorDisabled:   ");
		ioPort.SendLine(alertReg.bit.MotionCanceledMotorDisabled);

		ioPort.Send("MotorFaulted:                  ");
		ioPort.SendLine(alertReg.bit.MotorFaulted);

		ioPort.SendLine("--------------------------------");
	}


}// SendVerboseStatus()
//------------------------------------------------------------------------------



