// SPDX-License-Identifier: GPL-2.0+
/*
 * Microchip's LAN865x 10BASE-T1S MAC-PHY driver
 *
 * Author: Parthiban Veerasooran <parthiban.veerasooran@microchip.com>
 */

/* Enable verbose debug logging for register access (comment out for production) */
//#define CONFIG_LAN865X_DEBUG_VERBOSE

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/phy.h>
#include <linux/oa_tc6.h>
#include <linux/debugfs.h>
#include <linux/proc_fs.h>


#define DRV_NAME			"lan8650"

/* MAC Network Control Register */
#define LAN865X_REG_MAC_NET_CTL		0x00010000
#define MAC_NET_CTL_TXEN		BIT(3) /* Transmit Enable */
#define MAC_NET_CTL_RXEN		BIT(2) /* Receive Enable */

/* MAC Network Configuration Reg */
#define LAN865X_REG_MAC_NET_CFG		0x00010001
#define MAC_NET_CFG_PROMISCUOUS_MODE	BIT(4)
#define MAC_NET_CFG_MULTICAST_MODE	BIT(6)
#define MAC_NET_CFG_UNICAST_MODE	BIT(7)

/* MAC Hash Register Bottom */
#define LAN865X_REG_MAC_L_HASH		0x00010020
/* MAC Hash Register Top */
#define LAN865X_REG_MAC_H_HASH		0x00010021
/* MAC Specific Addr 1 Bottom Reg */
#define LAN865X_REG_MAC_L_SADDR1	0x00010022
/* MAC Specific Addr 1 Top Reg */
#define LAN865X_REG_MAC_H_SADDR1	0x00010023

/* MAC TSU Timer Increment Register */
#define LAN865X_REG_MAC_TSU_TIMER_INCR		0x00010077
#define MAC_TSU_TIMER_INCR_COUNT_NANOSECONDS	0x0028

struct lan865x_priv {
	struct work_struct multicast_work;
	struct net_device *netdev;
	struct spi_device *spi;
	struct oa_tc6 *tc6;
	
	/* Debug state */
	u32 last_reg_addr;
	u32 last_reg_value;
	bool debug_enabled;
	struct dentry *debugfs_dir;
	struct dentry *debugfs_regs;
};

static int lan865x_set_hw_macaddr_low_bytes(struct oa_tc6 *tc6, const u8 *mac)
{
	u32 regval;

	regval = (mac[3] << 24) | (mac[2] << 16) | (mac[1] << 8) | mac[0];

	return oa_tc6_write_register(tc6, LAN865X_REG_MAC_L_SADDR1, regval);
}

static int lan865x_set_hw_macaddr(struct lan865x_priv *priv, const u8 *mac)
{
	int restore_ret;
	u32 regval;
	int ret;

	/* Configure MAC address low bytes */
	ret = lan865x_set_hw_macaddr_low_bytes(priv->tc6, mac);
	if (ret)
		return ret;

	/* Prepare and configure MAC address high bytes */
	regval = (mac[5] << 8) | mac[4];
	ret = oa_tc6_write_register(priv->tc6, LAN865X_REG_MAC_H_SADDR1,
				    regval);
	if (!ret)
		return 0;

	/* Restore the old MAC address low bytes from netdev if the new MAC
	 * address high bytes setting failed.
	 */
	restore_ret = lan865x_set_hw_macaddr_low_bytes(priv->tc6,
						       priv->netdev->dev_addr);
	if (restore_ret)
		return restore_ret;

	return ret;
}

static const struct ethtool_ops lan865x_ethtool_ops = {
	.get_link_ksettings = phy_ethtool_get_link_ksettings,
	.set_link_ksettings = phy_ethtool_set_link_ksettings,
};

static int lan865x_set_mac_address(struct net_device *netdev, void *addr)
{
	struct lan865x_priv *priv = netdev_priv(netdev);
	struct sockaddr *address = addr;
	int ret;

	ret = eth_prepare_mac_addr_change(netdev, addr);
	if (ret < 0)
		return ret;

	if (ether_addr_equal(address->sa_data, netdev->dev_addr))
		return 0;

	ret = lan865x_set_hw_macaddr(priv, address->sa_data);
	if (ret)
		return ret;

	eth_commit_mac_addr_change(netdev, addr);

	return 0;
}

