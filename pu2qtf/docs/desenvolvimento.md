# Desenvolvimento

## Ambiente

macOS (vale igual para Linux):

```sh
brew install arduino-cli uv          # compilador Arduino e uv (Python)
git clone https://github.com/fernandofatech/ats-mini-pu2qtf.git
cd ats-mini-pu2qtf
git remote add upstream https://github.com/esp32-si4732/ats-mini.git
```

Leia também o guia oficial: [docs/source/development.md](../../docs/source/development.md)
e as regras do projeto em [AGENTS.md](../../AGENTS.md).

## Compilar

As placas e as versões das bibliotecas estão fixadas em `ats-mini/sketch.yaml`. Na
primeira vez, o `arduino-cli` baixa o toolchain do ESP32 (alguns minutos).

```sh
cd ats-mini
arduino-cli compile --profile esp32s3-qspi --build-path build/qspi .   # rádio QSPI
arduino-cli compile --profile esp32s3-ospi --build-path build/ospi .   # rádio OSPI
```

Os binários ficam em `build/<variante>/` (`ats-mini.ino.bin`, `*.bootloader.bin`, `*.partitions.bin`, `boot_app0.bin`).

## Gravar durante o desenvolvimento

Só a aplicação, mantendo memórias e configurações:

```sh
uvx esptool --chip esp32s3 --port /dev/cu.usbmodem* --baud 921600 \
  write-flash -z 0x10000 build/qspi/ats-mini.ino.bin
```

## Testar

```sh
make -C ats-mini test        # testes unitários do parser de comandos remotos
uv run prek run --all-files  # verificações do repositório (lint)
```

No rádio (com *USB Port → Ad hoc*):

```sh
T=pu2qtf/tools/ats-mini
$T info                          # Clock: set UTC / EiBi: loaded
$T intro                         # abertura
$T tune 14100k usb               # deve mostrar "Beacon <indicativo> <local>"
$T screenshot /tmp/tela.bmp      # inspecione a tela
$T tune 9970k                    # com a EiBi carregada: "Radio Casa AM"
```

Para gravar um GIF da abertura: envie `Q` e, em seguida, peça screenshots binários (`c`)
em sequência. Durante a abertura, cada `c` devolve o quadro atual (108866 bytes por BMP).

## Onde está cada coisa

| Arquivo | Conteúdo |
|---|---|
| `ats-mini/Intro.h` | **Configuração da abertura**: indicativo, texto em CW, mensagem da faixa, velocidade |
| `ats-mini/Intro.cpp` | Abertura: tarefa "keyer" (temporização do CW) e animação (antena, decodificador, cascata, faixa, tela 73) |
| `ats-mini/Station.cpp` | `findBeaconByFreq()`: identificação dos beacons NCDXF; prioridade do beacon sobre o nome da memória |
| `ats-mini/EIBI.cpp` | `eibiLastResult()` / `eibiFileSize()`: diagnóstico do download da EiBi |
| `ats-mini/ParserAdhoc.*`, `Remote.cpp` | Comandos remotos `Q`, `?`, `J` e screenshot durante a abertura |
| `ats-mini/Common.h` | `FIRMWARE_URL`: repositório consultado pelo *Update FW* |
| `changelog/+*.md` | Fragmentos de changelog (towncrier) |
| `pu2qtf/` | Documentação, ferramenta, memórias, varreduras e screenshots desta edição |

## Personalizar para outro indicativo

Edite `ats-mini/Intro.h`:

```c
#define INTRO_CALLSIGN   "PU2QTF"
#define INTRO_CW_TEXT    "CQ CQ CQ DE PU2QTF K"   // A-Z, 0-9, espaço, / ? . =
#define INTRO_MARQUEE    "Shack de escuta PU2QTF  *  ..."
#define INTRO_WPM        25
```

O grid locator e o subtítulo estão em `drawFrame()` (`Intro.cpp`). Se for publicar seu
próprio fork, troque `FIRMWARE_URL` em `Common.h` pelo endereço do seu repositório.

## Acompanhar o firmware oficial

Quando sair uma versão nova do firmware oficial (ex.: v2.46):

```sh
git fetch upstream --tags
git checkout main
git rebase v2.46            # reaplica as mudanças PU2QTF sobre a versão nova
# resolva conflitos, se houver, e então:
make -C ats-mini test
cd ats-mini && arduino-cli compile --profile esp32s3-qspi --build-path build/qspi . && cd ..
# teste no rádio e então:
git push --force-with-lease origin main
```

Os conflitos mais prováveis são em `ParserAdhoc.*`, `Remote.cpp` e `Station.cpp`, os
arquivos oficiais que esta edição altera. `Intro.*` é só desta edição.

## Publicar uma versão (release)

O *Update FW* do rádio aceita tags no formato `vX.YY` e procura o arquivo
`ats-mini-vX.YY-<variante>-ota.bin`. Use sempre o **mesmo número da versão oficial** em
que a edição se baseia.

```sh
V=v2.46
cd ats-mini
for v in qspi ospi; do arduino-cli compile --profile esp32s3-$v --build-path build/$v .; done
cd ..
mkdir -p dist
for v in qspi ospi; do
  d=dist/ats-mini-$V-$v; mkdir -p $d
  cp ats-mini/build/$v/ats-mini.ino.bin ats-mini/build/$v/ats-mini.ino.bootloader.bin \
     ats-mini/build/$v/ats-mini.ino.partitions.bin ats-mini/build/$v/boot_app0.bin CHANGELOG.md $d/
  cp ats-mini/build/$v/ats-mini.ino.bin dist/ats-mini-$V-$v-ota.bin
  (cd dist && zip -qr ats-mini-$V-$v.zip ats-mini-$V-$v)
done
git tag -f $V && git push -f origin $V
gh release create $V dist/*.zip dist/*-ota.bin --title "$V PU2QTF" --notes "..."
```

O GitHub Actions oficial (`.github/workflows/build.yml`) também compila tudo. Em forks ele
vem desligado: ative em *Actions* no GitHub se quiser builds automáticos a cada push.

## Ideias para evoluir

- Escolher o tema e a mensagem da abertura pela página web *Config*.
- Opção no menu para ligar/desligar a abertura ou deixar só o indicativo.
- Mapa de propagação simples a partir dos beacons ouvidos (registrar RSSI por beacon e banda).
- Faixas de radioamador (CW/FT8/SSB) da IARU Região 2 mostradas sob a frequência.
- Áudio na abertura com uma pequena modificação de hardware (ver [experimento-audio-rf.md](experimento-audio-rf.md)).
