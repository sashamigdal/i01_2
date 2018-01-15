#!/bin/bash

exit 1;
# Needs pcap sanity /integrity checks (make sure all packets are
# present & accounted for) before we can trust this with all of our data.

src=/media/fdo/storage/i01/data/pcap
dst=/var/tmp/
for i in $src/md*.pcap-ns.lz4; do
    lz4 -d $i $dst/`basename $i .lz4`
    reorderpcap $dst/`basename $i .lz4` $dst/`basename $i .lz4`.sorted
    ### INSERT PCAP SANITY / INTEGRITY CHECKS HERE
    rm $dst/`basename $i .lz4`
    lz4 -9 -z -f $dst/`basename $i .lz4`.sorted $src/$i
    rm $dst/`basename $i .lz4`.sorted
done
