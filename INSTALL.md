# Instruções de Instalação do ESP8266_RTOS_SDK

## Pré-requisitos

Este projeto requer o ESP8266_RTOS_SDK (ESP8266 Real-Time Operating System SDK) instalado no sistema.

### Instalação do ESP8266_RTOS_SDK

#### 1. Instalar dependências
```bash
sudo apt-get update
sudo apt-get install git wget flex bison gperf python3 python3-pip python3-setuptools cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0
```

#### 2. Clonar o ESP8266_RTOS_SDK
```bash
mkdir -p ~/esp
cd ~/esp
git clone --recursive https://github.com/espressif/ESP8266_RTOS_SDK.git
cd ESP8266_RTOS_SDK
git checkout v3.4  # Use a versão estável mais recente
```

#### 3. Instalar ferramentas
```bash
cd ~/esp/ESP8266_RTOS_SDK
./install.sh
```

#### 4. Configurar variáveis de ambiente
```bash
# Adicionar ao ~/.bashrc ou ~/.zshrc
echo 'export IDF_PATH=$HOME/esp/ESP8266_RTOS_SDK' >> ~/.bashrc
echo 'source $IDF_PATH/export.sh' >> ~/.bashrc
source ~/.bashrc
```

## Compilação do Projeto

Uma vez instalado o ESP8266_RTOS_SDK:

```bash
cd /home/conrado/health-check-monitor

# Configurar variáveis de ambiente (se não estiver no .bashrc)
export IDF_PATH=$HOME/esp/ESP8266_RTOS_SDK
source $IDF_PATH/export.sh

# Configurar projeto (opcional)
make menuconfig

# Compilar
make

# Flash (conectar o dispositivo via USB)
make flash

# Monitor serial
make monitor
```

## Configuração do Hardware

### SONOFF MINI
- Conectar via cabo serial TTL (3.3V)
- Pressionar botão durante boot para entrar em modo flash
- Usar adaptador USB-Serial compatível

### Pinout para programação:
- VCC: 3.3V
- GND: GND  
- TX: GPIO1
- RX: GPIO3

## Resolução de Problemas

### Erro de permissão USB
```bash
sudo usermod -a -G dialout $USER
# Reiniciar sessão
```

### Problemas de compilação
```bash
# Limpar build
make clean

# Reconfigurar completamente
make distclean
make
```
