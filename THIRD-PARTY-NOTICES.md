# Composants tiers

Le code de Top Arena est distribué sous licence MIT (voir [`LICENSE`](LICENSE)). Les paquets
publiés (AppImage, installateur et zip Windows, dmg macOS) contiennent aussi les composants
suivants, chacun sous sa propre licence. Les textes complets sont dans [`licenses/`](licenses)
et dans le jeu (Aide > Licences).

## Qt 6 — GNU LGPL v3

Bibliothèques Qt (Core, Gui, Widgets, Network, Svg, DBus, plateforme XCB) et leurs greffons,
© The Qt Company Ltd. et les contributeurs de Qt. Licence : GNU Lesser General Public License
v3 ([`licenses/LGPL-3.0.txt`](licenses/LGPL-3.0.txt)), qui complète la GNU General Public
License v3 ([`licenses/GPL-3.0.txt`](licenses/GPL-3.0.txt)).

Top Arena est lié dynamiquement à Qt : les bibliothèques Qt sont des fichiers séparés (DLL
Windows, `usr/lib` de l'AppImage, frameworks du bundle macOS) que l'on peut remplacer par une
autre version compatible de Qt. Les sources de Qt sont disponibles sur
<https://download.qt.io/archive/qt/> ; la version embarquée est indiquée dans Aide > Licences.
Qt intègre lui-même des composants tiers (FreeType, HarfBuzz, libpng, PCRE2, zlib…), listés avec
leurs licences sur <https://doc.qt.io/qt-6/licenses-used-in-qt.html>.

## Polices Chakra Petch et Rajdhani — SIL Open Font License 1.1

- Chakra Petch : Copyright 2018 The Chakra Petch Project Authors
  (https://github.com/m4rc1e/Chakra-Petch.git) — [`src/ui/fonts/OFL-ChakraPetch.txt`](src/ui/fonts/OFL-ChakraPetch.txt).
- Rajdhani : Copyright (c) 2014, Indian Type Foundry (info@indiantypefoundry.com) —
  [`src/ui/fonts/OFL-Rajdhani.txt`](src/ui/fonts/OFL-Rajdhani.txt).

## Bibliothèques système de l'AppImage (Linux)

L'AppImage embarque des bibliothèques d'Ubuntu 22.04. Le fichier de copyright de chacune est
joint dans l'AppImage (`usr/share/doc/<paquet>/copyright`) ; leurs sources sont sur
<https://packages.ubuntu.com/jammy/>.

- GNU LGPL v2.1 ([`licenses/LGPL-2.1.txt`](licenses/LGPL-2.1.txt)) : GLib (glib, gio,
  gobject, gmodule, gthread), libgcrypt, libsystemd, libmount, libblkid, libkeyutils.
- Licences permissives (MIT, BSD, ICU/Unicode, libpng, AFL…) : libxcb et ses extensions,
  libxkbcommon, libXau, libXdmcp, ICU, libpng, PCRE, Kerberos (krb5, gssapi), Zstandard, LZ4,
  XZ (liblzma), Brotli, libffi, libbsd, libmd, libcap, libselinux, D-Bus.

## Windows

- Microsoft Visual C++ Redistributable (`vc_redist.x64.exe`), D3DCompiler
  (`d3dcompiler_47.dll`) et DXIL (`dxil.dll`) : fichiers redistribuables de Microsoft, soumis
  aux conditions de Microsoft.
- DirectX Shader Compiler (`dxcompiler.dll`) : © Microsoft Corporation, University of
  Illinois/NCSA Open Source License.
- L'installateur est construit avec Inno Setup, © Jordan Russell et Martijn Laan.
