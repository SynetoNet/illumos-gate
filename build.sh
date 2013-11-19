#!/usr/bin/env bash
# Copyright (C) 2013 S.C. Syneto S.R.L.

WS=$(readlink -f `dirname $0`)
IPSFS=$WS/packages
ZFS=/usr/sbin/zfs

mv ${IPSFS} ${IPSFS}.orig
mkdir -p ${IPSFS}

/usr/gnu/bin/sed -e "s@^export CODEMGR_WS=.*@export CODEMGR_WS=$WS@" ./illumos.sh > ./illumos-local.sh

/opt/onbld/bin/nightly -n ./illumos-local.sh
RET=$?
if [ $RET -ne 0 ]; then
	rm -rf ${IPSFS}
	mv ${IPSFS}.orig ${IPSFS}
else
	rm -rf ${IPSFS}.orig
fi

exit $RET
