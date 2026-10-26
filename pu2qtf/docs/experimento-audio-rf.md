# Experimento: CW audível na abertura via RF

A ideia original era ouvir o "CQ CQ CQ DE PU2QTF" no alto-falante ao ligar o rádio. Este
documento registra o que foi tentado, o que foi medido e por que a abertura ficou só visual.

## O problema

No ATS-Mini todo o áudio sai do SI4732. O ESP32-S3 controla apenas o mute (`AUDIO_MUTE`,
GPIO3) e a habilitação do amplificador (`PIN_AMP_EN`, GPIO10). Não existe DAC nem PWM do
processador ligado ao alto-falante, e o clamp de áudio é só um transistor que curto-circuita a
linha: ele não gera tom.

## A ideia: transmitir para o próprio receptor

1. O periférico **RMT** do ESP32-S3 gera uma portadora de RF num pino (até 40 MHz, derivada do
   clock de 80 MHz; 4 MHz = 20 ciclos e 8 MHz = 10 ciclos, frequências exatas).
2. A portadora é ligada e desligada 700 vezes por segundo (AM com onda quadrada).
3. O SI4732, sintonizado em AM na mesma frequência, demodula isso como um **tom de 700 Hz**.
4. Chaveando o tom no ritmo do Morse, sai CW de verdade, transmitido e recebido dentro do rádio.

## O que foi medido

Medição pelo próprio rádio (RSSI em dBµV via comando remoto), com o gerador desligado e ligado:

| Portadora | Pino | Sem sinal | Com sinal |
|---|---|---|---|
| 4000 kHz | GPIO13, 11, 12, 14, 43, 44 (sem trilha) e 38 (backlight) | 44–47 | 45–49 (sem diferença) |
| 8000 kHz | os mesmos | 34–35 | 33–37 (sem diferença) |
| 8000 kHz | GPIO17 = SCL do I2C, que entra direto no SI4732 | 33–35 | 35–36 (+1 a 3 dB) |

- O piso de ruído em 4 MHz é alto (cerca de 47 dBµV, provavelmente harmônicos do clock de 80 MHz).
  Em 8 MHz ele cai para 34 dBµV.
- Um autoteste lendo o próprio pino confirmou que o RMT gerava o sinal: o pino ficava em nível
  alto 22–28% do tempo, o esperado para portadora a 50% × chaveamento a 50%.
- Pinos sem trilha praticamente não irradiam. A linha SCL acopla um pouco (sem condição de START
  no I2C o SI4732 ignora os pulsos, e o barramento é restaurado depois), mas **só 1–3 dB acima do
  ruído**. Na prática não dá para ouvir.

## Conclusão

Sem modificar o hardware, a abertura fica **visual**. O código do gerador foi removido da
edição para não deixar a linha I2C em uso fora do padrão.

## Se quiser tentar com hardware

Solde um fio fino de 10–20 cm num pino livre do módulo (GPIO11–14; atenção, os pads do módulo
ESP32-S3-WROOM são pequenos) e passe a ponta perto da entrada de antena. Assim a portadora deve
aparecer com folga acima do ruído. O núcleo do gerador usado no experimento:

```cpp
#include "esp32-hal-rmt.h"

#define CW_RF_PIN       13     // pino livre (com o fio soldado)
#define CW_CARRIER_KHZ  8000   // 80 MHz / 10: exato e com pouco ruído

static rmt_data_t toneSymbol;

static bool rfInit()
{
  // Ticks de 1 us no canal; a portadora vem do clock de grupo de 80 MHz
  if(!rmtInit(CW_RF_PIN, RMT_TX_MODE, RMT_MEM_NUM_BLOCKS_1, 1000000)) return false;
  if(!rmtSetCarrier(CW_RF_PIN, true, false, CW_CARRIER_KHZ * 1000UL, 0.5)) return false;
  return rmtSetEOT(CW_RF_PIN, 0);
}

// Portadora chaveada na frequência do tom: o SI4732 em AM ouve o tom
static void toneOn(uint16_t hz)
{
  uint16_t half = 500000UL / hz;
  toneSymbol.duration0 = half; toneSymbol.level0 = 1;
  toneSymbol.duration1 = half; toneSymbol.level1 = 0;
  rmtWriteLooping(CW_RF_PIN, &toneSymbol, 1);
}

static void toneOff()
{
  rmtWriteLooping(CW_RF_PIN, NULL, 0);
}

// Antes de tocar: rx.setAM(CW_CARRIER_KHZ - 100, CW_CARRIER_KHZ + 100, CW_CARRIER_KHZ, 1);
```
