#!/bin/sh

testdir=$(dirname "$0")

wireshark -r "output.pcap" \
 -o fast.port:"5000" \
 -o fast.template:"$testdir/templates.xml" \
 -o fast.enabled:true \
 -o fast.show_empty:true

