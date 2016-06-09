/*
 * Tegra 11x SoC-specific mcerr code.
 *
 * Copyright (c) 2010-2014, NVIDIA Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <mach/mcerr.h>

/*** Auto generated by `mcp.pl'. Do not modify! ***/

#define dummy_client   client("dummy", "dummy")

struct mc_client mc_clients[] = {
	client("ptc", "csr_ptcr"),
	client("dc", "csr_display0a"),
	client("dcb", "csr_display0ab"),
	client("dc", "csr_display0b"),
	client("dcb", "csr_display0bb"),
	client("dc", "csr_display0c"),
	client("dcb", "csr_display0cb"),
	dummy_client,
	dummy_client,
	client("epp", "cbr_eppup"),
	client("g2", "cbr_g2pr"),
	client("g2", "cbr_g2sr"),
	dummy_client,
	dummy_client,
	dummy_client,
	client("avpc", "csr_avpcarm7r"),
	client("dc", "csr_displayhc"),
	client("dcb", "csr_displayhcb"),
	client("nv", "csr_fdcdrd"),
	client("nv", "csr_fdcdrd2"),
	client("g2", "csr_g2dr"),
	client("hda", "csr_hdar"),
	client("hc", "csr_host1xdmar"),
	client("hc", "csr_host1xr"),
	client("nv", "csr_idxsrd"),
	dummy_client,
	dummy_client,
	dummy_client,
	client("msenc", "csr_msencsrd"),
	client("ppcs", "csr_ppcsahbdmar"),
	client("ppcs", "csr_ppcsahbslvr"),
	dummy_client,
	client("nv", "csr_texl2srd"),
	dummy_client,
	client("vde", "csr_vdebsevr"),
	client("vde", "csr_vdember"),
	client("vde", "csr_vdemcer"),
	client("vde", "csr_vdetper"),
	client("mpcorelp", "csr_mpcorelpr"),
	client("mpcore", "csr_mpcorer"),
	client("epp", "cbw_eppu"),
	client("epp", "cbw_eppv"),
	client("epp", "cbw_eppy"),
	client("msenc", "csw_msencswr"),
	client("vi", "cbw_viwsb"),
	client("vi", "cbw_viwu"),
	client("vi", "cbw_viwv"),
	client("vi", "cbw_viwy"),
	client("g2", "ccw_g2dw"),
	dummy_client,
	client("avpc", "csw_avpcarm7w"),
	client("nv", "csw_fdcdwr"),
	client("nv", "csw_fdcdwr2"),
	client("hda", "csw_hdaw"),
	client("hc", "csw_host1xw"),
	client("isp", "csw_ispw"),
	client("mpcorelp", "csw_mpcorelpw"),
	client("mpcore", "csw_mpcorew"),
	dummy_client,
	client("ppcs", "csw_ppcsahbdmaw"),
	client("ppcs", "csw_ppcsahbslvw"),
	dummy_client,
	client("vde", "csw_vdebsevw"),
	client("vde", "csw_vdedbgw"),
	client("vde", "csw_vdembew"),
	client("vde", "csw_vdetpmw"),
	dummy_client,
	dummy_client,
	dummy_client,
	dummy_client,
	dummy_client,
	dummy_client,
	dummy_client,
	dummy_client,
	client("xusb_host", "csr_xusb_hostr"),
	client("xusb_host", "csw_xusb_hostw"),
	client("xusb_dev", "csr_xusb_devr"),
	client("xusb_dev", "csw_xusb_devw"),
	client("nv", "csw_fdcdwr3"),
	client("nv", "csr_fdcdrd3"),
	client("nv", "csw_fdcdwr4"),
	client("nv", "csr_fdcdrd4"),
	client("emucif", "csr_emucifr"),
	client("emucif", "csw_emucifw"),
	client("tsec", "csr_tsecsrd"),
	client("tsec", "csw_tsecswr"),
};
int mc_client_last = ARRAY_SIZE(mc_clients) - 1;
/*** Done. ***/

static void mcerr_t11x_info_update(struct mc_client *c, u32 stat)
{
	if (stat & MC_INT_DECERR_EMEM)
		c->intr_counts[0]++;
	if (stat & MC_INT_SECURITY_VIOLATION)
		c->intr_counts[1]++;
	if (stat & MC_INT_INVALID_SMMU_PAGE)
		c->intr_counts[2]++;
	if (stat & MC_INT_DECERR_VPR)
		c->intr_counts[3]++;
	if (stat & MC_INT_SECERR_SEC)
		c->intr_counts[4]++;

	if (stat & ~MC_INT_EN_MASK)
		c->intr_counts[5]++;
}

/*
 * T11x reports addresses in a 32 byte range thus we can only give an
 * approximate location for the invalid memory request, not the exact address.
 */
static void mcerr_t11x_print(const struct mc_error *err,
			     const struct mc_client *client,
			     u32 status, phys_addr_t addr,
			     int secure, int rw, const char *smmu_info)
{
	pr_err("[mcerr] (%s) %s: %s\n", client->swgid, client->name, err->msg);
	pr_err("[mcerr]   status = 0x%08x; addr = [0x%08lx -> 0x%08lx]",
	       status, (ulong)(addr & ~0x1f), (ulong)(addr | 0x1f));
	pr_err("[mcerr]   secure: %s, access-type: %s, SMMU fault: %s\n",
	       secure ? "yes" : "no", rw ? "write" : "read",
	       smmu_info ? smmu_info : "none");
}

#define fmt_hdr "%-18s %-18s %-9s %-9s %-9s %-10s %-10s %-9s\n"
#define fmt_cli "%-18s %-18s %-9u %-9u %-9u %-10u %-10u %-9u\n"
static int mcerr_t11x_debugfs_show(struct seq_file *s, void *v)
{
	int i, j;
	int do_print;

	seq_printf(s, fmt_hdr,
		   "swgid", "client", "decerr", "secerr", "smmuerr",
		   "decerr-VPR", "secerr-SEC", "unknown");
	for (i = 0; i < ARRAY_SIZE(mc_clients); i++) {
		do_print = 0;
		if (strcmp(mc_clients[i].name, "dummy") == 0)
			continue;
		/* Only print clients who actually have errors. */
		for (j = 0; j < INTR_COUNT; j++) {
			if (mc_clients[i].intr_counts[j]) {
				do_print = 1;
				break;
			}
		}
		if (do_print)
			seq_printf(s, fmt_cli,
				   mc_clients[i].swgid,
				   mc_clients[i].name,
				   mc_clients[i].intr_counts[0],
				   mc_clients[i].intr_counts[1],
				   mc_clients[i].intr_counts[2],
				   mc_clients[i].intr_counts[3],
				   mc_clients[i].intr_counts[4],
				   mc_clients[i].intr_counts[5]);
	}
	return 0;
}

/*
 * Set up chip specific functions and data for handling this particular chip's
 * error decoding and logging.
 */
void mcerr_chip_specific_setup(struct mcerr_chip_specific *spec)
{
	spec->mcerr_print = mcerr_t11x_print;
	spec->mcerr_info_update = mcerr_t11x_info_update;
	spec->mcerr_debugfs_show = mcerr_t11x_debugfs_show;
	spec->nr_clients = ARRAY_SIZE(mc_clients);
	return;
}
