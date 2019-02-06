# Works with "UdpSendRecieveSensor.ino" firmware 

import socket
from time import time
import sys
import serial
import numpy as np
from matplotlib import pyplot as plt
import time as timer
import os, errno, sys, getopt
import telnetlib
import os as os

# These are used with router  (ESP is configured as station)
#LOCAL_UDP_IP 	=   "192.168.1.162"
#INO_UDP_IP 	=   "192.168.1.154"  		# Arduino's wifi IP

# These are used without router  (ESP is configured as access point)
LOCAL_UDP_IP 	=   "192.168.4.2"
INO_UDP_IP 	=   "192.168.4.1"  		# Arduino's wifi IP

UDP_RX_PORT	=   5005
UDP_TX_PORT 	=  10002       

sock = socket.socket(socket.AF_INET, # Internet
                     socket.SOCK_DGRAM) # UDP
sock.bind((LOCAL_UDP_IP, UDP_RX_PORT))

# set plot to animated
plt.ion()


timepoints =   [[],[],[],[]]
ydata = [[],[],[],[]]


yrange = [0, 40000] #2**(15)]
ylabel = "Resistance (Ohm)"

view_time =  5 # seconds of data to view at once
duration =  60 # total seconds to collect data


color = ['r-','g-','b-','k-']
line = []

#fig1 = plt.figure()
fig1, ax = plt.subplots(4, 1)
for i in range(1,5): 
	ch=i-1
	ax[ch].set_xlabel('time, seconds', fontsize='14', fontstyle='italic')
	ax[ch].set_ylabel(ylabel, fontsize='14', fontstyle='italic')
	ax[ch].grid(True)
	ax[ch].set_ylim(yrange)
	ax[ch].set_xlim([0,view_time])
	ax[ch].plot(ydata[ch],timepoints[ch], color[ch])
#	ax[ch].plot(ydata[ch], marker='.',markersize=1,linestyle='solid',markerfacecolor=color[ch],linecolor=color[ch])

run= True
datavol=0

# Open a tx socket to send a command to the Arduino 
sock_tx = socket.socket(socket.AF_INET, 	# Internet
	             socket.SOCK_DGRAM) # UDP


# Firmware expects "start" to111: 860SPS start data sending
print "starting the data sending"
MESSAGE = "start"
sock_tx.sendto(MESSAGE, (INO_UDP_IP, UDP_TX_PORT))

timer.sleep(1)

adc_bit_val 	= 0.0001875  	# volt/bit
Rs 		= 2700 		# Ohm
Vcc	 	= 5 		# volts

t0 =  time()
t1 =  t0
t2 =  t0
while run:

	t1=t2;
	t2=time();
	current_time = t2-t0;
	
	# For each of the ADC channels (0-3) 
	for ch in range(0,4):
		data, addr = sock.recvfrom(1024) # buffer size is 1024 bytes

		datanum = len(data) 
		wordsnum = datanum / 2
		
		adc_reading =  np.zeros(wordsnum).tolist()
		voltage	    =  np.zeros(wordsnum).tolist()
		resistance  =  np.zeros(wordsnum).tolist()
	
		#k=0
		for i in range(0,datanum,2):
			adc_reading[i/2]  = ord(data[i]) + (ord(data[i+1]) * 256)
			#k=k+1

		for i in range(0,wordsnum): 
			voltage[i]    = adc_bit_val *adc_reading[i]
			resistance[i] = (Rs*voltage[i]/Vcc)/(1-voltage[i]/Vcc) 
		#if (ch==0) : print ydata[0]
		#print ch 		
		#print type(adc_reading)
	#	ydata[ch].extend(voltage)
		ydata[ch].extend(resistance)
	#	ydata[ch].extend(adc_reading)
		
		#if (ch == 0)
		t_step = (t2-t1)/(wordsnum);
		cur_timepoints=np.arange(t1-t0, t2-t0, t_step);
		timepoints[ch].extend(cur_timepoints[:wordsnum])

		# update the plotted data
		ax[ch].lines[0].set_xdata(timepoints[ch])
		ax[ch].lines[0].set_ydata(ydata[ch])

		# slide the viewing frame along
		if current_time > view_time:
			ax[ch].set_xlim([current_time-view_time,current_time])

	#print "ch0: {0}, ch1: {1}, ch2: {2}, ch3: {3}, ".format(ydata[0][-1], ydata[1][-1], ydata[2][-1], ydata[3][-1])	
	fig1.canvas.draw()
	fig1.canvas.flush_events()

	#print "\n"			
	# update the plot

  
	# when time's up, kill the collect+plot loop
	if current_time > duration: 
		run=False

	
	datavol=datavol+wordsnum
	
	if ((t2-t0)> duration):	
		break;


#print ydata
#print timepoints

  
#print "received message:", data
print "\n"
print "total data words recieved {}".format (datavol)
sock.close()

print "stopping the data sending"
# Firmware expects "stop" to end data sending
MESSAGE = "stop"
sock_tx.sendto(MESSAGE, (INO_UDP_IP, UDP_TX_PORT))

# plot all of the data you collected

for ch in range(0,4):
	ax[ch].set_xlim([0, timepoints[ch][-1]])

fig1.show()
fig1.savefig('data/sensordata.pdf')

with open("data/sensordata.txt", "w") as output:
	output.write(str(timepoints))
	output.write(",")
	output.write(str(ydata))

output.close()
timer.sleep(1)
sock_tx.close()


