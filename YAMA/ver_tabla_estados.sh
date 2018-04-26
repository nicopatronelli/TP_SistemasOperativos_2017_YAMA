# Las líneas que empiezan por "#" son comentarios
# La primera línea o #! /bin/bash asegura que se interpreta como
# un script de bash, aunque se ejecute desde otro shell.

column -s, -t < tabla_estados_yama.csv | less -#2 -S
