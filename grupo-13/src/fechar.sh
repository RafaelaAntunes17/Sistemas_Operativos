#!/bin/bash

# Caminho para o executável dclient
DCLIENT_PATH="/home/utilizador/Desktop/Universidade/3º ano/so/Sistemas_Operativos/grupo-13/dclient"

# Função para fechar o servidor
function fechar() {
    "$DCLIENT_PATH" -f
}

# Chamar a função fechar
fechar