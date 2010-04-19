#!/bin/sh

alsactl -f /tmp/dtmfcmd.state store

alsactl -f /usr/share/dtmfcmd/dtmfcmd.state restore

amixer sset 'Capture Right Mux' 'Line or RXP-RXN'

amixer sset 'Capture Left Mux' 'Line or RXP-RXN'

amixer sset 'DAI Mode' 'DAI 2'


amixer sset 'Mono Mixer Left' on

amixer sset 'Mono Mixer Right Playback Switc' on

amixer sset 'PCM' 215