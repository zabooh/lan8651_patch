# LAN8651 Complete Register Map
*Basierend auf dem offiziellen Microchip LAN8650/1 Datenblatt (DS60001734)*

## Übersicht

Der LAN8651 10BASE-T1S MAC-PHY Controller implementiert die OPEN Alliance 10BASE-T1x MAC-PHY Serial Interface Spezifikation. Das Register-Layout ist nach Memory Map Selector (MMS) organisiert, welcher ein 4-Bit Feld ist, das bis zu 16 verschiedene Memory Maps unterstützt.

## Memory Map Selector (MMS) Organisation

| MMS | Größe | Beschreibung |
|-----|--------|-------------|
| 0   | 32/16  | Open Alliance 10BASE-T1x MAC-PHY Standard Registers inkl. PHY Clause 22 Basic Control/Status |
| 1   | 32     | MAC Registers |
| 2   | 16     | PHY PCS Registers |
| 3   | 16     | PHY PMA/PMD Registers |
| 4   | 16     | PHY Vendor Specific Registers |
| 5-9 | -      | Reserviert |
| 10  | 16     | Miscellaneous Register Descriptions |
| 11-15| -     | Reserviert |

---

## MMS 0: Open Alliance 10BASE-T1x MAC-PHY Standard Registers

### Standard Control/Status Registers (0x00-0x15)

| Addr | Register | Bits 31:24 | Bits 23:16 | Bits 15:8 | Bits 7:0 | Beschreibung |
|------|----------|------------|------------|-----------|----------|-------------|
| 0x00 | OA_ID | - | - | - | MAJVER[3:0], MINVER[3:0] | Open Alliance Identifikation |
| 0x01 | OA_PHYID | OUI[21:14] | OUI[13:6] | OUI[5:0], MODEL[5:4] | MODEL[3:0], REVISION[3:0] | PHY Identifikation |
| 0x02 | OA_STDCAP | - | - | -, TXFCSVC, IPRAC, DPRAC | CTC, FTSC, AIDC, SEQC, -, MINBPS[2:0] | Standard Capabilities |
| 0x03 | OA_RESET | - | - | - | -, -, -, -, -, -, -, SWRESET | Reset Control |
| 0x04 | OA_CONFIG0 | - | - | SYNC, TXFCSVE, RFA[1:0], TXCTHRESH[1:0], TXCTE, RXCTE | FTSE, FTSS, PROTE, SEQE, -, BPS[2:0] | Konfiguration 0 |
| 0x08 | OA_STATUS0 | - | - | -, -, -, CPDE, TXFCSE, TTSCAC, TTSCAB, TTSCAA | PHYINT, RESETC, HDRE, LOFE, RXBOE, TXBUE, TXBOE, TXPE | Status 0 |
| 0x09 | OA_STATUS1 | -, -, -, SEV, -, TTSCMC, TTSCMB, TTSCMA | TTSCOFC, TTSCOFB, TTSCOFA, BUSER, UV18, ECC, FSMSTER, - | - | -, -, -, -, -, -, TXNER, RXNER | Status 1 |
| 0x0B | OA_BUFSTS | - | - | TXC[7:0] | RBA[7:0] | Buffer Status |
| 0x0C | OA_IMASK0 | - | - | -, -, -, CPDEM, TXFCSEM, TTSCACM, TTSCABM, TTSCAAM | PHYINTM, RESETCM, HDREM, LOFEM, RXBOEM, TXBUEM, TXBOEM, TXPEM | Interrupt Mask 0 |
| 0x0D | OA_IMASK1 | -, -, -, SEVM, -, TTSCMCM, TTSCMBM, TTSCMAM | TTSCOFCM, TTSCOFBM, TTSCOFAM, BUSERM, UV18M, ECCM, FSMSTERM, - | - | -, -, -, -, -, -, TXNERM, RXNERM | Interrupt Mask 1 |

### Timestamp Capture Registers (0x10-0x15)

