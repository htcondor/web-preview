#!/bin/bash -p


if [ -z "$2" ]; then 
cat << EOFZZ
PluginVersion = "0.1"
PluginType = "FileTransfer"
SupportedMethods = "irods"
EOFZZ
exit 0
fi


cat .job.ad >> /tmp/nobody/zkm-p.$$
id >> /tmp/zkm-p.$$
id -ru >> /tmp/zkm-p.$$


echo $1 | grep -q '://'
if [ $? -eq 0 ]; then
  echo DO INPUT TRANSFER $1 TO $2 WITH PASWWORD $IRODS_PASS >> /tmp/zkm-p.$$
else
  echo DO OUTPUT TRANSFER $1 TO $2 WITH PASWWORD $IRODS_PASS >> /tmp/zkm-p.$$
fi


exit 0

