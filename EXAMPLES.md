# Exemplos de Uso - Monitor Health Checker

Este documento demonstra como usar o Monitor Health Checker para SONOFF MINI.

## Fluxo de Configuração

### 1. Primeira Inicialização
```bash
# Compilar e fazer flash
make flash

# Monitorar saída serial
make monitor
```

**Saída Esperada:**
```
I (xxx) MAIN: Starting Monitor Health Checker
I (xxx) MAIN: Device not configured, entering config mode
I (xxx) WIFI_MANAGER: Starting WiFi AP mode
I (xxx) CONFIG_SERVER: Starting configuration server
```

### 2. Conectar à Rede de Configuração
- **SSID**: `SONOFF-Monitor`
- **Senha**: `12345678`
- **IP do Device**: `192.168.4.1`

### 3. Configurar via Interface Web
Acesse `http://192.168.4.1` e configure:

```json
{
  "wifi_ssid": "MinhaRedeWiFi",
  "wifi_password": "minhaSenha123",
  "health_check_url": "http://192.168.1.100:8080/health",
  "check_interval": 30
}
```

### 4. Funcionamento Normal
Após configuração, o device:
- Conecta à rede WiFi configurada
- Inicia monitoramento da URL
- Controla o relé baseado no resultado

**URL**: `http://192.168.4.1`

### Exemplo de Configuração

```
WiFi SSID: MinhaRede
WiFi Password: minhaSenha123
Health Check URL: http://192.168.1.100:8080/health
Check Interval: 30 segundos
```

## Configuração via API REST

### Exemplo usando curl:

```bash
# Obter configuração atual
curl -X GET http://192.168.4.1/config

# Configurar dispositivo
curl -X POST http://192.168.4.1/config \
  -H "Content-Type: application/json" \
  -d '{
    "wifi_ssid": "MinhaRede",
    "wifi_password": "minhaSenha123", 
    "health_check_url": "http://192.168.1.100:8080/health",
    "check_interval": 30000
  }'

# Verificar status
curl -X GET http://192.168.4.1/status
```

## Exemplo de Endpoint Health Check

Seu servidor deve implementar um endpoint que retorna status HTTP 200 para indicar que está funcionando:

### Exemplo em Python/Flask:
```python
from flask import Flask
app = Flask(__name__)

@app.route('/health')
def health_check():
    return {'status': 'ok'}, 200

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=8080)
```

### Exemplo em Node.js/Express:
```javascript
const express = require('express');
const app = express();

app.get('/health', (req, res) => {
  res.status(200).json({ status: 'ok' });
});

app.listen(8080, () => {
  console.log('Server running on port 8080');
});
```

## Comportamento do Sistema

### Relé Ligado (Status OK)
- Health check retorna HTTP 200
- Relé GPIO12 = HIGH
- LED vermelho aceso

### Relé Desligado (Status Erro)
- Health check retorna erro ou timeout
- Relé GPIO12 = LOW  
- LED vermelho apagado

### Modo Configuração
- LED azul GPIO13 = HIGH
- AP mode ativo: SONOFF-Monitor
- Servidor web disponível

## Monitoramento via Serial

Para debugar, conecte um adaptador serial e monitore:

```bash
idf.py monitor
```

Logs disponíveis:
- Conexão WiFi
- Resultados do health check
- Estados do relé
- Mudanças de modo