static u32 get_address_bit(u8 addr[ETH_ALEN], u32 bit)
{
	return ((addr[bit / 8]) >> (bit % 8)) & 1;
}

static u32 lan865x_hash(u8 addr[ETH_ALEN])
{
	u32 hash_index = 0;

	for (int i = 0; i < 6; i++) {
		u32 hash = 0;

		for (int j = 0; j < 8; j++)
			hash ^= get_address_bit(addr, (j * 6) + i);

		hash_index |= (hash << i);
	}

	return hash_index;
}

static int lan865x_set_specific_multicast_addr(struct lan865x_priv *priv)
{
	struct netdev_hw_addr *ha;
	u32 hash_lo = 0;
	u32 hash_hi = 0;
	int ret;

	netdev_for_each_mc_addr(ha, priv->netdev) {
		u32 bit_num = lan865x_hash(ha->addr);

		if (bit_num >= BIT(5))
			hash_hi |= (1 << (bit_num - BIT(5)));
		else
			hash_lo |= (1 << bit_num);
	}

	/* Enabling specific multicast addresses */
	ret = oa_tc6_write_register(priv->tc6, LAN865X_REG_MAC_H_HASH, hash_hi);
	if (ret) {
		netdev_err(priv->netdev, "Failed to write reg_hashh: %d\n",
			   ret);
		return ret;
	}

	ret = oa_tc6_write_register(priv->tc6, LAN865X_REG_MAC_L_HASH, hash_lo);
	if (ret)
		netdev_err(priv->netdev, "Failed to write reg_hashl: %d\n",
			   ret);

	return ret;
}

static int lan865x_set_all_multicast_addr(struct lan865x_priv *priv)
{
	int ret;

	/* Enabling all multicast addresses */
	ret = oa_tc6_write_register(priv->tc6, LAN865X_REG_MAC_H_HASH,
				    0xffffffff);
	if (ret) {
		netdev_err(priv->netdev, "Failed to write reg_hashh: %d\n",
			   ret);
		return ret;
	}

	ret = oa_tc6_write_register(priv->tc6, LAN865X_REG_MAC_L_HASH,
				    0xffffffff);
	if (ret)
		netdev_err(priv->netdev, "Failed to write reg_hashl: %d\n",
			   ret);

	return ret;
}

static int lan865x_clear_all_multicast_addr(struct lan865x_priv *priv)
{
	int ret;

	ret = oa_tc6_write_register(priv->tc6, LAN865X_REG_MAC_H_HASH, 0);
	if (ret) {
		netdev_err(priv->netdev, "Failed to write reg_hashh: %d\n",
			   ret);
		return ret;
	}

	ret = oa_tc6_write_register(priv->tc6, LAN865X_REG_MAC_L_HASH, 0);
	if (ret)
		netdev_err(priv->netdev, "Failed to write reg_hashl: %d\n",
			   ret);

	return ret;
}

static void lan865x_multicast_work_handler(struct work_struct *work)
{
	struct lan865x_priv *priv = container_of(work, struct lan865x_priv,
						 multicast_work);
	u32 regval = 0;
	int ret;

	if (priv->netdev->flags & IFF_PROMISC) {
		/* Enabling promiscuous mode */
		regval |= MAC_NET_CFG_PROMISCUOUS_MODE;
		regval &= (~MAC_NET_CFG_MULTICAST_MODE);
		regval &= (~MAC_NET_CFG_UNICAST_MODE);
	} else if (priv->netdev->flags & IFF_ALLMULTI) {
		/* Enabling all multicast mode */
		if (lan865x_set_all_multicast_addr(priv))
			return;

		regval &= (~MAC_NET_CFG_PROMISCUOUS_MODE);
		regval |= MAC_NET_CFG_MULTICAST_MODE;
		regval &= (~MAC_NET_CFG_UNICAST_MODE);
	} else if (!netdev_mc_empty(priv->netdev)) {
		/* Enabling specific multicast mode */
		if (lan865x_set_specific_multicast_addr(priv))
			return;

		regval &= (~MAC_NET_CFG_PROMISCUOUS_MODE);
		regval |= MAC_NET_CFG_MULTICAST_MODE;
		regval &= (~MAC_NET_CFG_UNICAST_MODE);
	} else {
		/* Enabling local mac address only */
		if (lan865x_clear_all_multicast_addr(priv))
			return;
	}
	ret = oa_tc6_write_register(priv->tc6, LAN865X_REG_MAC_NET_CFG, regval);
	if (ret)
		netdev_err(priv->netdev, "Failed to enable promiscuous/multicast/normal mode: %d\n",
			   ret);
}

