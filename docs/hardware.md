# Hardware e ligações — Inclinómetro OverlandingLoon

[← Voltar ao README](../README.md) · [Ver diagrama SVG](wiring.svg)

## Configuração suportada pelo firmware atual

- **Seeed Round Display for XIAO 104030087:** LCD GC9A01, 240 × 240, toque CHSC6X e RTC integrados.
- **Seeed XIAO ESP32-S3:** encaixa diretamente na parte traseira do ecrã.
- **MPU6050 / GY-521:** orientação (I²C).
- **BMP390 ou BMP388:** pressão e altitude barométrica (I²C).
- **GNSS opcional u-blox M10 I²C:** referência de altitude (por exemplo SparkFun SAM-M10Q).
- **Alimentação:** 5 V estabilizados via USB-C; 3,3 V para os módulos externos compatíveis.

> **Importante:** o sketch em `firmware/` implementa o GNSS **por I²C**, não por UART. Diagramas anteriores que atribuíam D6/D7 ao GPS **não são válidos para esta versão**, porque esses pinos já são utilizados pelo Round Display.

## Pinagem dos módulos externos

| XIAO ESP32-S3 | MPU6050 (GY-521) | BMP390 | u-blox M10 **I²C** (opcional) |
|---|---|---|---|
| **3V3** | VCC | VCC | VCC (se a placa suportar 3,3 V) |
| **GND** | GND | GND | GND |
| **D4 / GPIO5** | SDA | SDA | SDA |
| **D5 / GPIO6** | SCL | SCL | SCL |

Os dispositivos I²C partilham **SDA, SCL e GND**. Confirmar a tensão admitida pelo breakout específico e os pull-ups que já possui, para evitar excesso de resistências de pull-up. Os GPIO do ESP32-S3 **não são tolerantes a 5 V**.

### Recursos já usados pelo ecrã

| XIAO | Função do Round Display |
|---|---|
| D1 / GPIO2 | CS do LCD |
| D2 / GPIO3 | CS do cartão microSD (mantido inativo pelo firmware) |
| D3 / GPIO4 | DC do LCD |
| D4 / GPIO5 | SDA I²C (toque, RTC e sensores) |
| D5 / GPIO6 | SCL I²C |
| D6 / GPIO43 | Retroiluminação (interruptor KE em ON) |
| D7 / GPIO44 | Interrupção do toque CHSC6X |
| D8 / GPIO7 | SPI SCK do LCD |
| D10 / GPIO9 | SPI MOSI do LCD |

**Não ligar GPS UART aos pinos D6/D7 nesta implementação.** Para usar um módulo GPS apenas UART, seria necessário **alterar o firmware e reservar outros GPIO fisicamente acessíveis**, após confirmar possíveis conflitos de hardware.

### Endereços I²C esperados

| Dispositivo | Endereço |
|---|---|
| CHSC6X, toque integrado | `0x2E` |
| RTC integrado | `0x51` |
| MPU6050 (AD0 a 0) | `0x68` |
| BMP390/BMP388 | `0x77` (por vezes `0x76`) |
| u-blox M10 I²C (opcional) | `0x42` |

Se o BMP390 responder em `0x76`, alterar `BMP3_I2C_ADDRESS` no sketch.

## Alimentação automóvel

**Bateria 12 V do veículo → fusível próximo da origem → proteção automóvel adequada → conversor DC/DC 12 V/5 V → USB-C do XIAO.**

Não aplicar 12 V ou 14,8 V diretamente na alimentação do ecrã ou nos GPIO. Um simples conversor de bancada pode não suportar picos e transientes automóveis; usar um conversor concebido para essa aplicação. Evitar alimentar o mesmo circuito simultaneamente por USB e fonte externa sem estudar os caminhos de corrente.

Os módulos I²C recebem alimentação de **3V3**, respeitando o consumo total permitido pelo regulador e pelas placas utilizadas. Confirmar o consumo do GNSS, e verificar que cada breakout tolera 3,3 V.

## Implantação e calibração

1. Montar a **IMU rigidamente alinhada com os eixos do veículo** e protegida de vibrações excessivas.
2. Arrancar com o veículo **parado e nivelado**. O firmware atual calcula os desvios da IMU durante o arranque e considera essa posição como zero.
3. Instalar o **barómetro** num local protegido do vento direto, da ventilação e do calor do tablier. Não o encapsular hermeticamente.
4. Colocar o **GNSS** numa zona com boa visibilidade do céu e antena adequada ao módulo específico.
5. Verificar a orientação do mostrador quando a viatura inclina para a esquerda/direita e sobe/desce; corrigir os sinais no firmware se necessário.
6. Ensaiar em bancada e depois em condições controladas antes de uma utilização permanente.

## Limitações de leitura

A pressão apresentada é **pressão atmosférica local**, não pressão reduzida ao nível do mar. A altitude barométrica assume inicialmente **1013,25 hPa**; só melhora quando a referência é calibrada. O GNSS pode ajudar a corrigir lentamente o nível de referência, mas a precisão vertical de cada fixação varia. O filtro complementar do MPU6050 reduz ruído, mas **não resolve totalmente as acelerações longitudinais e laterais** em circulação.

## Documentação dos fabricantes

- [Seeed Round Display](https://wiki.seeedstudio.com/get_start_round_display/)
- [Seeed XIAO ESP32-S3](https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/)
- [Adafruit BMP3XX](https://learn.adafruit.com/adafruit-bmp388-bmp390-bmp3xx/arduino)
- [SparkFun SAM-M10Q](https://docs.sparkfun.com/SparkFun_u-blox_SAM-M10Q/introduction/)