| Addr | Register | Beschreibung |
|------|----------|-------------|
| 0x10 | TTSCAH | Transmit Timestamp Capture A (High 32-bit) |
| 0x11 | TTSCAL | Transmit Timestamp Capture A (Low 32-bit) |
| 0x12 | TTSCBH | Transmit Timestamp Capture B (High 32-bit) |
| 0x13 | TTSCBL | Transmit Timestamp Capture B (Low 32-bit) |
| 0x14 | TTSCCH | Transmit Timestamp Capture C (High 32-bit) |
| 0x15 | TTSCCL | Transmit Timestamp Capture C (Low 32-bit) |

### Clause 22 CSR Registers (0xFF00-0xFF0E)

| Addr | Register | Bits 15:8 | Bits 7:0 | Beschreibung |
|------|----------|-----------|----------|-------------|
| 0xFF00 | BASIC_CONTROL | SW_RESET, LOOPBACK, SPD_SEL[0], AUTONEGEN, PD, -, REAUTONEG, DUPLEXMD | -, SPD_SEL[1], -, -, -, -, -, - | Basic Control Register |
| 0xFF01 | BASIC_STATUS | 100BT4A, 100BTXFDA, 100BTXHDA, 10BTFDA, 10BTHDA, 100BT2FDA, 100BT2HDA, EXTSTS | -, -, AUTONEGC, RMTFLTD, AUTONEGA, LNKSTS, JABDET, EXTCAPA | Basic Status Register |
| 0xFF02 | PHY_ID1 | OUI[2:9] | OUI[10:17] | PHY Identifier 1 |
| 0xFF03 | PHY_ID2 | OUI[18:23], MODEL[5:4] | MODEL[3:0], REV[3:0] | PHY Identifier 2 |
| 0xFF0D | MMDCTRL | FNCTN[1:0], -, -, -, -, -, - | -, -, -, DEVAD[4:0] | MMD Access Control |
| 0xFF0E | MMDAD | ADR_DATA[15:8] | ADR_DATA[7:0] | MMD Access Address/Data |

---

## MMS 1: MAC Registers

### MAC Control Registers (0x00-0x01)

| Addr | Register | Bits 31:24 | Bits 23:16 | Bits 15:8 | Bits 7:0 | Beschreibung |
|------|----------|------------|------------|-----------|----------|-------------|
| 0x00 | MAC_NCR | - | - | - | -, -, -, -, TXEN, RXEN, LBL, - | Network Control Register |
| 0x01 | MAC_NCFGR | -, -, RXBP, -, -, IRXFCS, EFRHD, - | -, -, -, -, -, -, RFCS, LFERD | Network Configuration Register |

### Hash Registers (0x20-0x21)

| Addr | Register | Beschreibung |
|------|----------|-------------|
| 0x20 | MAC_HRB | Hash Register Bottom (32-bit) |
| 0x21 | MAC_HRT | Hash Register Top (32-bit) |

### Specific Address Registers (0x22-0x29)

| Addr | Register | Beschreibung |
|------|----------|-------------|
| 0x22 | MAC_SAB1 | Specific Address 1 Bottom (32-bit) |
| 0x23 | MAC_SAT1 | Specific Address 1 Top (16-bit + Filter Type) |
| 0x24 | MAC_SAB2 | Specific Address 2 Bottom (32-bit) |
| 0x25 | MAC_SAT2 | Specific Address 2 Top (16-bit + Filter) |
| 0x26 | MAC_SAB3 | Specific Address 3 Bottom (32-bit) |
| 0x27 | MAC_SAT3 | Specific Address 3 Top (16-bit + Filter) |
| 0x28 | MAC_SAB4 | Specific Address 4 Bottom (32-bit) |
| 0x29 | MAC_SAT4 | Specific Address 4 Top (16-bit + Filter) |

### Type ID Match Registers (0x2A-0x2D)

| Addr | Register | Beschreibung |
|------|----------|-------------|
| 0x2A | MAC_TIDM1 | Type ID Match 1 (16-bit + Enable) |
| 0x2B | MAC_TIDM2 | Type ID Match 2 (16-bit + Enable) |
| 0x2C | MAC_TIDM3 | Type ID Match 3 (16-bit + Enable) |
| 0x2D | MAC_TIDM4 | Type ID Match 4 (16-bit + Enable) |

