# LAN865x Debugfs Interface

Dieses Dokument beschreibt die Verwendung des debugfs Interfaces im LAN865x 10BASE-T1S MAC-PHY Treiber.

## Funktionsweise des Debug-Mechanismus

⚡ **Paralleler und unabhängiger Betrieb:**

Der Debug-Mechanismus läuft vollständig parallel zur normalen Treiberfunktion:

- Der **normale Ethernet-Treiber** funktioniert völlig unabhängig - Netzwerk-Traffic, MAC-Konfiguration, Multicast-Handling etc. laufen weiter
- Das **debugfs Interface** ist ein zusätzlicher, separater Kanal nur für Debugging-Zwecke  
- **Keine Interferenz**: Debug-Zugriffe beeinträchtigen nicht den normalen Netzwerkbetrieb

🔒 **Zugriffsschutz durch debug_enabled Flag:**

Der Zugriff auf Register ist durch den `debug_enabled` Schalter geschützt:

```bash
# Debug aktivieren → Register-Zugriffe erlaubt
echo 1 > /sys/kernel/debug/lan865x/debug_enable

# Debug deaktivieren → Alle Register-Zugriffe blockiert  
echo 0 > /sys/kernel/debug/lan865x/debug_enable
```

**Sicherheitsaspekt:**
- **Produktionsumgebung**: Debug deaktiviert → Keine unauthorized Register-Manipulation
- **Entwicklung/Testing**: Debug aktiviert → Vollzugriff für Diagnose
- **Zur Laufzeit umschaltbar**: Flexibel je nach Bedarf

## Übersicht

Das debugfs Interface bietet eine umfassende Schnittstelle zum Debugging des LAN865x Ethernet-Treibers zur Laufzeit. Es ermöglicht direkten Zugriff auf Hardware-Register und bietet detaillierte Statusinformationen.

## Debugfs-Struktur

Das Interface erstellt ein debugfs-Verzeichnis unter `/sys/kernel/debug/lan865x/` mit folgenden Dateien:

- `regs` - Register Lese-/Schreibzugriff
- `debug_enable` - Debug-Status ein/ausschalten (boolean)

## Systemvoraussetzungen

- Linux Kernel mit CONFIG_DEBUG_FS aktiviert
- Root-Berechtigung für debugfs Zugriff
- **Kernel-Modul-Unterstützung** aktiviert
- LAN865x Treiber als **ladbares Modul** kompiliert

## Modul-Management

Der LAN865x Treiber ist als Kernel-Modul implementiert für flexible Entwicklung und Testing.

### Automatisches Laden beim Boot

```bash
# Module werden automatisch beim Systemstart geladen
/etc/init.d/lan865x-modules start
```

### Manuelles Modul-Management

```bash
# Module laden
modprobe oa_tc6     # OA-TC6 Bibliothek (Abhängigkeit)
modprobe lan865x    # LAN865x Treiber

# Module status prüfen
lsmod | grep -E "(oa_tc6|lan865x)"
/etc/init.d/lan865x-modules status

# Module entladen (für Entwicklung)
rmmod lan865x
rmmod oa_tc6

# Schneller Reload für Testing
/etc/init.d/lan865x-modules reload
```

### Entwickler-Workflow

```bash
# 1. Neue Modul-Version kopieren
cp lan865x.ko /lib/modules/$(uname -r)/kernel/drivers/net/ethernet/microchip/

# 2. Module-Cache aktualisieren  
depmod -a

# 3. Altes Modul entladen und neues laden
/etc/init.d/lan865x-modules reload

# 4. Treiber ist sofort aktiv - kein Reboot nötig!
```

**Vorteile der Modul-Implementierung:**
- ✅ Schnelle Entwicklungszyklen (kein kompletter Kernel-Rebuild)
- ✅ Runtime-Loading/-Unloading für Testing
- ✅ Einfache Aktualisierung ohne Neustart
- ✅ Debugging-freundlich

## Kernel-Konfiguration für Module

Für die Modul-Entwicklung ist eine spezifische Kernel-Konfiguration erforderlich. Diese wird über das `config_manager.sh` Script verwaltet:

### Erste Einrichtung (nach Git Clone)

```bash
# Gespeicherte Kernel-Konfiguration anwenden
./config_manager.sh apply

# Status prüfen
./config_manager.sh status

# Kernel mit Modul-Support neu kompilieren
cd /home/martin/AIoT/lan9662/mchp-brsdk-source-2025.12
make linux-rebuild O=output/mybuild_regacces
```

### Konfigurationsmanagement

