#Introduction 

This is the GNU Radio OOT module for command and control of the DRS Picoflexor and PicoTransceiver radios.  This block will allow the user to easily adjust the sample rate, frequency, attenuation and streaming setup to allow for IQ data to be streamed to a host machine.  This block only controls the SIGINT portion of the radios.  As a result this block is used for receiving data from the radio.

*Note - This block requires the radio to be in a mnemonic session to run.

#Installation 

If you have GNURadio installed, you can install the block through the following install process.

	git clone ADD REPO
	cd gr-pico
	mkdir build
	cd build
	cmake ..
	sudo make install
	sudo ldconfig

##Block Limitations

Both the Picoflexor and the PicoTranceiver use a USB2.0 to Ethernet interface for streaming data to a host machine.  This limits the transfer rate between the radio and the host machine to 27.3 Mbytes/sec.  As a result the maximum sample rate that the block can sustain is 6.7 Msps.

##Block Reporting

When data loss is detected in the block, it will respond with one of three print outs to the console.  Each has a different meaning and can help you optimize your settings for performance.
* Frame Loss ("F") - When an F is reported it means that the block detected an interrupt in the frame counter. Since each frame contains at least one or more packets at least one L is reported when an F is reported.  NOTE : When changing sample rate while streaming the block will may report F's and L's due to the stopping and starting of streams in rapid succession.
* Packet Loss ("L") - When an L is reported it means that the block detected an interrupt in the packet counter.  This means that the system failed to capture a packet that came across the wire.
* Overflow ("O") - When an O is reported the block failed to keep up with incoming data on the back end.  Each "O" represents a full buffer flush in udp_listener.  This backup can occur in a number of locations.  If you are running a graph with a lot of processing this can slow down the rate data is requested from the block, which will cause buffers to back up and overflow.  Improving your graphs performance (or capturing data ahead of time) can help with this.  You can also force the core affinity of all other blocks in the graph to be the same, this can also sometimes help performance.  If the graph is simple (just frequency or null sinks) then it could be that the block isn't using enough threads to process (or that it doesn't have enough room in the buffers).  Try increasing NUM_BUFFS in udp_listener or increasing NUM_THREADS in complex_manager.h.


