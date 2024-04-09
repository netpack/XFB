#!/bin/bash 
cd ../ftp/ 
ftp -v [IP] [PORT] << EOT 
prompt 
passive
prompt 
binary 
prompt 
cd TakeOver 
prompt 
mput *.xml 
prompt 
bye 
EOT
