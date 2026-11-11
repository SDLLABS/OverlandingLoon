# Car Inclinometer — versão com barómetro BMP390 e GNSS opcional

**Estado:** código atualizado e verificado estaticamente; **ainda não compilado nem ensaiado com o hardware real**.

O projeto original de LazyDays Creation foi disponibilizado com licença MIT (ver `LICENSE.md`). Mantiveram-se os recursos gráficos, o LVGL 8.3.11 e a lógica de inclinação. Além do porte original, esta versão acrescenta **uma segunda página, selecionável por toque**, com pressão ambiente e altitude barométrica, podendo utilizar um **recetor GNSS I²C u-blox M10** para corrigir lentamente a referência QNH.

## Hardware

- Seeed Round Display for XIAO, SKU **104030087**, LCD GC9A01 240x240, toque CHSC6X (0x2E)
- **Seeed XIAO ESP32-S3** (não ESP32 Dev Module genérico)
- **MPU6050** em placa adaptadora, com comunicação lógica a 3,3 V, endereço I2C 0x68
- **BMP390** (ou BMP388 compatível com Adafruit BMP3XX), breakout I2C 3,3 V, endereço **0x77** por omissão (noutros modelos, **0x76**)
- **Opcional:** recetor **u-blox M10 I2C** (ex.: SparkFun SAM-M10Q GPS-21834, endereço 0x42), com antena numa posição com céu visível
- Fonte **5 V regulada** via USB-C para alimentação (sem ligar diretamente 12 V de um veículo)
- O XIAO encaixa no conector traseiro do Round Display; os módulos MPU6050, BMP390 e GNSS partilham o I2C.

## Pinagem da placa de expansão

| XIAO | Função | GPIO real do ESP32-S3 |
|---|---|---|
| D1 | LCD CS | GPIO2 |
| D2 | microSD CS (mantido HIGH) | GPIO3 |
| D3 | LCD DC | GPIO4 |
| D4 | I2C SDA (partilhado) | GPIO5 |
| D5 | I2C SCL (partilhado) | GPIO6 |
| D6 | Retroiluminação (KE switch ON) | GPIO43 |
| D7 | Interrupção do toque | GPIO44 |
| D8 | LCD SPI SCK | GPIO7 |
| D10 | LCD SPI MOSI | GPIO9 |

**Ligações comuns I2C:** VCC → **3V3**; GND → **GND**; SDA → **D4**; SCL → **D5**.

| Dispositivo | Endereço I2C | Notas |
|---|---|---|
| MPU6050 | 0x68 | AD0 baixo |
| BMP390/BMP388 | 0x77 (ou 0x76) | Se usar 0x76, alterar `BMP3_I2C_ADDRESS` no `.ino` |
| u-blox M10 I2C, opcional | 0x42 | Não serve qualquer módulo GPS só com UART |
| Toque CHSC6X (integrado) | 0x2E | Já ligado pela placa |
| RTC (integrado) | 0x51 | Já ligado pela placa |

**Não ligar sinais de 5 V aos GPIO 3,3 V.** Ter atenção à quantidade de resistências pull-up I2C quando existem vários módulos; usar fios curtos. Em automóvel, colocar BMP390 ao abrigo do vento, da pressão dinâmica e de fontes fortes de calor; a pressão medida é a do interior/local do sensor.

## Instalação no Arduino IDE

1. Abrir a pasta `car-inclinometer-xiao-baro-gnss` e o ficheiro `car-inclinometer-xiao-baro-gnss.ino` (manter **todos** os `.c`, `.h` e `.hpp` juntos).
2. Instalar o pacote **esp32 by Espressif Systems**, a versão usada no manual original foi **3.3.3**.
3. Selecionar a placa **XIAO_ESP32S3** (nome na configuração do Arduino IDE pode aparecer como `Seeed XIAO ESP32S3`). **Não selecionar `ESP32S3 Dev Module`**, como aconselhava o manual para o hardware antigo.
4. Instalar as bibliotecas **lvgl 8.3.11**, **LovyanGFX**, **Adafruit MPU6050** e **Adafruit BMP3XX** (e Adafruit Unified Sensor / Adafruit BusIO se solicitadas). **Não instalar LVGL 9** para este projeto.
5. Se instalar GNSS I2C u-blox M10, instalar também **SparkFun u-blox GNSS v3** e mudar `#define USE_GNSS_I2C 0` para `1` no `.ino`. Sem GNSS, mantém-se a altitude estimada por barómetro com QNH padrão 1013,25 hPa.
6. Se o BMP390 tiver endereço 0x76, definir `BMP3_I2C_ADDRESS = 0x76`.
7. Confirmar que `lv_conf.h` do projeto é encontrado pelo LVGL (usa RGB565, LV_COLOR_DEPTH 16). Dependendo da instalação Arduino, poderá ser necessário colocá-lo no local de configuração indicado pelo LVGL.
8. Definir o interruptor **KE** da placa Round Display na posição **ON** para controlar a retroiluminação via D6. Ligar o XIAO por USB-C e carregar o código.
9. **Manter o MPU6050 imóvel e o veículo nivelado durante o arranque**: a calibração de origem define a posição inicial como zero.