static void lan865x_set_multicast_list(struct net_device *netdev)
{
	struct lan865x_priv *priv = netdev_priv(netdev);

	schedule_work(&priv->multicast_work);
}

static netdev_tx_t lan865x_send_packet(struct sk_buff *skb,
				       struct net_device *netdev)
{
	struct lan865x_priv *priv = netdev_priv(netdev);

	return oa_tc6_start_xmit(priv->tc6, skb);
}

static int lan865x_hw_disable(struct lan865x_priv *priv)
{
	u32 regval;

	if (oa_tc6_read_register(priv->tc6, LAN865X_REG_MAC_NET_CTL, &regval))
		return -ENODEV;

	regval &= ~(MAC_NET_CTL_TXEN | MAC_NET_CTL_RXEN);

	if (oa_tc6_write_register(priv->tc6, LAN865X_REG_MAC_NET_CTL, regval))
		return -ENODEV;

	return 0;
}

static int lan865x_net_close(struct net_device *netdev)
{
	struct lan865x_priv *priv = netdev_priv(netdev);
	int ret;

	netif_stop_queue(netdev);
	phy_stop(netdev->phydev);
	ret = lan865x_hw_disable(priv);
	if (ret) {
		netdev_err(netdev, "Failed to disable the hardware: %d\n", ret);
		return ret;
	}

	return 0;
}

static int lan865x_hw_enable(struct lan865x_priv *priv)
{
	u32 regval;

	if (oa_tc6_read_register(priv->tc6, LAN865X_REG_MAC_NET_CTL, &regval))
		return -ENODEV;

	regval |= MAC_NET_CTL_TXEN | MAC_NET_CTL_RXEN;

	if (oa_tc6_write_register(priv->tc6, LAN865X_REG_MAC_NET_CTL, regval))
		return -ENODEV;

	return 0;
}

static int lan865x_net_open(struct net_device *netdev)
{
	struct lan865x_priv *priv = netdev_priv(netdev);
	int ret;

	ret = lan865x_hw_enable(priv);
	if (ret) {
		netdev_err(netdev, "Failed to enable hardware: %d\n", ret);
		return ret;
	}

	phy_start(netdev->phydev);

	netif_start_queue(netdev);

	return 0;
}

static const struct net_device_ops lan865x_netdev_ops = {
	.ndo_open		= lan865x_net_open,
	.ndo_stop		= lan865x_net_close,
	.ndo_start_xmit		= lan865x_send_packet,
	.ndo_set_rx_mode	= lan865x_set_multicast_list,
	.ndo_set_mac_address	= lan865x_set_mac_address,
};

/* Enhanced debugfs interface for register access with comprehensive debugging */
static ssize_t lan865x_debugfs_reg_read(struct file *file, char __user *user_buf,
					size_t count, loff_t *ppos)
{
	struct lan865x_priv *priv = file->private_data;
	char buf[512];
	int len;
	u32 reg_val;
	int ret;
	
	if (!priv->debug_enabled) {
		len = snprintf(buf, sizeof(buf), "Debug disabled. Enable via echo 1 > debug_enable\n");
		goto out;
	}
	
	/* Read some key registers for debugging */
	ret = oa_tc6_read_register(priv->tc6, LAN865X_REG_MAC_NET_CTL, &reg_val);
	if (ret) {
		len = snprintf(buf, sizeof(buf), "Error reading MAC_NET_CTL: %d\n", ret);
		goto out;
	}
	
	len = snprintf(buf, sizeof(buf),
		       "=== LAN865x Register Debug Info ===\n"
		       "MAC_NET_CTL (0x%08x): 0x%08x\n"
		       "  TX_EN: %s\n"
		       "  RX_EN: %s\n"
		       "Last accessed: addr=0x%08x, val=0x%08x\n"
		       "Debug enabled: %s\n\n"
		       "Usage: echo 'addr value' > regs  # Write register\n"
		       "       echo 'addr' > regs        # Read register\n",
		       LAN865X_REG_MAC_NET_CTL, reg_val,
		       (reg_val & MAC_NET_CTL_TXEN) ? "ON" : "OFF",
		       (reg_val & MAC_NET_CTL_RXEN) ? "ON" : "OFF",
		       priv->last_reg_addr, priv->last_reg_value,
		       priv->debug_enabled ? "YES" : "NO");

out:
	return simple_read_from_buffer(user_buf, count, ppos, buf, len);
}

