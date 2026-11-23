# BlackBox-Environment-Control
This project, developed for the Design Laboratory course, provides an environment control system supporting ALICE FIT. Its primary function is to continuously measure essential environmental parameters, including temperature, humidity, and light within the chamber housing the sensitive equipment. To ensure reliable monitoring, the system actively logs data and establishes a seamless connection to the lab infrastructure.

Authors: Przemysław Łukaszyk, Bartłomiej Sokal

## Project Milestones:
Write code to test and read sensor data on Raspberry Pi Pico and verify that the readings are accurate and responsive to real world changes.

Implement data packaging and serial transmission. 

Develop functional server backend capable of handling and managing incoming sensor data.

Create a frontend user interface that displays live and historical data.

Test and validation of the complete system.

## Project Progress:
5.11.2026: <br />
&emsp; Defined project milestones <br />
&emsp; Compared sensor options to find a good fit for the project <br /> <br />
12.11.2026: <br />
&emsp; Received and familiarized ourselves with the Raspberry Pico platform and Kamami sensor dev board <br />
&emsp; Prototyped simple I2C polling code to verify communication and learn how the sensors behave <br /> <br />
19.11.2026: <br />
&emsp; Implemented temperature, humidity and light intensity sensor logic and moved all sensor code into seperate files <br />
&emsp; Creating a code outline for the server and measurement platform <br />