### Address Mask Registers (0x32-0x33)

| Addr | Register | Beschreibung |
|------|----------|-------------|
| 0x32 | MAC_SAMB1 | Specific Address Mask 1 Bottom (32-bit) |
| 0x33 | MAC_SAMT1 | Specific Address Mask 1 Top (16-bit) |

### Time Stamp Unit Registers (0x6F-0x77)

| Addr | Register | Beschreibung |
|------|----------|-------------|
| 0x6F | MAC_TISUBN | TSU Timer Increment Sub-nanoseconds |
| 0x70 | MAC_TSH | TSU Timer Seconds High (16-bit) |
| 0x74 | MAC_TSL | TSU Timer Seconds Low (32-bit) |
| 0x75 | MAC_TN | TSU Timer Nanoseconds (30-bit) |
| 0x76 | MAC_TA | TSU Timer Adjust (30-bit + ADJ) |
| 0x77 | MAC_TI | TSU Timer Increment (8-bit) |

### Buffer Manager Control (0x0200)

| Addr | Register | Beschreibung |
|------|----------|-------------|
| 0x0200 | BMGR_CTL | Buffer Manager Control (SNAPSTATS, CLRSTATS) |

### Statistics Registers (0x0208-0x0214)

| Addr | Register | Beschreibung |
|------|----------|-------------|
| 0x0208 | STATS0 | RX Symbol Error, Length Field Error, Oversize/Undersize RX |
| 0x0209 | STATS1 | RX Resource Error, Buffer/FIFO Overflow |
| 0x020A | STATS2 | Frame Check Sequence Error |
| 0x020B | STATS3 | Type ID Match Counters 1-4 |
| 0x020C | STATS4 | Specific Address Match Counters 1-4 |
| 0x020D | STATS5 | Unicast/Multicast Hash/Broadcast/VLAN RX |
| 0x020E | STATS6 | Total Frames RX (32-bit) |
| 0x020F | STATS7 | Frames RX (32-bit) |
| 0x0210 | STATS8 | TX Abort due to Internal Error |
| 0x0211 | STATS9 | TX Abort due to External Error, Underrun/Buffer Underrun |
| 0x0212 | STATS10 | Excessive Collisions |
| 0x0213 | STATS11 | Total Frames TX (32-bit) |
| 0x0214 | STATS12 | Frames TX (32-bit) |

---

## Häufig verwendete Register und deren Adressen

### Wichtige Kontrolle/Status-Register

```c
// MMS 0: Standard Register
#define LAN8651_OA_ID           0x0000  // Open Alliance ID
#define LAN8651_OA_PHYID        0x0001  // PHY Identifikation  
#define LAN8651_OA_STDCAP       0x0002  // Standard Capabilities
#define LAN8651_OA_RESET        0x0003  // Reset Control
#define LAN8651_OA_CONFIG0      0x0004  // Konfiguration 0
#define LAN8651_OA_STATUS0      0x0008  // Status 0
#define LAN8651_OA_STATUS1      0x0009  // Status 1
#define LAN8651_OA_BUFSTS       0x000B  // Buffer Status
#define LAN8651_OA_IMASK0       0x000C  // Interrupt Mask 0
#define LAN8651_OA_IMASK1       0x000D  // Interrupt Mask 1

// Clause 22 Register
#define LAN8651_BASIC_CONTROL   0xFF00  // Basic Control 
#define LAN8651_BASIC_STATUS    0xFF01  // Basic Status
#define LAN8651_PHY_ID1         0xFF02  // PHY Identifier 1
#define LAN8651_PHY_ID2         0xFF03  // PHY Identifier 2

// MMS 1: MAC Register (mit MMS=1 prefix)
#define LAN8651_MAC_NCR         0x10000 // MAC Network Control
#define LAN8651_MAC_NCFGR       0x10001 // MAC Network Configuration
#define LAN8651_MAC_HRB         0x10020 // MAC Hash Bottom
#define LAN8651_MAC_HRT         0x10021 // MAC Hash Top
#define LAN8651_BMGR_CTL        0x10200 // Buffer Manager Control
```