```bash
# Aktuelle Kernel-Config sichern (nach erfolgreichen Builds)
./config_manager.sh backup

# Unterschiede zwischen gespeichert und aktuell anzeigen
./config_manager.sh diff

# Gespeicherte Config wiederherstellen (bei Problemen)
./config_manager.sh apply

# Konfigurations-Status anzeigen
./config_manager.sh status

# Backup löschen
./config_manager.sh clean
```

### Wichtige Kernel-Einstellungen

Die gespeicherte `kernel.config` enthält:
- `CONFIG_LAN865X=m` - LAN865x als ladbares Modul
- `CONFIG_OA_TC6=m` - OA-TC6 Bibliothek als ladbares Modul  
- `CONFIG_MODULES=y` - Module-Support aktiviert
- `CONFIG_MODULE_UNLOAD=y` - Module können entladen werden
- Alle weiteren für LAN865x nötigen Abhängigkeiten

## Grundlegende Verwendung

### 1. Debug aktivieren/deaktivieren

```bash
# Debug aktivieren
echo 1 > /sys/kernel/debug/lan865x/debug_enable

# Debug deaktivieren  
echo 0 > /sys/kernel/debug/lan865x/debug_enable
```

### 2. Register-Status anzeigen

```bash
cat /sys/kernel/debug/lan865x/regs
```

**Beispiel-Output:**
```
=== LAN865x Register Debug Info ===
MAC_NET_CTL (0x00010000): 0x0000000c
  TX_EN: ON
  RX_EN: ON
Last accessed: addr=0x00010000, val=0x0000000c
Debug enabled: YES

Usage: echo 'addr value' > regs  # Write register
       echo 'addr' > regs        # Read register
```

### 3. Register lesen

```bash
# MAC Network Control Register lesen
echo "00010000" > /sys/kernel/debug/lan865x/regs

# MAC Network Configuration Register lesen
echo "00010001" > /sys/kernel/debug/lan865x/regs

# MAC Adresse Low Bytes lesen
echo "00010022" > /sys/kernel/debug/lan865x/regs
```

### 4. Register schreiben

```bash
# TX und RX aktivieren (Bits 2 und 3 setzen)
echo "00010000 0000000c" > /sys/kernel/debug/lan865x/regs

# Promiscuous Mode aktivieren (Bit 4 in NET_CFG setzen)
echo "00010001 00000010" > /sys/kernel/debug/lan865x/regs

# Multicast Mode aktivieren (Bit 6 in NET_CFG setzen)
echo "00010001 00000040" > /sys/kernel/debug/lan865x/regs
```

## Wichtige Register-Adressen

| Register | Adresse | Beschreibung |
|----------|---------|-------------|
| MAC_NET_CTL | 0x00010000 | Network Control (TX/RX Enable) |
| MAC_NET_CFG | 0x00010001 | Network Configuration (Promiscuous/Multicast) |
| MAC_L_HASH | 0x00010020 | MAC Hash Register Bottom |
| MAC_H_HASH | 0x00010021 | MAC Hash Register Top |
| MAC_L_SADDR1 | 0x00010022 | MAC Specific Address 1 Bottom |
| MAC_H_SADDR1 | 0x00010023 | MAC Specific Address 1 Top |
| MAC_TSU_TIMER_INCR | 0x00010077 | MAC TSU Timer Increment |

## Register-Bit-Definitionen

### MAC_NET_CTL (0x00010000)
- Bit 3: `MAC_NET_CTL_TXEN` - Transmit Enable
- Bit 2: `MAC_NET_CTL_RXEN` - Receive Enable

### MAC_NET_CFG (0x00010001)
- Bit 4: `MAC_NET_CFG_PROMISCUOUS_MODE` - Promiscuous Mode
- Bit 6: `MAC_NET_CFG_MULTICAST_MODE` - Multicast Mode
- Bit 7: `MAC_NET_CFG_UNICAST_MODE` - Unicast Mode

## Praktische Beispiele

### Hardware aktivieren/deaktivieren

```bash
# Hardware komplett aktivieren (TX + RX)
echo "00010000 0000000c" > /sys/kernel/debug/lan865x/regs

# Nur TX aktivieren
echo "00010000 00000008" > /sys/kernel/debug/lan865x/regs

# Nur RX aktivieren
echo "00010000 00000004" > /sys/kernel/debug/lan865x/regs

# Hardware deaktivieren
echo "00010000 00000000" > /sys/kernel/debug/lan865x/regs
```

### Netzwerk-Modi konfigurieren

```bash
# Promiscuous Mode
echo "00010001 00000010" > /sys/kernel/debug/lan865x/regs

# Multicast Mode
echo "00010001 00000040" > /sys/kernel/debug/lan865x/regs

# Normal Mode (nur lokale MAC-Adresse)
echo "00010001 00000000" > /sys/kernel/debug/lan865x/regs
```

