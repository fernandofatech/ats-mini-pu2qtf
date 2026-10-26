# Instalação

## 1. Descubra a variante do seu rádio (OSPI ou QSPI)

O ESP32-S3 do ATS-Mini vem com dois tipos de PSRAM, e cada um precisa de uma variante
do firmware. Com o rádio na USB:

```sh
uvx esptool --port /dev/cu.usbmodem* flash-id
```

Na linha `Features`:

| Aparece | Variante (em geral) |
|---|---|
| `Embedded PSRAM 2MB` (PSRAM *quad*, ex.: ESP32-S3R2 / N8R2) | **QSPI** |
| `Embedded PSRAM 8MB` (PSRAM *octal*, ex.: ESP32-S3R8 / N8R8) | **OSPI** |

Na dúvida, veja a tabela oficial em <https://esp32-si4732.github.io/ats-mini/flash.html>.
Com a variante certa, *Settings → About* mostra um valor de PSRAM diferente de zero. Com a
errada, o rádio mostra "PSRAM not detected" ao ligar. Nesse caso, grave a outra variante.

> O rádio do PU2QTF é **QSPI** (ESP32-S3 QFN56 com 2 MB de PSRAM embutida e flash de 8 MB).

## 2. Baixe o firmware

Na página [Releases](https://github.com/fernandofatech/ats-mini-pu2qtf/releases), baixe
`ats-mini-vX.YY-qspi.zip` (ou `-ospi.zip`) e descompacte. Dentro dele:

| Arquivo | Endereço |
|---|---|
| `ats-mini.ino.bootloader.bin` | `0x0` |
| `ats-mini.ino.partitions.bin` | `0x8000` |
| `boot_app0.bin` | `0xe000` |
| `ats-mini.ino.bin` | `0x10000` |

## 3. Grave

Antes, se quiser guardar o que está no rádio:

```sh
uvx esptool --chip esp32s3 --port /dev/cu.usbmodem* read-flash 0x0 0x800000 backup-8MB.bin
```

### Opção A: linha de comando (recomendada)

Para atualizar **mantendo memórias e configurações**, grave só a aplicação:

```sh
uvx esptool --chip esp32s3 --port /dev/cu.usbmodem* --baud 921600 \
  write-flash -z 0x10000 ats-mini.ino.bin
```

Para uma instalação completa (primeira vez, ou vindo de outro firmware):

```sh
uvx esptool --chip esp32s3 --port /dev/cu.usbmodem* --baud 921600 \
  --before default-reset --after hard-reset write-flash -z \
  --flash-mode keep --flash-freq keep --flash-size keep \
  0x0 ats-mini.ino.bootloader.bin 0x8000 ats-mini.ino.partitions.bin \
  0xe000 boot_app0.bin 0x10000 ats-mini.ino.bin
```

`uvx` vem do [uv](https://docs.astral.sh/uv/). Se não quiser instalá-lo, `pip install esptool`
e use `esptool` no lugar de `uvx esptool`.

### Opção B: pelo navegador (Chrome/Edge)

1. Abra <https://espressif.github.io/esptool-js/> e clique em **Connect**.
2. Escolha a porta do rádio.
3. Adicione os quatro arquivos nos endereços da tabela acima e clique em **Program**.

### Opção C: pelo Wi-Fi (já com esta edição instalada)

*Settings → Update FW → Check* e depois *Update*, ou a página **Update** em
`http://atsmini.local`. As versões vêm das releases deste repositório. Detalhes em
[uso.md](uso.md#atualização-pelo-wi-fi).

## 4. Primeiros ajustes recomendados (Brasil)

Pelo menu *Settings* ou pela página **Config** (`http://atsmini.local`):

| Ajuste | Valor | Por quê |
|---|---|---|
| UTC Offset | UTC-3 | Horário de Brasília (sem horário de verão) |
| FM Region | US (75 µs) | Padrão de pré-ênfase do FM no Brasil |
| RDS | ALL (US) | Mostra nome e texto das emissoras |
| USB Port | Ad hoc | Necessário para a ferramenta `ats-mini` |
| Wi-Fi | Connect / AP+Connect | Relógio pela internet, EiBi e atualizações |

Depois baixe a programação de ondas curtas: *Settings → Load EiBi* ou `ats-mini eibi`.

## 5. Teste rápido

```sh
pu2qtf/tools/ats-mini info      # Clock: set UTC / EiBi: loaded ...
pu2qtf/tools/ats-mini intro     # toca a abertura de novo
pu2qtf/tools/ats-mini tune 14100k usb && pu2qtf/tools/ats-mini screenshot beacon.bmp
```

## Voltar ao firmware oficial

Grave a release oficial em <https://github.com/esp32-si4732/ats-mini/releases> do mesmo
jeito, ou o seu `backup-8MB.bin` no endereço `0x0`.
