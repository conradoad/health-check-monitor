# Monitor Health Checker - ESP8266_RTOS_SDK Project

Este projeto implementa um monitor de health check para o SONOFF MINI que alterna entre modo de configuração e modo de execução.

## Funcionalidades

### Modo Configuração
- **Ativação**: Pressione o botão por 5 segundos
- **Rede AP**: `SONOFF-Monitor` (senha: `12345678`)
- **Interface Web**: Configuração via `http://192.168.4.1`
- **LED Azul**: Indicador de modo configuração ativo

### Modo Execução  
- **Monitoramento**: Verifica health check periodicamente
- **Controle de Relé**: Liga/desliga baseado no status HTTP
- **Status 200 OK**: Relé ligado
- **Erro HTTP**: Relé desligado

## Hardware (SONOFF MINI)

| Pin | Função |
|-----|--------|
| GPIO00 | Botão (entrada configuração) |
| GPIO01 | TX |
| GPIO02 | Disponível |
| GPIO03 | RX |
| GPIO04 | S2 (entrada switch externo) |
| GPIO12 | Relé e LED vermelho |
| GPIO13 | LED azul |
| GPIO16 | Jumper OTA |
| GND | S1 (entrada switch externo) |

## Configuração via API REST

### GET /config
Retorna configuração atual

### POST /config
Configura parâmetros do dispositivo:
```json
{
  "wifi_ssid": "MinhaRede",
  "wifi_password": "minhaSenha",
  "health_check_url": "http://example.com/health",
  "check_interval": 30
}
```

### GET /status
Retorna status do dispositivo

## Compilação

```bash
# Configurar ESP8266_RTOS_SDK
export IDF_PATH=$HOME/esp/ESP8266_RTOS_SDK
source $IDF_PATH/export.sh

# Configurar projeto
make menuconfig

# Compilar
make

# Flash
make flash

# Monitor
make monitor
```

## Configuração ESP8266_RTOS_SDK

Configure no `make menuconfig`:

1. **Serial flasher config**
   - Flash size: 1MB
   - Flash frequency: 40MHz
   - Flash mode: DIO

2. **Component config → Wi-Fi**
   - WiFi static RX buffer number: 10
   - WiFi dynamic RX buffer number: 32

3. **Component config → HTTP Server**
   - Max HTTP Request Header Length: 1024
   - Max HTTP URI Length: 512

## Uso

1. **Primeira configuração**:
   - Pressione o botão por 5s
   - Conecte à rede `SONOFF-Monitor`
   - Acesse `http://192.168.4.1`
   - Configure WiFi e URL de health check

2. **Funcionamento normal**:
   - Device conecta à rede configurada
   - Monitora URL periodicamente
   - Controla relé baseado no resultado

3. **Reconfiguração**:
   - Pressione o botão novamente por 5s
   - Processo volta ao modo configuração

## Estrutura do Projeto

```
main/
├── main.c              # Aplicação principal
├── config.h            # Configurações e constantes
├── wifi_manager.c/h    # Gerenciamento WiFi
├── config_server.c/h   # Servidor HTTP configuração
├── health_checker.c/h  # Monitor de health check
├── gpio_control.c/h    # Controle GPIO
├── component.mk        # Build configuration
└── CMakeLists.txt      # CMake configuration
```

## Dependências

- ESP8266_RTOS_SDK v3.4+
- Bibliotecas: WiFi, HTTP Client, HTTP Server, JSON, NVS

## Licença

Este projeto está sob domínio público.
