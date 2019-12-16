***************************README***********************

Total 5 tasks are executed:

1) Execute State(01) : All the Four LED can be Executed At once without saving into memory

Cmd:	S	len	cmd_id	  no_of_led	<led_no state> 	E
Ex: 	AA 08 01 03 01 01 02 01 03 00 FF

2) Configure Switch(02) : Based on the Saved config_id of led, input state of Led will 
			  be validated and saved into memory.

Cmd:	S	len	cmd_id	  no_of_led	<led_no state> 	E
Ex: 	AA 0A 01 04 01 10 02 77 03 46 04 56 FF

AA 0A 02 04 01 02 02 02 03 02 04 02 FF

AA 08 01 03 01 10 02 77 04 46 FF
AA 0A 01 04 01 10 02 77 03 46 04 56 FF

**Configure Switch
S	len	cmd_id    no_of_led	config_id	E
	
AA 04 02 01 03 01 FF 


**get switch state
S	LEN	CMD_ID	E

AA 01 03 FF