# Arquitetura — OverlandingLoon Inclinometer

[← README](../README.md)

## Módulos

1. **XIAO ESP32-S3:** inicialização e ciclo Arduino, I²C, SPI, filtragem, cálculo de altitude e interface.
2. **Round Display 104030087:** LCD GC9A01 via SPI e toque CHSC6X via I²C. O firmware usa LVGL 8.3.11 e LovyanGFX.
3. **MPU6050:** acelerómetro/giroscópio; o filtro complementar estima `roll` e `pitch` (aprox. 100 Hz no código).
4. **BMP390/BMP388:** pressão atmosférica em hPa; a altitude barométrica é calculada segundo um modelo de atmosfera padrão (leitura aproximadamente a cada 250 ms).
5. **GNSS u-blox M10 I²C (opcional):** altitude relativamente ao nível médio do mar e estimativa de precisão vertical; usado para ajustar lentamente a referência barométrica quando existem soluções adequadas.
6. **LVGL:** duas páginas, inclinómetro e ambiente; toque alterna entre ambas.

## Fluxo lógico

```text
MPU6050 (I²C) ──> calibração + filtro complementar ──> pitch/roll ──┐
                                                                     │
BMP390 (I²C) ───> pressão (hPa) ──> altitude barométrica ─────────────┼──> LVGL ─> GC9A01
                                         ▲                           │
GNSS M10 (I²C) ─> validação de fixação ─> referência QNH estimada ───┘
                                     (opcional)
Toque CHSC6X (I²C) ──> alternância das páginas
```

## Algoritmos e pressupostos

- **Inclinação:** o filtro complementar combina integração do giroscópio e ângulo derivado da gravidade; acelerações do veículo afetam a leitura.
- **Altitude:** `h ≈ 44330 × (1 − (P/P0)^0,19029495)`, com `P` em hPa e `P0` como pressão de referência. É uma aproximação de atmosfera padrão, não um altímetro aeronáutico calibrado.
- **GNSS opcional:** no código atual, `USE_GNSS_I2C` vem desligado. Quando ligado, são aplicadas condições mínimas de fix 3D, precisão vertical reportada, plausibilidade de altitude e suavização da referência.
- **Persistência:** a referência obtida por GNSS **não é guardada** em memória não volátil; cada arranque começa com referência padrão.
- **Interface:** o ecrã circular 240 × 240 mostra uma página de inclinação e outra de ambiente, alternadas por toque.

## Estado e riscos técnicos

Este repositório **não contém provas de compilação integrada, ensaios funcionais em hardware real ou testes em viatura**. Verificar conflitos de pinos e as versões concretas das bibliotecas antes de dar o protótipo como operacional. Esta interface não deve ser usada como sistema de prevenção de capotamento ou para decisões críticas de condução.

[Plano de testes e evolução →](roadmap.md)
