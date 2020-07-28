# Voip-App-PulseAudio
A Voip program written in C using PulseAudio client library with Opus codec. First, install PulseAudio client library and Opus codec development files by executing 
`sudo apt install libpulse-dev libopus-dev` and then run `make`. There is no server, the program works client to client.

Note: You need to forward a port in your router. Default port is 11000

I am actually working on a faster version of this voip program. In this version, there is a noticeable delay between capture and playback(I assume the delay is around 0.5 to 1 sec, maybe even more). I am trying to reduce the delay down to under 30-20 ms. For that, I am gonna be either using asynchronous API from PulseAudio or directly ALSA API.

By the way, you will start the program by `./VoipClient [ipaddress]`. Have fun!
