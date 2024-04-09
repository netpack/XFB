#!/bin/bash
cd ../ftp/
ftp -v [IP] [PORT] << EOT
prompt
passive
prompt
binary
cd Programs
ls
prompt
ls */
prompt
bye
EOT