## Diferenças relativamente ao original

- `LGFX_GC9A01.hpp`: pinos SPI/CS/DC corrigidos para XIAO, `RST=-1`, clock SPI de escrita a 40 MHz, SPI partilhado com microSD.
- `.ino`: D6 retroiluminação; D4/D5 I2C; D7 toque; D2 microSD CS; `Wire.begin(D4,D5)` explícito.
- Suporte mínimo do CHSC6X implementado no callback LVGL; a **interface original não tem botões ou ecrãs táteis**. A interação tátil fica pronta para funcionalidades futuras, mas neste porte não realiza nenhuma ação.
- Outros ficheiros de UI, imagens e lógica do filtro mantidos sem reformulação.

## Limitações e verificações necessárias

- Sem compilação nem ensaio na placa real: é uma **versão candidata**, não firmware validado.
- Confirmar orientação dos eixos na instalação física (constantes `ROLL_SIGN` e `PITCH_SIGN`).
- O filtro complementar original confunde inclinação com aceleração translacional durante a condução; **não é instrumento de segurança nem substitui avaliação real do terreno**.
- A calibração automática inicial não memoriza um zero calibrado; se o veículo arranca inclinado, esse ângulo fica definido como zero.
- O ecrã é pequeno (1,28 pol.); avaliar legibilidade à distância antes de montar permanentemente no painel.
- Evitar alimentação direta de 12–14,8 V, transientes automóveis e bateria de lítio em habitáculo quente sem desenho de segurança adequado. Usar conversor automóvel com proteção e 5 V estáveis.

## Documentação de referência

- Seeed — [Round Display hardware usage](https://wiki.seeedstudio.com/seeedstudio_round_display_usage/)
- Seeed — [Round Display getting started](https://wiki.seeedstudio.com/get_start_round_display/)
- Seeed — [XIAO ESP32-S3 pinout](https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/)
- Seeed — [CHSC6X driver](https://github.com/Seeed-Studio/Seeed_Arduino_RoundDisplay/blob/main/src/lv_xiao_round_screen.h)

## Licença

Código e recursos originais: copyright (c) 2026 LazyDays Creation, licença MIT. Consulte `LICENSE.md`.

## Interface e medições

- **Página 1:** inclinómetro original, com roll/pitch, ponteiros e desenhos dos veículos.
- **Página 2:** **pressão atmosférica local (hPa)**, **altitude estimada (m)**, referência atmosférica, dados GNSS (se ativado) e **QNH estimado**.
- **Alternar páginas:** tocar no ecrã. A leitura CHSC6X e a alternância dependem do hardware e necessitam de ensaio real.
- **Sem GNSS:** altitude barométrica calculada para pressão ao nível do mar de 1013,25 hPa (hipótese ISA); não confundir com altitude absoluta precisa.
- **Com GNSS:** apenas correções com fix 3D válido, verticalAcc <= 8 m e altitude MSL plausível. Após oito fixações boas, ajusta-se a referência; depois a atualização é lenta (máximo 0,03 hPa/segundo). A perda de GPS não interrompe o altímetro barométrico.
- A precisão relativa do BMP390 (mudanças de altitude) não corresponde a igual precisão de altitude absoluta.
- O valor QNH apresentado é **estimado pela fórmula de atmosfera padrão**; não constitui QNH meteorológico certificado. A referência GNSS perde-se ao desligar o equipamento (não existe persistência em flash nesta versão).
- O tempo de atualização do BMP3XX é 250 ms; a recolha GNSS é 1 s. O MPU6050 mantém o filtro a cerca de 100 Hz, salvo bloqueios pontuais das bibliotecas.

## Referências de hardware
- Bosch BMP390: https://www.bosch-sensortec.com/en/products/environmental-sensors/pressure-sensors/bmp390
- Adafruit BMP3XX: https://learn.adafruit.com/adafruit-bmp388-bmp390-bmp3xx/arduino
- SparkFun SAM-M10Q (GNSS I2C): https://docs.sparkfun.com/SparkFun_u-blox_SAM-M10Q/introduction/
- SparkFun GNSS v3: https://github.com/sparkfun/SparkFun_u-blox_GNSS_v3
