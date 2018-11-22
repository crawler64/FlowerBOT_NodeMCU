# FlowerBOT_NodeMCU

This project intends to monitor the moisture of a flower. If the moisture drops below an certain threshold, a message should be sent via telegram using a telegram bot. 

To achieve this a NodeMCU board used, which is equipped with an ESP8266 chip. To measure the moisture the Flying Fish MH-sensor-series board is being used.

NodeMCU is setup in the arduino IDE and following Libraries are needed to compile this INO File:
-ArduinoJSON
-UniversalTelegramBot
