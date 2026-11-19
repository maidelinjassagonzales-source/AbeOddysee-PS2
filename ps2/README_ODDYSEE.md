# Oddworld: Abe's Oddysee – Port para PlayStation 2

Port no oficial de **Oddworld: Abe's Oddysee** a PlayStation 2, basado en el proyecto de
descompilación [R.E.L.I.V.E.](https://github.com/AliveTeam/alive_reversing) (AliveTeam). Es el
hermano del port de Abe's Exoddus y comparte con él todo el código de PS2.

**Port creado por Aroy24 y William_piraton**, con ayuda de IA (Claude) para el código.

> Este repositorio contiene **solo código**. No incluye ningún archivo del juego.
> Para jugar necesitas tu propia copia de Abe's Oddysee para PC (por ejemplo, la de Steam).

## Updates

### v1.0 (19-11-2026)

- Primera versión de Abe's Oddysee para PS2, con todo lo del port de Exoddus (ver más abajo).
- **Cinemáticas a 15 fps**: las de Oddysee usan el formato de la PSX (vídeo MDEC y sonido XA), y su
  decodificador hacía las cuentas con `double`, que el EE de la PS2 emula por software: iban a
  4 fps. Ahora usa enteros y van a su velocidad normal, con la lectura anticipada del disco.
- **Botones del DualShock** en los letreros de ayuda y las pantallas LCD (R1, L1, CROSS...) en
  lugar de los del mando de Xbox (RB, LB, A...). También se aplica al port de Exoddus.
- **5 idiomas en un disco** (`DATA/EN`, `ES`, `FR`, `DE`, `IT`): el juego elige el idioma de la
  consola. Si el disco no lo lleva, usa el inglés, luego el español y luego lo que haya en `DATA/`.
  Con los 5 idiomas la ISO ocupa 2,3 GB (DVD de una capa).

## Cómo jugar (Windows)

1. Ten instalado **Oddworld: Abe's Oddysee** en Steam.
2. Descarga el zip de la página de *Releases* y descomprímelo (lleva `ODDYSEE.ELF`, `CrearISO.bat`,
   `CrearISO.ps1` y `Ps2Iso.cs`).
3. Doble clic en `CrearISO.bat`. Aparece `SLUS_999.97.ABE_ODDYSEE_PS2.iso` en la misma carpeta, con
   el idioma que tengas puesto en Steam.
4. Cárgala en OPL (HDD, USB o SMB) o en PCSX2.

Para meter más idiomas, cambia el idioma del juego en Steam, copia los archivos `*.lvl` y `*.ddv` a
otra carpeta y añádela: `CrearISO.bat -Extra EN=C:\ruta\ingles,FR=C:\ruta\frances`.

## Novedades de la versión PS2

- **60 FPS**: entre cada paso de la lógica del juego (30 por segundo) se dibuja un fotograma con
  todo a medio camino.
- **Cooperativo para 2 jugadores con dos Abes**: el jugador 2 pulsa START en el mando 2 para unirse
  (en una esquina aparece "PULSA START PARA UNIRTE") y su Abe sale azul. Puede cantar y poseer
  criaturas; la cámara sigue al jugador 1 y, si el jugador 2 se queda fuera de la pantalla o muere,
  vuelve junto al jugador 1 sin reiniciar el nivel. Para salir, mantiene START pulsado.
- **Intro de créditos del port** antes del logo de Oddworld Inhabitants.
- Colores corregidos para el GS de la PS2, mezclador de audio propio, vibración del mando con los
  temblores de pantalla y partidas guardadas en la tarjeta de memoria (`mc0:/ODDYSEE`).
- Funciona como un juego más en **OPL**: ISO instalada en el disco duro interno, en USB o por SMB.

## Estado

| Plataforma | Estado |
|---|---|
| PCSX2 | Arranque, cinemáticas, menú, Rupture Farms a 60 FPS y cooperativo probados. ISO de 5 idiomas probada |
| PS2 real (OPL) | Sin probar todavía |

Pendiente de probar: guardar y cargar en la tarjeta de memoria, la vibración y la posesión de
criaturas por el jugador 2. La memoria libre jugando en Rupture Farms es de unos 4,7 MB.

## Compilar

Mismo toolchain que el port de Exoddus (ps2dev en WSL Ubuntu 24.04 y SDL3 con los parches de
`ps2/scripts`). Cada juego es un ELF y un disco distintos (los dos motores no caben juntos en los
32 MB de la PS2): Oddysee se elige con la opción de CMake `-DRELIVE_PS2_GAME=AO`.

1. `build-ps2-ao.sh`: versión de pruebas para PCSX2 (lee los datos con `host:`), `out/ao/ODDYSEE.ELF`.
   `build-ps2-ao.sh disc`: versión para consola real e ISO, `out/ao/disc/ODDYSEE.ELF`.
2. `run-pcsx2-ao.ps1`: la prueba en PCSX2 (minimizado; con `-Iso` arranca una ISO).
3. `GAME=AO make-iso.sh`: genera la ISO `SLUS_999.97` (volumen `ABE_ODDYSEE`). Para varios idiomas,
   pon cada versión del juego en una carpeta `EN`, `ES`, `FR`, `DE` o `IT` dentro de `$LANGS`.
   La tabla de rutas del ISO 9660 va en el sector 257, donde la busca la BIOS en los DVD.

Las rutas de los scripts apuntan a la máquina donde se desarrolló el port. Cámbialas por las tuyas.

## Licencia

El código de R.E.L.I.V.E. está sujeto a la licencia de su proyecto original. Oddworld y Abe's Oddysee
son marcas de Oddworld Inhabitants. Este proyecto no está afiliado a Oddworld Inhabitants.