### MAC-Adresse auslesen

```bash
# MAC Low Bytes lesen
echo "00010022" > /sys/kernel/debug/lan865x/regs

# MAC High Bytes lesen
echo "00010023" > /sys/kernel/debug/lan865x/regs
```

## Logging und Monitoring

Alle Register-Zugriffe werden im Kernel-Log dokumentiert:

```bash
# Kernel-Log in Echtzeit verfolgen
dmesg -w | grep lan865x

# Letzte Einträge anzeigen
dmesg | tail -20 | grep "REG_READ\|REG_WRITE"
```

**Beispiel Log-Output:**
```
[  123.456] lan865x: REG_READ: 0x00010000 = 0x0000000c
[  124.567] lan865x: REG_WRITE: 0x00010001 = 0x00000010
```

### Verbose Debug Logging

Für detailliertes Register-Access-Logging steht eine bedingte Kompilierung zur Verfügung:

**Aktivierung:**
```c
/* Enable verbose debug logging for register access (comment out for production) */
#define CONFIG_LAN865X_DEBUG_VERBOSE
```

**Deaktivierung (für Produktion):**
```c
/* Enable verbose debug logging for register access (comment out for production) */
// #define CONFIG_LAN865X_DEBUG_VERBOSE
```

**Verhalten:**
- **Aktiviert**: Jeder debugfs Register-Zugriff wird zusätzlich ins Kernel-Log geschrieben
- **Deaktiviert**: Optimierte Performance, kein verbose logging (empfohlen für Produktion)
- **Debug-Info**: Bleibt immer über `cat /sys/kernel/debug/lan865x/regs` verfügbar

**Performance-Hinweis:** 
⚠️ Verbose logging kann bei vielen Register-Zugriffen das System verlangsamen. Nur für Testing/Debugging aktivieren!

## Repository-Dateien

Dieses Repository enthält folgende wichtige Dateien für die LAN865x Modul-Entwicklung:

### **Quellcode und Dokumentation**
- `lan865x.c` - Haupttreiber-Quellcode mit debugfs Interface
- `README.md` - Diese Dokumentation

### **Konfigurationsmanagement**
- `kernel.config` - Gespeicherte Kernel-Konfiguration mit Modul-Support
- `config_manager.sh` - Script zur Verwaltung der Kernel-Konfiguration

### **Verwendung der Repository-Dateien**

```bash
# Nach dem Klonen des Repositories:
git clone https://github.com/zabooh/lan8651_patch.git
cd lan8651_patch

# 1. Kernel-Konfiguration wiederherstellen
./config_manager.sh apply

# 2. Kernel mit korrekten Einstellungen kompilieren
cd /home/martin/AIoT/lan9662/mchp-brsdk-source-2025.12
make linux-rebuild O=output/mybuild_regacces

# 3. Module sind bereit für Entwicklung und Testing
```

**Selbst-dokumentierender Workflow:**
- Alle nötigen Konfigurationen sind im Repository gespeichert
- Reproduzierbare Builds auf verschiedenen Systemen
- Keine manuellen Konfigurationsschritte erforderlich

## LAN8651 Register-Access Tools

Zusätzlich zum debugfs Interface sind umfassende Tools für den direkten Zugriff auf LAN8651 Register verfügbar:

### 📦 Verfügbare Tools (Verzeichnis: `lan8651-regaccess/`)

#### **1. Python Tool - `lan8651_kernelfs.py` (✅ Funktionsfähig)**
Vollständiges Register-Access-Tool über das debugfs Interface:

```bash
# Register nach Name lesen
python3 lan8651_kernelfs.py read OA_STATUS0

# Register nach Adresse lesen  
python3 lan8651_kernelfs.py read 0x0008

# Register schreiben
python3 lan8651_kernelfs.py write OA_CONFIG0 0x12345678

# Alle verfügbaren Register auflisten
python3 lan8651_kernelfs.py list

# Device-Status Übersicht
python3 lan8651_kernelfs.py status

# Debug-Modus aktivieren
LAN8651_DEBUG=1 python3 lan8651_kernelfs.py status
```

**Features:**
- **Register-Name-Auflösung**: Verwendung von Namen statt Hexadezimal-Adressen
- **30+ offizielle Register** aus dem Microchip-Datenblatt
- **Bit-Field-Dekodierung**: Automatische Interpretation von Status/Control-Bits  
- **Automatische Interface-Erkennung**: Findet LAN8651 Devices automatisch
- **Umfassendes Debugging**: Detaillierte Debug-Ausgaben

