#!/bin/bash
cd ../ftp/
ftp -v [IP] [PORT] << EOT
prompt
passive
prompt
binary
cd TakeOver
ls
prompt
bye
EOT
