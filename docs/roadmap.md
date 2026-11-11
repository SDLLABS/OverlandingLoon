# Roadmap e validação

[← README](../README.md)

Este plano distingue o **código implementado** das funcionalidades **realmente validadas**. Uma caixa por marcar não significa falha: significa que ainda não há registo de conclusão no repositório.

## Fase 1 — Base de hardware e compilação

- [ ] Confirmar part numbers concretos do ecrã, XIAO, BMP390, MPU6050 e, se existir, GNSS.
- [ ] Compilar com Arduino ESP32 core compatível, **LVGL 8.3.11**, LovyanGFX e bibliotecas Adafruit.
- [ ] Confirmar interface GC9A01, inicialização e retroiluminação.
- [ ] Validar leitura do toque CHSC6X e alternância das páginas.
- [ ] Verificar varrimento I²C, endereços e integridade elétrica de 3,3 V.

## Fase 2 — Sensores

- [ ] Validar orientações e sinais de pitch/roll com movimentos conhecidos.
- [ ] Medir deriva, vibração, aceleração longitudinal e transversal.
- [ ] Comparar BMP390 com referência de pressão; testar `0x77` / `0x76`.
- [ ] Validar GNSS M10 **I²C** com fix 3D e avaliação de precisão vertical.
- [ ] Ensaiar filtro e transição entre altitude barométrica e referência GNSS.
- [ ] Definir calibração de zero manual e persistência segura em flash.

## Fase 3 — Integração automóvel

- [ ] Selecionar conversor 12 V/5 V com proteção automóvel e fusível.
- [ ] Medir consumo, temperatura, arranque/desligar e transientes.
- [ ] Fixar a IMU em posição reprodutível relativamente ao veículo.
- [ ] Avaliar legibilidade do ecrã à luz do sol e durante a condução.
- [ ] Realizar testes controlados sem depender das leituras para segurança.

## Fase 4 — Produto

- [ ] Separar módulos de sensores, interface e configurações para manutenção.
- [ ] Adicionar telemetria de diagnóstico e deteção de sensores ausentes.
- [ ] Considerar alarmes configuráveis, sem lhes atribuir carácter de segurança certificada.
- [ ] Criar instruções de caixa, instalação e manutenção.
- [ ] Automatizar compilação e validação de versões de dependências.

## Contribuições

Contribuições e correções são bem-vindas por pull request. Descrever o hardware utilizado, os testes realmente realizados e as versões das bibliotecas. Não marcar uma funcionalidade como validada sem evidência reproduzível.
