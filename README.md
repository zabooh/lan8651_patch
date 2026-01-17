# LAN865x Debugfs Interface

Dieses Dokument beschreibt die Verwendung des debugfs Interfaces im LAN865x 10BASE-T1S MAC-PHY Treiber.

## Übersicht

Das debugfs Interface bietet eine umfassende Schnittstelle zum Debugging des LAN865x Ethernet-Treibers zur Laufzeit. Es ermöglicht direkten Zugriff auf Hardware-Register und bietet detaillierte Statusinformationen.

## Debugfs-Struktur

Das Interface erstellt ein debugfs-Verzeichnis unter `/sys/kernel/debug/lan865x/` mit folgenden Dateien:

- `regs` - Register Lese-/Schreibzugriff
- `debug_enable` - Debug-Status ein/ausschalten (boolean)

## Systemvoraussetzungen

- Linux Kernel mit CONFIG_DEBUG_FS aktiviert
- Root-Berechtigung für debugfs Zugriff
- Geladener lan865x Treiber

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

## Support

Bei Problemen oder Fragen wenden Sie sich an:
- Maintainer: Parthiban Veerasooran <parthiban.veerasooran@microchip.com>
- Hardware-Dokumentation: Microchip LAN865x Datenblatt
- Application Note: AN1760 (Microchip)