### Register-Bit-Definitionen

```c
// OA_STATUS0 Bits
#define LAN8651_STATUS0_PHYINT  (1 << 7)  // PHY Interrupt
#define LAN8651_STATUS0_RESETC  (1 << 6)  // Reset Complete
#define LAN8651_STATUS0_HDRE    (1 << 5)  // Header Error
#define LAN8651_STATUS0_LOFE    (1 << 4)  // Loss of Frame Error
#define LAN8651_STATUS0_RXBOE   (1 << 3)  // RX Buffer Overflow Error
#define LAN8651_STATUS0_TXBUE   (1 << 2)  // TX Buffer Underflow Error
#define LAN8651_STATUS0_TXBOE   (1 << 1)  // TX Buffer Overflow Error
#define LAN8651_STATUS0_TXPE    (1 << 0)  // TX Protocol Error

// BASIC_CONTROL Bits
#define LAN8651_BMCR_RESET      (1 << 15) // Software Reset
#define LAN8651_BMCR_LOOPBACK   (1 << 14) // Loopback Enable
#define LAN8651_BMCR_SPEED_SEL  (1 << 13) // Speed Selection
#define LAN8651_BMCR_ANENABLE   (1 << 12) // Auto-Negotiation Enable
#define LAN8651_BMCR_PDOWN      (1 << 11) // Power Down
#define LAN8651_BMCR_ANRESTART  (1 << 9)  // Restart Auto-Negotiation
#define LAN8651_BMCR_FULLDPLX   (1 << 8)  // Full Duplex

// BASIC_STATUS Bits
#define LAN8651_BMSR_ESTATEN    (1 << 8)  // Extended Status
#define LAN8651_BMSR_ANEGCOMPLETE (1 << 5) // Auto-Negotiation Complete
#define LAN8651_BMSR_RFAULT     (1 << 4)  // Remote Fault
#define LAN8651_BMSR_ANEGCAPABLE (1 << 3) // Auto-Negotiation Capable
#define LAN8651_BMSR_LSTATUS    (1 << 2)  // Link Status
#define LAN8651_BMSR_JCD        (1 << 1)  // Jabber Detect
#define LAN8651_BMSR_ERCAP      (1 << 0)  // Extended Register Capable
```

## Register-Zugriff über TC6 Framework

```c
// Standard Register lesen (MMS 0)
uint32_t value;
oa_tc6_read_register(tc6_dev, LAN8651_OA_STATUS0, &value);

// MAC Register lesen (MMS 1) 
uint32_t mac_ctrl;
oa_tc6_read_register(tc6_dev, LAN8651_MAC_NCR, &mac_ctrl);

// Register schreiben
uint32_t config = 0x00000001; // Enable RX
oa_tc6_write_register(tc6_dev, LAN8651_MAC_NCR, config);
```

## Wichtige Hinweise

1. **Memory Map Selector (MMS)**: Register-Adressen müssen mit dem korrekten MMS-Wert verwendet werden
2. **Reservierte Bereiche**: Niemals in reservierte Adressbereiche schreiben
3. **Bit-Attribute**: Beachte Read-Only (RO), Write-Only (WO), Read-Write (RW) Eigenschaften
4. **Reset-Verhalten**: Verschiedene Register haben unterschiedliche Reset-Werte

## Referenzen

- **Datenblatt**: LAN8650/1 10BASE-T1S MAC-PHY Ethernet Controller with SPI Datasheet (DS60001734)
- **Standard**: OPEN Alliance 10BASE-T1x MAC-PHY Serial Interface
- **IEEE Standard**: IEEE 802.3cg (10BASE-T1S)

---
*Erstellt aus dem offiziellen Microchip LAN8650/1 Datenblatt v1.0*