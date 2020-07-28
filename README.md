# Voip-App-PulseAudio
A Voip program written in C using PulseAudio client library with Opus codec. First, install PulseAudio client library and Opus codec development files by executing 
`sudo apt install libpulse-dev libopus-dev` and then run `make`. There is no server, the program works client to client.
Note: You need to forward a port in your router. Default port is 11000

You will start the program by `./VoipClient [ipaddress]`. Have fun!