#### **2. C Tool - `lan8651_ethtool.c` (⚠️ Benötigt Treiber-Erweiterung)**
Ethtool-basiertes Register-Access-Tool:

```bash
# Kompilierte Binaries für verschiedene Architekturen
./lan8651_ethtool_arm_debug read 0x0008
./lan8651_ethtool_x86_debug write 0x0004 0x12345678
```

**Features:**
- **Cross-Platform**: ARM und x86 Binaries verfügbar
- **Ethtool-Integration**: Nutzt Standard-Linux-Ethtool-Interface
- **Debug-Unterstützung**: Compile-Time Debug-Optionen
- **Direkte Kernel-Kommunikation**: Über IOCTL ohne Dateisystem-Zugriffe

### 📚 Detaillierte Dokumentation

Das `lan8651-regaccess/` Verzeichnis enthält umfassende Dokumentation:

- **[LAN8651 Tools README](lan8651-regaccess/README.md)** - Vollständige Tool-Dokumentation
- **[Register Map](lan8651-regaccess/LAN8651_REGISTER_MAP.md)** - Komplette Register-Referenz aus dem Microchip-Datenblatt
- **[Debug Testing Guide](lan8651-regaccess/DEBUG_TESTING_GUIDE.md)** - Umfassendes Debug-Testing
- **[Register Update Summary](lan8651-regaccess/REGISTER_UPDATE_SUMMARY.md)** - Änderungsprotokoll

### 🔧 Build & Test Tools

```bash
# Tools kompilieren
lan8651-regaccess/build_tools.sh

# Tools testen
lan8651-regaccess/test_tools.sh

# Debug-Versionen 
lan8651-regaccess/build_tools_debug.sh
lan8651-regaccess/test_tools_debug.sh
```

### 🏗️ Architektur

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   User Tools    │    │ Patched Driver  │    │   Hardware      │
│                 │    │                 │    │                 │
│lan8651_kernelfs │◄──►│ lan865x driver  │◄──►│   LAN8651       │
│lan8651_ethtool* │    │   + debugfs     │    │   (SPI/TC6)     │
│                 │    │                 │    │                 │
└─────────────────┘    └─────────────────┘    └─────────────────┘
        │                        │                        │
        ▼                        ▼                        ▼
/sys/kernel/debug/lan865x    OA TC6 Framework         SPI Interface
```

**Integration:**
- **Python Tool**: Nutzt das bereits implementierte debugfs Interface  
- **C Tool**: Kann bei Bedarf durch zusätzliche Ethtool-Handler erweitert werden
- **Beide Tools**: Verwenden offizielle Register-Definitionen aus dem Microchip-Datenblatt

Die Tools bieten eine **vollständige Abstraktionsebene** für LAN8651 Register-Zugriffe und ergänzen perfekt das debugfs Interface für umfassende Hardware-Diagnose und -Entwicklung.

## Sicherheitsfeatures

- **Zugriffsschutz**: Debug-Zugriff nur wenn `debug_enabled = true`
- **Eingabevalidierung**: Automatische Überprüfung der Eingabeformate
- **Fehlerbehandlung**: Umfassende Fehlerausgabe bei fehlgeschlagenen Operationen
- **Berechtigung**: Root-Zugriff erforderlich (Dateiberechtigungen: 0600)

## Fehlerbehebung

### Debug Interface nicht verfügbar
```bash
# Überprüfen ob debugfs gemountet ist
mount | grep debugfs

# debugfs manuell mounten falls nötig
mount -t debugfs none /sys/kernel/debug
```

### Treiber nicht geladen
```bash
# Treiber-Status prüfen
lsmod | grep lan865x

# SPI-Geräte anzeigen
cat /sys/bus/spi/devices/*/modalias
```

### Zugriffsfehler
```bash
# Berechtigung prüfen
ls -la /sys/kernel/debug/lan865x/

# Als root ausführen
sudo bash
```

## Entwickler-Hinweise

Das debugfs Interface ist in folgenden Funktionen implementiert:

- `lan865x_debugfs_init()` - Interface-Initialisierung
- `lan865x_debugfs_reg_read()` - Register-Lesezugriff
- `lan865x_debugfs_reg_write()` - Register-Schreibzugriff  
- `lan865x_debugfs_remove()` - Interface-Cleanup

Die Debug-Funktionalität ist standardmäßig aktiviert (`debug_enabled = true`) und kann zur Laufzeit über die `debug_enable` Datei gesteuert werden.

## Warnung

⚠️ **Vorsicht beim Schreiben von Registern!** Unsachgemäße Register-Werte können die Hardware beschädigen oder zu instabilem Verhalten führen. Verwenden Sie das Interface nur wenn Sie die Hardware-Spezifikation verstehen.

