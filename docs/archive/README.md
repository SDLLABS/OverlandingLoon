# Diagramas históricos (não utilizar para montagem)

O ficheiro `diagrama-antigo-UART-NAO-UTILIZAR.png` foi preservado apenas para referência histórica.

**Atenção: este desenho NÃO corresponde ao firmware atual.** Representa um recetor GNSS ligado por UART aos pinos D6/D7. No Seeed Round Display for XIAO, **D6 controla a retroiluminação e D7 é usado para a interrupção do toque**, pelo que essa ligação colide com a pinagem do ecrã. O firmware atualmente publicado suporta GNSS **u-blox M10 por I²C**, no mesmo barramento D4 (SDA) / D5 (SCL) do MPU6050 e do BMP390.

Para montar o dispositivo, utilizar **apenas** o [novo diagrama SVG](../wiring.svg) e a [pinagem oficial documentada](../hardware.md).