static ssize_t lan865x_debugfs_reg_write(struct file *file, const char __user *user_buf,
					 size_t count, loff_t *ppos)
{
	struct lan865x_priv *priv = file->private_data;
	char buf[64];
	u32 addr, value;
	int ret, args;
	
	if (!priv->debug_enabled) {
		dev_err(&priv->spi->dev, "Debug access disabled\n");
		return -EPERM;
	}
	
	if (count >= sizeof(buf))
		return -EINVAL;
		
	if (copy_from_user(buf, user_buf, count))
		return -EFAULT;
		
	buf[count] = '\0';
	
	/* Parse input: "addr" for read, "addr value" for write */
	args = sscanf(buf, "%x %x", &addr, &value);
	
	if (args == 1) {
		/* Read operation */
		ret = oa_tc6_read_register(priv->tc6, addr, &value);
		if (ret) {
			dev_err(&priv->spi->dev, "Failed to read register 0x%08x: %d\n", addr, ret);
			return ret;
		}
		priv->last_reg_addr = addr;
		priv->last_reg_value = value;
#ifdef CONFIG_LAN865X_DEBUG_VERBOSE
		dev_info(&priv->spi->dev, "REG_READ: 0x%08x = 0x%08x\n", addr, value);
#endif
	} else if (args == 2) {
		/* Write operation */
		ret = oa_tc6_write_register(priv->tc6, addr, value);
		if (ret) {
			dev_err(&priv->spi->dev, "Failed to write register 0x%08x: %d\n", addr, ret);
			return ret;
		}
		priv->last_reg_addr = addr;
		priv->last_reg_value = value;
#ifdef CONFIG_LAN865X_DEBUG_VERBOSE
		dev_info(&priv->spi->dev, "REG_WRITE: 0x%08x = 0x%08x\n", addr, value);
#endif
	} else {
		dev_err(&priv->spi->dev, "Invalid format. Use: 'addr [value]'\n");
		return -EINVAL;
	}
	
	return count;
}

static const struct file_operations lan865x_debugfs_reg_fops = {
	.open = simple_open,
	.read = lan865x_debugfs_reg_read,
	.write = lan865x_debugfs_reg_write,
	.llseek = default_llseek,
};

static void lan865x_debugfs_init(struct lan865x_priv *priv)
{
	priv->debugfs_dir = debugfs_create_dir("lan865x", NULL);
	if (!priv->debugfs_dir)
		return;
		
	priv->debugfs_regs = debugfs_create_file("regs", 0600, priv->debugfs_dir,
						priv, &lan865x_debugfs_reg_fops);
						
	debugfs_create_bool("debug_enable", 0600, priv->debugfs_dir, &priv->debug_enabled);
	
	priv->debug_enabled = true;  /* Enable by default */
}

static void lan865x_debugfs_remove(struct lan865x_priv *priv)
{
	debugfs_remove_recursive(priv->debugfs_dir);
}

