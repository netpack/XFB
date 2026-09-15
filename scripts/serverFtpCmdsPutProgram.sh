#!/bin/bash 
cd ../ftp/ 
ftp -v [IP] [PORT] << EOT 
prompt 
passive
prompt 
binary 
prompt 
cd Programs 
prompt 
mput *.ogg *.mp3 *.opus 
prompt 
bye 
EOT