static int lan865x_probe(struct spi_device *spi)
{
	struct net_device *netdev;
	struct lan865x_priv *priv;
	int ret;

	netdev = alloc_etherdev(sizeof(struct lan865x_priv));
	if (!netdev)
		return -ENOMEM;

	priv = netdev_priv(netdev);
	priv->netdev = netdev;
	priv->spi = spi;
	spi_set_drvdata(spi, priv);
	INIT_WORK(&priv->multicast_work, lan865x_multicast_work_handler);

	priv->tc6 = oa_tc6_init(spi, netdev);
	if (!priv->tc6) {
		ret = -ENODEV;
		goto free_netdev;
	}

	/* LAN865x Rev.B0/B1 configuration parameters from AN1760
	 * As per the Configuration Application Note AN1760 published in the
	 * link, https://www.microchip.com/en-us/application-notes/an1760
	 * Revision F (DS60001760G - June 2024), configure the MAC to set time
	 * stamping at the end of the Start of Frame Delimiter (SFD) and set the
	 * Timer Increment reg to 40 ns to be used as a 25 MHz internal clock.
	 */
	ret = oa_tc6_write_register(priv->tc6, LAN865X_REG_MAC_TSU_TIMER_INCR,
				    MAC_TSU_TIMER_INCR_COUNT_NANOSECONDS);
	if (ret) {
		dev_err(&spi->dev, "Failed to config TSU Timer Incr reg: %d\n",
			ret);
		goto oa_tc6_exit;
	}

	/* As per the point s3 in the below errata, SPI receive Ethernet frame
	 * transfer may halt when starting the next frame in the same data block
	 * (chunk) as the end of a previous frame. The RFA field should be
	 * configured to 01b or 10b for proper operation. In these modes, only
	 * one receive Ethernet frame will be placed in a single data block.
	 * When the RFA field is written to 01b, received frames will be forced
	 * to only start in the first word of the data block payload (SWO=0). As
	 * recommended, enable zero align receive frame feature for proper
	 * operation.
	 *
	 * https://ww1.microchip.com/downloads/aemDocuments/documents/AIS/ProductDocuments/Errata/LAN8650-1-Errata-80001075.pdf
	 */
	ret = oa_tc6_zero_align_receive_frame_enable(priv->tc6);
	if (ret) {
		dev_err(&spi->dev, "Failed to set ZARFE: %d\n", ret);
		goto oa_tc6_exit;
	}

	/* Get the MAC address from the SPI device tree node */
	if (device_get_ethdev_address(&spi->dev, netdev))
		eth_hw_addr_random(netdev);

	ret = lan865x_set_hw_macaddr(priv, netdev->dev_addr);
	if (ret) {
		dev_err(&spi->dev, "Failed to configure MAC: %d\n", ret);
		goto oa_tc6_exit;
	}

	netdev->if_port = IF_PORT_10BASET;
	netdev->irq = spi->irq;
	netdev->netdev_ops = &lan865x_netdev_ops;
	netdev->ethtool_ops = &lan865x_ethtool_ops;

	/* Initialize debugfs interface */
	lan865x_debugfs_init(priv);

	ret = register_netdev(netdev);
	if (ret) {
		dev_err(&spi->dev, "Register netdev failed (ret = %d)", ret);
		goto debugfs_cleanup;
	}

	return 0;

debugfs_cleanup:
	lan865x_debugfs_remove(priv);

oa_tc6_exit:
	oa_tc6_exit(priv->tc6);
free_netdev:
	free_netdev(priv->netdev);
	return ret;
}

static void lan865x_remove(struct spi_device *spi)
{
	struct lan865x_priv *priv = spi_get_drvdata(spi);

	cancel_work_sync(&priv->multicast_work);
	unregister_netdev(priv->netdev);
	lan865x_debugfs_remove(priv);
	oa_tc6_exit(priv->tc6);
	free_netdev(priv->netdev);
}

static const struct spi_device_id lan865x_ids[] = {
	{ .name = "lan8650" },
	{ .name = "lan8651" },
	{},
};
MODULE_DEVICE_TABLE(spi, lan865x_ids);

static const struct of_device_id lan865x_dt_ids[] = {
	{ .compatible = "microchip,lan8650" },
	{ .compatible = "microchip,lan8651" },
	{ /* Sentinel */ }
};
MODULE_DEVICE_TABLE(of, lan865x_dt_ids);

static struct spi_driver lan865x_driver = {
	.driver = {
		.name = DRV_NAME,
		.of_match_table = lan865x_dt_ids,
	 },
	.probe = lan865x_probe,
	.remove = lan865x_remove,
	.id_table = lan865x_ids,
};
module_spi_driver(lan865x_driver);

MODULE_DESCRIPTION(DRV_NAME " 10Base-T1S MACPHY Ethernet Driver");
MODULE_AUTHOR("Parthiban Veerasooran <parthiban.veerasooran@microchip.com>");
MODULE_LICENSE("GPL");
